#!/usr/bin/python3
# -*- coding: utf-8 -*-
"""
Patch DiFX/CALC .im file delay polynomials using RadioAstron closed-loop correction files.

Short usage: raPatchClosedloop.py [-h] [-a ANTENNA] [-P FRIFILE_PIMA]
                            [-R REFANT_PIMA] [-r DDLYRATE] [-t DTSHIFT]
                            [-N MAXORDER]
                            dlypolyfile [imfiles [imfiles ...]]
"""

from __future__ import print_function, division
from datetime import datetime, timedelta
from calendar import timegm
import argparse, math, sys, time

__author__ = 'Jan Wagner (MPIfR)'
__copyright__ = 'Copyright 2019, MPIfR'
__license__ = 'GNU GPL 3'
__version__ = '1.0.4'


SCALE_DELAY = 1e6	# scaling to get from RA_C_COH.TXT units (secs) to .im units (usec)
SCALE_UVW = 1		# scaling to get from RA_C_COH_uvw.txt units (m?) to .im units (m)

parser = argparse.ArgumentParser(add_help=True, formatter_class=argparse.RawDescriptionHelpFormatter, description=
"""
Patch DiFX/CALC .im file delay polynomials using RadioAstron closed-loop correction files.
""", epilog=
"""
Iterative refinement is possible by correlating first with the ASC-provided initial delay
polynomials, fringe fitting the data in PIMA, then patching the delay polynomial data with
the residuals of the fringe fit, and correlating again.

Input:
    dly_polys.txt             text file with RadioAstron closed-loop delay polynomials
    basename_1.im             original DiFX/CALC .im file to patch prior to DiFX correlation

Output in -P <pimafile> mode:
    dly_polys.txt.rev{n+1}    new closed-loop delay polys corrected by PIMA fringe fit

Output:
    basename_1.im.closedloop  new closed-loop DiFX/CALC .im to use in DiFX correlation
"""
)
parser.add_argument('-a', '--antenna', default='GT', help='DiFX name of ground station; R1 or GT')
parser.add_argument('-P', '--pima', default=None, dest='frifile_pima', help='PIMA fringe fit file with residual rates and accelerations; experiment.fri')
parser.add_argument('-R', '--refant', default='GBT-VLBA', dest='refant_pima', help='Name of reference antenna in PIMA .fri fringe fit file')
parser.add_argument('-r', '--drate', default=0.0, dest='ddlyrate', help='Residual delay rate [s/s] to add to dly polynomial')
parser.add_argument('-t', '--dt', default=0.0, dest='dtshift', help='Time shift the polynomials p_i(t) to p_i(t+dt) by dt seconds via changing the polynomial coefficients')
parser.add_argument('-T', '--disable-timeshift', action='store_false', dest="autoTimeShift", help='Do not permit shift of ASC polynomials in time when no exact match to a DiFX poly timerange found.') 
parser.add_argument('-N', '--maxorder', default=12, dest='maxorder', help='Maximum polynomial order; higher coeffs if present are set to zero i.e. the polynomials are truncated')
parser.add_argument('-V', '--version', action='version', version='%(prog)s {version}'.format(version=__version__))
parser.add_argument('dlypolyfile', nargs='?', help='Input file with delay polynomials (dly_polys.txt)')
parser.add_argument('imfiles', nargs='*', help='Input DiFX/CALC .im file(s) to patch; <basename_1.im> ...')


