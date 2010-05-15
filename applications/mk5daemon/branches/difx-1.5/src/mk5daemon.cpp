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
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include <expat.h>
#include <difxmessage.h>
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

int *signalDie = 0;
typedef void (*sighandler_t)(int);
sighandler_t oldsigintHandler;

int usage(const char *pgm)
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
	fprintf(stderr, "  --condition-watch\n");
	fprintf(stderr, "  -w             Start the condition-watch program\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  --quiet\n");
	fprintf(stderr, "  -q             Don't multicast any status\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  --log-path <path>\n");
	fprintf(stderr, "  -l <path>      Put log files in <path>\n"); 
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
	fprintf(stderr, "\n");

	return 0;
}

Mk5Daemon *newMk5Daemon(const char *logPath)
{
	Mk5Daemon *D;

	D = (Mk5Daemon *)calloc(1, sizeof(Mk5Daemon));
	
	D->log = newLogger(logPath);
	D->process = PROCESS_NONE;
	D->loadMonInterval = 10;	/* seconds */
	gethostname(D->hostName, 32);
	D->isMk5 = strncasecmp(D->hostName, "mark5", 5) == 0 ? 1 : 0;
	printf("isMk5 = %d hostname = %s\n", D->isMk5, D->hostName);
	signalDie = &D->dieNow;
	Mk5Daemon_startMonitor(D);
	pthread_mutex_init(&D->processLock, 0);
	difxMessageSendDifxInfo("mk5daemon starting");

	return D;
}

int Mk5Daemon_system(const Mk5Daemon *D, const char *command, int verbose)
{
	int v;
	char message[MAX_MESSAGE_SIZE];

	if(verbose)
	{
		snprintf(message, MAX_MESSAGE_SIZE, "Executing: %s\n", command);
	
		Logger_logData(D->log, message);
	}

	v = system(command);

	// For some reason system returns -1 even if successful
	// possibly because of the fork()ed, threaded, ... situation?
#if 0
	if(v == -1)
	{
		snprintf(message, MAX_MESSAGE_SIZE, 
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
		if(D->process == PROCESS_MARK5A)
		{
			Mk5Daemon_stopMark5A(D);
			while(!D->processDone)
			{
				usleep(100000);
			}
			pthread_mutex_lock(&D->processLock);
			pthread_join(D->processThread, 0);
			pthread_mutex_unlock(&D->processLock);
		}
		deleteLogger(D->log);
		free(D);
	}
}

/* FIXME -- move to a /proc query */
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
	char message[MAX_MESSAGE_SIZE];

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
		/* FIXME -- watch for timeout here */
		D->process = PROCESS_NONE;
		pthread_mutex_unlock(&D->processLock);
	}

	if(t - D->lastMpifxcorrUpdate > 20 &&
		D->process == PROCESS_DATASTREAM)
	{
		pthread_mutex_lock(&D->processLock);
		if(!running("mpifxcorr"))
		{
			snprintf(message, MAX_MESSAGE_SIZE,
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

void startConditionWatch(const Mk5Daemon *D)
{
	const char *user;
	char command[MAX_COMMAND_SIZE];

	Logger_logData(D->log, "Starting condition_watch");

	if(fork())
	{
		return;
	}
	
	user = getenv("DIFX_USER_ID");
	if(!user)
	{
		user = difxUser;
	}

	snprintf(command, MAX_COMMAND_SIZE, "ssh -f %s@%s condition_watch", 
		user, D->hostName);

	Mk5Daemon_system(D, command, 1);

	exit(0);
}

int main(int argc, char **argv)
{
	Mk5Daemon *D;
	time_t t, lastTime, firstTime;
	char message[MAX_MESSAGE_SIZE];
	char str[16];
	int isHeadNode = 0;
	int i, ok=0;
	int justStarted = 1;
	int halfInterval;
	char logPath[256];
	const char *p;
	double mjd;
	int startCW = 0;

	p = getenv("DIFX_LOG_PATH");
	if(p)
	{
		strcpy(logPath, p);
	}
	else
	{
		strcpy(logPath, DefaultLogPath);
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
		else if(strcmp(argv[i], "-w") == 0 ||
		   strcmp(argv[i], "--condition-watch") == 0)
		{
			startCW = 1;
		}
		else if(strcmp(argv[i], "-h") == 0 ||
		   strcmp(argv[i], "--help") == 0)
		{
			return usage(argv[0]);
		}
		else if(strcmp(argv[i], "-q") == 0 ||
		   strcmp(argv[i], "--quiet") == 0)
		{
			setenv("DIFX_MESSAGE_PORT", "-1", 1);
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
				return usage(argv[0]);
			}
		}
		else
		{
			return usage(argv[0]);
		}
	}

	if(setuid(0) != 0)
	{
		fprintf(stderr, "Need suid root permission.  Bailing.\n");
		return 0;
	}

	if(fork())
	{
		printf("*** %s ver. %s spawned ***\n", program, version);
		return 0;
	}

	/* the next line is so that later calls to fork() don't end up with defunct
	 * children.  see http://www.ecst.csuchico.edu/~beej/guide/ipc/fork.html
	 */
	signal(SIGCLD, SIG_IGN);

	umask(02);

	difxMessageInit(-1, program);
	difxMessageSendDifxAlert("mk5daemon starting", DIFX_ALERT_LEVEL_INFO);

	difxMessagePrint();

	D = newMk5Daemon(logPath);
	D->isHeadNode = isHeadNode;

	snprintf(message, MAX_MESSAGE_SIZE, "Starting %s ver. %s\n", 
		program, version);
	Logger_logData(D->log, message);

	if(startCW && isHeadNode)
	{
		startConditionWatch(D);
	}

	oldsigintHandler = signal(SIGINT, sigintHandler);

	firstTime = lastTime = time(0);

	halfInterval = D->loadMonInterval/2;

	while(!D->dieNow)	/* program event loop */
	{
		t = time(0);
		mjd = 40587.0 + t/86400.0;

		if(t != lastTime)	/* we crossed a 1 second tick */
		{
			lastTime = t;

			ok = (checkStreamstor(D, t) == 0);

			if( (t % D->loadMonInterval) == 0)
			{
				Mk5Daemon_loadMon(D, mjd);
			}
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
				}
				justStarted = 0;
			}
		}

		usleep(100000);
	}

	snprintf(message, MAX_MESSAGE_SIZE, "Stopping %s ver. %s\n", 
		program, version);
	Logger_logData(D->log, message);

	deleteMk5Daemon(D);

	return 0;
}
