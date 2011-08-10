/***************************************************************************
 *   Copyright (C) 2008-2011 by Walter Brisken                             *
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
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include <expat.h>
#include <difxmessage.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "mk5daemon.h"
#include "../config.h"
#include "logger.h"
#include "proc.h"

const char program[] = PACKAGE_NAME;
const char author[]  = PACKAGE_BUGREPORT;
const char version[] = VERSION;

const int DefaultDifxMonitorPort = 50200;
const char DefaultDifxGroup[] = "224.2.2.1";
const char DefaultLogPath[] = "/tmp";
const char headNode[] = "swc000";
const char difxUser[] = "difx";

const int maxIdle = 25;		/* if streamstor card is idle this long */
				/* set current process to NONE */

const int defaultPacketSize = 5008;
const int defaultPayloadOffset = 40;
const int defaultDataFrameOffset = 0;
const int defaultPSNMode = 0;
const int defaultPSNOffset = 0;
const int defaultMACFilterControl = 1;

const int defaultStatsRange[] = { 75000, 150000, 300000, 600000, 1200000, 2400000, 4800000, -1 };

int *signalDie = 0;
typedef void (*sighandler_t)(int);
sighandler_t oldsigintHandler;


const char recordStateStrings[][10] =
{
	"off",
	"on",
	"halted",
	"throttled",
	"overflow",
	"waiting",

	"error"	/* last entry */
};

const char netProtocolStrings[][10] =
{
	"UDP",
	"L2",

	"Error"	/* last entry */
};


static void usage(const char *pgm)
{
	fprintf(stderr, "\n%s ver. %s   %s\n\n",
		program, version, author);
	fprintf(stderr, "A program to control Mark5A, handle Mark5 allocation "
		"manage VSNs, and\n");
	fprintf(stderr, "log all of the above.  Root permissions required.\n");
	fprintf(stderr, "\nUsage : %s [options]\n\n", pgm);
	fprintf(stderr, "options can include:\n");
	fprintf(stderr, "  --help\n");
	fprintf(stderr, "  -h             Print this help message\n"); 
	fprintf(stderr, "\n");
	fprintf(stderr, "  --headnode\n");
	fprintf(stderr, "  -H             Give head node capabilities\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  --quiet\n");
	fprintf(stderr, "  -q             Don't multicast any status\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  --log-path <path>\n");
	fprintf(stderr, "  -l <path>      Put log files in <path>\n"); 
	fprintf(stderr, "\n");
	fprintf(stderr, "  --user <user>\n");
	fprintf(stderr, "  -u <user>      use <user> when executing remote commands (default is 'difx') \n"); 
	fprintf(stderr, "\n");
	fprintf(stderr, "  --isMk5 \n");
	fprintf(stderr, "  -m             force mk5daemon on this host to act as Mark5 regardless of hostname \n"); 
	fprintf(stderr, "\n");
	fprintf(stderr, "Note: This program responds to the following "
			"environment variables:\n");
	fprintf(stderr, "  DIFX_LOG_DIR : change log path from default [%s]\n",
		DefaultLogPath);
	fprintf(stderr, "  DIFX_MESSAGE_GROUP : change multicast group "
		"from default [%s]\n", DefaultDifxGroup);
	fprintf(stderr, "  DIFX_MESSAGE_PORT : change multicast port "
		"from default [%d]\n", DefaultDifxMonitorPort);
	fprintf(stderr, "  STREAMSTOR_BIB_PATH : change streamstor firmware "
		"path from default\n");
	fprintf(stderr, "  DIFX_USER_ID : change user account for executing remote commands from default [%s]\n", difxUser);
	fprintf(stderr, "\n");
}