class PolyCoeffs:
	"""A single polynomial with coefficients"""

	source = ''
	Ncoeffs = 0
	dims = 0
	coeffs = []
	coeffscale = 1.0
	shifted = False
	tstart = datetime.utcnow() 
	tstop = tstart
	interval = 0

	def getArgs(self, line):
		return line.split('=')[-1].strip()

	def raStrToTime(self, s):
		"""Parse 'dd/mm/yyyy HHhMMmSSs' into datetime"""
		# https://aboutsimon.com/blog/2013/06/06/Datetime-hell-Time-zone-aware-to-UNIX-timestamp.html
		gm = timegm( time.strptime(s + ' GMT', '%d/%m/%Y %Hh%Mm%Ss %Z') )
		d = datetime.utcfromtimestamp(gm)
		return d

	def timeToRaStr(self, t):
		return t.strftime('%d/%m/%Y %Hh%Mm%Ss')

	def __init__(self, details, coeffscale):
		"""Initialize coeffs using a list of strings stored in 'details'.
		The list of strings should have the following format, with 'source',
		'start', 'stop', and a varying number of P0..P<n> coefficient lines.
		Coefficient line k should contain one (1D) or more (e.g. 3D) coefficients
		for the k-th power terms (x^k * Px_k + y^k * Py_k + z^k * Pz_k + ...)

		source = 0716+714
		start = 19/10/2017 09h00m00s
		stop = 19/10/2017 09h01m00s
		P0 = -1.00975710299507e-001, -2.49717010234864e-001, 5.28185040398344e-001
		P1 = 9.18124737968820e-007, -4.59083189789325e-006, 1.99785028077570e-006
		P2 = 3.57024938682854e-012, 8.83446311517487e-012, -1.87750268902654e-011
		P3 = -7.78908442044021e-016, 1.48057049755645e-015, 8.27468673485702e-016
		P4 = 1.41735592450728e-017, -2.51428930101850e-017, -1.61417000499427e-017
		P5 = -9.33938621249164e-020, 1.57978134340036e-019, 1.12339170239300e-019
		"""
		self.Ncoeffs = len(details) - 3
		assert(self.Ncoeffs >= 1)
		self.source = self.getArgs(details[0])
		self.tstart = self.raStrToTime( self.getArgs(details[1]) )
		self.tstop = self.raStrToTime( self.getArgs(details[2]) )
		self.interval = (self.tstop - self.tstart).total_seconds()
		self.coeffs = []
		self.coeffscale = coeffscale
		for n in range(self.Ncoeffs):
			cstr = self.getArgs(details[3+n])
			c = [coeffscale*float(s) for s in cstr.split(',')]
			self.dims = len(c)
			self.coeffs.append(c)

	def add(self, corrections):
		'''Element-wise addition of values in list 'correction' to the coefficients'''
		for n in range(min(self.Ncoeffs,len(corrections))):
			self.coeffs[n] = [val + corrections[n] for val in self.coeffs[n]]

	def eval(self, t_sec):
		'''Evaluate poly at time t'''
		# note: could use numpy.polyval()
		polyval = []
		for dim in range(self.dims):
			v = 0.0
			for k in range(self.Ncoeffs):
				v += self.coeffs[k][dim]*math.pow(t_sec,k)
			polyval.append(v)
		return polyval

	def trunc(self, maxorder):
		'''Set to zero all coefficients that are present past 'maxorder' (0=const-only, 1=linear-only, etc).'''
		for n in range(self.Ncoeffs):
			if n > maxorder:
				self.coeffs[n] =  [0.0]*self.dims

	def binom(self, n):
		'''Binomial coeffs, computed by the multipliative formula'''
		n = int(n)
		coeff_k = [0]*(n+1)
		for k in range(n+1):
			c = 1.0
			for i in range(1, k+1):
				c *= float((n+1-i))/float(i)
			coeff_k[k] = int(c)
		return coeff_k

	def timeshift(self, dt):
		'''Time shift polynomials p(t) to p(t+dt) via change of coefficients'''
		# Ideally would use "Taylor shift" e.g. https://math.stackexchange.com/questions/694565/polynomial-shift
		# but since we don't need performance, can use polynomial expansion at t+dt and update of coefficients.
		#
		# Illustration of the method:
		#
		# Shift p(t)    = a + b*t + c*t^2 + ...
		# gives p(t+dt) = <below>
		#
		# order 0:  p(t+dt) = a
		# order 1:  p(t+dt) = a + (b*t + b*dt) = (a + b*dt) + b*t
		#           a' = a + b*dt
		#           b' = b
		# order 2:  p(t+dt) = a + (b*t + b*dt) + (c*t^2 + c*2*dt*t + c*dt^2)
		#           a' = a + b*dt + c*dt^2
		#           b' = b + c*2*dt
		#           c' = c
		# order 3:  p(t+dt) = a + (b*t + b*dt) + (c*t^2 + c*2*dt*t + c*dt^2) + (d*dt^3 + d*3*dt^2*t + d*3*dt*t^2 + d*t^3)
		#           a' = a + b*dt + c*dt^2 + d*dt^3
		#           b' = b + c*2*dt + d*3*dt^2
		#           c' = c + d*3*dt
		#           d' = d
		# order 4:  add e*dt^4  + e*4*dt^3*t + e*6*dt^2*t^2 + e*4*dt*t^3 + e*t^4 
		#           a' = a + b*dt + c*dt^2 + d*dt^3 + e*dt^4
		#           b' = b + c*2*dt + d*3*dt^2 + e*4*dt^3
		#           c' = c + d*3*dt + e*6*dt^2
		#           d' = d + e*4*dt
		#           e' = e
		# order 5:  add f*dt^5 + f*5*dt^4*t + f*10*dt^3*t^2 + f*10*dt^2*t^3 + f*5*dt*t^4 + f*t^5
		#           a' = a + b*dt + c*dt^2 + d*dt^3 + e*dt^4 + f*dt^5
		#           b' = b + c*2*dt + d*3*dt^2 + e*4*dt^3 + f*5*dt^4
		#           c' = c + d*3*dt + e*6*dt^2 + f*10*dt^3
		#           d' = d + e*4*dt + f*10*dt^2
		#           e' = e + f*5*dt
		#           f' = f
		# --> diagonals of Pascal's triangle

		if not dt or dt==0:
			return

		# Precompute Pascal's triangle
		binom_nk = [[]] * (self.Ncoeffs+1)
		for n in range(self.Ncoeffs+1):
			binom_nk[n] = self.binom(n)
		# print (self.Ncoeffs, binom_nk)

		# Precompute dt^n for n=0..Ncoeff
		dtpow = [math.pow(dt,n) for n in range(self.Ncoeffs+1)]

		# Shift the polynomial. Need to shift each poly/dimension (dly: 1D, uvw: 3D).
		for d in range(self.dims):
			oldcoeffs = [self.coeffs[n][d] for n in range(self.Ncoeffs)]
			newcoeffs = [0.0] * self.Ncoeffs
			for n in range(self.Ncoeffs):
				# print ('c[%d] = 0 ' % (n)),
				for k in range(n, self.Ncoeffs):
					newcoeffs[n] += oldcoeffs[k]*binom_nk[k][n]*dtpow[k-n]
					# print (' + old[%d]*%d*dt^%d' % (k,binom_nk[k][n],k-n)), 
				# print ('')
			# print ('old:', oldcoeffs)
			# print ('new:', newcoeffs)
			# print ('')
			for n in range(self.Ncoeffs):
				self.coeffs[n][d] = newcoeffs[n]
		self.shifted = True


