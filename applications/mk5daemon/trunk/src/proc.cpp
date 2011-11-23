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
#include <cstring>
#include <cstdlib>
#include "proc.h"

/* routines to get useful information from /proc */

int procGetMem(int *memused, int *memtot)
{
	const int MaxLineLength=256;
	FILE *in;
	char line[MaxLineLength+1];
	char key[MaxLineLength+1];
	char *c;
	int val;

	*memused = 0;
	*memtot = 0;
	
	in = fopen("/proc/meminfo", "r");
	if(!in)
	{
		return -1;
	}
	
	for(;;)
	{
		c = fgets(line, MaxLineLength, in);
		if(!c)
		{
			break;
		}
		sscanf(line, "%s%d", key, &val);
		if(strcmp(key, "MemTotal:") == 0)
		{
			*memtot = val;
			*memused += val;
		}
		if(strcmp(key, "MemFree:") == 0 ||
		   strcmp(key, "Buffers:") == 0 ||
		   strcmp(key, "Cached:") == 0)
		{
			*memused -= val;
		}
	}

	fclose(in);

	return 0;
}

int procGetNet(long long *rx, long long *tx)
{
	const int MaxInterfaces=10;
	const int MaxLineLength=256;
	static long long lastrx[MaxInterfaces] = {0};
	static long long lasttx[MaxInterfaces] = {0};
	FILE *in;
	char line[MaxLineLength+1];
	char *c;
	long long a, b;
	int v;

	*rx = 0LL;
	*tx = 0LL;

	in = fopen("/proc/net/dev", "r");
	if(!in)
	{
		return -1;
	}

	for(int i = 0; i < MaxInterfaces; ++i)
	{
		c = fgets(line, MaxLineLength, in);
		if(!c)
		{
			break;
		}
		if(strncmp(line, "  eth", 5) == 0 || strncmp(line, "   ib", 5) == 0)
		{
			v = sscanf(line+7, "%lld%*d%*d%*d%*d%*d%*d%*d%lld", &a, &b);
			if(v >= 2)
			{

				/* take into account 32 bit counters on 32-bit machines */
				if(a < lastrx[i])
				{
					a = (a & 0xFFFFFFFFLL) | (lastrx[i] & 0xFFFFFFFF00000000LL);
					if(a < lastrx[i])
					{
						a += 0x100000000LL;
					}
				}
				if(b < lasttx[i])
				{
					b = (b & 0xFFFFFFFFLL) | (lasttx[i] & 0xFFFFFFFF00000000LL);
					if(b < lasttx[i])
					{
						b += 0x100000000LL;
					}
				}
				*rx += a;
				*tx += b;

				lastrx[i] = a;
				lasttx[i] = b;
			}
		}
	}

	fclose(in);

	return 0;
}

int procGetCPU(float *l1, float *l5, float *l15)
{
	const int MaxLineLength=256;
	FILE *in;
	char line[MaxLineLength+1];
	char *c;

	in = fopen("/proc/loadavg", "r");
	if(!in)
	{
		return -1;
	}

	c = fgets(line, MaxLineLength, in);
	if(c)
	{
		sscanf(line, "%f%f%f", l1, l5, l15);
	}
	else
	{
		*l1 = *l5 = *l15 = 0;
	}

	fclose(in);

	return 0;
}

int procGetStreamstor(int *busy)
{
	const int MaxLineLength=256;
	FILE *in;
	char line[MaxLineLength+1];
	char *c;

	*busy = 0;

	in = fopen("/proc/modules", "r");
	if(!in)
	{
		return -1;
	}

	for(;;)
	{
		c = fgets(line, MaxLineLength, in);
		if(!c)
		{
			break;
		}
		if(strncmp(line, "windrvr6 ", 9) == 0)
		{
			sscanf(line+9, "%*d %d", busy);

			break;
		}
	}

	fclose(in);

	return 0;
}

/* returns number of processes killed */
int killSuProcesses()
{
	const int MaxStrLen=256;
	const int MaxCmdLen=64;
	FILE *p;
	char cmd[] = "ps aux | grep su | grep difxlog";
	char str[MaxStrLen];
	char *r;
	char owner[16], timestr[16];
	int pid;
	float pcpu;
	int nKill=0;
	int n, t;

	n = snprintf(cmd, MaxCmdLen, "ps -C su -o user,pid,pcpu,time");
	if(n >= MaxCmdLen)
	{
		fprintf(stderr, "Warning: MaxCmdLen too small = %d vs. %d\n", MaxCmdLen, n);

		return -1;
	}
	p = popen(cmd, "r");

	str[MaxStrLen-1] = 0;
	for(;;)
	{
		r = fgets(str, MaxStrLen-1, p);
		if(!r)
		{
			break;
		}
		n = sscanf(str, "%s %d %f %s", owner, &pid, &pcpu, timestr);

		if(n < 4)
		{
			return -2;
		}
		if(strcmp(owner, "root") != 0)
		{
			continue;
		}
		if(pcpu < 90)
		{
			continue;
		}
		if(strlen(timestr) != 8 || timestr[2] != ':' || timestr[5] != ':')
		{
			continue;
		}
		timestr[2] = timestr[5] = 0;
		t = 3600*atoi(timestr) + 60*atoi(timestr+2) + atoi(timestr+5);
		if(t > 100)
		{
			n = snprintf(cmd, MaxCmdLen, "kill -9 %d", pid);
			if(n >= MaxCmdLen)
			{
				fprintf(stderr, "Warning: MaxCmdLen too small = %d vs. %d\n", MaxCmdLen, n);

				return -1;
			}
			if(system(cmd) != -1)
			{
				++nKill;
			}
		}
	}

	return nKill;
}
