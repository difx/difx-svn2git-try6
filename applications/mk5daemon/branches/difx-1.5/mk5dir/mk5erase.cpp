/***************************************************************************
 *   Copyright (C) 2010 by Walter Brisken                                  *
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
#include <string.h> 
#include <unistd.h>
#include <ctype.h>
#include <sys/timeb.h>
#include <signal.h>
#include "xlrapi.h"
#include "watchdog.h"
#include "difxmessage.h"
#include "mark5dir.h"

/* Note: this program is largely based on Haystack's SSErase.  Thanks
 * to John Ball and Dan Smythe for providing a nice template.
 * Improvements to the original include:
 *  1. Integration to the difxmessage system
 *  2. Watchdog around all XLR function calls
 *  3. The module rate is based on lowest performance portion of test
 *  4. Fast modes (read-only or write-only) possible
 *
 * Other differences:
 *  1. Can only operate on one module at a time
 *  2. Options to influence the directory structure that is left on the module
 *  3. Reported progress considers 2 passes
 *  4. Increased computer friendliness of text output
 */

/* TODO
 *  Find a way to save the hostname to the condition database
 */

const char program[] = "mk5erase";
const char author[]  = "Walter Brisken";
const char version[] = "0.1";
const char verdate[] = "20100526";


#define MJD_UNIX0       40587.0
#define SEC_DAY         86400.0
#define MSEC_DAY        86400000.0 


int die = 0;
const int statsRange[] = { 75000, 150000, 300000, 600000, 1200000, 2400000, 4800000, -1 };

typedef void (*sighandler_t)(int);

sighandler_t oldsiginthand;


enum ConditionMode
{
	CONDITION_ERASE_ONLY = 0,
	CONDITION_READ_WRITE,
	CONDITION_READ_ONLY,
	CONDITION_WRITE_ONLY
};

struct DriveInformation
{
	char model[XLR_MAX_DRIVENAME+1];
	char serial[XLR_MAX_DRIVESERIAL+1];
	char rev[XLR_MAX_DRIVEREV+1];
	int failed;
	long long capacity;	/* in bytes */
};

int usage(const char *pgm)
{
	printf("\n%s ver. %s   %s %s\n\n", program, version, author, verdate);
	printf("A program to erase or condition a Mark5 module.\n");
	printf("\nUsage : %s [<options>] <vsn>\n\n", pgm);
	printf("options can include:\n");
	printf("  --help\n");
	printf("  -h             Print this help message\n\n");
	printf("  --verbose\n");
	printf("  -v             Be more verbose\n\n");
	printf("  --condition\n");
	printf("  -c             Do full conditioning, not just erasing\n\n");
	printf("  --readonly\n");
	printf("  -r             Perform read-only conditioning mode\n\n");
	printf("  --writeonly\n");
	printf("  -w             Perform write-only conditioning mode\n\n");
	printf("  --force\n");
	printf("  -f             Don't ask to continue\n\n");
	printf("  --getdata\n");
	printf("  -d             Save time domain performance data\n\n");
	printf("  --legacydir\n");
	printf("  -l             Force a legacy directory on the module\n\n");
	printf("  --newdir\n");
	printf("  -n             Force a new directory structure on the module\n\n");
	printf("<vsn> is a valid module VSN (8 characters)\n\n");
	printf("Note: A single Mark5 unit needs to be installed in bank A for\n");
	printf("proper operation.  If the VSN is not set, use the  vsn  utility\n");
	printf("To assign it prior to erasure or conditioning.\n\n");

	return 0;
}

void siginthand(int j)
{
	printf("Being killed\n");
	die = 1;
}

void trim(char *out, const char *in)
{
	int i, s=-1, e=0;

	for(i = 0; in[i]; i++)
	{
		if(in[i] > ' ')
		{
			if(s == -1) 
			{
				s = e = i;
			}
			else
			{
				e = i;
			}
		}
	}

	if(s == -1)
	{
		out[0] = 0;
	}
	else
	{
		strncpy(out, in+s, e-s+1);
		out[e-s+1] = 0;
	}
}

int roundSize(long long a)
{
	a /= 1000000000;
	a = (a+2)/5;

	return a*5;
}

