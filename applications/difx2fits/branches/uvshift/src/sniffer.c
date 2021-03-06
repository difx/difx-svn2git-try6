/***************************************************************************
 *   Copyright (C) 2008, 2009 by Walter Brisken                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/*===========================================================================
 * SVN properties (DO NOT CHANGE)
 *
 * $Id: sniffer.c 1417 2009-08-27 23:17:55Z WalterBrisken $
 * $HeadURL: https://svn.atnf.csiro.au/difx/applications/difx2fits/trunk/src/sniffer.c $
 * $LastChangedRevision: 1417 $
 * $Author: WalterBrisken $
 * $LastChangedDate: 2009-08-27 17:17:55 -0600 (Thu, 27 Aug 2009) $
 *
 *==========================================================================*/


#include <stdlib.h>
#include <string.h>
#include "sniffer.h"

typedef struct
{
	fftw_complex ***spectrum;	/* [BBC][Time][Chan] */
	int a1, a2, sourceId;
	double mjdStart, mjdMax;
	int nTime, nChan;
	int nBBC;
	double *weightSum;
	double *weightMin;
	double *weightMax;
	double *lastDump;
	int *nRec;
	int *isLSB;
} Accumulator;

struct _Sniffer
{
	FILE *apd;	/* amp, phase, dalay (, rate) file */
	FILE *apc;	/* amp, phase, chan (, rate) file */
	FILE *wts;
	FILE *acb;
	FILE *xcb;
	double solInt;			/* (sec) FFT interval */
	double bw;			/* (MHz) IF bandwidth */
	double deltaT;			/* (sec) grid spacing */
	double deltaF;			/* (MHz) grid spacing */
	const DifxInput *D;
	int nRec;			/* total records sniffed */
	int nPol, nStokes, nIF, nTime, nChan, nAntenna;
	int nComplex;
	int minInt;
	int configId;
	int fftOversample;
	fftw_plan plan1;
	fftw_plan plan2;
	fftw_complex *fftbuffer;
	int fft_nx, fft_ny;
	Accumulator **accum;
	int *fitsSourceId2SourceId;
};

void resetAccumulator(Accumulator *A)
{
	int i, t;

	for(i = 0; i < A->nBBC; i++)
	{
		for(t = 0; t < A->nTime; t++)
		{
			memset(A->spectrum[i][t], 0, 
				A->nChan*sizeof(fftw_complex));
		}
		A->weightMin[i] = 1000.0;
		A->weightMax[i] = 0.0;
		A->weightSum[i] = 0.0;
		A->nRec[i] = 0;
	}
	A->mjdStart = 0;
}

Accumulator *newAccumulatorArray(Sniffer *S, int n)
{
	Accumulator *A;
	int i, t, a;
	int nBBC;

	nBBC = S->nIF*S->nPol;

	A = (Accumulator *)calloc(n, sizeof(Accumulator));
	for(a = 0; a < n; a++)
	{
		A[a].nBBC = nBBC;
		A[a].nChan = S->nChan;
		A[a].nTime = S->nTime;
		A[a].spectrum = (fftw_complex ***)malloc(
			nBBC*sizeof(fftw_complex **));
		for(i = 0; i < nBBC; i++)
		{
			A[a].spectrum[i] = (fftw_complex **)malloc(
				S->nTime*sizeof(fftw_complex *));
			for(t = 0; t < A[a].nTime; t++)
			{
				A[a].spectrum[i][t] = (fftw_complex *)calloc(
					1, S->nChan*sizeof(fftw_complex));
			}
		}
		A[a].nRec = (int *)calloc(nBBC, sizeof(int));
		A[a].isLSB = (int *)calloc(nBBC, sizeof(int));
		A[a].weightSum = (double *)calloc(nBBC, sizeof(double));
		A[a].weightMin = (double *)calloc(nBBC, sizeof(double));
		A[a].weightMax = (double *)calloc(nBBC, sizeof(double));
		A[a].lastDump = (double *)calloc(S->D->nSource, sizeof(double));
		A[a].sourceId = -1;
	}

	return A;
}

