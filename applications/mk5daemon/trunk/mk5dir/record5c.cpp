/***************************************************************************
 *   Copyright (C) 2011 by Walter Brisken                                  *
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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include <difxmessage.h>
#include <signal.h>
#include <mark5ipc.h>
#include <xlrapi.h>
#include "config.h"
#include "mark5dir.h"
#include "mark5directorystructs.h"
#include "watchdog.h"
#include "../config.h"

const char program[] = "record5c";
const char author[]  = "Walter Brisken";
const char version[] = "0.1";
const char verdate[] = "20110802";

const int defaultPacketSize = 5008;
const int defaultPayloadOffset = 40;
const int defaultDataFrameOffset = 0;
const int defaultPSNMode = 0;
const int defaultPSNOffset = 0;
const int defaultMACFilterControl = 1;
const unsigned int defaultStreamstorChannel = 28;

const int MaxLabelLength = 40;
const int Mark5BFrameSize = 10016;
const UINT32 Mark5BSyncWord=0xABADDEED;

typedef void (*sighandler_t)(int);
sighandler_t oldsiginthand;
int die = 0;

#define MAC_FLTR_CTRL		0x02
#define DATA_PAYLD_OFFSET 	0x03
#define DATA_FRAME_OFFSET	0x04
#define PSN_OFFSET		0x05
#define BYTE_LENGTH		0x06
#define FILL_PATTERN_ADDR	0x07
#define TOTAL_PACKETS		0x08
#define ETH_FILTER		0x0C
#define ETH_REJECT_PACKETS	0x11

/* Note that the text going to stdout is interpreted by other code, so change with caution */

static void usage(const char *pgm)
{
	printf("\n%s ver. %s   %s %s\n\n", program, version, author, verdate);
	printf("A program that attempts to recover a Mark5 module\n\n");
	printf("Usage: %s [<options>] <bank> <scanname>\n\n", pgm);
	printf("<options> can include:\n\n");
	printf("  --help\n");
	printf("  -h         Print help info and quit\n\n");
	printf("  --verbose\n");
	printf("  -v         Be more verbose in execution\n\n");

	/* FIXME: options for packet */
}


void siginthand(int j)
{
	die = 1;
	signal(SIGINT, oldsiginthand);
}

