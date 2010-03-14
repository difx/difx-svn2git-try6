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
#include <pwd.h>
#include <sys/statvfs.h>
#include "mk5daemon.h"

const char defaultMpiWrapper[] = "mpirun";
const char defaultMpiOptions[] = "--mca btl ^udapl,openib --mca mpi_yield_when_idle 1";
const char defaultDifxProgram[] = "mpifxcorr";

typedef struct
{
	char hostname[DIFX_MESSAGE_PARAM_LENGTH];
	int n;
} Uses;

static int addUse(Uses *U, const char *hostname)
{
	int i;

	for(i = 0; U[i].n; i++)
	{
		if(strcmp(hostname, U[i].hostname) == 0)
		{
			U[i].n++;
			return i;
		}
	}

	strcpy(U[i].hostname, hostname);
	U[i].n = 1;

	return i;
}

static int getUse(const Uses *U, const char *hostname)
{
	for(int i = 0; U[i].n; i++)
	{
		if(strcmp(hostname, U[i].hostname) == 0)
		{
			return U[i].n;
		}
	}

	return 0;
}

static int checkDiskFree(const char *path, long long minFree)
{
	struct statvfs fiData;
	char message[MAX_MESSAGE_SIZE] = "";
	int v;
	long long freeSpace;
	
	v = statvfs(path, &fiData);

	if(v == 0)
	{
		freeSpace = fiData.f_bsize * fiData.f_bavail;
		if(freeSpace < minFree)
		{
			snprintf(message, MAX_MESSAGE_SIZE, 
				"%s has less than %lld bytes free.  mpifxcorr will likely crash!", 
				path, minFree);
		}
		else if(fiData.f_ffree < 3)
		{
			snprintf(message, MAX_MESSAGE_SIZE, 
				"%s has no free inodes.  mpifxcorr will likely crash!", 
				path);
		}
	}
	else
	{
		snprintf(message, MAX_MESSAGE_SIZE, 
			"statvfs failed when accessing directory %s", 
			path);
	}

	if(message[0])
	{
		difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_ERROR);
		return -1;
	}

	return 0;
}

static int isUsed(const Mk5Daemon *D, const DifxMessageStart *S)
{
	if(strcmp(S->headNode, D->hostName) == 0)
	{
		return 1;
	}
	for(int i = 0; i < S->nDatastream; i++)
	{
		if(strcmp(S->datastreamNode[i], D->hostName) == 0)
		{
			return 1;
		}
	}
	for(int i = 0; i < S->nProcess; i++)
	{
		if(strcmp(S->processNode[i], D->hostName) == 0)
		{
			return 1;
		}
	}

	return 0;
}