class PolySet:
	"""Storage of a set of polynomials"""

	piecewisePolys = []
	startSec = 1e99
	dims = 0
	order = 0

	def __init__(self, filename, coeffscale=1):
		"""
		Create object and load a set of polynomials and their coefficients from a file.
		"""
		self.piecewisePolys = []
		with open(filename) as f:
			lines = f.readlines()
			lines = [l.strip() for l in lines]
		if len(lines) < 1:
			print ('Error: could not read %s' % (filename))
			return
		if not 'RASTRON' in lines[0]:
			print ('Unexpected delay poly file content')
			return
		N = int(lines[1].split('=')[1])
		lnr = 3
		while lnr < len(lines):
			assert('source' in lines[lnr])
			polyDefinition = lines[lnr:(lnr+3+N)]
			self.piecewisePolys.append( PolyCoeffs(polyDefinition,coeffscale) )
			self.dims = self.piecewisePolys[-1].dims
			self.startSec = min(self.startSec, self.piecewisePolys[-1].tstart.second)
			self.order = self.piecewisePolys[-1].Ncoeffs - 1
			lnr += 3 + N

	def __len__(self):
		return len(self.piecewisePolys)

	def datetimeFromMJDSec(self,MJD,sec):
		mjd_t0 = datetime(1858,11,17,0,0,0,0) # MJD 0 = 17 November 1858 at 00:00 UTC
		T = mjd_t0 + timedelta(days=MJD) + timedelta(seconds=sec)
		return T

	def lookupPolyFor(self,MJD,sec,interval_sec=0,allowShift=False):
		"""
		Lookup up poly that has start time identical to the given MJD and second-of-day
		"""
		tlookup = self.datetimeFromMJDSec(MJD,sec)
		last = self.piecewisePolys[-1]
		for poly in self.piecewisePolys:
			if poly.tstart == tlookup:
				return poly

		toffsets = []
		for poly in self.piecewisePolys:
			dt_to_start = (tlookup - poly.tstart).total_seconds()
			dt_to_stop = (tlookup - poly.tstop).total_seconds()
			toffsets.append(dt_to_start)
		absoffsets = [abs(dt) for dt in toffsets]

		closest = absoffsets.index(min(absoffsets))
		poly = self.piecewisePolys[closest]
		dt = toffsets[closest]

		if dt <= interval_sec:
			if allowShift:
				print('Info: Time shifting poly coeffs by %+d seconds to get DiFX %s covered by RA poly %s--%s.' % (dt,str(tlookup),str(poly.tstart),str(poly.tstop)))
				# y0 = poly.eval(dt)
				poly.timeshift(dt)
				# y1 = poly.eval(0)
				# print(y0,y1)
				return poly
			else:				
				print ('Warning: Time %d MJD %d sec (%s) is %+d seconds from RA poly %s--%s.' % (MJD,sec,str(tlookup),dt,str(poly.tstart),str(poly.tstop)))

		return None

	def add(self, corrections):
		'''Element-wise addition of values in list 'correction' to coeffs of all polynomials'''
		for poly in self.piecewisePolys:
			poly.add(corrections)

	def trunc(self, maxorder):
		'''Set to zero all coefficients that are present past 'maxorder' (0=const-only, 1=linear-only, etc).'''
		for poly in self.piecewisePolys:
			poly.trunc(int(maxorder))

	def timeshift(self, dt):
		'''Time shift polynomials p(t) to p(t+dt) via change of coefficients'''
		if not dt or dt==0:
			return
		for poly in self.piecewisePolys:
			poly.timeshift(float(dt))



