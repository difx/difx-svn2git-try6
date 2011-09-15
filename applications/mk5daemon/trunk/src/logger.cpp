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
#include <time.h>
#include <unistd.h>
#include "logger.h"

static int Logger_newFile(Logger *log, int mjd)
{
	char filename[256];

	if(log->out)
	{
		fclose(log->out);
	}

	if(mjd > 0)
	{
		sprintf(filename, "%s/%s.%05d.log",
			log->logPath, log->hostName, mjd);

		log->out = fopen(filename, "a");
	}
	else
	{
		log->out = 0;
	}

	return 0;
}

Logger *newLogger(const char *logPath)
{
	Logger *log;
	time_t t;
	int mjd;

	t = time(0);

	log = (Logger *)calloc(1, sizeof(Logger));

	if(logPath == 0)
	{
		log->logPath[0] = 0;
	}
	else
	{
		strcpy(log->logPath, logPath);
	}
	gethostname(log->hostName, 32);
	log->hostName[31] = 0;
	if(logPath && logPath[0])
	{
		mjd = (int)(40587.0 + t/86400.0);
	}
	else
	{
		mjd = -1;	/* use stdout */
	}
	Logger_newFile(log, mjd);

	pthread_mutex_init(&log->lock, 0);

	return log;
}

void deleteLogger(Logger *log)
{
	if(log)
	{
		pthread_mutex_destroy(&log->lock);
		if(log->out)
		{
			fclose(log->out);
		}
		free(log);
	}
}

int Logger_logData(Logger *log, const char *message)
{
	time_t t;
	struct tm curTime;
	double mjd;

	pthread_mutex_lock(&log->lock);

	if(!log->out)	/* i.e., if in embedded mode */
	{
		printf("%s", message);
		fflush(stdout);
	}
	else
	{
		t = time(0);
		gmtime_r(&t, &curTime);
		
		if(t != log->lastTime)
		{
			mjd = 40587.0 + t/86400.0;

			if(t/86400 != log->lastTime/86400)
			{
				Logger_newFile(log, (int)(mjd+0.1));
			}
			log->lastTime = t;
			fprintf(log->out, "\n");
			fprintf(log->out, "%04d/%03d %02d:%02d:%02d = %13.7f\n",
				curTime.tm_year+1900, curTime.tm_yday+1,
				curTime.tm_hour, curTime.tm_min, curTime.tm_sec,
				mjd);
		}
		fputs(message, log->out);
		fflush(log->out);
	}

	pthread_mutex_unlock(&log->lock);

	return 0;
}
