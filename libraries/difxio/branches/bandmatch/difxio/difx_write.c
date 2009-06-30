/***************************************************************************
 *   Copyright (C) 2008 by Walter Brisken                                  *
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
//===========================================================================
// SVN properties (DO NOT CHANGE)
//
// $Id$
// $HeadURL$
// $LastChangedRevision$
// $Author$
// $LastChangedDate$
//
//============================================================================

#include "difxio/difx_write.h"

static int mjd2date(long mjd, int *pYear, int *pMonth, int *pDay)
/*
 * RETURNS OK = 0 | ERROR = -1
 *
 * This function converts the given date to a year, month, and day.  If the 
 * given date does not fall between 0001JAN01 AD (MJD = -678,575) and 
 * 10000JAN00 AD (MJD = 2,973,483) ERROR is returned.
 */
{
/* 2,400,000 (difference between Julian Date and Modified Julian Date) 
   minus # days from jan 1, 4713 BC (beginning of Julian calendar) */
#define AD 678576

	static int monlen[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	int icen4, icen, iyr4, iyr, imon, iday;

	/* check input range and calc days since jan 1 1 AD (Gregorian Calendar) */
	if (mjd > 2973483)
	{
		return -1;
	}
	if ((mjd += AD - 1) < 0)
	{
		return -1;
	}
	/* calc number of fours of Gregorian centuries */
	icen4 = mjd / 146097;

	/* calc number of centuries since last 
	fours of Gregorian centuries (e.g. since 1600 or 2000) */
	mjd -= (icen4 * 146097);
	if ((icen = mjd / 36524) == 4)
	{
		icen = 3; 
	}

	/* calc number of quadrenia(four years) since jan 1, 1901 */
	mjd -= (icen * 36524);
	iyr4 = mjd / 1461;

	/* calc number of years since last quadrenia */
	mjd -= (iyr4 * 1461);
	if ((iyr = mjd / 365) == 4)
	{
		iyr = 3;
	}

	/* calc number of months, days since jan 1 of current year */
	iday = mjd - iyr * 365;
	for(imon = 0; iday >= 0; imon++)
	{
		iday = iday - monlen[imon] - ((iyr == 3 && imon == 1) ? 1 : 0);
	}
	imon--;		/* restore imon, iday to last loop value */
	iday = iday + monlen[imon] + ((iyr == 3 && imon == 1) ? 1 : 0);

	/* calc return values */
	*pYear = icen4 * 400 + icen * 100 + iyr4 * 4 + iyr + 1;
	*pMonth = imon + 1;
	*pDay = iday + 1;

	return 0;
}

int writeDifxLine(FILE *out, const char *key, const char *value)
{
	char line[1024];
	int i;

	for(i = 0; i < 32 && key[i]; i++)
	{
		line[i] = key[i];
	}
	line[i++] = ':';
	while(i < 20)
	{
		line[i++] = ' ';
	}
	value -= i;
	for(; i < 1023 && value[i]; i++)
	{
		line[i] = value[i];
	}
	line[i++] = 0;
	fprintf(out, "%s\n", line);

	return 0;
}

int writeDifxLine1(FILE *out, const char *key, int i1, const char *value)
{
	char k[128];

	sprintf(k, key, i1);
	
	return writeDifxLine(out, k, value);
}

int writeDifxLine2(FILE *out, const char *key, int i1, int i2, 
	const char *value)
{
	char k[128];

	sprintf(k, key, i1, i2);
	
	return writeDifxLine(out, k, value);
}

int writeDifxLine3(FILE *out, const char *key, int i1, int i2, int i3, 
	const char *value)
{
	char k[128];

	sprintf(k, key, i1, i2, i3);
	
	return writeDifxLine(out, k, value);
}

int writeDifxLineInt(FILE *out, const char *key, int value)
{
	char v[32];

	sprintf(v, "%d", value);

	return writeDifxLine(out, key, v);
}

int writeDifxLineInt1(FILE *out, const char *key, int i1, int value)
{
	char v[32];
	char k[128];

	sprintf(v, "%d", value);

	sprintf(k, key, i1);
	
	return writeDifxLine(out, k, v);
}

int writeDifxLineInt2(FILE *out, const char *key, int i1, int i2, int value)
{
	char v[32];
	char k[128];

	sprintf(v, "%d", value);

	sprintf(k, key, i1, i2);
	
	return writeDifxLine(out, k, v);
}

int writeDifxLineDouble(FILE *out, const char *key, const char *format, 
	double value)
{
	char v[32];

	if(!format)
	{
		format = "%f";
	}
	if(!format[0])
	{
		format = "%f";
	}

	sprintf(v, format, value);

	return writeDifxLine(out, key, v);
}

int writeDifxLineDouble1(FILE *out, const char *key, int i1, 
	const char *format, double value)
{
	char v[32];
	char k[128];

	if(!format)
	{
		format = "%f";
	}
	if(!format[0])
	{
		format = "%f";
	}

	sprintf(v, format, value);

	sprintf(k, key, i1);
	
	return writeDifxLine(out, k, v);
}

int writeDifxLineDouble2(FILE *out, const char *key, int i1, int i2,
	const char *format, double value)
{
	char v[32];
	char k[128];

	if(!format)
	{
		format = "%f";
	}
	if(!format[0])
	{
		format = "%f";
	}

	sprintf(v, format, value);

	sprintf(k, key, i1, i2);
	
	return writeDifxLine(out, k, v);
}

int writeDifxLineArray(FILE *out, const char *key, double *array, int n)
{
	char v[1024];
	int i, l;

	l = 0;
	for(i = 0; i < n; i++)
	{
		l += sprintf(v+l, "%22.15e\t", array[i]);
	}

	return writeDifxLine(out, key, v);
}

int writeDifxLineArray1(FILE *out, const char *key, int i1, double *array,
	int n)
{
	char k[128];

	sprintf(k, key, i1);
	
	return writeDifxLineArray(out, k, array, n);
}
int writeDifxLineArray2(FILE *out, const char *key, int i1, int i2,
	double *array, int n)
{
	char k[128];
	sprintf(k, key, i1, i2);
	return writeDifxLineArray(out, k, array, n);
}

int writeDifxDateLines(FILE *out, double mjd)
{
	printf("Writing out mjd %15.8f\n", mjd);
	int yr=0, mo=0, da=0, hr, mi, se;
	int mjdint;
	double mjdfrac;
	
	mjd += 1.0/100000000.0;
	mjdint = (int)mjd;
	mjd2date(mjdint, &yr, &mo, &da);
	mjdfrac = (mjd-mjdint)*24.0;
	hr = mjdfrac;
	mjdfrac -= hr;
	mjdfrac *= 60.0;
	mi = mjdfrac;
	mjdfrac -= mi;
	mjdfrac *= 60.0;
	se = mjdfrac;

	writeDifxLineInt(out, "START YEAR", yr);
	writeDifxLineInt(out, "START MONTH", mo);
	writeDifxLineInt(out, "START DAY", da);
	writeDifxLineInt(out, "START HOUR", hr);
	writeDifxLineInt(out, "START MINUTE", mi);
	writeDifxLineInt(out, "START SECOND", se);

	return 0;
}