int resetDirectory(SSHANDLE *xlrDevice, const char *vsn, int dirVersion, int totalCapacity, int rate)
{
	int dirLength;
	char *dirData;
	struct Mark5DirectoryHeaderVer1 *dirHeader;

	/* clear the user directory */
	WATCHDOG( dirLength = XLRGetUserDirLength(*xlrDevice) );
	
	dirData = 0;
	if(dirVersion < 0)
	{
		if(dirLength < 80000 || dirLength % 128 == 0)
		{
			dirLength = 128;
			dirData = (char *)calloc(dirLength, 1);
			WATCHDOGTEST( XLRGetUserDir(*xlrDevice, dirLength, 0, dirData) );
			dirHeader = (struct Mark5DirectoryHeaderVer1 *)dirData;
			dirVersion = dirHeader->version;
			if(dirVersion < 0 || dirVersion > 100)
			{
				dirVersion = 1;
			}
			memset(dirData, 0, 128);
			dirHeader->version = dirVersion;
			sprintf(dirHeader->vsn, "%s/%d/%d", vsn, totalCapacity, rate);
		}
		else
		{
			dirVersion = 0;
		}
	}
	else if(dirVersion == 0)
	{
		dirLength = 83424;
	}
	else
	{
		dirLength = 128;
		dirData = (char *)calloc(dirLength, 1);
		dirHeader = (struct Mark5DirectoryHeaderVer1 *)dirData;
		dirHeader->version = dirVersion;
		sprintf(dirHeader->vsn, "%s/%d/%d", vsn, totalCapacity, rate);
	}

	if(dirData == 0)
	{
		dirData = (char *)calloc(dirLength, 1);
	}

	printf("> Dir Size = %d  Dir Version = %d\n", dirLength, dirVersion);

	WATCHDOGTEST( XLRSetUserDir(*xlrDevice, dirData, dirLength) );
	free(dirData);

	return 0;
}

int resetLabel(SSHANDLE *xlrDevice, const char *vsn, int totalCapacity, int rate, const char *moduleState)
{
	const char RecordSeparator = 30;
	char label[XLR_LABEL_LENGTH+1];

	/* reset the label */
	snprintf(label, XLR_LABEL_LENGTH, "%8s/%d/%d%c%s", 
		vsn, totalCapacity, rate, RecordSeparator,
		moduleState);
	printf("> New label = %s\n", label);
	WATCHDOGTEST( XLRSetLabel(*xlrDevice, label, strlen(label)) );

	return 0;
}

int erase(SSHANDLE *xlrDevice)
{
	WATCHDOGTEST( XLRErase(*xlrDevice, SS_OVERWRITE_NONE) );

	return 0;
}

