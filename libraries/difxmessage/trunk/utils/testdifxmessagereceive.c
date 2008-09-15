/***************************************************************************
 *   Copyright (C) 2008 by Walter Brisken                                  *
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
 * $HeadURL: https://svn.atnf.csiro.au/difx/libraries/mark5access/trunk/mark5access/mark5_stream.c $
 * $LastChangedRevision$
 * $Author$
 * $LastChangedDate$
 *
 *==========================================================================*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "difxmessage.h"


int main(int argc, char **argv)
{
	int sock;
	int l;
	char message[1024], from[32];
	time_t t;
	char timestr[32];
	DifxMessageGeneric G;

	difxMessageInit(-1, argv[0]);
	difxMessagePrint();

	sock = difxMessageReceiveOpen();

	for(;;)
	{
		from[0] = 0;
		l = difxMessageReceive(sock, message, 1023, from);
		if(l < 0)
		{
			usleep(100000);
			continue;
		}
		message[l] = 0;
		difxMessageParse(&G, message);
		difxMessageGenericPrint(&G);
		if(strncmp(message, "exit", 4) == 0)
		{
			break;
		}
		time(&t);
		strcpy(timestr, ctime(&t));
		timestr[strlen(timestr)-1] = 0;
		printf("[%s %s] %s\n", timestr, from, message);
		fflush(stdout);
	}

	difxMessageReceiveClose(sock);

	return 0;
}