Mk5Daemon *newMk5Daemon(const char *logPath, const char *userID, int isMk5)
{
	Mk5Daemon *D;

	D = (Mk5Daemon *)calloc(1, sizeof(Mk5Daemon));
	
	D->log = newLogger(logPath);
	D->process = PROCESS_NONE;
	D->loadMonInterval = 10;	/* seconds */
	gethostname(D->hostName, 32);
	D->isMk5 = strncasecmp(D->hostName, "mark5", 5) == 0 ? 1 : 0;
	if (isMk5)
	{
		D->isMk5 = 1;
	}
	strncpy(D->userID, userID, 255);
	printf("isMk5 = %d hostname = %s\n", D->isMk5, D->hostName);
	signalDie = &D->dieNow;
	Mk5Daemon_startMonitor(D);
	Mk5Daemon_startVSIS(D);
	pthread_mutex_init(&D->processLock, 0);
	difxMessageSendDifxInfo("mk5daemon starting");
	D->activeBank = -1;

	D->packetSize = defaultPacketSize;
	D->dataFrameOffset = defaultDataFrameOffset;
	D->payloadOffset = defaultPayloadOffset;
	D->psnOffset = defaultPSNOffset;
	D->psnMode = defaultPSNMode;
	D->macFilterControl = defaultMACFilterControl;
	D->fillPattern = MARK5_FILL_PATTERN;
	D->netProtocol = NET_PROTOCOL_UDP;
	strcpy(D->dataSource, "ext");
	//strcpy(D->format, "mark5b");
	D->bitstreamMask = 0xFFFFFFFF;
	D->decimationRatio = 1;

	D->recordRate = 2048;	/* just a reference value to start with */

#ifdef HAVE_XLRAPI_H
	for(int b = 0; b < XLR_MAXBINS; b++)
	{
		D->driveStatsConfig[b].range = defaultStatsRange[b];
	}
#endif
	for(int b = 0; b < N_BANK; b++)
	{
		D->driveFail[b] = -1;	/* no failure */
	}

	const char *dmsMask = getenv("DEFAULT_DMS_MASK");
	if(dmsMask)
	{
		D->diskStateMask = atoi(dmsMask);
	}

	return D;
}

int Mk5Daemon_system(const Mk5Daemon *D, const char *command, int verbose)
{
	int v;
	char message[DIFX_MESSAGE_LENGTH];

	if(verbose)
	{
		snprintf(message, DIFX_MESSAGE_LENGTH, "Executing: %s\n", command);
	
		Logger_logData(D->log, message);
	}

	v = system(command);

	// For some reason system returns -1 even if successful
	// possibly because of the fork()ed, threaded, ... situation?
#if 0
	if(v == -1)
	{
		snprintf(message, DIFX_MESSAGE_LENGTH, 
			"system() failed running: %s\n", command);

		Logger_logData(D->log, message);
	}
#endif

	return v;
}

void deleteMk5Daemon(Mk5Daemon *D)
{
	difxMessageSendDifxInfo("mk5daemon stopping");
	signalDie = 0;
	if(D)
	{
		D->dieNow = 1;
		Mk5Daemon_stopMonitor(D);
		Mk5Daemon_stopVSIS(D);
		deleteLogger(D->log);
		free(D);
	}
}

