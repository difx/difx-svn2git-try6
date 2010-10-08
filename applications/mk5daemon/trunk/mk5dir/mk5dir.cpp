/***************************************************************************
 *   Copyright (C) 2008-2010 by Walter Brisken                             *
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
/*===========================================================================
 * SVN properties (DO NOT CHANGE)
 *
 * $Id$
 * $HeadURL$
 * $LastChangedRevision$
 * $Author$
 * $LastChangedDate$
 *
 *==========================================================================*/


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <difxmessage.h>
#include <mark5ipc.h>
#include "mark5dir.h"
#include "watchdog.h"
#include "config.h"

const char program[] = "mk5dir";
const char author[]  = "Walter Brisken";
const char version[] = "0.7";
const char verdate[] = "20101007";

enum DMS_Mode
{
	DMS_MODE_UPDATE = 0,
	DMS_MODE_NO_UPDATE,
	DMS_MODE_UPDATE_IF_SAFE,
	DMS_MODE_FAIL_UNLESS_SAFE
};

int verbose = 0;
int die = 0;
SSHANDLE xlrDevice;

typedef void (*sighandler_t)(int);

sighandler_t oldsiginthand;

void siginthand(int j)
{
	if(verbose)
	{
		printf("Being killed\n");
	}
	die = 1;
}


int usage(const char *pgm)
{
	printf("\n%s ver. %s   %s %s\n\n", program, version, author, verdate);
	printf("A program to extract Mark5 module directory information via XLR calls\n");
	printf("\nUsage : %s [<options>] { <bank> | <vsn> }\n\n", pgm);
	printf("options can include:\n");
	printf("  --help\n");
	printf("  -h             Print this help message\n\n");
	printf("  --verbose\n");
	printf("  -v             Be more verbose\n\n");
	printf("  --force\n");
	printf("  -f             Reread directory even if not needed\n\n");
	printf("  --fast\n");
	printf("  -F             Use fast mode (user dir version >=1 only)\n\n");
	printf("  --nodms\n");
	printf("  -n             Get directory but don't change disk module state\n\n");
	printf("  --safedms\n");
	printf("  -s             Only change disk module state if SDK8 unit or new dir type\n\n");
	printf("  --dmsforce\n");
	printf("  -d             Proceed with change of DMS even if ths makes module unreadable in SDK8\n\n");
	printf("<bank> is either A or B\n\n");
	printf("<vsn> is a valid module VSN (8 characters)\n\n");
	printf("Environment variable MARK5_DIR_PATH should point to the location of\n");
	printf("the directory to be written.  The output filename will be:\n");
	printf("  $MARK5_DIR_PATH/<vsn>.dir\n\n");

	return 0;
}

static int getBankInfo(SSHANDLE xlrDevice, DifxMessageMk5Status * mk5status, char bank)
{
	S_BANKSTATUS bank_stat;

	if(bank == 'A' || bank == 'a' || bank == ' ')
	{
		WATCHDOGTEST( XLRGetBankStatus(xlrDevice, BANK_A, &bank_stat) );
		if(bank_stat.Label[8] == '/')
		{
			strncpy(mk5status->vsnA, bank_stat.Label, 8);
			mk5status->vsnA[8] = 0;
		}
		else
		{
			mk5status->vsnA[0] = 0;
		}
	}
	if(bank == 'B' || bank == 'b' || bank == ' ')
	{
		WATCHDOGTEST( XLRGetBankStatus(xlrDevice, BANK_B, &bank_stat) );
		if(bank_stat.Label[8] == '/')
		{
			strncpy(mk5status->vsnB, bank_stat.Label, 8);
			mk5status->vsnB[8] = 0;
		}
		else
		{
			mk5status->vsnB[0] = 0;
		}
	}

	return 0;
}

int dirCallback(int scan, int nscan, int status, void *data)
{
	static long long seconds=0;
	struct timeval t;
	DifxMessageMk5Status *mk5status;
	int v;

	mk5status = (DifxMessageMk5Status *)data;
	mk5status->scanNumber = scan;
	mk5status->position = nscan;
	snprintf(mk5status->scanName, DIFX_MESSAGE_MAX_SCANNAME_LEN, "%s", Mark5DirDescription[status]);
	difxMessageSendMark5Status(mk5status);

	if(verbose)
	{
		printf("%d/%d %d\n", scan, nscan, status);
	}

	gettimeofday(&t, 0);
	if(seconds == 0)
	{
		seconds = t.tv_sec;
	}
	if(t.tv_sec - seconds > 10)
	{
		seconds = t.tv_sec;
		v = getBankInfo(xlrDevice, mk5status, mk5status->activeBank == 'B' ? 'A' : 'B');
		if(v < 0)
		{
			die = 1;
		}
	}

	return die;
}