class Fri(object):
	"""
	This class represents PIMA fringe file.
	(C) Petr Voytsik, some small additions by Jan Wagner
	"""
	ver100 = '# PIMA Fringe results  v  1.00  Format version of 2010.04.05'
	ver101 = '# PIMA Fringe results  v  1.01  Format version of 2014.02.08'
	ver102 = '# PIMA Fringe results  v  1.02  Format version of 2014.12.24'

	ED = 2. * 6371e5  # Earth diameter
	C = 2.99792458e10  # Speed of light
	TAI_leapsecs = 37 # PIMA writes TAI time. Leaps secs while RadioAstron was still operational were 37s.

	def __init__(self, fri_file=None):
		self.records = []
		self.index = 0
		if fri_file is None:
			return

		started = False
		header = {}

		with open(fri_file, 'r') as fil:
			line = fil.readline()
			if not (line.startswith(self.ver100) or
					line.startswith(self.ver101) or
					line.startswith(self.ver102)):
				print (line)
				raise Exception('{} is not PIMA fri-file'.format(fri_file))
			for line in fil:
				if line.startswith('# PIMA_FRINGE started'):
					started = True
					header.clear()
					continue
				if line.startswith('# PIMA_FRINGE ended'):
					started = False
					continue
				if not started:
					continue
				toks = line.split()
				if len(toks) < 3:
					continue
				if toks[0] == '#':
					if toks[1] == 'Control':
						header['cnt_file'] = toks[-1]
					elif toks[1] == 'Experiment':
						header['exper_code'] = toks[-1]
					elif toks[1] == 'Session':
						header['session_code'] = toks[-1]
					elif toks[1] == 'FRINGE_FITTING_STYLE:':
						header['FRINGE_FITTING_STYLE'] = toks[-1]
					elif toks[1] == 'FRIB.POLAR:':
						header['polar'] = toks[2]
					elif toks[1] == 'FRIB.FINE_SEARCH:':
						header['FRIB.FINE_SEARCH'] = toks[-1]
					elif toks[1] == 'PHASE_ACCELERATION:':
						header['accel'] = float(toks[2].replace('D', 'e'))
				elif toks[6] != 'FAILURE':
					self.records.append({})
					self.records[-1].update(header)
					self.records[-1]['obs'] = int(toks[0])
					self.records[-1]['scan'] = int(toks[1])
					self.records[-1]['time_code'] = toks[2]
					self.records[-1]['source'] = toks[3]
					self.records[-1]['sta1'] = toks[4]
					self.records[-1]['sta2'] = toks[5]
					self.records[-1]['SNR'] = float(toks[7])
					self.records[-1]['start_time'] = datetime.strptime(
						toks[11], '%Y.%m.%d-%H:%M:%S.%f,')
					self.records[-1]['stop_time'] = datetime.strptime(
						toks[12], '%Y.%m.%d-%H:%M:%S.%f')
					self.records[-1]['ampl_lsq'] = \
						float(toks[15].replace('D', 'e'))
					self.records[-1]['delay'] = \
						float(toks[23].replace('D', 'e'))
					self.records[-1]['rate'] = \
						float(toks[31].replace('D', 'e'))
					self.records[-1]['ph_acc'] = \
						float(toks[37].replace('D', 'e'))
					self.records[-1]['duration'] = \
						float(toks[79].replace('D', 'e'))
					self.records[-1]['U'] = \
						float(toks[84].replace('D', 'e'))
					self.records[-1]['V'] = \
						float(toks[85].replace('D', 'e'))
					self.records[-1]['ref_freq'] = \
						float(toks[94].replace('D', 'e'))

					# Calculated parameters

					# UV-radius in lambda
					self.records[-1]['uv_rad'] = math.hypot(
						self.records[-1]['U'], self.records[-1]['V'])

					# Position angle
					self.records[-1]['PA'] = math.degrees(math.atan2(
						self.records[-1]['U'], self.records[-1]['V']))
					wave_len = self.C / self.records[-1]['ref_freq']

					# UV-radius in Earth diameters
					self.records[-1]['uv_rad_ed'] = \
						self.records[-1]['uv_rad'] * wave_len / self.ED

					# Go from TAI time to UT
					self.records[-1]['start_time'] -= timedelta(seconds=self.TAI_leapsecs)
					self.records[-1]['stop_time'] -= timedelta(seconds=self.TAI_leapsecs)


	def max_snr(self, station=None):
		"""
		Return observation record with maximum SNR.

		Parameters
		----------
		station : str, optional
			If station name is given select observation with this station only.

		Returns
		-------
		rec : dict
			Returns fri-record -- dictionary with parameters of the selected
			observation.

		"""
		rec = {}

		# Select observations with 'station'
		if station:
			records = list(filter(lambda rec: station in [rec['sta1'], rec['sta2']], self.records))
		else:
			records = self.records

		# Sort records b SNR
		records = sorted(records, key=lambda rec: rec['SNR'], reverse=True)
		if len(records) > 0:
			rec = records[0]

		return rec

	def append(self, rec):
		"""
		Append fri-file record to the end of the list

		"""
		self.records.append(rec)

	def __str__(self):
		out = "#Obs Timecode   Source      Sta1/Sta2         SNR    Delay    \
Rate      Accel      Base   Base   PA    Dur    Ampl\n"
		out = out + "#    ddd-hhmm                                          us\
     s/s       s/s^2      Mlam    ED    deg    s\n"
		for rec in self.records:
			if 'accel' in rec:
				if rec['FRIB.FINE_SEARCH'] == 'ACC':
					accel = rec['ph_acc']
				else:
					accel = rec['accel']
			else:
				accel = 0

			line = '{:>3}{:>10} {:>8} {:>8}/{:>8} {:8.2f} \
{:8.3f} {:10.3e} {:9.2e} {:8.2f} {:5.1f} {:6.1f} {:6.1f} {:8.2e}\n'.\
				format(rec['obs'], rec['time_code'], rec['source'],
					   rec['sta1'], rec['sta2'], rec['SNR'],
					   1e6 * rec['delay'], rec['rate'], accel,
					   1e-6 * rec['uv_rad'], rec['uv_rad_ed'], rec['PA'],
					   rec['duration'], rec['ampl_lsq'])
			out = out + line

		return out

	def __iter__(self):
		return self

	def __getitem__(self, ind):
		return self.records[ind]

	def __next__(self):
		if self.index == len(self.records):
			raise StopIteration
		self.index = self.index + 1
		return self.records[self.index - 1]

	def __len__(self):
		return len(self.records)