void Mk5Daemon_startMpifxcorr(Mk5Daemon *D, const DifxMessageGeneric *G)
{
	int l, n;
	int childPid;
	char filebase[DIFX_MESSAGE_FILENAME_LENGTH];
	char filename[DIFX_MESSAGE_FILENAME_LENGTH];
	char destdir[DIFX_MESSAGE_FILENAME_LENGTH];
	char message[MAX_MESSAGE_SIZE];
	char command[MAX_COMMAND_SIZE];
	FILE *out;
	Uses *uses;
	const char *jobName;
	const DifxMessageStart *S;
	int outputExists = 0;
	const char *mpiOptions;
	const char *mpiWrapper;
	const char *difxProgram;
	int returnValue;

	if(!G)
	{
		difxMessageSendDifxAlert(
			"Developer error: Mk5Daemon_startMpifxcorr received null DifxMessageGeneric",
			DIFX_ALERT_LEVEL_ERROR);
		return;
	}

	S = &G->body.start;

	if(S->headNode[0] == 0 || S->nDatastream <= 0 || S->nProcess <= 0 || S->inputFilename[0] != '/')
	{
		difxMessageSendDifxAlert("Malformed DifxStart message received", DIFX_ALERT_LEVEL_ERROR);
		Logger_logData(D->log, "Mk5Daemon_startMpifxcorr: degenerate request\n");
		return;
	}

	if(G->nTo != 1)
	{
		return;
	}

	if(isUsed(D, S))
	{
		returnValue = checkDiskFree("/tmp", 24000);
	}
	else
	{
		returnValue = 0;
	}

	if(strcmp(G->to[0], D->hostName) != 0)
	{
		return;
	}

	if(returnValue < 0)
	{
		difxMessageSendDifxAlert("Since /tmp is full, mpifxcorr will not be started.",
			DIFX_ALERT_LEVEL_ERROR);
		return;
	}


	/* Check to make sure the destination directory has some free space */
	strcpy(destdir, S->inputFilename);
	n = 0;
	for(l = 0; destdir[l]; l++)
	{
		if(destdir[l] == '/')
		{
			n = l;
		}
	}
	destdir[n+1] = 0;
	returnValue = checkDiskFree(destdir, 100000000);
	if(returnValue < 0)
	{
		snprintf(message, MAX_MESSAGE_SIZE, 
			"The output directory %s is full, mpifxcorr will not be started.", 
			destdir);
		difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_ERROR);
		return;
	}


	if(!D->isHeadNode)
	{
		difxMessageSendDifxAlert("Attempt to start job from non head node", DIFX_ALERT_LEVEL_ERROR);
		Logger_logData(D->log, "Mk5Daemon_startMpifxcorr: I am not a head node\n");
		return;
	}

	if(strlen(S->inputFilename) + 12 > DIFX_MESSAGE_FILENAME_LENGTH)
	{
		difxMessageSendDifxAlert("Attempt to start job with filename that is too long", DIFX_ALERT_LEVEL_ERROR);

		return;
	}

	/* generate filebase */
	strcpy(filebase, S->inputFilename);
	l = strlen(filebase);
	for(int i = l-1; i > 0; i--)
	{
		if(filebase[i] == '.')
		{
			filebase[i] = 0;
			break;
		}
	}
	jobName = filebase;
	for(int i = 0; filebase[i]; i++)
	{
		if(filebase[i] == '/')
		{
			jobName = filebase + i + 1;
		}
	}

	if(access(S->inputFilename, F_OK) != 0)
	{
		snprintf(message, MAX_MESSAGE_SIZE, 
			"Input file %s does not exist.  Aborting correlation.",
			S->inputFilename);
		difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_ERROR);

		snprintf(message, MAX_MESSAGE_SIZE,
			"Mk5Daemon_startMpifxcorr: input file %s does not exist\n", 
			S->inputFilename);
		Logger_logData(D->log, message);

		return;
	}

	sprintf(filename, "%s.difx", filebase);
	if(access(filename, F_OK) == 0)
	{
		outputExists = 1;
	}
	
	if(outputExists && !S->force)
	{
		snprintf(message, MAX_MESSAGE_SIZE, 
			"Output file %s exists.  Aborting correlation.", 
			filename);
		difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_ERROR);
		
		snprintf(message, MAX_MESSAGE_SIZE,
			"Mk5Daemon_startMpifxcorr: output file %s exists\n", 
			filename);
		Logger_logData(D->log, message);

		return;
	}

	/* lock state.  Make sure to unlock if early return happens! */
	pthread_mutex_lock(&D->processLock);

	/* determine usage of each node */
	uses = (Uses *)calloc(1 + S->nProcess + S->nDatastream, sizeof(Uses));
	addUse(uses, S->headNode);
	for(int i = 0; i < S->nProcess; i++)
	{
		addUse(uses, S->processNode[i]);
	}
	for(int i = 0; i < S->nDatastream; i++)
	{
		addUse(uses, S->datastreamNode[i]);
	}


	/* write machines file */
	sprintf(filename, "%s.machines", filebase);
	out = fopen(filename, "w");
	if(!out)
	{
		snprintf(message, MAX_MESSAGE_SIZE,
			"Cannot open %s for write", filename);
		difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_ERROR);

		snprintf(message, MAX_MESSAGE_SIZE,
			"Mk5Daemon_startMpifxcorr: cannot open %s for write\n", 
			filename);
		Logger_logData(D->log, message);

		pthread_mutex_unlock(&D->processLock);
		free(uses);
		
		return;
	}

	fprintf(out, "%s slots=1 max-slots=%d\n", S->headNode, getUse(uses, S->headNode));
	for(int i = 0; i < S->nDatastream; i++)
	{
		n = getUse(uses, S->datastreamNode[i]);
		fprintf(out, "%s slots=1 max-slots=%d\n", S->datastreamNode[i], n);
	}
	for(int i = 0; i < S->nProcess; i++)
	{
		n = getUse(uses, S->processNode[i]);
		fprintf(out, "%s slots=1 max-slots=%d\n", S->processNode[i], n);
	}

	fclose(out);
	/* change ownership and permissions to match the input file */
	snprintf(command, MAX_COMMAND_SIZE, "chown --reference=%s %s", 
		S->inputFilename, filename);
	Mk5Daemon_system(D, command, 1);
	
	snprintf(command, MAX_COMMAND_SIZE, "chmod --reference=%s %s", 
		S->inputFilename, filename);
	Mk5Daemon_system(D, command, 1);


	/* write threads file */
	sprintf(filename, "%s.threads", filebase);
	
	out = fopen(filename, "w");
	if(!out)
	{
		snprintf(message, MAX_MESSAGE_SIZE, "Cannot open %s for write", 
			filename);
		difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_ERROR);
		
		snprintf(message, MAX_MESSAGE_SIZE, 
			"Mk5Daemon_startMpifxcorr: cannot open %s for write\n", 
			filename);

		Logger_logData(D->log, message);
		pthread_mutex_unlock(&D->processLock);
		free(uses);

		return;
	}

	fprintf(out, "NUMBER OF CORES:    %d\n", S->nProcess);

	for(int i = 0; i < S->nProcess; i++)
	{
		n = S->nThread[i] - getUse(uses, S->processNode[i]) + 1;
		if(n <= 0)
		{
			n = 1;
		}
		fprintf(out, "%d\n", n);
	}

	fclose(out);
	/* change ownership and permissions to match the input file */
	snprintf(command, MAX_COMMAND_SIZE, "chown --reference=%s %s", 
		S->inputFilename, filename);
	Mk5Daemon_system(D, command, 1);

	snprintf(command, MAX_COMMAND_SIZE, "chmod --reference=%s %s", 
		S->inputFilename, filename);
	Mk5Daemon_system(D, command, 1);


	/* Don't need usage info anymore */
	free(uses);

	pthread_mutex_unlock(&D->processLock);

	if(S->mpiOptions[0])
	{
		mpiOptions = S->mpiOptions;
	}
	else
	{
		mpiOptions = defaultMpiOptions;
	}

	if(S->mpiWrapper[0])
	{
		mpiWrapper = S->mpiWrapper;
	}
	else
	{
		mpiWrapper = defaultMpiWrapper;
	}

	if(S->difxProgram[0])
	{
		difxProgram = S->difxProgram;
	}
	else
	{
		difxProgram = defaultDifxProgram;
	}

	childPid = fork();

	/* here is where the spawning of mpifxcorr happens... */
	if(childPid == 0)
	{
		const char *user;

		user = getenv("DIFX_USER_ID");
		if(!user)
		{
			user = difxUser;
		}

		if(S->force && outputExists)
		{
			snprintf(command, MAX_COMMAND_SIZE, 
				"/bin/rm -rf %s.difx/", filebase);

			difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_INFO);
		
			Mk5Daemon_system(D, command, 1);
		}

		difxMessageSendDifxAlert("mpifxcorr spawning process", DIFX_ALERT_LEVEL_INFO);

		snprintf(command, MAX_COMMAND_SIZE, "su - %s -c 'ssh -x %s \"%s -np %d --bynode --hostfile %s.machines %s %s %s\"'", 
			user,
			S->headNode,
			mpiWrapper,
			1 + S->nDatastream + S->nProcess,
			filebase,
			mpiOptions,
			difxProgram,
			S->inputFilename);

		snprintf(message, MAX_MESSAGE_SIZE, "Executing: %s", command);
		difxMessageSendDifxAlert(message, DIFX_ALERT_LEVEL_INFO);

		snprintf(message, MAX_MESSAGE_SIZE, "Spawning %d processes", 
			1 + S->nDatastream + S->nProcess);
		difxMessageSendDifxStatus2(jobName, DIFX_STATE_SPAWNING, 
			message);

		/* register this job with the Transient Event Monitor */
		EventQueue *Q = D->eventManager.startJob(jobName);
		for(int i = 0; i < S->nDatastream; i++)
		{
			Q->addMark5Unit(S->datastreamNode[i]);
		}
		Q->setUser(user);
		/* cause mpifxcorr to run! */
		returnValue = Mk5Daemon_system(D, command, 1);

		/* deregister this job with the Transient Event Monitor after 
		 * performing any needed data copying
		 */
		D->eventManager.stopJob(jobName);

		/* FIXME -- make use of returnValue  */
		difxMessageSendDifxStatus2(jobName, DIFX_STATE_MPIDONE, "");
		difxMessageSendDifxAlert("mpifxcorr process done", 
			DIFX_ALERT_LEVEL_INFO);

		/* change ownership to match input file */
		snprintf(command, MAX_COMMAND_SIZE, 
			"chown --recursive --reference=%s %s.difx", 
			S->inputFilename, filebase);
		Mk5Daemon_system(D, command, 1);

		snprintf(command, MAX_COMMAND_SIZE,
			"chmod g+w %s.difx", filebase);
		Mk5Daemon_system(D, command, 1);

		snprintf(command, MAX_COMMAND_SIZE, 
			"chmod --reference=%s %s.difx/*", 
			S->inputFilename, filebase);
		Mk5Daemon_system(D, command, 1);

		exit(0);
	}

	/* if we got here, we are the parent process */
	/* now spawn the difxlog process. */
	if(fork() == 0)
	{
		const char *user;

		user = getenv("DIFX_USER_ID");
		if(!user)
		{
			user = difxUser;
		}

		snprintf(command, MAX_COMMAND_SIZE,
			"su - %s -c 'ssh -x %s \"difxlog %s %s.difxlog 4 %d\"'",
			user, S->headNode, jobName, filebase, childPid);
		Mk5Daemon_system(D, command, 1);

		/* change ownership to match input file */
		snprintf(command, MAX_COMMAND_SIZE,
			"chown --reference=%s %s.difxlog", 
			S->inputFilename, filebase);
		Mk5Daemon_system(D, command, 1);

		snprintf(command, MAX_COMMAND_SIZE, 
			"chmod --reference=%s %s.difxlog", 
			S->inputFilename, filebase);
		Mk5Daemon_system(D, command, 1);

		exit(0);
	}
}