void deleteAccumulatorArray(Accumulator *A, int n)
{
	int a, i, t;

	if(!A)
	{
		return;
	}

	for(a = 0; a < n; a++)
	{
		if(A[a].spectrum)
		{
			for(i = 0; i < A[a].nBBC; i++)
			{
				for(t = 0; t < A[a].nTime; t++)
				{
					free(A[a].spectrum[i][t]);
				}
				free(A[a].spectrum[i]);
			}
			free(A[a].spectrum);
			free(A[a].nRec);
			free(A[a].isLSB);
			free(A[a].weightSum);
			free(A[a].weightMin);
			free(A[a].weightMax);
			free(A[a].lastDump);
		}
	}

	free(A);
}

Sniffer *newSniffer(const DifxInput *D, int nComplex, 
	const char *filebase, double solInt)
{
	Sniffer *S;
	char filename[256];
	int a1, a2, c;
	double tMax = 0.0;
	FILE *log;
	int i, m;

	/* write summary to log file */
	sprintf(filename, "%s.log", filebase);
	log = fopen(filename, "w");
	fprintDifxInputSummary(log, D);
	fclose(log);

	for(c = 0; c < D->nConfig; c++)
	{
		if(D->config[c].tInt > tMax)
		{
			tMax = D->config[c].tInt;
		}
	}

	S = (Sniffer *)calloc(1, sizeof(Sniffer));

	m = 1;
	for(i = 0; i < D->nSource; i++)
	{
		if(D->source[i].fitsSourceId > m)
		{
			m = D->source[i].fitsSourceId;
		}
	}
	S->fitsSourceId2SourceId = (int *)malloc((m+1)*sizeof(int));
	for(i = 0; i <= m; i++)
	{
		S->fitsSourceId2SourceId[i] = -1;
	}
	for(i = 0; i < D->nSource; i++)
	{
		if(D->source[i].fitsSourceId >= 0)
		{
			S->fitsSourceId2SourceId[D->source[i].fitsSourceId] = i;
		}
	}

	S->deltaT = tMax;
	S->deltaF = D->chanBW;
	S->bw = D->chanBW;
	S->fftOversample = 3;

	S->nAntenna = D->nAntenna;
	S->D = D;
	S->nIF = D->nIF;
	S->nPol = D->nPol;
	S->nStokes = D->nPolar;
	S->nComplex = nComplex;
	S->configId = -1;
	S->nChan = D->nOutChan;
	S->nTime = solInt/tMax;
	if(S->nTime <= 1)
	{
		S->nTime = 1;
		fprintf(stderr, "\nWarning: sniffer interval is not long "
			"compared to integration time.\n");
		fprintf(stderr, "Changing to %f seconds.\n\n",
			tMax * S->nTime);
	}
	S->solInt = tMax * S->nTime;
	
	/* Open fringe fit files */
	sprintf(filename, "%s.apd", filebase);
	S->apd = fopen(filename, "w");
	if(!S->apd)
	{
		fprintf(stderr, "Cannot open %s for write\n", filename);
		deleteSniffer(S);
		return 0;
	}
	fprintf(S->apd, "obscode:  %s\n", D->job->obsCode);

	sprintf(filename, "%s.apc", filebase);
	S->apc = fopen(filename, "w");
	if(!S->apc)
	{
		fprintf(stderr, "Cannot open %s for write\n", filename);
		deleteSniffer(S);
		return 0;
	}
	fprintf(S->apc, "obscode:  %s\n", D->job->obsCode);

	/* Open weights file */
	sprintf(filename, "%s.wts", filebase);
	S->wts = fopen(filename, "w");
	if(!S->wts)
	{
		fprintf(stderr, "Cannot open %s for write\n", filename);
		deleteSniffer(S);
		return 0;
	}
	fprintf(S->wts, "PLOTWT summary: %s\n", D->job->obsCode);

	/* Open acband file */
	sprintf(filename, "%s.acb", filebase);
	S->acb = fopen(filename, "w");
	if(!S->acb)
	{
		fprintf(stderr, "Cannot open %s for write\n", filename);
		deleteSniffer(S);
		return 0;
	}

	/* Open xcband file */
	sprintf(filename, "%s.xcb", filebase);
	S->xcb = fopen(filename, "w");
	if(!S->xcb)
	{
		fprintf(stderr, "Cannot open %s for write\n", filename);
		deleteSniffer(S);
		return 0;
	}
	
	S->accum = (Accumulator **)malloc(
		S->nAntenna*sizeof(Accumulator *));
	for(a1 = 0; a1 < S->nAntenna; a1++)
	{
		S->accum[a1] = newAccumulatorArray(S, S->nAntenna);
		for(a2 = 0; a2 < S->nAntenna; a2++)
		{
			S->accum[a1][a2].a1 = a1;
			S->accum[a1][a2].a2 = a2;
		}
	}

	/* Prepare FFT stuff */

	S->fft_nx = S->fftOversample*S->nChan;
	S->fft_ny = S->fftOversample*S->nTime;
	S->fftbuffer = (fftw_complex*)fftw_malloc(S->fft_nx*S->fft_ny*
		sizeof(fftw_complex));
	S->plan1 = fftw_plan_many_dft(1, &(S->fft_ny), S->fft_nx,
		S->fftbuffer, 0,
		S->fft_nx, 1,
		S->fftbuffer, 0,
		S->fft_nx, 1,
		FFTW_FORWARD, FFTW_MEASURE);
	S->plan2 = fftw_plan_many_dft(1, &(S->fft_nx), S->fft_ny,
		S->fftbuffer, 0,
		1, S->fft_nx,
		S->fftbuffer, 0,
		1, S->fft_nx,
		FFTW_FORWARD, FFTW_MEASURE);

	return S;
}