def getKey(s):
	"""Return key part of DiFX .im file type "key: option" string"""
	return s.split(':')[0].strip()


def getOpt(s):
	"""Return option part of DiFX .im file type "key: option" string"""
	return s.split(':')[1].strip()


def getKeyOpt(s):
	return (getKey(s),getOpt(s))


def imGetLineWith(lines,nstart,keys):
	"""
	Looks for the next line containing all key(s).
	Returns (linenr,linecontent).
	"""
	if not(type(keys) is list):
		keys = [keys]
	n = nstart
	while n < len(lines):
		if all([k in lines[n] for k in keys]):
			return (n,lines[n])
		n += 1
	return (len(lines),'')


def imDetectNextScanblock(lines,nstart):
	"""
	Look for next SCAN block in IM file lines, starting from line nr 'nstart'.
	Returns (istart,istop,srcname,mjd,sec) of the next scan block, or None.
	"""
	n,srcline = imGetLineWith(lines,nstart,['SCAN','POINTING SRC'])
	if (n + 6) >= len(lines):
		return None

	n,mjdline = imGetLineWith(lines,n,['POLY','MJD'])
	iscanstart = n
	n,secline = imGetLineWith(lines,n,['POLY','SEC'])

	srcname = getOpt(srcline)
	mjd = int(getOpt(mjdline))
	sec = int(getOpt(secline))

	iscanstop,tmp = imGetLineWith(lines,n,['SCAN','POINTING SRC'])
	return (iscanstart,iscanstop,srcname,mjd,sec)


def imDetectNextScanpolyblock(lines,nstart,nstop):
	"""
	Looks for next "SCAN <n> POLY <m>" block in file lines, starting from line nr 'nstart'.
	Returns (istart,istop,mjd,sec) of the next scan poly block, or None.
	"""
	mjd, sec = 0, 0
	n,mjdline = imGetLineWith(lines,nstart,['POLY','MJD'])
	if n >= nstop:
		return None
	n,secline = imGetLineWith(lines,n,['POLY','SEC'])
	mjd = int(getOpt(mjdline))
	sec = int(getOpt(secline))

	n += 1
	ipolystart = n
	while n < len(lines) and n < nstop:
		if 'SCAN' in lines[n] and ('POINTING SRC' in lines[n] or 'POLY' in lines[n]):
			break
		n += 1
	ipolystop = n
	blk = (ipolystart,ipolystop,mjd,sec)
	return blk