static int decode5B(SSHANDLE xlrDevice, UINT64 pointer, int framesToRead, UINT64 *timeBCD, int *firstFrame, int *byteOffset, int *headerMJD, int *headerSeconds)
{
	int bufferSize;
	UINT32 *buffer;
	S_READDESC readdesc;
	int rem;
	int returnValue = 0;

	bufferSize = abs(framesToRead)*Mark5BFrameSize;
	buffer = (UINT32 *)malloc(bufferSize);

	if(framesToRead < 0)
	{
		pointer -= bufferSize;
		if(pointer < 0)
		{
			pointer = 0;
		}
	}

	rem = pointer % 4;
	if(rem)
	{
		pointer += (4 - rem);
	}

	/* First read at start of byte range */
	readdesc.AddrLo = pointer & 0xFFFFFFFF;
	readdesc.AddrHi = pointer >> 32;
	readdesc.XferLength = bufferSize;
	readdesc.BufferAddr = buffer;

	printf("Read %Ld %d\n", pointer, bufferSize);

	WATCHDOGTEST( XLRRead(xlrDevice, &readdesc) );

	// Mark5B search
	// Look for first sync word and require that at least the next frame also has this
	int i;
	if(framesToRead > 1)
	{
		const int searchRange=(bufferSize-Mark5BFrameSize)/4 - 8;
		for(i = 0; i < searchRange; i++)
		{
			if(buffer[i] == Mark5BSyncWord)
			{
				printf("Sync 1 1 %d\n", i);
			}
			if((buffer[i] == Mark5BSyncWord) && (buffer[Mark5BFrameSize/4 + i] == Mark5BSyncWord))
			{
				printf("Sync 1 2 %d\n", Mark5BFrameSize/4 + i);
				
				break;
			}
		}
		if(i >= searchRange)
		{
			returnValue = -1;
		}
	}
	else
	{
		const int searchRange=(bufferSize-Mark5BFrameSize)/4 - 32;
		for(i = searchRange - 1; i >= 0; i--)
		{
			if(buffer[i] == Mark5BSyncWord)
			{
				printf("Sync 2 1 %d\n", i);
			}
			if(buffer[i] == Mark5BSyncWord && buffer[Mark5BFrameSize/4 + i] == Mark5BSyncWord)
			{
				printf("Sync 2 2 %d\n", Mark5BFrameSize/4 + i);

				break;
			}
		}
		if(i < 0)
		{
			returnValue = -1;
		}
		i += Mark5BFrameSize/4;	/* reposition on the last frame */
	}
	if(returnValue == 0)	/* sync words must have been found */
	{
		UINT32 k = buffer[i+2];
		int seconds, mjd;
		int mjdnow;

		// Use system to get first 2 digits of mjd
		mjdnow = (int)(40587.0 + time(0)/86400.0);

		seconds = 0;
		for(int m = 1; m <= 10000; m*=10)
		{
			seconds += (k & 0xF) * m;
			k >>= 4;
		}
		mjd = 0;
		for(int m = 1; m <= 100; m*=10)
		{
			mjd += (k & 0xF) * m;
			k >>= 4;
		}

		// Compute the millenia
		mjd += 1000*( (mjdnow-mjd+500)/1000 );

		if(timeBCD)
		{
			int h, m, s;
			int yr=0, mo=0, da=0, doy;

			mjd2ymd(mjd, &yr, &mo, &da);
			doy = ymd2doy(yr, mo, da);

			s = seconds;
			h = s/3600;
			s -= h*3600;
			m = s/60;
			s -= m*60;
			*timeBCD = 
				((s%10) << 0) +
				((s/10) << 4) +
				((m%10) << 8) +
				((m/10) << 12) + 
				((h%10) << 16) +
				((h/10) << 20);
			for(int k = 0; k < 3; k++)
			{
				*timeBCD += (UINT64)(doy % 10) << (24+4*k);
				doy /= 10;
			}
			for(int k = 0; k < 4; k++)
			{
				*timeBCD += (UINT64)(yr % 10) << (36+4*k);
				yr /= 10;
			}
		}
		if(firstFrame)
		{
			// second word of frame, 15 least sig bits
			*firstFrame = buffer[i+1] & 0x00007FFF;
		}
		if(byteOffset)
		{
			// convert back to bytes from 32-bit words
			*byteOffset = i*4;
		}
		if(headerMJD)
		{
			*headerMJD = mjd;
		}
		if(headerSeconds)
		{
			*headerSeconds = seconds;
		}
		returnValue = 0;
	}

	free(buffer);

	return returnValue;
}