#warning FIXME: move this to a proc query
int running(const char *name)
{
	const int MaxLineLength = 512;
	FILE *pin;
	int n;
	char command[MAX_COMMAND_SIZE];
	char line[MaxLineLength];

	snprintf(command, MAX_COMMAND_SIZE,  "ps -e | grep %s", name);

	pin = popen(command, "r");
	if(!pin)
	{
		printf("ERROR Cannot run ps\n");
		return 1;
	}

	n = fread(line, 1, MaxLineLength, pin);
	line[MaxLineLength-1] = 0;
	pclose(pin);

	if(n > 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int checkStreamstor(Mk5Daemon *D, time_t t)
{
	int v, busy;
	char message[DIFX_MESSAGE_LENGTH];

	if(!D->isMk5)
	{
		return 0;
	}

	v = procGetStreamstor(&busy);
	if(v < 0 && D->noDriver == 0)
	{
		D->noDriver = t;
		Logger_logData(D->log, 
			"windrvr6 driver went missing!\n");
		difxMessageSendDifxAlert(
			"windrvr6 disappeared!", 
			DIFX_ALERT_LEVEL_ERROR);
	}
	else
	{
		if(D->noDriver != 0)
		{
			D->noDriver = 0;
			Logger_logData(D->log, 
				"windrvr6 driver came back!\n");
			difxMessageSendDifxAlert(
				"windrvr6 reappeared!", 
				DIFX_ALERT_LEVEL_WARNING);
		}
	}

	if(D->noDriver)
	{
		return -1;
	}

	if(busy > 2)
	{
		D->idleCount = 0;
		pthread_mutex_lock(&D->processLock);
		if(D->process == PROCESS_NONE)
		{
			Logger_logData(D->log, 
				"Some process took control of windrvr6\n");
			difxMessageSendDifxAlert(
				"Some process took control of windrvr6", 
				DIFX_ALERT_LEVEL_INFO);
			D->process = PROCESS_UNKNOWN;
		}
		pthread_mutex_unlock(&D->processLock);
	}
	else
	{
		D->idleCount++;
	}

	if(D->idleCount > maxIdle && D->process != PROCESS_NONE && !D->processDone)
	{
		pthread_mutex_lock(&D->processLock);
		Logger_logData(D->log, 
			"Some process gave back control of windrvr6\n");
		difxMessageSendDifxAlert(
			"Some process gave back control of windrvr6", 
			DIFX_ALERT_LEVEL_INFO);

#warning FIXME: watch for a timeout here

		D->process = PROCESS_NONE;
		pthread_mutex_unlock(&D->processLock);
	}

	if(t - D->lastMpifxcorrUpdate > 20 &&
		D->process == PROCESS_DATASTREAM)
	{
		pthread_mutex_lock(&D->processLock);
		if(!running("mpifxcorr"))
		{
			snprintf(message, DIFX_MESSAGE_LENGTH,
				"Detected premature end of mpifxcorr at %s\n",
				ctime(&t));
			Logger_logData(D->log, message);

			D->process = PROCESS_NONE;
		}
		else
		{
			/* note that it is still alive */
			D->lastMpifxcorrUpdate = t;
		}
		pthread_mutex_unlock(&D->processLock);
	}

	return 0;
}

void sigintHandler(int j)
{
	if(signalDie)
	{
		*signalDie = 1;
	}
	signal(SIGINT, oldsigintHandler);
}

int main(int argc, char **argv)
{
	Mk5Daemon *D;
	time_t t, lastTime, firstTime;
	char message[DIFX_MESSAGE_LENGTH];
	char str[16];
	int isHeadNode = 0;
	int isMk5 = 0;
	int i;
	int halfInterval;
	char logPath[256];
	const char *p, *u;
	char userID[256];
	double mjd;
	fd_set socks;
	struct timeval timeout;
	int readSocks;
	int highSock;
#ifdef HAVE_XLRAPI_H
	int ok = 0;	/* FIXME: combine with D->ready? */
	int justStarted = 1;
	int recordFD;
#endif

	// Prevent any zombies
	signal(SIGCHLD, SIG_IGN);

	p = getenv("DIFX_LOG_PATH");
	if(p)
	{
		strcpy(logPath, p);
	}
	else
	{
		strcpy(logPath, DefaultLogPath);
	}

	u = getenv ("DIFX_USER_ID");
	if (u)
	{
		strcpy(userID, u);
	}
	else
	{
                strcpy(userID, difxUser);

	}

	sprintf(str, "%d", DefaultDifxMonitorPort);
	setenv("DIFX_MESSAGE_PORT", str, 0);
	setenv("DIFX_MESSAGE_GROUP", DefaultDifxGroup, 0);
	setenv("STREAMSTOR_BIB_PATH", "/usr/share/streamstor/bib", 0);

	if(argc > 1) for(i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "-H") == 0 ||
		   strcmp(argv[i], "--headnode") == 0)
		{
			isHeadNode = 1;
		}
		else if(strcmp(argv[i], "-h") == 0 ||
		   strcmp(argv[i], "--help") == 0)
		{
			usage(argv[0]);

			return EXIT_SUCCESS;
		}
		else if(strcmp(argv[i], "-q") == 0 ||
		   strcmp(argv[i], "--quiet") == 0)
		{
			setenv("DIFX_MESSAGE_PORT", "-1", 1);
		}
		else if( strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--user") == 0)
		{
			i++;
			strcpy(userID, argv[i]);
		}
		else if ( strcmp(argv[i], "-m") == 0 ||  strcmp(argv[i], "--isMk5") == 0)
		{
                        strcpy(userID, argv[i]);
			isMk5 = 1;
                }
		else if(i < argc-1)
		{
			if(strcmp(argv[i], "-l") == 0 ||
			   strcmp(argv[i], "--log-path") == 0)
			{
				i++;
				strcpy(logPath, argv[i]);
			}
			else
			{
				usage(argv[0]);

				return EXIT_FAILURE;
			}
		}
		else
		{
			usage(argv[0]);

			return EXIT_FAILURE;
		}
	}

	if(setuid(0) != 0)
	{
		fprintf(stderr, "Needs to run with root permission.  Bailing.\n");
		return EXIT_FAILURE;
	}

	if(fork())
	{
		printf("*** %s ver. %s spawned ***\n", program, version);

		return EXIT_SUCCESS;
	}

	/* the next line is so that later calls to fork() don't end up with defunct
	 * children.  see http://www.ecst.csuchico.edu/~beej/guide/ipc/fork.html
	 */
	signal(SIGCLD, SIG_IGN);

	umask(02);

	difxMessageInit(-1, program);
	difxMessageSendDifxAlert("mk5daemon starting", DIFX_ALERT_LEVEL_INFO);

	difxMessagePrint();

	D = newMk5Daemon(logPath, userID, isMk5);
	D->isHeadNode = isHeadNode;

	snprintf(message, DIFX_MESSAGE_LENGTH, "Starting %s ver. %s\n", 
		program, version);
	Logger_logData(D->log, message);

	oldsigintHandler = signal(SIGINT, sigintHandler);

	firstTime = lastTime = time(0);

	halfInterval = D->loadMonInterval/2;

	while(!D->dieNow)
	{
		t = time(0);
		mjd = 40587.0 + t/86400.0;

		if(t != lastTime)	/* we crossed a 1 second tick */
		{
			lastTime = t;

#ifdef HAVE_XLRAPI_H
			ok = (checkStreamstor(D, t) == 0);
#endif

			if( (t % D->loadMonInterval) == 0)
			{
				Mk5Daemon_loadMon(D, mjd);
			}
#ifdef HAVE_XLRAPI_H
			else if( (t % D->loadMonInterval) == halfInterval)
			{
				if(D->skipGetModule)
				{
					D->skipGetModule = 0;
				}
				else
				{
					Mk5Daemon_getModules(D);
				}
			}
			if(t - firstTime > 15 && D->isMk5 &&
				strncasecmp(D->hostName, "mark5", 5) == 0)
			{
				if(justStarted)
				{
					Mk5Daemon_getStreamstorVersions(D);
					logStreamstorVersions(D);
					Mk5Daemon_getModules(D);
					logMk5Smart(D, BANK_A);
					logMk5Smart(D, BANK_B);
					D->systemReady = 1;	/* initial check-out done */
				}
				justStarted = 0;
			}
#endif
		}

		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		highSock = 0;
		FD_ZERO(&socks);

		if(D->acceptSock)
		{
			FD_SET(D->acceptSock, &socks);
			if(D->acceptSock > highSock)
			{
				highSock = D->acceptSock;
			}
			for(int c = 0; c < MaxConnections; c++)
			{
				if(D->clientSocks[c] > 0)
				{
					FD_SET(D->clientSocks[c], &socks);
					if(D->clientSocks[c] > highSock)
					{
						highSock = D->clientSocks[c];
					}
				}
			}
		}
#ifdef HAVE_XLRAPI_H
		if(D->recordPipe)
		{
			recordFD = fileno(D->recordPipe);
			FD_SET(recordFD, &socks);
			if(recordFD > highSock)
			{
				highSock = recordFD;
			}
		}
		else
		{
			recordFD = -2;
		}
#endif
		if(D->difxSock)
		{
			FD_SET(D->difxSock, &socks);
			if(D->difxSock > highSock)
			{
				highSock = D->difxSock;
			}
		}

		readSocks = select(highSock+1, &socks, 0, 0, &timeout);

		if(readSocks < 0)
		{
			snprintf(message, DIFX_MESSAGE_LENGTH, "Select returned %d\n", readSocks);
			Logger_logData(D->log, message);

			D->dieNow = 1;
		}

		if(readSocks == 0)
		{
			continue;
		}

		if(D->acceptSock)
		{
			if(FD_ISSET(D->acceptSock, &socks))
			{
				int newSock = accept(D->acceptSock, 0, 0);
				if(newSock < 0)
				{
					snprintf(message, DIFX_MESSAGE_LENGTH, "VSI-S accept failure%d\n", newSock);
					Logger_logData(D->log, message);
				}
				else
				{
					/* find a slot for it */
					for(int c = 0; c < MaxConnections; c++)
					{
						if(D->clientSocks[c] == 0)
						{
							D->clientSocks[c] = newSock;
							newSock = -1;

							snprintf(message, DIFX_MESSAGE_LENGTH, "New VSI-S connection into slot %d of %d\n", c, MaxConnections);
							Logger_logData(D->log, message);

							break;
						}
					}
					if(newSock != -1)
					{
						Logger_logData(D->log, "No room for new VSI-S connection\n");

						close(newSock);
					}
				}
			}

			for(int c = 0; c < MaxConnections; c++)
			{
				if(FD_ISSET(D->clientSocks[c], &socks))
				{
					int v = handleVSIS(D, D->clientSocks[c]);
					
					if(v < 0) /* connection closed? */
					{
						close(D->clientSocks[c]);
						D->clientSocks[c] = 0;
						snprintf(message, DIFX_MESSAGE_LENGTH, "VSI-S connection on slot %d closed\n", c);
						Logger_logData(D->log, message);
					}
				}
			}
		}
#ifdef HAVE_XLRAPI_H
		if(recordFD >= 0 && FD_ISSET(recordFD, &socks))
		{
			char *r = fgets(message, DIFX_MESSAGE_LENGTH-1, D->recordPipe);
			if(r)
			{
				char A[15][40];
				int n = sscanf(message, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", 
					A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[8], A[9], A[10], A[11], A[12], A[13], A[14]);
				if(n > 0)
				{
					if(strcmp(A[0], "Pointer") == 0 && n > 2)
					{
						D->bytesUsed[D->activeBank] = atoll(A[1]);
						D->recordRate = atof(A[2]);
					}
					else if(strcmp(A[0], "Stats") == 0 && n >= 10)
					{
						int drive = atoi(A[1]);
						for(int b = 0; b < 8; b++)
						{
							D->driveStats[D->activeBank][drive][b].count += atoi(A[2+b]);
						}
					}
					else if(strcmp(A[0], "Error") == 0)
					{
						D->errorFlag[D->activeBank] = 1;
					}
					else if(strcmp(A[0], "SystemError") == 0)
					{
						D->errorFlag[D->activeBank] = 1;
					}
					else if(strcmp(A[0], "Halted") == 0)
					{
						D->recordState = RECORD_HALTED;
					}
					else if(strcmp(A[0], "Overflow") == 0)
					{
						D->recordState = RECORD_OVERFLOW;
					}
					else if(strcmp(A[0], "Drive") == 0 && n == 3)
					{
						if(strcmp(A[2], "failed") == 0)
						{
							D->driveFail[D->activeBank] = atoi(A[1]);
						}
					}
					else if(strcmp(A[0], "Bank") == 0 && n == 13)
					{
						/* new module inserted */
						int bank = A[1][0]-'A';
						if(bank >= 0 && bank < N_BANK && bank != D->activeBank)
						{
							clearModuleInfo(D, bank);
							clearMk5Stats(D, bank);
							if(strcmp(A[2], "none") == 0)
							{
								D->vsns[bank][0] = 0;
							}
							else
							{
								strncpy(D->vsns[bank], A[2], 8);
								D->vsns[bank][8] = 0;
							}
						}
					}
				}
			}
			if(feof(D->recordPipe))
			{
				pclose(D->recordPipe);
				D->recordPipe = 0;
				D->recordState = RECORD_OFF;

				clearModuleInfo(D, D->activeBank);
				Mk5Daemon_getModules(D);
			}
		}
#endif
		if(D->difxSock && FD_ISSET(D->difxSock, &socks))
		{
			int n, v;
			DifxMessageGeneric G;
			char from[20];

			n = difxMessageReceive(D->difxSock, message, DIFX_MESSAGE_LENGTH-1, from);

			if(n > 0)
			{
				message[n] = 0;
				v = difxMessageParse(&G, message);
				switch(G.type)
				{
				case DIFX_MESSAGE_MARK5STATUS:
					handleMk5Status(D, &G);
					break;
				case DIFX_MESSAGE_COMMAND:
					handleCommand(D, &G);
					break;
				case DIFX_MESSAGE_START:
					Mk5Daemon_startMpifxcorr(D, &G);
					break;
				case DIFX_MESSAGE_CONDITION:
					handleCondition(D, &G);
					break;
				default:
					break;
				}
			}
		}
		if(D->processDone)
		{
			pthread_mutex_lock(&D->processLock);

			pthread_join(D->processThread, 0);
			D->process = PROCESS_NONE;
			D->processDone = 0;
			
			pthread_mutex_unlock(&D->processLock);
		}	
	}

#ifdef HAVE_XLRAPI_H
	if(D->recordPipe > 0)
	{
		pclose(D->recordPipe);
		D->recordPipe = 0;
		D->recordState = RECORD_OFF;
	}
#endif

	Mk5Daemon_stopMonitor(D);
	Mk5Daemon_stopVSIS(D);

	snprintf(message, DIFX_MESSAGE_LENGTH, "Stopping %s ver. %s\n", program, version);
	Logger_logData(D->log, message);

	deleteMk5Daemon(D);

	return EXIT_SUCCESS;
}