def imApplyPolyCoeffs(telescope_id,lines,polystart,polystop,dpoly,uvwpoly,sign=-1,baseOffsets=None,polyEndDelays=None):
	N_updated = 0
	map_tag_to_poly = {
		# line identifier                      storage   axis   type     linecount
		('ANT %d DELAY (us)' % telescope_id): [dpoly,    0,     'T',     0],
		('ANT %d U (m)' % telescope_id):      [uvwpoly,  0,     'uvw',   0],
		('ANT %d V (m)' % telescope_id):      [uvwpoly,  1,     'uvw',   0],
		('ANT %d W (m)' % telescope_id):      [uvwpoly,  2,     'uvw',   0]
	}

	for n in range(polystart,polystop):
		for tag in map_tag_to_poly:

			if not tag in lines[n]:
				continue

			P, col, type, linecount = map_tag_to_poly[tag]
			map_tag_to_poly[tag][-1] += 1
			if not P:
				continue

			C = [ pp[col] for pp in P.coeffs ]
			key,oldcoeffs = getKeyOpt(lines[n])
			if 'SRC' in key:
				sourcenr = int(key.split(' ')[1])
			else:
				sourcenr = None
			oldcoeffs = [float(v) for v in oldcoeffs.split('\t')]
			newcoeffs = list(oldcoeffs)

			# Element wise copy of new coeffs into polynomial
			N = min(len(newcoeffs), len(C)) # truncate to either poly
			if type=='uvw':

				# UVW poly: copy
				#for k in range(N):
				#	newcoeffs[k] = C[k]

				# UVW poly: might also simply not copy! units, coordinate system, something is different between CALC9 and ASC (huge difference in respective polys)
				pass

			elif type=='T':

				# Delay poly: mostly copy

				# Keep the CALC9 0th coeff but adjust for ASC poly drift and user delta-rate drift
				if baseOffsets==None:
					baseOffsets = {}
				if polyEndDelays==None:
					polyEndDelays = {}

				#if sourcenr not in baseOffsets:
				#	# Remember starting points of CALC9 and ASC dly offsets
				#	polydlydelta = P.eval(float(P.interval))[0] - P.eval(0.0)[0]
				#	baseOffsets[sourcenr] = [oldcoeffs[0], polydlydelta]
				#	newcoeffs[0] = oldcoeffs[0]
				#else:
				#	# Adjust CALC9 dly offset to be contiguous across different polys
				#	#newcoeffs[0] = oldcoeffs[0]  # old method, copy 1:1, will introduce clock jumps!
				#	newcoeffs[0] = baseOffsets[sourcenr][0] + sign*baseOffsets[sourcenr][1]
				#	polydlydelta = P.eval(float(P.interval))[0] - P.eval(0.0)[0]
				#	baseOffsets[sourcenr] = [newcoeffs[0], polydlydelta]

				if sourcenr not in baseOffsets:
					# Remember starting points of CALC9 and ASC dly offsets
					baseOffsets[sourcenr] = sign*C[0] - oldcoeffs[0]
					polyEndDelays[sourcenr] = oldcoeffs[0] + (P.eval(float(P.interval))[0] - P.eval(0.0)[0])
					newcoeffs[0] = oldcoeffs[0]
				else:
					# Adjust CALC9 dly offset to be contiguous across different polys
					#newcoeffs[0] = oldcoeffs[0]  # old method, copy 1:1, will introduce clock jumps! CALC9 delays do not connect with ASC delays
					if P.shifted:
						# Time shifted poly that is 'extrapolated' to outside the original range is likely to
						# have a starting delay that is not aligned with the end of the earlier poly.
						# Especially if the shift was large, the delay model will be poor.
						# Make matters better (or worse!) by forcing shifted poly to start from end of previous poly
						newcoeffs[0] = sign*polyEndDelays[sourcenr]
					else:
						newcoeffs[0] = sign*C[0] - baseOffsets[sourcenr]  # use new coeff, but shifted to start from very first CALC9 delay

				polyEndDelays[sourcenr] = sign*newcoeffs[0] + (P.eval(float(P.interval))[0] - P.eval(0.0)[0])

				# Copy high-order ASC poly coeffs
				for k in range(1,N):
					newcoeffs[k] = sign*C[k]

			newcoeffs_str = ' '.join(['%.16e\t ' % v for v in newcoeffs])
			newline = '%s:  %s' % (key.strip(),newcoeffs_str)

			lines[n] = newline
			N_updated += 1

	# print (map_tag_to_poly)

	return N_updated,baseOffsets,polyEndDelays


def areTimerangesMatched(startA, stopA, startB, stopB, granularity_secs=1.024):
	'''
	Compare datetime objects of two time ranges.
	Returns True if the time ranges roughly match, namely,
	if timerange A resides inside timerange B (or vice versa)
	at a given coarse granularity.
	'''
	t0 = startA - timedelta(microseconds=granularity_secs*1e6)
	t1 = stopA + timedelta(microseconds=granularity_secs*1e6)
	if (startB >= t0 and stopB <= t1):
		return True
	t0 = startB - timedelta(microseconds=granularity_secs*1e6)
	t1 = stopB + timedelta(microseconds=granularity_secs*1e6)
	if (startA >= t0 and stopA <= t1):
		return True
	return False