int condition(SSHANDLE *xlrDevice, const char *vsn, enum ConditionMode mode, DifxMessageMk5Status *mk5status, int verbose, int getData, const struct DriveInformation drive[8], int *rate)
{
	XLR_RETURN_CODE xlrRC;
	S_DEVSTATUS devStatus;
	S_DRIVESTATS driveStats[XLR_MAXBINS];
	S_DEVINFO devInfo;
	S_DRIVEINFO driveInfo;
	long long len, lenLast=-1;
	long long lenFirst=0;
	struct timeb time1, time2;
	double dt;
	int nPass, pass = 0;
	char opName[10] = "";
	FILE *out=0;
	double lowestRate = 1e9;	/* Unphysically fast! */
	double highestRate = 0.0;
	double averageRate = 0.0;
	int nRate = 0;
	char message[DIFX_MESSAGE_LENGTH];
	DifxMessageCondition condMessage;
	const int printInterval = 10;

	mk5status->state = MARK5_STATE_CONDITION;
	difxMessageSendMark5Status(mk5status);
	
	WATCHDOGTEST( XLRGetDeviceInfo(*xlrDevice, &devInfo) );

	/* configure collection of drive statistics */
	WATCHDOGTEST( XLRSetOption(*xlrDevice, SS_OPT_DRVSTATS) );
	for(int b = 0; b < XLR_MAXBINS; b++)
	{
		driveStats[b].range = statsRange[b];
		driveStats[b].count = 0;
	}
	WATCHDOGTEST( XLRSetDriveStats(*xlrDevice, driveStats) );

	if(mode == CONDITION_WRITE_ONLY)
	{
		WATCHDOGTEST( XLRErase(*xlrDevice, SS_OVERWRITE_RANDOM_PATTERN) );
		nPass = 1;
		strcpy(opName, "W");
	}
	else if(mode == CONDITION_READ_ONLY)
	{
		WATCHDOGTEST( XLRErase(*xlrDevice, SS_OVERWRITE_RW_PATTERN) );
		nPass = 1;
		strcpy(opName, "R");
	}
	else
	{
		WATCHDOGTEST( XLRErase(*xlrDevice, SS_OVERWRITE_RW_PATTERN) );
		nPass = 2;
		strcpy(opName, "RW");
	}

	ftime(&time1);

	if(getData)
	{
		char fileName[128];
		sprintf(fileName, "%s.timedata", vsn);
		printf("Opening %s for output...\n", fileName);
		out = fopen(fileName, "w");
		if(!out)
		{
			fprintf(stderr, "Warning: cannot open %s for output.\nContuniung anyway.\n",
				fileName);
		}
	}

	for(int n = 0; ; n++)
	{
		WATCHDOG( len = XLRGetLength(*xlrDevice) );
		if(lenLast < 0)
		{
			lenFirst = lenLast = len;
		}
		WATCHDOGTEST( XLRGetDeviceStatus(*xlrDevice, &devStatus) );
		if(!devStatus.Recording)
		{
			break;	/* must be done */
		}
		if(die)
		{
			snprintf(message, DIFX_MESSAGE_LENGTH, "Conditioning aborted.");
			fprintf(stderr, "%s\n", message);
			difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_WARNING);

			break;
		}
		if(n == printInterval)
		{
			long long bytes;
			double done;

			n = 0;

			bytes = -(len-lenLast);	// It is counting down
			if(lenLast < len)
			{
				pass++;
				if(pass >= nPass)
				{
					break;
				}
				bytes += lenFirst;
			}
			done = 100.0*(double)(lenFirst-len)/(double)lenFirst;
			done = done/nPass + 50.0*pass;
			mk5status->position = devInfo.NumBuses*len;
			mk5status->rate = 8*devInfo.NumBuses*bytes/(printInterval*1000000.0);
			sprintf(mk5status->scanName, "%s[%4.2f%%]", opName, done);
			if(bytes > 0)
			{
				if(mk5status->rate < lowestRate)
				{
					lowestRate = mk5status->rate;
				}
				if(mk5status->rate > highestRate)
				{
					highestRate = mk5status->rate;
				}
				nRate++;
				averageRate += mk5status->rate;
			}
			ftime(&time2);
			dt = time2.time + time2.millitm/1000.0 
			   - time1.time - time1.millitm/1000.0;
			
			if(len != lenLast)
			{
				mk5status->state = MARK5_STATE_CONDITION;
			}
			else
			{
				mk5status->state = MARK5_STATE_COND_ERROR;
			}
			mk5status->dataMJD = dt/SEC_DAY;
			difxMessageSendMark5Status(mk5status);

			sprintf(message, ". Time = %8.3f  Pos = %14Ld  Rate = %7.2f  Done = %5.2f %%\n",
				dt, mk5status->position, mk5status->rate, done);
			printf(message);
			if(out)
			{
				fprintf(out, message);
				fflush(out);
			}

			lenLast = len;
		}
		sleep(1);
	}

	if(nRate > 0)
	{
		averageRate /= nRate;
	}

	if(out)
	{
		fclose(out);
	}

	ftime(&time2);
	dt = time2.time + time2.millitm/1000.0 
	   - time1.time - time1.millitm/1000.0;

	printf("\n");

	if(devStatus.Recording)
	{
		WATCHDOG( xlrRC = XLRStop(*xlrDevice) );
	}

	if(!die)
	{
		char hostname[32];

		printf("> %s Conditioning %s took %7.2f seconds\n", opName, vsn, dt);

		gethostname(hostname, 32);
		printf("> Hostname %s\n", hostname);

		*rate = 128*(int)(lowestRate/128.0);

		condMessage.startMJD = MJD_UNIX0 + time1.time/SEC_DAY + time1.millitm/MSEC_DAY;
		condMessage.stopMJD = MJD_UNIX0 + time2.time/SEC_DAY + time2.millitm/MSEC_DAY;
		strncpy(condMessage.moduleVSN, vsn, DIFX_MESSAGE_MARK5_VSN_LENGTH);
		condMessage.moduleVSN[DIFX_MESSAGE_MARK5_VSN_LENGTH] = 0;

		for(int d = 0; d < 8; d++)
		{
			WATCHDOG( xlrRC = XLRGetDriveInfo(*xlrDevice, d/2, d%2, &driveInfo) );
			if(xlrRC == XLR_SUCCESS)
			{
				condMessage.moduleSlot = d;
				strncpy(condMessage.serialNumber, 
					drive[d].serial, 
					DIFX_MESSAGE_DISC_SERIAL_LENGTH);
				strncpy(condMessage.modelNumber,
					drive[d].model,
					DIFX_MESSAGE_DISC_MODEL_LENGTH);
				condMessage.diskSize = drive[d].capacity/1000000000LL;
				printf("> Disk %d stats : %s", d, drive[d].serial);
				for(int i = 0; i < DIFX_MESSAGE_N_CONDITION_BINS; i++)
				{
					condMessage.bin[i] = -1;
				}
				WATCHDOG( xlrRC = XLRGetDriveStats(*xlrDevice, d/2, d%2, driveStats) );
				if(xlrRC == XLR_SUCCESS)
				{
					for(int i = 0; i < XLR_MAXBINS; i++)
					{
						if(i < DIFX_MESSAGE_N_CONDITION_BINS)
						{
							condMessage.bin[i] = driveStats[i].count;
							printf(" : %d", condMessage.bin[i]);
						}
					}
				}
				else
				{
					XLRGetErrorMessage(message, XLRGetLastError( ));
					printf(" : %s", message);
				}
				printf("\n");
				difxMessageSendCondition(&condMessage);
			}
			else
			{
				printf("! Disk %d stats : Not found!\n", d);
			}
		}
		printf("> Rates (Mpbs): Lowest = %7.2f  Average = %7.2f  Highest = %7.2f\n",
			lowestRate, averageRate, highestRate);
	}

	return 0;
}

