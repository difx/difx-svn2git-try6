#include <math.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <strings.h>
#include "config.h"
#include "difx2fits.h"
#include "other.h"

static int parseFlag(char *line, int refday, char *antname, float timerange[2], 
	char *reason, int *polId, int *bandId)
{
	int l;
	int n;

	n = sscanf(line, "%s%f%f%d%d%n", antname, timerange+0, timerange+1,
		polId, bandId, &l);

	if(n < 5)
	{
		return 0;
	}

	timerange[0] -= refday;
	timerange[1] -= refday;
	
	copyQuotedString(reason, line+n, 40);
	
	return 1;
}

const DifxInput *DifxInput2FitsFG(const DifxInput *D,
	struct fits_keywords *p_fits_keys, struct fitsPrivate *out)
{
	char bandFormInt[4];

	/*  define the flag FITS table columns */
	struct fitsBinTableColumn columns[] =
	{
		{"SOURCE_ID", "1J", "source id number from source tbl", 0},
		{"ARRAY", "1J", "????", 0},
		{"ANTS", "2J", "antenna id from antennas tbl", 0},
		{"FREQID", "1J", "freq id from frequency tbl", 0},
		{"TIMERANG", "2E", "time flag condition begins, ends", "DAYS"},
		{"BANDS", bandFormInt, "true if the baseband is bad", 0},
		{"CHANS", "2J", "channel range to be flagged", 0},
		{"PFLAGS", "4J", "flag array for polarization", 0},
		{"REASON", "40A", "reason for data to be flagged bad", 0},
		{"SEVERITY", "1J", "severity code", 0}
	};

	int nColumn;
	int n_row_bytes;
	char *fitsbuf, *p_fitsbuf;
	double start, stop;
	char line[1000];
	char reason[64], antname[10];
	int refday;
	int i, no_band;
	float timerange[2];
	int chans[2];
	int polId, bandId, baselineId[2], sourceId, freqId, arrayId;
	int severity;
	int polMask[4], bandMask[32];
	FILE *in;
	
	no_band = p_fits_keys->no_band;
	sprintf(bandFormInt, "%dJ", no_band);
	
	in = fopen("flag", "r");
	
	if(!in || D == 0)
	{
		return D;
	}

	nColumn = NELEMENTS(columns);
	n_row_bytes = FitsBinTableSize(columns, nColumn);

	/* calloc space for storing table in FITS format */
	if ((fitsbuf = (char *)calloc (n_row_bytes, 1)) == 0)
	{
		return 0;
	}

	fitsWriteBinTable(out, nColumn, columns, n_row_bytes, "FLAG");

	arrayWriteKeys (p_fits_keys, out);
	fitsWriteInteger(out, "TABREV", 2, "");
	fitsWriteEnd(out);

	start = D->mjdStart - (int)D->mjdStart;
	stop = start + D->duration/86400.0;

	mjd2dayno((int)(D->mjdStart), &refday);
	
	/* some constant values */
	sourceId = 0;
	freqId = 0;
	arrayId = 0;
	severity = -1;
	chans[0] = chans[1] = 0;
	polMask[2] = polMask[3] = 1;
	baselineId[1] = 0;
	
	for(;;)
	{
		fgets(line, 999, in);
		if(feof(in))
		{
			break;
		}
			
		/* ignore possible comment lines */
		if(line[0] == '#')
		{
			continue;
		}
		else if(parseFlag(line, refday, antname, timerange, 
			reason, &polId, &bandId))
		{
			if(strncmp(reason, "recorder", 8) == 0)
			{
				continue;
			}
			
			if(strcmp(reason, "observing system idle") == 0)
			{
				timerange[1] = 1.0;
			}
			
			if(timerange[0] > stop || timerange[1] < start)
			{
				continue;
			}
			
			if(timerange[0] < start)
			{
				timerange[0] = start;
			}
			if(timerange[1] > stop)
			{
				timerange[1] = stop;
			}

			baselineId[0] = DifxInputGetAntennaId(D, antname) + 1;
			if(baselineId[0] <= 0)
			{
				continue;
			}

			p_fitsbuf = fitsbuf;

			/* SOURCE_ID */
			bcopy((char *)&sourceId, p_fitsbuf, sizeof(sourceId));
			p_fitsbuf += sizeof(sourceId);

			/* ARRAY */
			bcopy((char *)&arrayId, p_fitsbuf, sizeof(arrayId));
			p_fitsbuf += sizeof(arrayId);

			/* ANTENNAS */
			bcopy((char *)baselineId, p_fitsbuf, 
				sizeof(baselineId));
			p_fitsbuf += sizeof(baselineId);

			/* FREQ_ID */
			bcopy((char *)&freqId, p_fitsbuf, sizeof(freqId));
			p_fitsbuf += sizeof(freqId);

			/* TIMERANGE */
			bcopy((char *)timerange, p_fitsbuf, sizeof(timerange));
			p_fitsbuf += sizeof(timerange);

			/* BANDS */
			if(bandId < 0)
			{
				for(i = 0; i < no_band; i++)
				{
					bandMask[i] = 1;
				}
			}
			else
			{
				for(i = 0; i < no_band; i++)
				{
					bandMask[i] = 0;
				}
				bandMask[bandId-1] = 0;
			}
			bcopy((char *)bandMask, p_fitsbuf, sizeof(int)*no_band);
			p_fitsbuf += sizeof(int)*no_band;

			/* CHANNELS */
			bcopy((char *)&chans, p_fitsbuf, sizeof(chans));
			p_fitsbuf += sizeof(chans);

			/* POLARIZATION FLAGS */
			if(polId < 0)
			{
				polMask[0] = 1;
				polMask[1] = 1;
			}
			else if(polId == 1)
			{
				polMask[0] = 1;
				polMask[1] = 0;
			}
			else
			{
				polMask[0] = 0;
				polMask[1] = 1;
			}
			bcopy((char *)polMask, p_fitsbuf, sizeof(polMask));
			p_fitsbuf += sizeof(polMask);

			/* REASON */
			bcopy(reason, p_fitsbuf, 40);
			p_fitsbuf += 40;

			/* SEVERITY */
			bcopy((char *)&severity, p_fitsbuf, sizeof(severity));
			p_fitsbuf += sizeof(severity);

#ifndef WORDS_BIGENDIAN	
			FitsBinRowByteSwap(columns, nColumn, fitsbuf);
#endif
			fitsWriteBinRow(out, fitsbuf);
		}
	}

	/* close the file, free memory, and return */
	fclose(in);
	free(fitsbuf);

	return D;
}
