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


DifxInput *mergeDifxInputs(const DifxInput *D1, const DifxInput *D2)
{
	DifxInput *D;
	int i;
	
	int *freqIdRemap = 0;
	int *antennaIdRemap = 0;
	int *baselineIdRemap = 0;
	int *datastreamIdRemap = 0;
	int *pulsarIdRemap = 0;
	int *configIdRemap = 0;

	if(!D1 || !D2)
	{
		return 0;
	}

	/* allocate some scratch space */
	freqIdRemap       = (int *)calloc(D2->nFreq, sizeof(int));
	antennaIdRemap    = (int *)calloc(D2->nAntenna, sizeof(int));
	baselineIdRemap   = (int *)calloc(D2->nBaseline, sizeof(int));
	datastreamIdRemap = (int *)calloc(D2->nDatastream, sizeof(int));
	if(D2->nPulsar > 0)
	{
		pulsarIdRemap = (int *)calloc(D2->nPulsar, sizeof(int));
	}
	configIdRemap     = (int *)calloc(D2->nConfig, sizeof(int));

	/* merge DifxFreq table */
	D->freq = mergeDifxFreqArrays(D1->freq, D1->nFreq,
		D2->freq, D2->nFreq, freqIdRemap);
	D->nFreq = D1->nFreq;
	for(i = 0; i < D2->nFreq; i++)
	{
		if(freqIdRemap[i]+1 > D->nFreq)
		{
			D->nFreq = freqIdRemap[i]+1;
		}
	}

	/* merge DifxAntenna table */
	D->antenna = mergeDifxAntennaArrays(D1->antenna, D1->nAntenna,
		D2->antenna, D2->nAntenna, antennaIdRemap);
	D->nAntenna = D1->nAntenna;
	for(i = 0; i < D2->nAntenna; i++)
	{
		if(antennaIdRemap[i]+1 > D->nAntenna)
		{
			D->nAntenna = antennaIdRemap[i]+1;
		}
	}

	/* merge DifxBaseline table */
	D->baseline = mergeDifxBaselineArrays(D1->baseline, D1->nBaseline,
		D2->baseline, D2->nBaseline, baselineIdRemap);
	D->nBaseline = D1->nBaseline;
	for(i = 0; i < D2->nBaseline; i++)
	{
		if(baselineIdRemap[i]+1 > D->nBaseline)
		{
			D->nBaseline = baselineIdRemap[i]+1;
		}
	}

	/* merge DifxDatastream table */
	D->datastream = mergeDifxDatastreamArrays(D1->datastream, 
		D1->nDatastream, D2->datastream, D2->nDatastream,
		datastreamIdRemap, freqIdRemap, antennaIdRemap);
	D->nDatastream = D1->nDatastream;
	for(i = 0; i < D2->nDatastream; i++)
	{
		if(datastreamIdRemap[i]+1 > D->nDatastream)
		{
			D->nDatastream = datastreamIdRemap[i]+1;
		}
	}

	/* merge DifxPulsar table */
	/* FIXME -- pulsar table does not yet exist */

	/* merge DifxConfig table */


	/* clean up */
	free(freqIdRemap);
	free(antennaIdRemap);
	freq(baselineIdRemap);
	free(datastreamIdRemap);
	if(pulsarIdRemap)
	{
		free(pulsarIdRemap);
	}
	free(configIdRemap);

	return D;
}
