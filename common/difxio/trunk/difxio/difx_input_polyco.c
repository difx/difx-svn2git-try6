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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "difxio/difx_input.h"


DifxPolyco *newDifxPolycoArray(int nPolyco)
{
	DifxPolyco *dp;

	dp = (DifxPolyco *)calloc(nPolyco, sizeof(DifxPolyco));

	return dp;
}

void deleteDifxPolycoArray(DifxPolyco *dp, int nPolyco)
{
	int p;

	if(dp)
	{
		for(p = 0; p < nPolyco; p++)
		{
			if(dp[p].coef)
			{
				free(dp[p].coef);
			}
		}
		free(dp);
	}
}

void printDifxPolycoArray(const DifxPolyco *dp, int nPolyco)
{
	const DifxPolyco *p;
	int i, c;
	
	if(!dp)
	{
		return;
	}
	
	for(i = 0; i < nPolyco; i++)
	{
		p = dp + i;
		printf("      Polyco file = %s\n", p->fileName);
		printf("      D.M. = %f\n", p->dm);
		printf("      refFreq = %f\n", p->refFreq);
		printf("      mjd mid = %f\n", p->mjd);
		printf("      nBlk = %d\n", p->nBlk);
		printf("      P0 = %f turns\n", p->p0);
		printf("      F0 = %f Hz\n", p->f0);
		printf("      nCoef = %d\n", p->nCoef);
		for(c = 0; c < p->nCoef; c++)
		{
			printf("        %22.16e\n", p->coef[c]);
		}
	}
}

void copyDifxPolyco(DifxPolyco *dest, const DifxPolyco *src)
{
	int c;

	strcpy(dest->fileName, src->fileName);
	if(dest->coef)
	{
		free(dest->coef);
		dest->coef = 0;
		dest->nCoef = 0;
	}
	if(src->coef)
	{
		dest->nCoef = src->nCoef;
		dest->coef = (double *)malloc(dest->nCoef*sizeof(double));
		for(c = 0; c < dest->nCoef; c++)
		{
			dest->coef[c] = src->coef[c];
		}
	}
	dest->dm = src->dm;
	dest->refFreq = src->refFreq;
	dest->mjd = src->mjd;
	dest->nBlk = src->nBlk;
	dest->p0 = src->p0;
	dest->f0 = src->f0;
}

DifxPolyco *dupDifxPolycoArray(const DifxPolyco *src, int nPolyco)
{
	DifxPolyco *dp;
	int p;

	dp = newDifxPolycoArray(nPolyco);
	for(p = 0; p < nPolyco; p++)
	{
		copyDifxPolyco(dp + p, src + p);
	}

	return dp;
}

int loadPulsarPolycoFile(DifxPolyco *dp, const char *filename)
{
	FILE *in;
	char buffer[160];
	int r, c, len;

	strcpy(dp->fileName, filename);
	
	in = fopen(filename, "r");
	if(!in)
	{
		fprintf(stderr, "Cannot open %s for read\n", filename);
		return -1;
	}
	
	fgets(buffer, 159, in);
	if(feof(in))
	{
		fprintf(stderr, "Early EOF in %s\n", filename);
		fclose(in);
		return -1;
	}
	r = sscanf(buffer, "%*s%*s%*f%lf%lf", &dp->mjd, &dp->dm);
	if(r != 2)
	{
		fprintf(stderr, "Error parsing [%s] from %s\n",
			buffer, filename);
		fclose(in);
		return -1;
	}

	fgets(buffer, 159, in);
	if(feof(in))
	{
		fprintf(stderr, "Early EOF in %s\n", filename);
		fclose(in);
		return -1;
	}
	r = sscanf(buffer, "%*d%lf%lf%*d%d%d%lf", 
		&dp->p0, &dp->f0, &dp->nBlk, &dp->nCoef, &dp->refFreq);
	if(r != 5)
	{
		fprintf(stderr, "Error parsing [%s] from %s\n",
			buffer, filename);
		fclose(in);
		return -1;
	}

	if(dp->nCoef < 1 || dp->nCoef > 24)
	{
		fprintf(stderr, "Too many coefs(%d) in file %s\n",
			dp->nCoef, filename);
		fclose(in);
		return -1;
	}

	dp->coef = (double *)calloc(dp->nCoef, sizeof(double));

	for(c = 0; c < dp->nCoef; c++)
	{
		fscanf(in, "%s", buffer);
		if(feof(in))
		{
			fprintf(stderr, "Early EOF in %s\n", filename);
			fclose(in);
			return -1;
		}
		len = strlen(buffer);
		if(buffer[len-4] == 'D')
		{
			buffer[len-4] = 'e';
		}
		dp->coef[c] = atof(buffer);
	}
	
	// Correct for "FUT time" 
	if(dp->mjd < 20000.0)
	{
		dp->mjd += 39126;
	}
	
	fclose(in);

	return 0;
}