void deleteSniffer(Sniffer *S)
{
	int a;

	if(S)
	{
		if(S->fitsSourceId2SourceId)
		{
			free(S->fitsSourceId2SourceId);
			S->fitsSourceId2SourceId = 0;
		}
		if(S->apd)
		{
			fclose(S->apd);
			S->apd = 0;
		}
		if(S->apc)
		{
			fclose(S->apc);
			S->apc = 0;
		}
		if(S->wts)
		{
			fclose(S->wts);
			S->wts = 0;
		}
		if(S->acb)
		{
			fclose(S->acb);
			S->acb = 0;
		}
		if(S->xcb)
		{
			fclose(S->xcb);
			S->xcb = 0;
		}
		if(S->accum)
		{
			for(a = 0; a < S->nAntenna; a++)
			{
				deleteAccumulatorArray(S->accum[a], 
					S->nAntenna);
			}
			free(S->accum);
		}
		if(S->fftbuffer)
		{
			free(S->fftbuffer);
		}
		if(S->plan1)
		{
			fftw_destroy_plan(S->plan1);
		}
		if(S->plan2)
		{
			fftw_destroy_plan(S->plan2);
		}
		free(S);
	}
}

double peakup(double peak[3], int i, int n, double w)
{
	double d, f;

	if(i >= n/2)
	{
		i -= n;
	}

	d = 2.0*peak[1]-peak[0]-peak[2];
	if(d <= 0.0)
	{
		f = i;
	}
	else
	{
		f = i + (peak[2]-peak[0])/(2.0*d);
	}

	return f/w;
}