static int record(int bank, const char *label, int packetSize, int payloadOffset, int dataFrameOffset, int psnOffset, int psnMode, int macFilterControl, int verbose)
{
	const int BufferSize = 1<<18;
	unsigned int channel = defaultStreamstorChannel;
	SSHANDLE xlrDevice;
	XLR_RETURN_CODE xlrRC;
	S_BANKSTATUS bankStat;
	S_READDESC readdesc;
	S_DIR dir;
	UINT32 *buffer = 0;
	int go = 1;
	int len;
	char str[10];
	char *rv;
	char *dirData = 0;
	char vsn[XLR_LABEL_LENGTH+1];
	int moduleStatus = MODULE_STATUS_UNKNOWN;
	int v;
	long long ptr;	/* record pointer */
	long long startByte;
	struct Mark5DirectoryHeaderVer1 *dirHeader;
	struct Mark5DirectoryScanHeaderVer1 *p;
	struct Mark5DirectoryLegacyBodyVer1 *q;
	UINT64 timeBCD;
	int frame, byteOffset;
	int mjd1=0, mjd2=0, sec1=0, sec2=0;
	char labelCopy[100];
	char *parts[3];
	int nPart = 0;

	WATCHDOGTEST( XLROpen(1, &xlrDevice) );
	WATCHDOGTEST( XLRSetBankMode(xlrDevice, SS_BANKMODE_NORMAL) );
	WATCHDOGTEST( XLRGetBankStatus(xlrDevice, bank, &bankStat) );
	if(bankStat.State != STATE_READY)
	{
		printf("Error 6 Bank %c not ready\n", 'A'+bank);
		WATCHDOG( XLRClose(xlrDevice) );
		
		return -1;
	}
	if(!bankStat.Selected)
	{
		WATCHDOGTEST( XLRSelectBank(xlrDevice, bank) );
	}

	/* the following line is essential to work around an apparent streamstor bug */
	WATCHDOGTEST( XLRGetDirectory(xlrDevice, &dir) );

	WATCHDOGTEST( XLRGetLabel(xlrDevice, vsn) );
	vsn[XLR_LABEL_LENGTH] = 0;

	v = parseModuleLabel(vsn, 0, 0, 0, &moduleStatus);
	if(v >= 0)
	{
		printf("VSN %c %s\n", bank+'A', vsn);
		if(moduleStatus > 0)
		{
			printf("DMS is %s\n", moduleStatusName(moduleStatus));
		}
	}
	else
	{
		printf("Error 7 No VSN set\n");
		WATCHDOG( XLRClose(xlrDevice) );

		return -1;
	}

	startByte = dir.Length;
	printf("Used %Ld %Ld\n", startByte, 0LL);	/* FIXME: 0-> disk size */

	WATCHDOG( len = XLRGetUserDirLength(xlrDevice) );
	if((len < 128 && len != 0) || len % 128 != 0)
	{
		printf("Error 8 directory format problem\n");
		WATCHDOG( XLRClose(xlrDevice) );

		return -1;
	}

	if(len == 0)
	{
		len = 128;
	}

	buffer = (UINT32 *)malloc(BufferSize);
	dirData = (char *)calloc(len+128, 1);
	WATCHDOGTEST( XLRGetUserDir(xlrDevice, len, 0, dirData) );
	for(int i = 0; i < 128; i++)
	{
		dirData[len+i] = 0;
	}
	dirHeader = (struct Mark5DirectoryHeaderVer1 *)dirData;
	printf("Directory %d %d\n", dirHeader->version, len/128-1);
	p = (struct Mark5DirectoryScanHeaderVer1 *)(dirData + len);
	q = (struct Mark5DirectoryLegacyBodyVer1 *)(dirData + len + sizeof(struct Mark5DirectoryScanHeaderVer1));

	WATCHDOGTEST( XLRSetMode(xlrDevice, SS_MODE_SINGLE_CHANNEL) );
	WATCHDOGTEST( XLRClearChannels(xlrDevice) );
	WATCHDOGTEST( XLRBindInputChannel(xlrDevice, channel) );
	WATCHDOGTEST( XLRSelectChannel(xlrDevice, channel) );

	/* configure 10G input daughter board */
	WATCHDOGTEST( XLRWriteDBReg32(xlrDevice, DATA_PAYLD_OFFSET, payloadOffset) );
	printf("WR_DB %d %d\n", DATA_PAYLD_OFFSET, payloadOffset);
	WATCHDOGTEST( XLRWriteDBReg32(xlrDevice, DATA_FRAME_OFFSET, dataFrameOffset) );
	printf("WR_DB %d %d\n", DATA_FRAME_OFFSET, dataFrameOffset);
	WATCHDOGTEST( XLRWriteDBReg32(xlrDevice, BYTE_LENGTH, packetSize) );
	printf("WR_DB %d %d\n", BYTE_LENGTH, packetSize);
	WATCHDOGTEST( XLRWriteDBReg32(xlrDevice, PSN_OFFSET, psnOffset) );
	printf("WR_DB %d %d\n", PSN_OFFSET, psnOffset);
	WATCHDOGTEST( XLRWriteDBReg32(xlrDevice, MAC_FLTR_CTRL, macFilterControl) );
	printf("WR_DB %d %d\n", MAC_FLTR_CTRL, macFilterControl);

	WATCHDOG( ptr = XLRGetLength(xlrDevice) );

	printf("Record %s %Ld\n", label, ptr);
	if(startByte == 0LL)
	{
		WATCHDOGTEST( XLRRecord(xlrDevice, 0, 1) );
	}
	else
	{
		WATCHDOGTEST( XLRAppend(xlrDevice) );
	}

	for(int n = 1; !die; n++)
	{
		usleep(1000);
		
		if(n % 1000 == 0)
		{
			WATCHDOG( ptr = XLRGetLength(xlrDevice) );
			/* query position and print */
			printf("Pointer %Ld\n", ptr);
		}
	}

	printf("Stop %s %Ld\n", label, ptr);
	WATCHDOGTEST( XLRStop(xlrDevice) );

	WATCHDOGTEST( XLRSetMode(xlrDevice, SS_MODE_SINGLE_CHANNEL) );
	WATCHDOGTEST( XLRClearChannels(xlrDevice) );
	WATCHDOGTEST( XLRSelectChannel(xlrDevice, 0) );
	WATCHDOGTEST( XLRBindOutputChannel(xlrDevice, 0) );

	strcpy(labelCopy, label);
	parts[0] = labelCopy;
	nPart = 1;
	for(int i = 0; labelCopy[i] && nPart < 3; i++)
	{
		if(labelCopy[i] == '_' && labelCopy[i+1])
		{
			labelCopy[i] = 0;
			parts[nPart] = labelCopy + i + 1;
			nPart++;
		}
	}

	/* Update directory */
	WATCHDOGTEST( XLRGetDirectory(xlrDevice, &dir) );
	p->typeNumber = 9 + (len/128)*256;	/* format and scan number */
	p->frameLength = 10016;	/* FIXME */
	switch(nPart)
	{
		case 1:
			strncpy(p->scanName, parts[0], MODULE_SCAN_NAME_LENGTH);
			break;
		case 2:
			strncpy(p->expName, parts[0], 8);
			strncpy(p->scanName, parts[1], MODULE_SCAN_NAME_LENGTH);
			break;
		case 3:
			strncpy(p->expName, parts[0], 8);
			strncpy(p->station, parts[1], 2);
			strncpy(p->scanName, parts[2], MODULE_SCAN_NAME_LENGTH);
			break;
	}
	p->startByte = startByte;
	p->stopByte = dir.Length;

	dirHeader->status = MODULE_STATUS_RECORDED;

	v = decode5B(xlrDevice, startByte, 10, &timeBCD, &frame, &byteOffset, &mjd1, &sec1);
	if(v == 0)
	{
		int deltat, words, samplesPerWord;
		UINT64 length = p->stopByte - p->startByte;

		for(int i = 0; i < 8; i++)
		{
			q->timeBCD[i] = ((unsigned char *)(&timeBCD))[i];
		}
		q->firstFrame = frame;
		q->byteOffset = byteOffset;
		q->nTrack = 0xFFFFFFFF;	/* FIXME */
		v = decode5B(xlrDevice, p->stopByte, -10, 0, &frame, 0, &mjd2, &sec2);
		if(v == 0)
		{

			deltat = sec2 - sec1 + 86400*(mjd2 - mjd1) + 1;
			samplesPerWord = 32/upround2(countbits(q->nTrack));
			words = (length/Mark5BFrameSize)*2500;
			/* The Mark5A/B sample rate must be 2^n.  Round up to the nearest */
			for(q->trackRate = 1; q->trackRate < 8192; q->trackRate *= 2)
			{
				if(deltat*q->trackRate*1000000ULL >= words*samplesPerWord)
				{
					break;
				}
			}

		}
		else
		{
			printf("Decode failure 2\n");
		}
	}
	else
	{
		printf("Decode failure 1\n");
	}

	WATCHDOGTEST( XLRSetUserDir(xlrDevice, dirData, len+128) );

	WATCHDOG( XLRClose(xlrDevice) );

	if(dirData)
	{
		free(dirData);
	}
	if(buffer)
	{
		free(buffer);
	}

	return 0;
}

