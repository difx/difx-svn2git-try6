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


DifxDatastream *newDifxDatastreamArray(int nDatastream)
{
	DifxDatastream *ds;

	ds = (DifxDatastream *)calloc(nDatastream, sizeof(DifxDatastream));

	return ds;
}

void deleteDifxDatastreamArray(DifxDatastream *ds, int nDatastream)
{
	int e;

	if(ds)
	{
		for(e = 0; e < nDatastream; e++)
		{
			if(ds[e].nPol)
			{
				free(ds[e].nPol);
			}
			if(ds[e].freqId)
			{
				free(ds[e].freqId);
			}
			if(ds[e].clockOffset)
			{
				free(ds[e].clockOffset);
			}
			if(ds[e].RCfreqId)
			{
				free(ds[e].RCfreqId);
			}
			if(ds[e].RCpolName)
			{
				free(ds[e].RCpolName);
			}
		}
		free(ds);
	}
}

void printDifxDatastream(const DifxDatastream *ds)
{
	int f;
	printf("  Difx Datastream Entry[antennaId=%d] : %p\n", 
		ds->antennaId, ds);
	printf("    format = %s\n", ds->dataFormat);
	printf("    quantization bits = %d\n", ds->quantBits);
	printf("    nFreq = %d\n", ds->nFreq);
	printf("    nRecChan = %d\n", ds->nRecChan);
	printf("    (freqId, nPol)[freq] =");
	for(f = 0; f < ds->nFreq; f++)
	{
		printf(" (%d, %d)", ds->freqId[f], ds->nPol[f]);
	}
	printf("\n");
	printf("    (freqId, pol)[recchan] =");
	for(f = 0; f < ds->nRecChan; f++)
	{
		printf(" (%d, %c)", ds->RCfreqId[f], ds->RCpolName[f]);
	}
	printf("\n");
}

int isSameDifxDatastream(const DifxDatastream *dd1, const DifxDatastream *dd2,
	const int *freqIdRemap, const int *antennaIdRemap)
{
	int f, c;
	
	if(dd1->antennaId != antennaIdRemap[dd2->antennaId] ||
	   strcmp(dd1->dataFormat, dd2->dataFormat) != 0 ||
	   dd1->nFreq != dd2->nFreq ||
	   dd1->nRecChan != dd2->nRecChan)
	{
		return 0;
	}
	for(f = 0; f < dd1->nFreq; f++)
	{
		if(dd1->nPol[f] != dd2->nPol[f] ||
		   dd1->freqId[f] != freqIdRemap[dd2->freqId[f]])
		{
			return 0;
		}
	}
	for(c = 0; c < dd1->nRecChan; c++)
	{
		if(dd1->RCfreqId[c]  != freqIdRemap[dd2->RCfreqId[c]] ||
		   dd1->RCpolName[c] != dd2->RCpolName[c])
		{
			return 0;
		}
	}

	return 1;
}

void copyDifxDatastream(DifxDatastream *dest, const DifxDatastream *src,
	const int *freqIdRemap, const int *antennaIdRemap)
{
	int f, c;
	
	if(antennaIdRemap != 0)
	{
		dest->antennaId = antennaIdRemap[src->antennaId];
	}
	else
	{
		dest->antennaId = src->antennaId;
	}
	strcpy(dest->dataFormat, src->dataFormat);
	dest->quantBits = src->quantBits;
	dest->nFreq = src->nFreq;
	dest->nRecChan = src->nRecChan;
	dest->nPol = (int *)calloc(dest->nFreq, sizeof(int));
	dest->freqId = (int *)calloc(dest->nFreq, sizeof(int));
	dest->clockOffset = (double *)calloc(dest->nFreq, sizeof(double));
	dest->RCfreqId = (int *)calloc(dest->nRecChan, sizeof(int));
	dest->RCpolName = (char *)calloc(dest->nRecChan, sizeof(char));
	for(f = 0; f < dest->nFreq; f++)
	{
		dest->nPol[f] = src->nPol[f];
		if(freqIdRemap)
		{
			dest->freqId[f] = freqIdRemap[src->freqId[f]];
		}
		else
		{
			dest->freqId[f] = src->freqId[f];
		}
		dest->clockOffset[f] = src->clockOffset[f];
	}
	for(c = 0; c < dest->nRecChan; c++)
	{
		if(freqIdRemap)
		{
			dest->RCfreqId[c] = freqIdRemap[src->RCfreqId[c]];
		}
		else
		{
			dest->RCfreqId[c] = src->RCfreqId[c];
		}
		dest->RCpolName[c] = src->RCpolName[c];
	}
}

DifxDatastream *mergeDifxDatastreamArrays(const DifxDatastream *dd1, int ndd1,
	const DifxDatastream *dd2, int ndd2, int *datastreamIdRemap,
	const int *freqIdRemap, const int *antennaIdRemap)
{
	int ndd;
	int i, j;
	DifxDatastream *dd;

	ndd = ndd1;

	/* first identify entries that differ and assign new datastreamIds */
	for(j = 0; j < ndd2; j++)
	{
		for(i = 0; i < ndd1; i++)
		{
			if(isSameDifxDatastream(dd1 + i, dd2 + j,
				freqIdRemap, antennaIdRemap))
			{
				datastreamIdRemap[j] = i;
				break;
			}
		}
		if(i == ndd1)
		{
			datastreamIdRemap[j] = ndd;
			ndd++;
		}
	}

	dd = newDifxDatastreamArray(ndd);
	
	/* now copy dd1 */
	for(i = 0; i < ndd1; i++)
	{
		copyDifxDatastream(dd + i, dd1 + i, 0, 0);
	}

	/* now copy unique members of dd2 */
	for(j = 0; j < ndd2; j++)
	{
		if(datastreamIdRemap[j] >= ndd1)
		{
			copyDifxDatastream(dd + datastreamIdRemap[j], dd2 + j,
				freqIdRemap, antennaIdRemap);
		}
	}

	return dd;
}