static int dump(Sniffer *S, Accumulator *A, double mjd)
{
	int bbc, i, j, p, a1, a2, besti, bestj;
	fftw_complex **array;
	fftw_complex z;
	double amp2, max2, x, y;
	double amp, phase, delay, rate;
	double specAmp, specPhase, specRate;
	int specChan;
	double peak[3];
	double w;
	char startStr[32], stopStr[32];
	FILE *fp;
	const DifxConfig *config;
	const DifxIF *IF;
	char pol, side;
	double freq;
	int chan=1, b, f, t;
	int maxNRec = 0;

	if(A->sourceId < 0 || S->configId < 0)
	{
		return 0;
	}

	config = S->D->config + S->configId;

	a1 = A->a1;
	a2 = A->a2;

	for(b = 0; b < A->nBBC; b++)
	{
		if(A->nRec[b] > maxNRec)
		{
			maxNRec = A->nRec[b];
		}
	}

	/* dump XC/AC bandpass at most every 15 minutes each source,
	   and only if at least 1 IF has >= 75% valid records */
	if(A->mjdStart > A->lastDump[A->sourceId] + 15.0/1440.0 &&
	   maxNRec >= A->nTime*3/4)
	{
		A->lastDump[A->sourceId] = A->mjdStart;

		srvMjd2str(A->mjdStart, startStr);
		srvMjd2str(A->mjdMax, stopStr);

		if(a1 == a2)	/* Autocorrelation? */
		{
			fp = S->acb;
		}
		else			/* Cross corr? */
		{
			fp = S->xcb;
		}

		fprintf(fp, "timerange: %s %s obscode: %s chans: %d x %d\n",
			startStr, stopStr, S->D->job->obsCode,
			S->D->nOutChan, A->nBBC);
		fprintf(fp, "source: %s bandw: %6.3f MHz\n",
			S->D->source[A->sourceId].name, S->bw);
		for(i = 0; i < S->nIF; i++)
		{
			IF = config->IF + i;
			freq = IF->freq/1000.0;	/* freq in GHz */
			side = IF->sideband;
			for(p = 0; p < S->nPol; p++)
			{
				pol = IF->pol[p];
				fprintf(fp, "bandfreq: %9.6f GHz polar: %c%c side: %c bbchan: 0\n",
					freq, pol, pol, side);
			}
		}
		
		if(a1 == a2)	/* Autocorrelation? */
		{
			for(b = 0; b < A->nBBC; b++)
			{
				for(f = 0; f < A->nChan; f++)
				{
					z = 0.0;
					for(t = 0; t < A->nTime; t++)
					{
						z += A->spectrum[b][t][f];
					}
					z /= A->weightSum[b];
					fprintf(fp, "%2d %-3s %5d %7.5f\n",
						a1+1, S->D->antenna[a1].name,
						chan, creal(z));
					chan++;
				}
			}
		}
		else		/* Cross corr? */
		{
			for(b = 0; b < A->nBBC; b++)
			{
				for(f = 0; f < A->nChan; f++)
				{
					z = 0.0;
					for(t = 0; t < A->nTime; t++)
					{
						z += A->spectrum[b][t][f];
					}
					z /= A->weightSum[b];
					x = creal(z);
					y = cimag(z);
					fprintf(fp, "%2d %2d %-3s %-3s %5d %7.5f %8.3f\n",
						a1+1, a2+1, 
						S->D->antenna[a1].name,
						S->D->antenna[a2].name,
						chan, sqrt(x*x+y*y), 
						atan2(y, x)*180.0/M_PI);
					chan++;
				}
			}
		}
	}

	if(a1 == a2) /* Autocorrelation? */
	{
		/* weights file */
		fprintf(S->wts, "%5d %8.5f %2d %-3s %2d",
			(int)mjd, 24.0*(mjd-(int)mjd), A->a1+1,
			S->D->antenna[A->a1].name,
			A->nBBC);

		for(bbc = 0; bbc < A->nBBC; bbc++)
		{
			if(A->nRec[bbc] == 0)
			{
				w = 0.0;
			}
			else
			{
				w = A->weightSum[bbc]/A->nRec[bbc];
			}
			fprintf(S->wts, " %5.3f", w);
		}
		for(bbc = 0; bbc < A->nBBC; bbc++)
		{
			if(A->nRec[bbc] == 0)
			{
				w = 0.0;
			}
			else
			{
				w = A->weightMin[bbc];
			}
			fprintf(S->wts, " %5.3f", w);
		}
		for(bbc = 0; bbc < A->nBBC; bbc++)
		{
			if(A->nRec[bbc] == 0)
			{
				w = 0.0;
			}
			else
			{
				w = A->weightMax[bbc];
			}
			fprintf(S->wts, " %5.3f", w);
		}
		fprintf(S->wts, "\n");
	}

	else
	{
		/* fringe fit */

		fprintf(S->apd, "%5d %8.5f %2d %-10s %2d %2d %-3s %-3s %2d",
			(int)mjd, 24.0*(mjd-(int)mjd), A->sourceId+1,
			S->D->source[A->sourceId].name, a1+1, a2+1,
			S->D->antenna[a1].name,
			S->D->antenna[a2].name,
			A->nBBC);

		fprintf(S->apc, "%5d %8.5f %2d %-10s %2d %2d %-3s %-3s %2d",
			(int)mjd, 24.0*(mjd-(int)mjd), A->sourceId+1,
			S->D->source[A->sourceId].name, a1+1, a2+1,
			S->D->antenna[a1].name,
			S->D->antenna[a2].name,
			A->nBBC);

		for(bbc = 0; bbc < A->nBBC; bbc++)
		{
			if(A->nRec[bbc] < S->nTime/2 || 
			   A->weightSum[bbc] == 0.0)
			{
				fprintf(S->apd, " 0 0 0 0");
				fprintf(S->apc, " 0 0 0 0");
				continue;
			}
			array = A->spectrum[bbc];
			memset(S->fftbuffer, 0, 
				S->fft_nx*S->fft_ny*sizeof(fftw_complex));
			for(j = 0; j < A->nTime; j++)
			{
				for(i = 0; i < A->nChan; i++)
				{
					S->fftbuffer[j*S->fft_nx + i] = 
						array[j][i];
				}
			}

			/* First transform in time to form rates.  Here we do
			 * the spectral line sniffing to look for peak in 
			 * rate/chan space*/
			fftw_execute(S->plan1);
			max2 = 0.0;
			besti = bestj = 0;
			for(j = 0; j < S->fft_ny; j++)
			{
				for(i = 0; i < S->fft_nx; i++)
				{
					z = S->fftbuffer[j*S->fft_nx + i];
					amp2 = z*~z;
					if(amp2 > max2)
					{
						besti = i;
						bestj = j;
						max2 = amp2;
					}
				}
			}
			z = S->fftbuffer[bestj*S->fft_nx + besti];
			specAmp = sqrt(max2);
			specChan = besti;
			specPhase = (180.0/M_PI)*atan2(cimag(z), creal(z));
			
			peak[1] = specAmp;
			if(bestj == 0)
			{
				z = S->fftbuffer[(S->fft_ny-1)*S->fft_nx+besti];
			}
			else
			{
				z = S->fftbuffer[(bestj-1)*S->fft_nx + besti];
			}
			peak[0] = sqrt(z*~z);
			if(bestj == S->fft_ny-1)
			{
				z = S->fftbuffer[besti];
			}
			else
			{
				z = S->fftbuffer[(bestj+1)*S->fft_nx + besti];
			}
			peak[2] = sqrt(z*~z);
			specRate = peakup(peak, bestj, 
				S->fft_ny, S->solInt*S->fftOversample);

			/* Now do second axis of FFT -- in frequency to look for 
			 * a peak in rate/delay space */
			fftw_execute(S->plan2);

			max2 = 0.0;
			besti = bestj = 0;
			for(j = 0; j < S->fft_ny; j++)
			{
				for(i = 0; i < S->fft_nx; i++)
				{
					z = S->fftbuffer[j*S->fft_nx + i];
					amp2 = z*~z;
					if(amp2 > max2)
					{
						besti = i;
						bestj = j;
						max2 = amp2;
					}
				}
			}
			z = S->fftbuffer[bestj*S->fft_nx + besti];
			phase = (180.0/M_PI)*atan2(cimag(z), creal(z));
			amp = sqrt(max2);
			peak[1] = amp;
			if(besti == 0)
			{
				z = S->fftbuffer[(bestj+1)*S->fft_nx - 1];
			}
			else
			{
				z = S->fftbuffer[bestj*S->fft_nx + besti - 1];
			}
			peak[0] = sqrt(z*~z);
			if(besti == S->fft_nx-1)
			{
				z = S->fftbuffer[bestj*S->fft_nx];
			}
			else
			{
				z = S->fftbuffer[bestj*S->fft_nx + besti + 1];
			}
			peak[2] = sqrt(z*~z);
			delay = peakup(peak, besti, 
				S->fft_nx, S->bw*S->fftOversample/1000.0);
			if(bestj == 0)
			{
				z = S->fftbuffer[(S->fft_ny-1)*S->fft_nx+besti];
			}
			else
			{
				z = S->fftbuffer[(bestj-1)*S->fft_nx + besti];
			}
			peak[0] = sqrt(z*~z);
			if(bestj == S->fft_ny-1)
			{
				z = S->fftbuffer[besti];
			}
			else
			{
				z = S->fftbuffer[(bestj+1)*S->fft_nx + besti];
			}
			peak[2] = sqrt(z*~z);
			rate = peakup(peak, bestj, 
				S->fft_ny, S->solInt*S->fftOversample);

			/* correct for negative frequency axis if LSB */
			if(A->isLSB[bbc])
			{
				phase = -phase;
				rate = -rate;
				specPhase = -specPhase;
				specChan = S->fft_nx-1-specChan;
				specRate = -specRate;
			}

			fprintf(S->apd, " %10.4f %6.4f %10.4f %10.6f", 
				delay, 
				2.0*amp/(A->weightSum[bbc]*S->nChan), 
				phase, 
				rate);

			fprintf(S->apc, " %4d %6.4f %10.4f %10.6f", 
				specChan+1, 
				2.0*specAmp/(A->weightSum[bbc]*S->nChan), 
				specPhase, 
				specRate);
		}
		fprintf(S->apd, "\n");
		fprintf(S->apc, "\n");
	}

	return 0;
}