int getDriveInformation(SSHANDLE *xlrDevice, struct DriveInformation drive[8], int *totalCapacity)
{
	XLR_RETURN_CODE xlrRC;
	S_DRIVEINFO driveInfo;
	int nDrive = 0;
	long long minCapacity = 0;
	char message[DIFX_MESSAGE_LENGTH];

	for(int d = 0; d < 8; d++)
	{
		WATCHDOG( xlrRC = XLRGetDriveInfo(*xlrDevice, d/2, d%2, &driveInfo) );
		if(xlrRC != XLR_SUCCESS)
		{
			snprintf(message, DIFX_MESSAGE_LENGTH,
				"XLRGetDriveInfo failed for disk %d", d);
			printf("message\n");
			difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_WARNING);

			drive[d].model[0] = 0;
			drive[d].serial[0] = 0;
			drive[d].rev[0] = 0;
			drive[d].failed = 0;
		
			continue;
		}

		trim(drive[d].model, driveInfo.Model);
		trim(drive[d].serial, driveInfo.Serial);
		trim(drive[d].rev, driveInfo.Revision);
		drive[d].capacity = driveInfo.Capacity * 512LL;

		if(driveInfo.SMARTCapable && !driveInfo.SMARTState)
		{
			drive[d].failed = 1;
		}
		else
		{
			drive[d].failed = 0;
		}
		if(drive[d].capacity > 0)
		{
			nDrive++;
		}
		if(minCapacity == 0 || 
		   (drive[d].capacity > 0 && drive[d].capacity < minCapacity))
		{
			minCapacity = drive[d].capacity;
		}
	}

	*totalCapacity = (nDrive*minCapacity/10000000000LL)*10;

	return nDrive;
}