def patchPima2Dly(dlypolys, dlyfilename, frifilename, antname_pima='RADIO-AS', refant_pima='GBT-VLBA', outfilename=None):
	'''
	Read a PIMA fringe-fit file and incorporate the residual rate and acceleration
	into data from an ASC delay polynomial file. Store the result in a new file.
	'''

	# Check delay poly
	if len(dlypolys) < 1:
		print ("Error: no delay polynomials in '%s'" % (dlyfilename))
		sys.exit(-1)

	# Load PIMA fringe fit data
	try:
		fri = Fri(frifilename)
	except IOError as err:
		print("IOError: ", err, file=sys.stderr)
		return 1

	# Patch delay polys with PIMA residuals
	for poly in dlypolys.piecewisePolys:
		for rec in fri.records:
			st = [rec['sta1'], rec['sta2']]
			if not(antname_pima in st and refant_pima in st):
				continue
			if not areTimerangesMatched(poly.tstart, poly.tstop, rec['start_time'], rec['stop_time']):
				continue
			if refant_pima in rec['sta1']:
				sign = +1
			else:
				sign = -1
			if 'accel' in rec:
				if rec['FRIB.FINE_SEARCH'] == 'ACC':
					accel = rec['ph_acc']
				else:
					accel = rec['accel']
			else:
				accel = 0
			pimaresiduals_usec = [0, sign*rec['rate']*1e6, sign*accel*1e6]
			poly.add(pimaresiduals_usec)
			print ('Applying PIMA residuals of %s--%s to poly time %s + %d sec' % (rec['start_time'],rec['stop_time'],str(poly.tstart),poly.interval))

	# Output file name
	if not outfilename:
		rev = 0
		base = dlyfilename
		if '.rev' in dlyfilename:
			base,rev = dlyfilename.split('.rev')
		outfilename = '%s.rev%d' % (base, int(rev)+1)

	# Dump the updated polys into the output file	
	fo = open(outfilename, 'w')
	fo.write('telescope = RASTRON\n')
	fo.write('order = %d\n\n' % (dlypolys.order+1))
	for poly in dlypolys.piecewisePolys:
		fo.write('source = %s\n' % (poly.source))
		fo.write('start = %s\n' % (poly.timeToRaStr(poly.tstart)))
		fo.write('stop = %s\n' % (poly.timeToRaStr(poly.tstop)))
		for n in range(len(poly.coeffs)):
			fo.write('P%d = %.16e\n' % (n, poly.coeffs[n][0] / SCALE_DELAY))

	print ('Wrote %s' % (outfilename))