static int getDirCore(struct Mark5Module *module, char *vsn, DifxMessageMk5Status *mk5status, int force, int fast, enum DMS_Mode dmsMode)
{
	int v;
	int mjdnow;
	const char *mk5dirpath;
	float replacedFrac;
	char message[DIFX_MESSAGE_LENGTH];
	int dmsUpdate = 0;
	int len;

	if(dmsMode == DMS_MODE_FAIL_UNLESS_SAFE)
	{
		WATCHDOG( len = XLRGetUserDirLength(xlrDevice) );
		if(len % 128 != 0 && SDKVERSION >= 9)
		{
			snprintf(message, DIFX_MESSAGE_LENGTH,
				"Not reading the module directory because setting the DMS will render "
				"this module unreadable with SDK8.  Use the --dmsforce option to override.");
			difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_ERROR);
			fprintf(stderr, "Error: %s\n", message);

			return -1;
		}
		else
		{
			dmsMode = DMS_MODE_UPDATE;
		}
	}

	mjdnow = (int)(40587.0 + time(0)/86400.0);

	mk5dirpath = getenv("MARK5_DIR_PATH");
	if(mk5dirpath == 0)
	{
		mk5dirpath = ".";
	}

	v = getCachedMark5Module(module, xlrDevice, mjdnow, 
		vsn, mk5dirpath, &dirCallback, 
		mk5status, &replacedFrac, force, fast, 0);
	if(replacedFrac > 0.01)
	{
		snprintf(message, DIFX_MESSAGE_LENGTH, 
			"Module %s directory read encountered %4.2f%% data replacement rate",
			vsn, replacedFrac);
		difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_WARNING);
		fprintf(stderr, "Warning: %s\n", message);
	}
	if(v < 0)
	{
		if(!die)
		{
			snprintf(message, DIFX_MESSAGE_LENGTH, 
				"Directory read for module %s unsuccessful, error code=%d",
				vsn, v);
			difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_ERROR);
			fprintf(stderr, "Error: %s\n", message);
		}
	}
	else if(v == 0 && verbose > 0)
	{
		printMark5Module(module);
	}

	switch(dmsMode)
	{
	case DMS_MODE_NO_UPDATE:
		fprintf(stderr, "FYI: Not setting disk module state to Played for %s\n\n", vsn);
		dmsUpdate = 0;
		break;
	case DMS_MODE_UPDATE:
		dmsUpdate = 1;
		break;
	case DMS_MODE_UPDATE_IF_SAFE:
		if(SDKVERSION < 9 || module->dirVersion == 0)
		{
			dmsUpdate = 1;
		}
		else
		{
			snprintf(message, DIFX_MESSAGE_LENGTH,
				"Not setting disk module state of %s to Played because "
				"this would make the module unreadable in SDK8 Mark5 units.", vsn);
			fprintf(stderr, "Warning: %s\n\n", message);
			difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_ERROR);
			dmsUpdate = 0;
		}
		break;
	case DMS_MODE_FAIL_UNLESS_SAFE:	
		if(SDKVERSION < 9 || module->dirVersion == 0)
		{
			dmsUpdate = 1;
		}
		else
		{
			/* should not get here */
			snprintf(message, DIFX_MESSAGE_LENGTH,
				"Developer error: DMS_MODE_FAIL_UNLESS_SAFE reached after "
				"directory read!  vsn = %s", vsn);
			difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_ERROR);
			fprintf(stderr, "Error: %s\n", message);
			dmsUpdate = 0;
		}
		break;
	}
	if(dmsUpdate)
	{
		if(module->dirVersion == 0)
		{
			setDiscModuleStateLegacy(xlrDevice, MODULE_STATUS_PLAYED);
		}
		else
		{
			setDiscModuleStateNew(xlrDevice, MODULE_STATUS_PLAYED);
		}
	}

	return v;
}