int mk5erase(const char *vsn, enum ConditionMode mode, int verbose, int dirVersion, int getData)
{
	SSHANDLE xlrDevice;
	S_BANKSTATUS bankStatus;
	S_DEVSTATUS devStatus;
	char label[XLR_LABEL_LENGTH+1];
	struct DriveInformation drive[8];
	int nDrive;
	int totalCapacity;		/* in GB (approx) */
	int rate = 1024;		/* Mbps, for label */
	int v;
	DifxMessageMk5Status mk5status;
	char message[DIFX_MESSAGE_LENGTH];

	memset((char *)(&mk5status), 0, sizeof(mk5status));

	WATCHDOGTEST( XLROpen(1, &xlrDevice) );
	WATCHDOGTEST( XLRSetBankMode(xlrDevice, SS_BANKMODE_NORMAL) );
	WATCHDOGTEST( XLRGetBankStatus(xlrDevice, BANK_A, &bankStatus) );
	if(bankStatus.State != STATE_READY)
	{
		snprintf(message, DIFX_MESSAGE_LENGTH, "Bank A is not ready");
		fprintf(stderr,  "%s\n", message);
		difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_ERROR);
		WATCHDOG( XLRClose(xlrDevice) );
		return -1;
	}
	if(!bankStatus.Selected)
	{
		snprintf(message, DIFX_MESSAGE_LENGTH, 
			"To continue, the target module needs to be alone in bank A.");
		fprintf(stderr,  "%s\n", message);
		difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_ERROR);
		WATCHDOG( XLRClose(xlrDevice) );
		return -1;
	}
	
	/* get label */
	WATCHDOGTEST( XLRGetLabel(xlrDevice, label) );
	if(strncasecmp(label, vsn, 8) != 0)
	{
		snprintf(message, DIFX_MESSAGE_LENGTH, 
			"VSN mismatch: %s is not %s", vsn, label);
		fprintf(stderr,  "%s\n", message);
		difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_ERROR);
		WATCHDOG( XLRClose(xlrDevice) );
		return -1;
		
	}

	if(strlen(label) > 10)
	{
		int c, r, n;

		n = sscanf(label+9, "%d/%d", &c, &r);
		if(n == 2 && c > 0 && r > 0 && r < 100000)
		{
			rate = r;
		}
		else
		{
			snprintf(message, DIFX_MESSAGE_LENGTH,
				"Extended VSN is corrupt.  Assuming rate = 1024 Mbps.");
			fprintf(stderr,  "Warning: %s\n", message);
			difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_ERROR);
			rate = 1024;
		}
	}
	else
	{
		snprintf(message, DIFX_MESSAGE_LENGTH,
			"No extended VSN found.  Assuming rate = 1024 Mbps.");
		fprintf(stderr,  "Warning: %s\n", message);
		difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_ERROR);
		rate = 1024;
	}

	/* set operational mode */
	WATCHDOGTEST( XLRSetMode(xlrDevice, SS_MODE_PCI) );
	WATCHDOGTEST( XLRGetDeviceStatus(xlrDevice, &devStatus) );
	WATCHDOGTEST( XLRClearOption(xlrDevice, SS_OPT_SKIPCHECKDIR) );
	WATCHDOGTEST( XLRClearWriteProtect(xlrDevice) );

	/* Get drive info */
	nDrive = getDriveInformation(&xlrDevice, drive, &totalCapacity);

	printf("> Module %s consists of %d drives totalling about %d GB:\n",
		vsn, nDrive, totalCapacity);
	for(int d = 0; d < 8; d++)
	{
		if(drive[d].model[0] == 0)
		{
			continue;
		}
		printf("> Disk %d info : %s : %s : %s : %d : %s\n",
			d, drive[d].model, drive[d].serial, drive[d].rev, 
			roundSize(drive[d].capacity),
			drive[d].failed ? "FAILED" : "OK");
	}

	strcpy(mk5status.vsnA, vsn);
	mk5status.activeBank = 'A';
	/* note: there is no module in bank B */

	if(mode == CONDITION_ERASE_ONLY)
	{
		/* here just erasing */
		v = erase(&xlrDevice);
		if(v < 0)
		{
			/* Something bad happened.  Bail! */
			return v;
		}
	}
	else
	{
		/* here doing the full condition */
		v = condition(&xlrDevice, vsn, mode, &mk5status, verbose, getData, drive, &rate);
		if(v < 0)
		{
			/* Something bad happened.  Bail! */
			return v;
		}
	}

	v = resetDirectory(&xlrDevice, vsn, dirVersion, totalCapacity, rate);

	if(v < 0)
	{
		/* Something bad happened to the XLR device.  Bail! */
		return v;
	}
	v = resetLabel(&xlrDevice, vsn, totalCapacity, rate, (die ? "Error" : "Erased") );
	if(v < 0)
	{
		/* Something bad happened to the XLR device.  Bail! */
		return v;
	}

	/* finally close the device */
	WATCHDOG( XLRClose(xlrDevice) );

	mk5status.scanName[0] = 0;
	mk5status.state = MARK5_STATE_IDLE;
	mk5status.rate = 0;
	mk5status.position = 0;
	difxMessageSendMark5Status(&mk5status);

	return 0;
}

