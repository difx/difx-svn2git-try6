#include <stdlib.h>
#include <sys/types.h>
#include <strings.h>
#include <sys/time.h>
#include "config.h"
#include "difx2fits.h"

#define array_N_POLY 6

double current_mjd()
{
	struct timeval t;
	const double MJD_UNIX0=40587.0; /* MJD at beginning of unix time */

	gettimeofday(&t, 0);
	
	return MJD_UNIX0 + (t.tv_sec + t.tv_usec*1.0e-6)/86400.0;
}
		
/* given four samples, calculate polynomial referenced to time of second one */
void calcPolynomial(double gpoly[array_N_POLY], 
	double a, double b, double c, double d,
	double deltaT)
{
	int i;

	for(i = 0; i < array_N_POLY; i++)
	{
		gpoly[i] = 0.0;
	}

	/* FIXME -- for now just assume linear! */
	a = d = 0.0; /* line to prevent compiler warning */

	/* don't convert to convert to seconds from nanosec */
	gpoly[0] = b * 1.0e-6;
	gpoly[1] = (c-b) * 1.0e-6 / deltaT;
}

const DifxInput *DifxInput2FitsML(const DifxInput *D,
	struct fits_keywords *p_fits_keys, struct fitsPrivate *out)
{
	char bandFormDouble[4];
	char bandFormFloat[4];

	struct fitsBinTableColumn columns[] =
	{
		{"TIME", "1D", "time of model start", "DAYS"},
		{"TIME_INTERVAL", "1E", "model interval", "DAYS"},
		{"SOURCE_ID", "1J", "source id from sources tbl", 0},
		{"ANTENNA_NO", "1J", "antenna number from antennas tbl", 0},
		{"ARRAY", "1J", "array id number", 0},
		{"FREQID", "1J", "frequency id number from frequency tbl", 0},
		{"I.FAR.ROT", "1E", "ionospheric faraday rotation", 
			"RAD/METER**2"},
		{"FREQ.VAR", bandFormFloat, "time variable freq. offset", "HZ"},
		{"PDELAY_1", bandFormDouble, "total phase delay at ref time", 
			"TURNS"},
		{"GDELAY_1", "6D", "total group delay at ref time", "SECONDS"},
		{"PRATE_1", bandFormDouble, "phase delay rate", "HZ"},
		{"GRATE_1", "6D", "group delay rate", "SEC/SEC"},
		{"DISP_1", "1E", "dispersive delay for polar.1", "SECONDS"},
		{"DDISP_1", "1E", "dispersive delay rate for polar. 1", 
			"SEC/SEC"},
		{"PDELAY_2", bandFormDouble, "total phase delay at ref time", 
			"TURNS"},
		{"GDELAY_2", "6D", "total group delay at ref time", "SECONDS"},
		{"PRATE_2", bandFormDouble, "phase delay rate", "HZ"},
		{"GRATE_2", "6D", "group delay rate", "SEC/SEC"},
		{"DISP_2", "1E", "dispersive delay for polar.2", "SECONDS"},
		{"DDISP_2", "1E", "dispersive delay rate for polar. 2", 
			"SEC/SEC"}
	};

	char *fitsbuf;		/* compose FITS file rows here */
	int nBand;
	int nColumn;
	int nRowBytes;
	char str[80];
	int a, i, j, k, p, s, antId;
	double ppoly[array_MAX_BANDS][array_N_POLY];
	double gpoly[array_N_POLY];
	double prate[array_MAX_BANDS][array_N_POLY];
	double grate[array_N_POLY];
	float freqVar[array_MAX_BANDS];
	float faraday;
	int configId, jobId;
	double time;
	float timeInt, polyInt;
	int nPol;
	char *p_fitsbuf;
	DifxScan *scan;
	DifxJob *job;
	DifxConfig *config;
	DifxModel *M;
	DifxPolyModel *P;
	float dispDelay;
	float dispDelayRate;
	double modelInc;
	double start;
	double deltat;
	double freq;
	int *skip;
	int skipped=0;
	int printed=0;
	/* 1-based indices for FITS */
	int32_t sourceId1, freqId1, arrayId1, antId1;
	double clockRate;

	if(D == 0)
	{
		return 0;
	}

	nBand = p_fits_keys->no_band;
	nPol = D->nPol;
  
	/* set FITS header to reflect number of bands in observation */
	sprintf(bandFormDouble, "%dD", array_N_POLY * nBand);  
	sprintf(bandFormFloat, "%dE", nBand);  
  
