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

#include <stdlib.h>
#include <string.h>
#include "difxio/difx_write.h"

int writeDifxDelay(const DifxInput *D, const char *filename)
{
	FILE *out;
	DifxScan *scan;
	int a, s, i;
	int l;
	char value[1024], temp[32];

	if(!D)
	{
		return -1;
	}
	if(D->nAntenna < 1 || !D->job || D->nScan < 0)
	{
		return 0;
	}

	out = fopen(filename, "w");
	if(!out)
	{
		fprintf(stderr, "Cannot open %s for write\n", filename);
		return -1;
	}

	/* FIXME : date/time */

	sprintf(value, "%8.6f", D->job->modelInc);
	writeDifxLine(out, "INCREMENT (SECS)", value);

	writeDifxLineInt(out, "NUM TELESCOPES", D->nAntenna);

	for(a = 0; a < D->nAntenna; a++)
	{
		writeDifxLine1(out, "TELESCOPE %d NAME", a, D->antenna[a].name);
	}

	writeDifxLineInt(out, "NUM SCANS", D->nScan);

	for(s = 0; s < D->nScan; s++)
	{
		scan = D->scan + s;
		writeDifxLineInt1(out, "SCAN %d POINTS", s, scan->nPoint);
		writeDifxLineInt1(out, "SCAN %d START PT", s,
			scan->startPoint);
		writeDifxLine1(out, "SCAN %d SRC NAME", s, scan->name);
		for(i = -1; i <= scan->nPoint+1; i++)
		{
			value[0] = 0;
			l = 0;
			for(a = 0; a < D->nAntenna; a++)
			{
				l += sprintf(value+l, "%12.10f \t", 
					scan->model[a][i].t);
			}
			writeDifxLine1(out, "RELATIVE INC %d", i, value);
		}
	}

	fclose(out);

	return 0;
}
