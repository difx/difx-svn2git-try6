#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "transient_wrapper_data.h"

#define min(x,y) (((x) < (y)) ? (x) : (y))
#define max(x,y) (((x) > (y)) ? (x) : (y))

TransientWrapperData *newTransientWrapperData()
{
	TransientWrapperData *T;

	T = (TransientWrapperData *)calloc(1, sizeof(TransientWrapperData));

	T->difxState = DIFX_STATE_SPAWNING;
	T->maxCopyOverhead = 0.04;

	return T;
}

void deleteTransientWrapperData(TransientWrapperData *T)
{
	if(T)
	{
		if(T->D)
		{
			deleteDifxInput(T->D);
		}

		if(T->filePrefix)
		{
			free(T->filePrefix);
		}
	}
}

void printTransientWrapperData(const TransientWrapperData *T)
{
	int e;

	printf("TransientWrapperData [%p]\n", T);
	if(T)
	{
		printf("  identifier = %s\n", T->identifier);
		printf("  rank = %d\n", T->rank);
		printf("  DifxState = %s [%d]\n", DifxStateStrings[T->difxState], T->difxState);
		printf("  outputPath = %s\n", T->outputPath);
		printf("  filePrefix = %s\n", T->filePrefix);
		printf("  monitorThreadDie = %d\n", T->monitorThreadDie);
		printf("  verbose = %d\n", T->verbose);
		printf("  doCopy = %d\n", T->doCopy);
		printf("  executeTime = %d\n", T->executeTime);
		printf("  maxCopyOverhead = %f\n", T->maxCopyOverhead);
		printf("  nTransient = %d\n", T->nTransient);
		printf("  nMerged = %d\n", T->nMerged);
		printf("  nEvent = %d\n", T->nEvent);
		for(e = 0; e < T->nEvent; e++)
		{
			printf("    event[%d] = [%12.6f,%12.6f], %f\n", e, 
				T->event[e].startMJD, T->event[e].stopMJD,
				T->event[e].priority);
		}
	}
}

/* Note this sorts on priority only and puts the _highest_ priority events first */
static int eventCompare(const void *p1, const void *p2)
{
	const TransientEvent *e1, *e2;

	e1 = (TransientEvent *)p1;
	e2 = (TransientEvent *)p2;

	if(e1->priority > e2->priority)
	{
		return -1;
	}
	else if(e1->priority == e2->priority)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

void sortEvents(TransientWrapperData *T)
{
	if(T->nEvent > 1)
	{
		qsort(T->event, T->nEvent, sizeof(TransientEvent), eventCompare);

		/* trim down list to maximum allowed if needed */
		if(T->nEvent > MAX_EVENTS)
		{
			T->nEvent = MAX_EVENTS;
		}
	}
}

void addEvent(TransientWrapperData *T, const DifxMessageTransient *transient)
{
	char message[DIFX_MESSAGE_LENGTH];
	int merged = 0;

	T->nTransient++;

	if(transient->startMJD > T->D->job->jobStop || transient->stopMJD < T->D->job->jobStart)
	{
		snprintf(message, DIFX_MESSAGE_LENGTH,
			"Transient received out of job time range ([%12.6f,%12.6f] not in [%12.6f,%12.6f])",
			transient->startMJD, transient->stopMJD,
			T->D->job->jobStart, T->D->job->jobStop);
		difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_WARNING);
	}
	else
	{
#if 0
		if(T->nEvent > 0)
		{
			printf("Cool.  Merging two events!\n");
			if(min(T->event[T->nEvent-1].stopMJD,  transient->stopMJD) >= 
			   max(T->event[T->nEvent-1].startMJD, transient->startMJD))
			{
				T->event[T->nEvent-1].startMJD = min(T->event[T->nEvent-1].startMJD, transient->startMJD);
				T->event[T->nEvent-1].stopMJD  = max(T->event[T->nEvent-1].stopMJD,  transient->stopMJD);
				T->event[T->nEvent-1].priority = sqrt(T->event[T->nEvent-1].priority*T->event[T->nEvent-1].priority +
					transient->priority*transient->priority);
			}
			merged = 1;
		}
#endif
		if(merged == 0)
		{
			if(T->nEvent >= MAX_EVENTS + EXTRA_EVENTS)
			{
				sortEvents(T);
			}

			T->event[T->nEvent].startMJD = transient->startMJD;
			T->event[T->nEvent].stopMJD  = transient->stopMJD;
			T->event[T->nEvent].priority = transient->priority;
			T->nEvent++;
		}
	}

	T->nMerged += merged;
}