	/* determine size of records for FITS file */
	if(nPol == 2)
	{
		nColumn = NELEMENTS(columns);
	}
	else  /* don't populate last 6 columns if not full polar */
	{
		nColumn = NELEMENTS(columns) - 6;
	}
	nRowBytes = FitsBinTableSize(columns, nColumn);

	/* write "binary file extension description" to output file */
	fitsWriteBinTable(out, nColumn, columns, nRowBytes,
		     "INTERFEROMETER_MODEL");
  
	/* calloc space for storing table in FITS order */
	fitsbuf = (char *)calloc(nRowBytes, 1);
	if(fitsbuf == 0)
	{
		return 0;
	}
  
	/* write standard FITS header keywords and values to output file */
	arrayWriteKeys(p_fits_keys, out);
  
	/* and write some more keywords and values to output file */
	fitsWriteInteger(out, "TABREV", 2, "");
	fitsWriteInteger(out, "NO_POL", p_fits_keys->no_pol, "");
	fitsWriteFloat(out, "GSTIA0", 0.0, "");
	fitsWriteFloat(out, "DEGPDY", 0.0, "");
	mjd2fits(p_fits_keys->ref_date, str);
	fitsWriteString(out, "RDATE", str, "");
	mjd2fits((int)current_mjd(), str);
	fitsWriteString(out, "CDATE", str, ""); 
	fitsWriteInteger(out, "NPOLY", array_N_POLY, "");
	fitsWriteFloat(out, "REVISION", 1.00, "");

	fitsWriteEnd(out);

	arrayId1 = 1;

	/* some values that are always zero */
	dispDelay = 0.0;
	dispDelayRate = 0.0;
	faraday = 0.0;
	for(i = 0; i < nBand; i++)
	{
		freqVar[i] = 0.0;
	}

	skip = (int *)calloc(D->nAntenna, sizeof(int));

