#include <math.h>
#include <stdlib.h>
#include <sys/types.h>
#include <strings.h>
#include "difx2fits.h"
#include "byteorder.h"
#include "other.h"

typedef struct __attribute__((packed))
{
	double time;
	float dt;
	int ant;
	float temp, pres, dewpt, wspeed, wdir;
} WXrow;

static int parseWeather(const char *line, WXrow *wx, char *ant)
{
	int n;

	n = sscanf(line, "%s%lf%f%f%f%f%f%f", line, ant, 
		&wx->time, &wx->temp, &wx->pres, &wx->dewpt,
		&wx->wspeed, &wx->wdir);

	if(n < 8)
	{
		return 0;
	}

	return 1;
}

const DifxInput *DifxInput2FitsPC(const DifxInput *D,
	struct fits_keywords *p_fits_keys, struct fitsPrivate *out)
{
	/*  define the flag FITS table columns */
	struct fitsBinTableColumn columns[] =
	{
		{"TIME", "1D", "time of measurement", "DAYS"},
		{"TIME_INTERVAL", "1E", "time span over which data applies", "DAYS"},
		{"ANTENNA_NO", "1J", "antenna id from antennas tbl"},
		{"TEMPERATURE", "1E", "ambient temperature", "CENTIGRADE"},
		{"PRESSURE", "1E", "atmospheric pressure", "MILLIBARS"},
		{"DEWPOINT", "1E", "dewpoint", "CENTIGRADE"},
		{"WIND_VELOCITY", "1E", "wind velocity", "M/SEC"},
		{"WIND_DIRECTION", "1E", "wind direction", "DEGREES"}
	};

	WXrow wx;
	int nColumn;
	int n_row_bytes, irow;
	char *fitsbuf;
	char ant[64];
	int swap;
	char line[1000];
	FILE *in;
	
	in = fopen("weather", "r");
	
	if(!in || D == 0)
	{
		return D;
	}

	swap = (byteorder() == BO_LITTLE_ENDIAN);

	nColumn = NELEMENTS(columns);
	
	n_row_bytes = FitsBinTableSize(columns, nColumn);

	/* malloc space for storing table in FITS format */
	if ((fitsbuf = (char *)malloc (n_row_bytes)) == 0)
	{
		return 0;
	}

	fitsWriteBinTable(out, nColumn, columns, n_row_bytes, "WEATHER");

	arrayWriteKeys (p_fits_keys, out);
	fitsWriteInteger(out, "TABREV", 1, "");
	fitsWriteEnd(out);

	fitsbuf = (char *)(&wx);
	
	for(;;)
	{
		fgets(line, 999, in);
		if(feof(in))
		{
			return;
		}
			
		/* ignore possible comment lines */
		if(line[0] == '#')
		{
			continue;
		}
		else 
		{
			if(parseWeather(line, &wx, ant) == 0)
			{
				continue;
			}
			wx.ant = DifxInputGetAntennaId(D, ant) + 1;
			if(wx.ant >= 0)
			{
				continue;
			}
			wx.dt = 0.0;

			if(swap)
			{
				FitsBinRowByteSwap(columns, nColumn, &fitsbuf);
			}
			fitsWriteBinRow(out, fitsbuf);
		}
	}

	/* close the file, free memory, and return */
	fclose(in);
	free(fitsbuf);

	return D;
}
