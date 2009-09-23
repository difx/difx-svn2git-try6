/***************************************************************************
 *   Copyright (C) 2009 by Adam Deller                                     *
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
// $Id: $
// $HeadURL: $
// $LastChangedRevision: $
// $Author: $
// $LastChangedDate: $
//
//============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "difxio/difx_input.h"
#include "difxio/difx_write.h"

DifxRule *newDifxRuleArray(int nRule)
{
        DifxRule *dr;
	int r;

        dr = (DifxRule *)calloc(nRule, sizeof(DifxRule));
        for(r = 0; r < nRule; r++)
        {
		sprintf(dr[r].configName, "");
                sprintf(dr[r].sourcename, "");
                sprintf(dr[r].scanId, "");
		sprintf(dr[r].calCode, "");
		dr[r].qual = -1;
		dr[r].mjdStart = -1.0;
		dr[r].mjdStop = -1.0;
        }

        return dr;
}

void deleteDifxRuleArray(DifxRule *dr)
{
	free(dr);
}

void fprintDifxRule(FILE *fp, const DifxRule *dr)
{
	fprintf(fp, "  Difx Rule for config %s : %p\n", dr->configName, dr);
	fprintf(fp, "    source  = %s\n", dr->sourcename);
	fprintf(fp, "    scanId  = %s\n", dr->scanId);
	fprintf(fp, "    calCode = %s\n", dr->calCode);
	fprintf(fp, "    qual    = %d\n", dr->qual);
	fprintf(fp, "    mjdStart= %f\n", dr->mjdStart);
	fprintf(fp, "    mjdStop = %f\n", dr->mjdStop);
}

void printDifxRule(const DifxRule *dr)
{
        fprintDifxRule(stdout, dr);
}

int writeDifxRuleArray(FILE *out, const DifxInput *D)
{
	int n; //number of lines written
	int i;
	DifxRule * dr;

	writeDifxLineInt(out, "NUM RULES", D->nRule);
	n = 1;
	for(i=0;i<D->nRule;i++)
	{
		dr = D->rule + i;
		if(strcmp(dr->sourcename, "") != 0) {
			writeDifxLine1(out, "RULE %d SOURCE", i, dr->sourcename);
			n = n+1;
		}
		if(strcmp(dr->scanId, "") != 0) {
			writeDifxLine1(out, "RULE %d SCAN ID", i, dr->scanId);
			n = n+1;
		}
		if(strcmp(dr->calCode, "") != 0) {
			writeDifxLine1(out, "RULE %d CALCODE", i, dr->calCode);
			n = n+1;
		}
		if(dr->qual >= 0) {
			writeDifxLineInt1(out, "RULE %d QUAL", i, dr->qual);
			n = n+1;
		}
		if(dr->mjdStart > 0.0) {
			writeDifxLineDouble1(out, "RULE %d MJD START", 
					     i, "%15.8f", dr->mjdStart);
			n = n+1;
		}
		if(dr->mjdStop > 0.0) {
			writeDifxLineDouble1(out, "RULE %d MJD STOP",
					     i, "%15.8f", dr->mjdStop);
			n = n+1;
		}
		writeDifxLine1(out, "RULE %d CONFIG NAME", i, dr->configName);
		n = n+1;
	}
	return n;
}

int ruleAppliesToScanSource(const DifxRule * dr, const DifxScan * ds, const DifxSource * src)
{
	if((strcmp(dr->sourcename, "") != 0 && strcmp(src->name, dr->sourcename) != 0) ||
	   (strcmp(dr->scanId, "") != 0  && strcmp(ds->identifier, dr->scanId) != 0) ||
	   (strcmp(dr->calCode, "") != 0 && strcmp(src->calCode, dr->calCode) != 0) ||
	   (dr->qual >= 0 && src->qual != dr->qual) ||
	   (dr->mjdStart > 0.0 && ds->mjdStart < dr->mjdStart) || 
	   (dr->mjdStop > 0.0 && ds->mjdEnd > ds->mjdEnd))
	{
		return 0;
	}
	return 1;
}



