#include <stdio.h>
#include <string.h>
#include <difxmessage.h>
#include "mk5daemon.h"
#include "proc.h"

int Mk5Daemon_loadMon(Mk5Daemon *D, double mjd)
{
	long long curRX, curTX;
	long long d;
	float l1, l5, l15;
	int memused, memtot;
	char message[MAX_MESSAGE_SIZE];
	int v;

	/* LOAD */
	v = procGetCPU(&l1, &l5, &l15);
	if(v < 0)
	{
		return v;
	}

	/* MEMORY USAGE */
	v = procGetMem(&memused, &memtot);
	if(v < 0)
	{
		return v;
	}

	/* NETWORK */
	v = procGetNet(&curRX, &curTX);
	if(v < 0)
	{
		return v;
	}

	/* on 32 bit machines, proc stores only 32 bit values */
	if(D->lastRX > 0 || D->lastTX > 0) 
	{
		d = curRX - D->lastRX;
		D->load.netRXRate = d/D->loadMonInterval;

		d = curTX - D->lastTX;
		D->load.netTXRate = d/D->loadMonInterval;
	}
	else
	{
		D->load.netRXRate = 0;
		D->load.netTXRate = 0;
	}

	D->lastRX = curRX;
	D->lastTX = curTX;
	
	D->load.cpuLoad = l1;
	D->load.totalMemory = memtot;
	D->load.usedMemory = memused;

	snprintf(message, MAX_MESSAGE_SIZE,
		"LOAD: %13.7f %4.2f %d %d %5.3f %5.3f  %d  %d\n", mjd,
		D->load.cpuLoad, D->load.usedMemory, D->load.totalMemory,
		D->load.netRXRate*8.0e-6, D->load.netTXRate*8.0e-6, D->process, D->processDone);
	message[MAX_MESSAGE_SIZE-1] = 0;

	Logger_logData(D->log, message);
	
	return difxMessageSendLoad(&D->load);
}
