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


/* FIXME -- add condition structure */
int areDifxInputsMergable(const DifxInput *D1, const DifxInput *D2)
{
	if(D1->specAvg != D2->specAvg ||
	   D1->nFFT != D2->nFFT ||
	   D1->startChan != D2->startChan ||
	   D1->nInChan != D2->nInChan ||
	   D1->nOutChan != D2->nOutChan)
	{
		return 0;
	}
	
	return 1;
}

static void printRemap(const char *name, const int *Remap, int n)
{
	int i;
	printf("%s Remap =", name);
	if(n == 0 || Remap == 0)
	{
		printf(" Null\n");
	}
	else
	{
		for(i = 0; i < n; i++)
		{
			printf(" %d", Remap[i]);
		}
		printf("\n");
	}
}

DifxInput *mergeDifxInputs(const DifxInput *D1, const DifxInput *D2)
{
	DifxInput *D;
	int *jobIdRemap = 0;
	int *freqIdRemap = 0;
	int *antennaIdRemap = 0;
	int *datastreamIdRemap = 0;
	int *baselineIdRemap = 0;
	int *pulsarIdRemap = 0;
	int *configIdRemap = 0;
	int *spacecraftIdRemap = 0;

	if(!D1 || !D2)
	{
		return 0;
	}

	/* allocate some scratch space */
	jobIdRemap        = (int *)calloc(D2->nJob, sizeof(int));
	freqIdRemap       = (int *)calloc(D2->nFreq, sizeof(int));
	antennaIdRemap    = (int *)calloc(D2->nAntenna, sizeof(int));
	datastreamIdRemap = (int *)calloc(D2->nDatastream, sizeof(int));
	baselineIdRemap   = (int *)calloc(D2->nBaseline, sizeof(int));
	if(D2->nPulsar > 0)
	{
		pulsarIdRemap = (int *)calloc(D2->nPulsar, sizeof(int));
	}
	configIdRemap     = (int *)calloc(D2->nConfig, sizeof(int));
	if(D2->nSpacecraft > 0)
	{
		spacecraftIdRemap = (int *)calloc(D2->nSpacecraft, sizeof(int));
	}

	/* allocate the big D */
	D = newDifxInput();

	/* copy over / merge some of DifxInput top level parameters */
	D->specAvg = D1->specAvg;
	D->nFFT = D1->nFFT;
	D->startChan = D1->startChan;
	D->nInChan = D1->nInChan;
	D->nOutChan = D1->nOutChan;
	if(D1->mjdStart < D2->mjdStart)
	{
		D->mjdStart = D1->mjdStart;
	}
	else
	{
		D->mjdStart = D2->mjdStart;
	}
	if(D1->mjdStop > D2->mjdStop)
	{
		D->mjdStop = D1->mjdStop;
	}
	else
	{
		D->mjdStop = D2->mjdStop;
	}

	/* merge DifxJob table */
	D->job = mergeDifxJobArrays(D1->job, D1->nJob, D2->job, D2->nJob,
		jobIdRemap, &(D->nJob));

	/* merge DifxFreq table */
	D->freq = mergeDifxFreqArrays(D1->freq, D1->nFreq,
		D2->freq, D2->nFreq, freqIdRemap, &(D->nFreq));

	/* merge DifxAntenna table */
	D->antenna = mergeDifxAntennaArrays(D1->antenna, D1->nAntenna,
		D2->antenna, D2->nAntenna, antennaIdRemap, &(D->nAntenna));

	/* merge DifxDatastream table */
	D->datastream = mergeDifxDatastreamArrays(D1->datastream, 
		D1->nDatastream, D2->datastream, D2->nDatastream,
		datastreamIdRemap, freqIdRemap, antennaIdRemap,
		&(D->nDatastream));

	/* merge DifxBaseline table */
	D->baseline = mergeDifxBaselineArrays(D1->baseline, D1->nBaseline,
		D2->baseline, D2->nBaseline, baselineIdRemap,
		datastreamIdRemap, &(D->nBaseline));

	/* merge DifxPulsar table */
	D->pulsar = mergeDifxPulsarArrays(D1->pulsar, D1->nPulsar,
		D2->pulsar, D2->nPulsar, pulsarIdRemap, &(D->nPulsar));

	/* merge DifxConfig table */
	D->config = mergeDifxConfigArrays(D1->config, D1->nConfig, 
		D2->config, D2->nConfig, configIdRemap, 
		baselineIdRemap, datastreamIdRemap, pulsarIdRemap, 
		&(D->nConfig));

	/* merge DifxScan table */
	D->scan = mergeDifxScanArrays(D1->scan, D1->nScan, D2->scan, D2->nScan,
		jobIdRemap, antennaIdRemap, configIdRemap, &(D->nScan));

	/* merge DifxEOP table */
	D->eop = mergeDifxEOPArrays(D1->eop, D1->nEOP, D2->eop, D2->nEOP,
		&(D->nEOP));
	
	/* merge DifxSpacecraft table */
	D->spacecraft = mergeDifxSpacecraft(D1->spacecraft, D1->nSpacecraft,
		D2->spacecraft, D2->nSpacecraft,
		spacecraftIdRemap, &(D->nSpacecraft));

	/* merge DifxAntennaFlag table */
	D->flag = mergeDifxAntennaFlagArrays(D1->flag, D1->nFlag,
		D2->flag, D2->nFlag, antennaIdRemap, &(D->nFlag));

	/* print remappings */
	printRemap("jobId", jobIdRemap, D2->nJob);
	printRemap("freqId", freqIdRemap, D2->nFreq);
	printRemap("antennaId", antennaIdRemap, D2->nAntenna);
	printRemap("datastreamId", datastreamIdRemap, D2->nDatastream);
	printRemap("baselineId", baselineIdRemap, D2->nBaseline);
	printRemap("pulsarId", pulsarIdRemap, D2->nPulsar);
	printRemap("configId", configIdRemap, D2->nConfig);
	printRemap("spacecraftId", spacecraftIdRemap, D2->nSpacecraft);
	
	/* clean up */
	free(jobIdRemap);
	free(freqIdRemap);
	free(antennaIdRemap);
	free(datastreamIdRemap);
	free(baselineIdRemap);
	if(pulsarIdRemap)
	{
		free(pulsarIdRemap);
	}
	free(configIdRemap);
	if(spacecraftIdRemap)
	{
		free(spacecraftIdRemap);
	}

	return D;
}
