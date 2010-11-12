#include <stdlib.h>
#include <time.h>
#include "transient_wrapper_data.h"

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
		printf("  DifxState = %s [%d]\n", DifxStateStrings[T->difxState], T->difxState);
		printf("  outputPath = %s\n", T->outputPath);
		printf("  filePrefix = %s\n", T->filePrefix);
		printf("  identifier = %s\n", T->identifier);
		printf("  monitorThreadDie = %d\n", T->monitorThreadDie);
		printf("  verbose = %d\n", T->verbose);
		printf("  rank = %d\n", T->rank);
		printf("  doCopy = %d\n", T->doCopy);
		printf("  executeTime = %d\n", T->executeTime);
		printf("  maxCopyOverhead = %f\n", T->maxCopyOverhead);
		printf("  nTransient = %d\n", T->nTransient);
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

	T->nTransient++;

	if(transient->startMJD > T->D->mjdStop || transient->stopMJD < T->D->mjdStart)
	{
		snprintf(message, DIFX_MESSAGE_LENGTH,
			"Transient received out of job time range ([%12.6f,%12.6f] not in [%12.6f,%12.6f])",
			transient->startMJD, transient->stopMJD,
			T->D->mjdStart, T->D->mjdStop);
		difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_WARNING);
	}

	if(T->nEvent >= MAX_EVENTS + EXTRA_EVENTS)
	{
		sortEvents(T);
	}

	T->event[T->nEvent].startMJD = transient->startMJD;
	T->event[T->nEvent].stopMJD  = transient->stopMJD;
	T->event[T->nEvent].priority = transient->priority;
	T->nEvent++;
}

int copyBasebandData(const TransientWrapperData *T)
{
	const unsigned int MaxCommandLength = 1024;
	char command[MaxCommandLength];
	char message[DIFX_MESSAGE_LENGTH];
	time_t t1, t2;
	int e;

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
			T->outputPath);

		snprintf(message, DIFX_MESSAGE_LENGTH, "Executing: %s",
			command);
		difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_INFO);
		//system(command);

		t2 = time(0);
	}

	return e;
}