int main(int argc, char **argv)
{
	int a, v, i;
	int bank = -1;
	int verbose = 0;
	int force = 0;
	int packetSize = defaultPacketSize;
	int dataFrameOffset = defaultDataFrameOffset;
	int payloadOffset = defaultPayloadOffset;
	int psnOffset = defaultPSNOffset;
	int psnMode = defaultPSNMode;
	int macFilterControl = defaultMACFilterControl;
	int retval = EXIT_SUCCESS;
	char label[MaxLabelLength] = "";

	for(a = 1; a < argc; a++)
	{
		if(argv[a][0] == '-')
		{
			if(strcmp(argv[a], "-v") == 0 ||
			   strcmp(argv[a], "--verbose") == 0)
			{
				verbose++;
			}
			else if(strcmp(argv[a], "-h") == 0 ||
			   strcmp(argv[a], "--help") == 0)
			{
				usage(argv[0]);

				return EXIT_SUCCESS;
			}
			else
			{
				printf("Unknown option: %s\n", argv[a]);
				
				return EXIT_FAILURE;
			}
		}
		else if(bank < 0)
		{
			if(strlen(argv[a]) != 1)
			{
				printf("Error 1 bank parameter (%s) not understood.\n", argv[a]);
				
				return EXIT_FAILURE;
			}
			else if(argv[a][0] == 'A' || argv[a][0] == 'a')
			{
				bank = BANK_A;
			}
			else if(argv[a][0] == 'B' || argv[a][0] == 'b')
			{
				bank = BANK_B;
			}
			else
			{
				printf("Error 1 bank parameter (%s) not understood.\n", argv[a]);
				
				return EXIT_FAILURE;
			}
		}
		else if(label[0] == 0)
		{
			i = snprintf(label, MaxLabelLength, "%s", argv[a]);
			if(i >= MaxLabelLength)
			{
				printf("Error: scan name too long (%d > %d)\n", i, MaxLabelLength-1);

				return EXIT_FAILURE;
			}
		}
		else
		{
			printf("Error 2 too many arguments given.\n");

			return EXIT_FAILURE;
		}
	}

	if(bank < 0)
	{
		printf("Error 3 incomplete command line\n");
		
		return EXIT_FAILURE;
	}

	v = initWatchdog();
	if(v < 0)
	{
		printf("Error 4 initWatchdog() failed.\n");

		return EXIT_FAILURE;
	}

	/* 60 seconds should be enough to complete any XLR command */
	setWatchdogTimeout(60);

	setWatchdogVerbosity(verbose);

	/* *********** */

	v = lockMark5(3);

	oldsiginthand = signal(SIGINT, siginthand);

	if(v < 0)
	{
		printf("Error 5 Another process (pid=%d) has a lock on this Mark5 unit\n", getMark5LockPID());
	}
	else
	{
		v = record(bank, label, packetSize, payloadOffset, dataFrameOffset, psnOffset, psnMode, macFilterControl, verbose);
		if(v < 0)
		{
			if(watchdogXLRError[0] != 0)
			{
				char message[DIFX_MESSAGE_LENGTH];
				snprintf(message, DIFX_MESSAGE_LENGTH, 
					"Streamstor error executing: %s : %s",
					watchdogStatement, watchdogXLRError);
				difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_ERROR);
			}

			retval = EXIT_FAILURE;
		}
	}

	unlockMark5();

	/* *********** */

	stopWatchdog();

	return retval;
}