def patchImFile(basename, dlypolys, uvwpolys, antname='GT', allowTimeShift=False):
	if basename.endswith(('.difx','input','.calc','.im')):
		basename = basename[:basename.rfind('.')]
	imname = basename + '.im'
	imoutname = basename + '.im.closedloop'

	lines = []
	with open(imname) as f:
		lines = f.readlines()
		lines = [l.strip() for l in lines]
	if len(lines)<16:
		print ('Error: problem loading a valid %s!' % (imname))
		return False

	print ("\n%s\n" % (imname))

	# Find Model start time
	# Note: difx mpifxcorr/trunk/src/model.cpp reads
	#		'START SECOND'+'MINUTE'+... of .im and .calc into a fractional "modelmjd"
	#       which apparently is the vlbi scan start time
	#       and apparently calc polynomials can start offsetted from the model
	im_start_sec = -1
	for line in lines:
		if 'START SECOND' in line:
			im_start_sec = float(line.split(':')[-1])
			break

	# Find IM poly start time, make sure no time-shift is necessary
	# Note: difx mpifxcorr/trunk/src/model.cpp
	#     reads 'SCAN <n> POLY <m> MJD' into integer  polystartmjd
	#     reads 'SCAN <n> POLY <m> SEC' into integer  polystartseconds
	im_poly_start_sec = -1
	for line in lines:
		if 'SCAN 0 POLY 0 SEC' in line:
			im_poly_start_sec = float(line.split(':')[-1]) # second of day
			break

	# Find telescope, poly order, poly interval
	telescope_id = None
	im_poly_order = 0
	im_poly_interval_s = 0
	antname = antname.upper()
	for line in lines:
		# 'TELESCOPE 0 NAME:   GT'
		if 'TELESCOPE' in line and 'NAME' in line:
			[key,val] = getKeyOpt(line)
			if val.strip().upper() == antname:
				telescope_id = int(key.split()[1])
				break
		# 'POLYNOMIAL ORDER:   5'
		if 'POLYNOMIAL ORDER' in line:
			im_poly_order = int(getOpt(line))
		# 'INTERVAL (SECS):    120'
		if 'INTERVAL (SECS)' in line:
			im_poly_interval_s = int(getOpt(line))

	# Consistency check IM <-> RA coeffs
	# is_polyorder_mismatched = [poly.Ncoeffs > (im_poly_order+1) for poly in dlypolys.piecewisePolys + uvwpolys.piecewisePolys]
	is_polyorder_mismatched = [poly.Ncoeffs > (im_poly_order+1) for poly in dlypolys.piecewisePolys]
	is_polystartsec_mismatched = (im_poly_start_sec % im_poly_interval_s) != dlypolys.startSec
	# is_polyinterval_mismatched = [poly.interval < im_poly_interval_s for poly in dlypolys.piecewisePolys + uvwpolys.piecewisePolys]
	is_polyinterval_mismatched = [poly.interval < im_poly_interval_s for poly in dlypolys.piecewisePolys]
	if telescope_id == None:
		print ('Error: could not find telescope %s in %s' % (antname,imname))
		return False
	if any(is_polyorder_mismatched):
		print ('Warning: mismatch in polynomial order of .im file (%d) and closed-loop file (%d).' % (im_poly_order, poly.Ncoeffs-1))
	if any(is_polyinterval_mismatched):
		print ('Error: too long polynomial validity interval in .im file (%d sec) for appyling closed-loop polys (%d sec).' % (im_poly_interval_s, poly.interval))
		return False
	if is_polystartsec_mismatched:
		print ('Error: mismatch between .im base start time at second %d and poly start at second %d!' % (im_poly_start_sec, dlypolys.startSec))
		print ('Currently no support for polynomial time-shifting. Please edit these files and re-run calcif3 and patching:')
		print ('   .calc  file set START SECOND to 0 or %d sec multiple' % (im_poly_interval_s))
		print ('   .input file adjust START SECONDS to fall on 0 sec or %d sec boundary' % (im_poly_interval_s))
		print ('or alternatively edit')
		print ('   .vex   file adjust scan start=<start> to fall on 0 sec or %d sec boundary' % (im_poly_interval_s))
		print ('          and extend scan length by %d seconds' % (im_poly_start_sec % im_poly_interval_s))
		return False

	# Patch all relevant IM file lines
	nupdated_total = 0
	n = 0
	baseoffsets = None
	polyenddelays = None
	print ("Applying closed-loop coefficients to telescope %s with id %d" % (antname,telescope_id))
	while n < len(lines):

		# Find next 'SCAN' block
		blk = imDetectNextScanblock(lines,n)
		if blk == None:
			break
		(blkstart,blkstop,srcname,mjd,sec) = blk

		# Check all 'SCAN <n> POLY <m>' sections of the block
		pstart = blkstart
		while True:

			# Get next 'POLY <m>'
			blkpoly = imDetectNextScanpolyblock(lines,pstart,blkstop)
			if blkpoly == None:
				break
			(polystart,polystop,mjd,sec) = blkpoly

			# Get the matching Closed Loop polynomial coeffs sets
			dp = dlypolys.lookupPolyFor(mjd,sec,im_poly_interval_s,allowTimeShift)
			# uvwp = uvwpolys.lookupPolyFor(mjd,sec)
			if dp == None: # or uvwp == None:
				T = dlypolys.datetimeFromMJDSec(mjd,sec)
				print('Warning: No ASC poly matches DiFX MJD %d sec %d (%s)!' % (mjd,sec,str(T)))
				pstart = polystop
				continue
			# if dp.source != uvwp.source:
			# 	print("Error: RA delay poly source '%s' does not match UVW poly source '%s'!" % (dp.source,uvwp.source))
			if dp.source != srcname:
				print("Error: IM source '%s' does not match poly coeff set source '%s'!" % (srcname,dp.source))
				return False

			# Apply the coeffs
			nupdated,baseoffsets,polyenddelays = imApplyPolyCoeffs(telescope_id,lines,polystart,polystop,dp,None,baseOffsets=baseoffsets,polyEndDelays=polyenddelays)
			nupdated_total += nupdated

			# Next poly segment
			pstart = polystop

		# Next poly block
		#n = blkstop + 1
		n = blkstop

	# Write the new IM file
	f = open(imoutname, 'w')
	for line in lines:
		f.write(line + '\n')

	print ("Wrote new file '%s' with %d updated coefficient lines." % (imoutname,nupdated_total))
	return True


if __name__ == "__main__":

	for i, arg in enumerate(sys.argv):
		# Workaround from https://stackoverflow.com/questions/9025204/python-argparse-issue-with-optional-arguments-which-are-negative-numbers
		# required since --dt <time delta> argument can be negative
		if (arg[0] == '-') and arg[1].isdigit():
			sys.argv[i] = ' ' + arg

	args = parser.parse_args(sys.argv[1:])

	userresiduals = [0.0, float(args.ddlyrate)]

	if args.dlypolyfile == None:
		sys.exit(0)

	dly = PolySet(args.dlypolyfile, coeffscale=SCALE_DELAY)
	if len(dly) < 1:
		print ("Error: could not load delay polynomials from '%s'" % (args.dlypolyfile))
		sys.exit(-1)

	dly.add(userresiduals)
	dly.trunc(args.maxorder)
	dly.timeshift(args.dtshift)

	if args.frifile_pima:

		# Patch the poly .txt file and produce a new .txt.rev<N>
		patchPima2Dly(dly, args.dlypolyfile, args.frifile_pima, refant_pima=args.refant_pima)

	else:

		# Patch the .im file using poly
		for difxf in args.imfiles:
			ok = patchImFile(difxf, dly, None, args.antenna, args.autoTimeShift)
			if not ok:
				print ("Error: failed to patch %s" % (difxf))