int main(int argc, char **argv)
{
	enum ConditionMode mode = CONDITION_ERASE_ONLY;
	int verbose = 0;
	int force = 0;
	int getData = 0;
	char vsn[10] = "";
	char resp[12] = " ";
	char *rv;
	int v;
	int dirVersion = -1;

	if(argc < 2)
	{
		printf("Please run with -h for help\n");
		return 0;
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
		else if(strcmp(argv[a], "-c") == 0 ||
		        strcmp(argv[a], "--condition") == 0)
		{
			mode = CONDITION_READ_WRITE;
		}
		else if(strcmp(argv[a], "-r") == 0 ||
		        strcmp(argv[a], "--readonly") == 0)
		{
			mode = CONDITION_READ_ONLY;
		}
		else if(strcmp(argv[a], "-w") == 0 ||
		        strcmp(argv[a], "--writeonly") == 0)
		{
			mode = CONDITION_WRITE_ONLY;
		}
		else if(strcmp(argv[a], "-f") == 0 ||
		        strcmp(argv[a], "--force") == 0)
		{
			force = 1;
		}
		else if(strcmp(argv[a], "-l") == 0 ||
		        strcmp(argv[a], "--legacydir") == 0)
		{
			dirVersion = 0;
		}
		else if(strcmp(argv[a], "-n") == 0 ||
		        strcmp(argv[a], "--newdir") == 0)
		{
			dirVersion = 1;
		}
		else if(strcmp(argv[a], "-d") == 0 ||
		        strcmp(argv[a], "--getdata") == 0)
		{
			getData = 1;
		}
		else if(argv[a][0] == '-')
		{
			fprintf(stderr, "Unknown option %s provided\n", argv[a]);
			return 0;
		}
		else
		{
			if(vsn[0])
			{
				fprintf(stderr, "Error: two VSNs provided : %s and %s\n",
					vsn, argv[a]);
				return 0;
			}
			strncpy(vsn, argv[a], 10);
			vsn[9] = 0;
			if(strlen(vsn) != 8)
			{
				fprintf(stderr, "Error: VSN %s not 8 chars long!\n", argv[a]);
				return 0;
			}
			for(int i = 0; i < 8; i++)
			{
				vsn[i] = toupper(vsn[i]);
			}
		}
	}

	if(vsn[0] == 0)
	{
		printf("Error: no VSN provided!\n");
		return 0;
	}

	printf("About to proceed.  verbose=%d mode=%d force=%d vsn=%s\n",
		verbose, mode, force, vsn);

	if(!force)
	{
		if(mode != CONDITION_ERASE_ONLY)
		{
			printf("\nAbout to condition module %s.  This will destroy all\n", vsn);
			printf("contents of the module and will take a long time.  Are\n");
			printf("you sure you want to continue? [y|n]\n");
		}
		else
		{
			printf("\nAbout to erase module %s.  This will destroy all\n", vsn);
			printf("contents of the module.  Are you sure you want to\n");
			printf("continue? [y|n]\n");
		}
		for(;;)
		{
			rv = fgets(resp, 10, stdin);
			if(strcmp(resp, "Y\n") == 0 || strcmp(resp, "y\n") == 0)
			{
				if(verbose)
				{
					printf("OK -- continuing\n\n");
				}
				break;
			}
			else if(strcmp(resp, "N\n") == 0 || strcmp(resp, "n\n") == 0)
			{
				printf("OK -- I won't continue.\n\n");
				return 0;
			}
			else
			{
				printf("Your response was not understood.\n");
				printf("Continue? [y|n]\n");
			}
		}
	}

	v = initWatchdog();
	if(v < 0)
	{
		fprintf(stderr, "Error: Cannot start watchdog!\n");
		return -1;
	}

	oldsiginthand = signal(SIGINT, siginthand);

	/* 60 seconds should be enough to complete any XLR command */
	setWatchdogTimeout(60);

	difxMessageInit(-1, program);

	/* *********** */

	v = mk5erase(vsn, mode, verbose, dirVersion, getData);
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

	/* *********** */

	stopWatchdog();

	return 0;
}