static void genDifxFiles(const TransientWrapperData *T, int eventId)
{
	DifxInput *newD;
	int i, v;
	FILE *out;
	double mjd;
	double mjd1, mjd2;
	char outDir[DIFXIO_FILENAME_LENGTH];
	char baseName[DIFXIO_FILENAME_LENGTH];
	char fileName[DIFXIO_FILENAME_LENGTH];
	DifxScan *S;
	int scanId, configId, dsId, freqId;

	/* use center of range for scan id */
	mjd = (T->event[eventId].startMJD + T->event[eventId].stopMJD)/2.0;

	mjd1 = T->event[eventId].startMJD;
	mjd2 = T->event[eventId].stopMJD;

	mjd1 = floor(mjd1*86400.0)/86400.0;
	mjd2 = ceil(mjd2*86400.0)/86400.0;

	scanId = DifxInputGetScanId(T->D, mjd);
	if(scanId < 0 || scanId >= T->D->nScan)
	{
		fprintf(stderr, "Error: mjd=%f not claimed by any scan\n", mjd);
		
		return;
	}
	configId = T->D->scan[scanId].configId;
	if(configId < 0 || configId >= T->D->nConfig)
	{
		fprintf(stderr, "Error: configId=%d for scanId=%d\n", configId, scanId);

		return;
	}

	v = snprintf(outDir, DIFXIO_FILENAME_LENGTH, 
		"%s/%s%s/%s",
		T->outputPath, T->D->job->obsCode, T->D->job->obsSession, T->identifier);

	snprintf(baseName, DIFXIO_FILENAME_LENGTH,
		"%s/transient_%03d", outDir, eventId+1);

	newD = loadDifxInput(T->filePrefix);

	/* MODIFY THE CONTENTS TO MAKE A NEW JOB */

	/* First change the name of the job and all of the paths */
	snprintf(newD->job->inputFile,   DIFXIO_FILENAME_LENGTH, "%s.input", baseName);
	snprintf(newD->job->calcFile,    DIFXIO_FILENAME_LENGTH, "%s.calc", baseName);
	snprintf(newD->job->imFile,      DIFXIO_FILENAME_LENGTH, "%s.im", baseName);
	snprintf(newD->job->flagFile,    DIFXIO_FILENAME_LENGTH, "%s.flag", baseName);
	snprintf(newD->job->threadsFile, DIFXIO_FILENAME_LENGTH, "%s.threads", baseName);
	snprintf(newD->job->outputFile,  DIFXIO_FILENAME_LENGTH, "%s.difx", baseName);
	
	/* Then select the appropriate scan and reduce its timerange */
	S = newDifxScanArray(1);
	copyDifxScan(S, newD->scan+scanId, 0, 0, 0, 0);
	deleteDifxScanArray(newD->scan, newD->nScan);
	newD->scan = S;
	newD->nScan = 1;
	newD->mjdStart = newD->scan->mjdStart = newD->job->jobStart = newD->job->mjdStart = mjd1;
	newD->mjdStop  = newD->scan->mjdEnd   = newD->job->jobStop  = mjd2;
	newD->job->duration = (int)(86400.0*(mjd2-mjd1) + 0.00001);

	/* Then change all data sources to FILE and point to those files */
	for(dsId = 0; dsId < newD->nDatastream; dsId++)
	{
		newD->datastream[dsId].dataSource = DataSourceFile;
		if(newD->datastream[dsId].nFile == 1)
		{
			snprintf(fileName, DIFXIO_FILENAME_LENGTH,
				"%s/%d/%s_%12.6f_%12.6f_0", outDir, dsId+1, 
				newD->datastream[dsId].file[0],
				T->event[eventId].startMJD,
				T->event[eventId].stopMJD);
			free(newD->datastream[dsId].file[0]);
			newD->datastream[dsId].file[0] = strdup(fileName);
		}
	}

	/* Finally change correlation parameters */
	newD->config[configId].tInt = 0.000256;
	newD->config[configId].subintNS = 256000;
	for(freqId = 0; freqId <= newD->nFreq; freqId++)
	{
		newD->freq[freqId].nChan = 128;
		newD->freq[freqId].specAvg = 2;
	}
	DifxInputAllocThreads(newD, 2);
	DifxInputSetThreads(newD, 1);

	/* And write it! */
	writeDifxInput(newD);
	writeDifxCalc(newD);
	writeDifxIM(newD);
	DifxInputWriteThreads(newD);

	snprintf(fileName, DIFXIO_FILENAME_LENGTH, "%s.machines", baseName);
	out = fopen(fileName, "w");
	for(i = 0; i < newD->nDatastream+3; i++)
	{
		fprintf(out, "boom\n");
	}
	fclose(out);

	deleteDifxInput(newD);
}

int copyBasebandData(const TransientWrapperData *T)
{
	const unsigned int MaxCommandLength = 1024;
	char command[MaxCommandLength];
	char message[DIFX_MESSAGE_LENGTH];
	time_t t1, t2;
	char outDir[DIFXIO_FILENAME_LENGTH];
	int v, e;

	v = snprintf(outDir, DIFXIO_FILENAME_LENGTH, 
		"%s/%s%s/%s/%d",
		T->outputPath, T->D->job->obsCode, T->D->job->obsSession, T->identifier, T->rank);
	
	if(v >= DIFXIO_FILENAME_LENGTH)
	{
		fprintf(stderr, "Error: pathname is too long (%d vs. %d)\n",
			v, DIFXIO_FILENAME_LENGTH);
		
		return 0;
	}

	snprintf(command, MaxCommandLength, 
		"mkdir -p %s", outDir);
	v = system(command);
	if(v == -1)
	{
		snprintf(message, DIFX_MESSAGE_LENGTH, 
			"Error: cannot execute %s\n", command);
		fprintf(stderr, "%s\n", message);
		difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_ERROR);
		
		return 0;
	}

	t1 = t2 = time(0);
	for(e = 0; e < T->nEvent; e++)
	{
		if(t2-t1 > T->executeTime*T->maxCopyOverhead)
		{
			break;
		}

		snprintf(command, MaxCommandLength, 
			"mk5cp Active %12.6f_%12.6f %s", 
			T->event[e].startMJD, T->event[e].stopMJD,
			outDir);

		snprintf(message, DIFX_MESSAGE_LENGTH, "Executing: %s", command);
		difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_INFO);
		
		v = system(command);

		if(T->rank == 1) /* only generate files once */
		{
			genDifxFiles(T, e);
		}
		
		if(v == -1)
		{
			snprintf(message, DIFX_MESSAGE_LENGTH, 
				"Error: cannot execute %s\n", command);
			fprintf(stderr, "%s\n", message);
			difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_ERROR);

			return e;
		}

		t2 = time(0);
	}

	return e;
}