	for(s = 0; s < D->nScan; s++)
	{
	   scan = D->scan + s;
	   jobId = scan->jobId;
	   job = D->job + jobId;
	   configId = scan->configId;
	   config = D->config + configId;
	   freqId1 = config->freqId + 1;
	   sourceId1 = D->source[scan->sourceId].fitsSourceId + 1;

	   modelInc = job->modelInc;
	   timeInt = modelInc / 86400.0;
	   start = D->scan[s].mjdStart - (int)(D->mjdStart);
	   polyInt = job->polyInterval / 86400.0;
	   
	   if(scan->im) for(p = 0; p < scan->nPoly; p++)
	   {
	      printf("\n    Scan %d -- using polynomial model", s);
	      printed++;
	      for(a = 0; a < scan->nAntenna; a++)
	      {
	       	P = scan->im[a];
		antId = config->datastreamId[a];
	       
	        /* skip antennas with no model data */
		if(P == 0)
		{
		  if(skip[antId] == 0)
		  {
		    printf("\n    Warning : skipping antId %d", antId);
		    printed++;
		    skipped++;
		  }
		  skip[antId]++;
		  continue;
		}
	        time = P->mjd - (int)(job->mjdStart) + P->sec/86400.0;
		deltat = (P->mjd - job->mjdStart)*86400.0 + P->sec;

		p_fitsbuf = fitsbuf;
	       
	        for(k = 0; k < array_N_POLY; k++)
		{
		  gpoly[k] = P->delay[k] * 1.0e-6;
		}

		clockRate = D->antenna[antId].rate * 1.0e-6;

		gpoly[0] -= (D->antenna[antId].delay * 1.0e-6 +
			clockRate*deltat);
		gpoly[1] -= clockRate;		

		/* All others are derived from gpoly */
		for(k = 1; k < array_N_POLY; k++)
		{
			grate[k-1] = k*gpoly[k];
		}
		grate[array_N_POLY-1] = 0.0;
		
		for(j = 0; j < nBand; j++)
		{
			freq = config->IF[j].freq * 1.0e6;
			for(k = 0; k < array_N_POLY; k++)
			{
				/* FIXME -- is this right? */
				ppoly[j][k] = gpoly[k]*freq;
				prate[j][k] = grate[k]*freq;
			}
		}
		antId1 = antId + 1;

		FITS_WRITE_ITEM (time, p_fitsbuf);
		FITS_WRITE_ITEM (polyInt, p_fitsbuf);
		FITS_WRITE_ITEM (sourceId1, p_fitsbuf);
		FITS_WRITE_ITEM (antId1, p_fitsbuf);
		FITS_WRITE_ITEM (arrayId1, p_fitsbuf);
		FITS_WRITE_ITEM (freqId1, p_fitsbuf);
		FITS_WRITE_ITEM (faraday, p_fitsbuf);
		FITS_WRITE_ARRAY(freqVar, p_fitsbuf, nBand);

		for(i = 0; i < nPol; i++)
		{
			FITS_WRITE_ARRAY(ppoly[0], p_fitsbuf,
				nBand*array_N_POLY);
			FITS_WRITE_ARRAY(gpoly, p_fitsbuf, array_N_POLY);
			FITS_WRITE_ARRAY(prate[0], p_fitsbuf,
				nBand*array_N_POLY);
			FITS_WRITE_ARRAY(grate, p_fitsbuf, array_N_POLY);
			FITS_WRITE_ITEM (dispDelay, p_fitsbuf);
			FITS_WRITE_ITEM (dispDelayRate, p_fitsbuf);
		} /* Polar loop */

		testFitsBufBytes(p_fitsbuf - fitsbuf, nRowBytes, "ML");
      
#ifndef WORDS_BIGENDIAN
		FitsBinRowByteSwap(columns, nColumn, fitsbuf);
#endif
		fitsWriteBinRow(out, fitsbuf);
		
	      } /* end antenna loop for poly model */
	   } /* end poly model for loop */
	   else for(p = 0; p < scan->nPoint; p++)
	   {
	      time = start + timeInt*p;
	      
	      for(a = 0; a < scan->nAntenna; a++)
	      {
		M = scan->model[a];
		antId = config->datastreamId[a];

		/* skip antennas with no model data */
		if(M == 0)
		{
		  if(skip[antId] == 0)
		  {
		    printf("\n    Warning : skipping antId %d", antId);
		    printed++;
		    skipped++;
		  }
		  skip[antId]++;
		  continue;
		}

		p_fitsbuf = fitsbuf;
	       
		calcPolynomial(gpoly,
			-M[p-1].t, -M[p  ].t, -M[p+1].t, -M[p+2].t, modelInc);

		clockRate = D->antenna[antId].rate * 1.0e-6;

		gpoly[0] -= (D->antenna[antId].delay * 1.0e-6 +
			clockRate*job->modelInc*p);
		gpoly[1] -= clockRate;		

		/* All others are derived from gpoly */
		for(k = 1; k < array_N_POLY; k++)
		{
			grate[k-1] = k*gpoly[k];
		}
		grate[array_N_POLY-1] = 0.0;
		
		for(j = 0; j < nBand; j++)
		{
			freq = config->IF[j].freq * 1.0e6;
			for(k = 0; k < array_N_POLY; k++)
			{
				/* FIXME -- is this right? */
				ppoly[j][k] = gpoly[k]*freq;
				prate[j][k] = grate[k]*freq;
			}
		}
		antId1 = antId + 1;

		FITS_WRITE_ITEM (time, p_fitsbuf);
		FITS_WRITE_ITEM (timeInt, p_fitsbuf);
		FITS_WRITE_ITEM (sourceId1, p_fitsbuf);
		FITS_WRITE_ITEM (antId1, p_fitsbuf);
		FITS_WRITE_ITEM (arrayId1, p_fitsbuf);
		FITS_WRITE_ITEM (freqId1, p_fitsbuf);
		FITS_WRITE_ITEM (faraday, p_fitsbuf);
		FITS_WRITE_ARRAY(freqVar, p_fitsbuf, nBand);

		for(i = 0; i < nPol; i++)
		{
			FITS_WRITE_ARRAY(ppoly[0], p_fitsbuf,
				nBand*array_N_POLY);
			FITS_WRITE_ARRAY(gpoly, p_fitsbuf, array_N_POLY);
			FITS_WRITE_ARRAY(prate[0], p_fitsbuf,
				nBand*array_N_POLY);
			FITS_WRITE_ARRAY(grate, p_fitsbuf, array_N_POLY);
			FITS_WRITE_ITEM (dispDelay, p_fitsbuf);
			FITS_WRITE_ITEM (dispDelayRate, p_fitsbuf);
		} /* Polar loop */

		testFitsBufBytes(p_fitsbuf - fitsbuf, nRowBytes, "ML");
      
#ifndef WORDS_BIGENDIAN
		FitsBinRowByteSwap(columns, nColumn, fitsbuf);
#endif
		fitsWriteBinRow(out, fitsbuf);
		
	     } /* Antenna loop */
	   } /* Intervals in scan loop (tabulated model) */
	} /* Scan loop */

  	if(printed)
	{
		printf("\n                            ");
	}
  
	/* release buffer space */
	free(fitsbuf);
	free(skip);

	return D;
}
