#include <math.h>
#include <stdlib.h>
#include <sys/types.h>
#include <strings.h>
#include "config.h"
#include "difx2fits.h"

const DifxInput *DifxInput2FitsMC(const DifxInput *D,
	struct fits_keywords *p_fits_keys, struct fitsPrivate *out)
{
	char bandFormFloat[4];

	struct fitsBinTableColumn columns[] =
	{
		{"TIME", "1D", "Time of center of interval", "DAYS"},
		{"SOURCE_ID", "1J", "source id from sources tbl", 0},
		{"ANTENNA_NO", "1J", "antenna id from antennas tbl", 0},
		{"ARRAY", "1J", "array id number", 0},
		{"FREQID", "1J", "freq id from frequency tbl", 0},
		{"ATMOS", "1D", "atmospheric group delay", "SECONDS"},
		{"DATMOS", "1D", "atmospheric group delay rate", "SEC/SEC"},
		{"GDELAY", "1D", "CALC geometric delay", "SECONDS"},
		{"GRATE", "1D", "CALC geometric delay rate", "SEC/SEC"},
		{"CLOCK_1", "1D", "electronic delay", "SECONDS"},
		{"DCLOCK_1", "1D", "electronic delay rate", "SEC/SEC"},
		{"LO_OFFSET_1", bandFormFloat, 
			"station lo_offset for polar. 1", "HZ"},
		{"DLO_OFFSET_1", bandFormFloat, 
			"station lo_offset rate for polar. 1", "HZ/SEC"},
		{"DISP_1", "1E", "dispersive delay", "SECONDS"},
		{"DDISP_1", "1E", "dispersive delay rate", "SEC/SEC"},
		{"CLOCK_2", "1D", "electronic delay", "SECONDS"},
		{"DCLOCK_2", "1D", "electronic delay rate", "SEC/SEC"},
		{"LO_OFFSET_2", bandFormFloat, 
			"station lo_offset for polar. 2", "HZ"},
		{"DLO_OFFSET_2", bandFormFloat, 
			"station lo_offset rate for polar. 2", "HZ/SEC"},
		{"DISP_2", "1E", "dispersive delay for polar 2", "SECONDS"},
		{"DDISP_2", "1E", "dispersive delay rate for polar 2", 
			"SEC/SEC"}
	};

	int nColumn;
 	int nRowBytes;
	char *p_fitsbuf, *fitsbuf;
	int nBand, nPol;
	int b, j, s, p, ant;
	int configId;
	float LOOffset[array_MAX_BANDS];
	float LORate[array_MAX_BANDS];
	float dispDelay;
	float dispDelayRate;
	double time;      
	double delay, delayRate;
	double atmosDelay, atmosRate;
	double clock, clockRate;
	/* 1-based indices for FITS file */
	int32_t antId1, arrayId1, sourceId1, freqId1;

	if(D == 0)
	{
		return 0;
	}

	nBand = p_fits_keys->no_band;
	sprintf (bandFormFloat, "%1dE", nBand);  
  
	nPol = D->nPol;
	if(nPol == 2)
	{
		nColumn = NELEMENTS(columns);
	}
	else	/* don't populate last 6 columns if not full polar */
	{
		nColumn = NELEMENTS(columns) - 6;
	}
	nRowBytes = FitsBinTableSize(columns, nColumn);

	/* calloc space for storing table in FITS order */
	fitsbuf = (char *)calloc(nRowBytes, 1);
	if(fitsbuf == 0)
	{
		return 0;
	}
  
	fitsWriteBinTable(out, nColumn, columns, nRowBytes, "MODEL_COMPS");
	arrayWriteKeys(p_fits_keys, out);
	fitsWriteInteger(out, "NO_POL", nPol, "");
	fitsWriteInteger(out, "FFT_SIZE", D->nOutChan*D->specAvg*2, "");
	fitsWriteInteger(out, "OVERSAMP", 0, "");
	fitsWriteInteger(out, "ZERO_PAD", 0, "");
	fitsWriteInteger(out, "FFT_TWID", 1, 
		"Version of FFT twiddle table used");
	fitsWriteString(out, "TAPER_FN", D->taperFunction, "");
	fitsWriteInteger(out, "TABREV", 1, "");
	
	fitsWriteEnd(out);
              
	for(b = 0; b < nBand; b++)
	{
		LOOffset[b] = 0.0;
		LORate[b] = 0.0;
	}
	dispDelay = 0.0;
	dispDelayRate = 0.0;

	arrayId1 = 1;
	for(s = 0; s < D->nScan; s++)
	{
	   configId = D->scan[s].configId;
	   freqId1 = D->config[configId].freqId + 1;
	   sourceId1 = D->scan[s].sourceId + 1;
	   for(p = 0; p < D->scan[s].nPoint; p++)
	   {
	      time = D->scan[s].mjdStart - (int)D->mjdStart + 
	      	D->modelInc*p/86400.0;
		
	      for(ant = 0; ant < D->scan[s].nAntenna; ant++)
	      {
	      	if(D->scan[s].model[ant] == 0)
		{
		  continue;
		}
		antId1 = ant + 1;
	        p_fitsbuf = fitsbuf;

		/* in general, convert from (us) to (sec) */
		atmosDelay = D->scan[s].model[ant][p].a  * 1.0e-6;
		atmosRate = D->scan[s].model[ant][p].da * 1.0e-6;
		/* here correct the sign of delay, and remove atmospheric
		 * portion of it. */
		delay = -D->scan[s].model[ant][p].t  * 1.0e-6 - atmosDelay;
		delayRate = -D->scan[s].model[ant][p].dt * 1.0e-6 - atmosRate;
		
		clockRate = D->antenna[ant].rate  * 1.0e-6;
		clock = D->antenna[ant].delay * 1.0e-6 + 
			clockRate*D->modelInc*p;
          
		FITS_WRITE_ITEM (time, p_fitsbuf);
		FITS_WRITE_ITEM (sourceId1, p_fitsbuf);
		FITS_WRITE_ITEM (antId1, p_fitsbuf);
		FITS_WRITE_ITEM (arrayId1, p_fitsbuf);
		FITS_WRITE_ITEM (freqId1, p_fitsbuf);
		FITS_WRITE_ITEM (atmosDelay, p_fitsbuf);
		FITS_WRITE_ITEM (atmosRate, p_fitsbuf);
		FITS_WRITE_ITEM (delay, p_fitsbuf);
		FITS_WRITE_ITEM (delayRate, p_fitsbuf);
	  
		for(j = 0; j < nPol; j++)
                {
			FITS_WRITE_ITEM (clock, p_fitsbuf);
			FITS_WRITE_ITEM (clockRate, p_fitsbuf);
			FITS_WRITE_ARRAY(LOOffset, p_fitsbuf, nBand);
			FITS_WRITE_ARRAY(LORate, p_fitsbuf, nBand);
			FITS_WRITE_ITEM (dispDelay, p_fitsbuf);
			FITS_WRITE_ITEM (dispDelayRate, p_fitsbuf);
		} /* Polar loop */

      		testFitsBufBytes(p_fitsbuf - fitsbuf, nRowBytes, "MC");
      
#ifndef WORDS_BIGENDIAN
		FitsBinRowByteSwap(columns, nColumn, fitsbuf);
#endif
		fitsWriteBinRow(out, fitsbuf);
		
	     } /* Antenna loop */
	   } /* Intervals in scan loop */
	} /* Scan loop */
  
	/* release buffer space */
	free(fitsbuf);

	return D;
}