static int add(Accumulator *A, int bbc, int index, float weight, 
	const float *data, int stride, int isLSB)
{
	fftw_complex *array;
	complex float *z;
	int c;

	array = A->spectrum[bbc][index];
	for(c = 0; c < A->nChan; c++)
	{
		z = (complex float *)(data + c*stride);
		array[c] += (*z)*weight;
	}

	A->nRec[bbc]++;
	A->isLSB[bbc] = isLSB;
	A->weightSum[bbc] += weight;
	if(weight > A->weightMax[bbc])
	{
		A->weightMax[bbc] = weight;
	}
	if(weight < A->weightMin[bbc])
	{
		A->weightMin[bbc] = weight;
	}

	return A->nRec[bbc];
}

int feedSnifferFITS(Sniffer *S, const struct UVrow *data)
{
	double mjd;
	Accumulator *A;
	int a1, a2;
	int i, p;
	int configId, sourceId;
	float weight;
	int isLSB;
	int stride, offset, bbc, index;

	if(!S)
	{
		return 0;
	}

	if(data->sourceId1 < 1)
	{
		return 0;
	}

	sourceId = S->fitsSourceId2SourceId[data->sourceId1-1];
	if(sourceId < 0 || sourceId >= S->D->nSource)
	{
		return 0;
	}

	configId = S->D->source[sourceId].configId;
	if(configId < 0 || configId >= S->D->nConfig)
	{
		return 0;
	}

	if(configId != S->configId)
	{
		S->nIF = S->D->config[configId].nIF;
		S->nPol = S->D->config[configId].nPol;
		S->configId = configId;
	}

	mjd = data->jd - 2400000.5;
	mjd += data->iat;
	a1 = data->baseline/256 - 1;
	a2 = data->baseline%256 - 1;

	A = &(S->accum[a1][a2]);

	index = (mjd - A->mjdStart)/(S->deltaT/86400.0);

	if(index >= A->nTime || mjd > A->mjdMax || A->sourceId != sourceId)
	{
		dump(S, A, mjd);
		resetAccumulator(A);
		A->sourceId = sourceId;
	}
	if(A->mjdStart < 50000.0)
	{
		A->mjdStart = mjd - 0.5*S->deltaT/86400.0;
		A->mjdMax = A->mjdStart + S->solInt/86400.0;
	}

	index = (mjd - A->mjdStart)/(S->deltaT/86400.0);

	if(index < 0 || index >= A->nTime)
	{
		fprintf(stderr, "Developer Error: Sniffer: bad time index=%d a1=%d a2=%d A->nTime=%d mjd=%15.9f mjdStart=%15.9f\n", index, a1, a2, A->nTime, mjd, A->mjdStart);
		return -1;
	}

	stride = S->nComplex*S->nStokes;

	for(i = 0; i < S->nIF; i++)
	{
		isLSB = (S->D->config[configId].IF[i].sideband == 'L');

		for(p = 0; p < S->nPol; p++)
		{
			bbc = i*S->nPol + p;
			weight = data->data[p + S->nStokes*i];
			offset = S->nStokes*S->nIF + stride*S->nChan*i + p*S->nComplex;
			add(A, bbc, index, weight, data->data+offset, stride, isLSB);
		}
	}

	S->nRec++;

	return S->nRec;
}