static int mk5dir(char *vsn, int force, int fast, enum DMS_Mode dmsMode)
{
	struct Mark5Module module;
	DifxMessageMk5Status mk5status;
	char message[DIFX_MESSAGE_LENGTH];
	char modules[100] = "";
	int v;

	memset(&mk5status, 0, sizeof(mk5status));

	WATCHDOGTEST( XLROpen(1, &xlrDevice) );
	WATCHDOGTEST( XLRSetFillData(xlrDevice, MARK5_FILL_PATTERN) );
	WATCHDOGTEST( XLRSetOption(xlrDevice, SS_OPT_SKIPCHECKDIR) );
	WATCHDOGTEST( XLRSetBankMode(xlrDevice, SS_BANKMODE_NORMAL) );

	v = getBankInfo(xlrDevice, &mk5status, ' ');
	if(v < 0)
	{
		XLRClose(xlrDevice);

		return -1;
	}

	if(strcasecmp(vsn, "A") == 0 && mk5status.vsnA[0] != 0)
	{
		mk5status.activeBank = 'A';
		strcpy(vsn, mk5status.vsnA);
	}
	if(strcasecmp(vsn, "B") == 0 && mk5status.vsnB[0] != 0)
	{
		mk5status.activeBank = 'B';
		strcpy(vsn, mk5status.vsnB);
	}

	mk5status.state = MARK5_STATE_GETDIR;
	difxMessageSendMark5Status(&mk5status);

	memset(&module, 0, sizeof(module));
	module.bank = -1;

	oldsiginthand = signal(SIGINT, siginthand);

	if(strcmp(vsn, "AB") == 0)
	{
		if(strlen(mk5status.vsnA) == 8)
		{
			mk5status.activeBank = 'A';
			v = getDirCore(&module, mk5status.vsnA, &mk5status, force, fast, dmsMode);
			if(v >= 0)
			{
				strcpy(modules, mk5status.vsnA);
			}
		}

		memset(&module, 0, sizeof(module));
		module.bank = -1;

		if(strlen(mk5status.vsnB) == 8)
		{
			mk5status.activeBank = 'B';
			v = getDirCore(&module, mk5status.vsnB, &mk5status, force, fast, dmsMode);
			if(v >= 0)
			{
				if(modules[0])
				{
					strcat(modules, " and ");
					strcat(modules, mk5status.vsnB);
				}
				else
				{
					strcpy(modules, mk5status.vsnB);
				}
			}
		}
	}
	else
	{
		if(strlen(vsn) == 8)
		{
			if(strncmp(vsn, mk5status.vsnA, 8) == 0)
			{
				mk5status.activeBank = 'A';
			}
			else if(strncmp(vsn, mk5status.vsnB, 8) == 0)
			{
				mk5status.activeBank = 'B';
			}

			v = getDirCore(&module, vsn, &mk5status, force, fast, dmsMode);
			if(v >= 0)
			{
				strcpy(modules, vsn);
			}
		}

	}

	if(die)
	{
		difxMessageSendDifxAlert("Directory read terminated by signal.", DIFX_ALERT_LEVEL_WARNING);
	}
	else if(modules[0])
	{
		snprintf(message, DIFX_MESSAGE_LENGTH,
			"Successful directory read for %s", modules);
		difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_VERBOSE);
	}

	WATCHDOG( XLRClose(xlrDevice) );

	mk5status.state = MARK5_STATE_IDLE;
	mk5status.scanNumber = module.nscans;
	mk5status.scanName[0] = 0;
	mk5status.position = 0;
	mk5status.activeBank = ' ';
	difxMessageSendMark5Status(&mk5status);

	return 0;
}

int main(int argc, char **argv)
{
	char vsn[16] = "";
	int force=0;
	int fast=0;
	enum DMS_Mode dmsMode = DMS_MODE_FAIL_UNLESS_SAFE;
	int v;
	const char *dmsMaskStr;
	int dmsMask = 7;

	dmsMaskStr = getenv("DEFAULT_DMS_MASK");
	if(dmsMaskStr)
	{
		dmsMask = atoi(dmsMaskStr);
		if(dmsMask & 2 == 0)
		{
			dmsMode = DMS_MODE_NO_UPDATE;
		}
	}

	difxMessageInit(-1, "mk5dir");

	if(argc < 2)
	{
		return usage(argv[0]);
	}

	for(int a = 1; a < argc; a++)
	{
		if(strcmp(argv[a], "-h") == 0 ||
		   strcmp(argv[a], "--help") == 0)
		{
			return usage(argv[0]);
		}
		else if(strcmp(argv[a], "-v") == 0 ||
		        strcmp(argv[a], "--verbose") == 0)
		{
			verbose++;
		}
		else if(strcmp(argv[a], "-f") == 0 ||
			strcmp(argv[a], "--force") == 0)
		{
			force = 1;
		}
		else if(strcmp(argv[a], "-F") == 0 ||
			strcmp(argv[a], "--fast") == 0)
		{
			fast = 1;
		}
		else if(strcmp(argv[a], "-n") == 0 ||
			strcmp(argv[a], "--nodms") == 0)
		{
			dmsMode = DMS_MODE_NO_UPDATE;
		}
		else if(strcmp(argv[a], "-s") == 0 ||
			strcmp(argv[a], "--safedms") == 0)
		{
			dmsMode = DMS_MODE_UPDATE_IF_SAFE;
		}
		else if(strcmp(argv[a], "-d") == 0 ||
			strcmp(argv[a], "--dmsforce") == 0)
		{
			dmsMode = DMS_MODE_UPDATE;
		}
		else if(vsn[0] == 0)
		{
			strncpy(vsn, argv[a], 8);
			vsn[8] = 0;
		}
		else
		{
			return usage(argv[0]);
		}
	}

	v = initWatchdog();
	if(v < 0)
	{
		return 0;
	}

	/* 60 seconds should be enough to complete any XLR command */
	setWatchdogTimeout(60);

	setWatchdogVerbosity(verbose);

	/* *********** */

	v = lockMark5(MARK5_LOCK_DONT_WAIT);

	if(v < 0)
	{
		fprintf(stderr, "Another process (pid=%d) has a lock on this Mark5 unit\n", getMark5LockPID());
	}
	else
	{
		v = mk5dir(vsn, force, fast, dmsMode);
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
		}
	}

	unlockMark5();

	/* *********** */

	stopWatchdog();

	return 0;
}
