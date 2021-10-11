/***************************************************************************
 *   Copyright (C) 2021 by Walter Brisken                                  *
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
 * $Id:  $
 * $HeadURL:  $
 * $LastChangedRevision: $
 * $Author: $
 * $LastChangedDate:  $
 *
 *==========================================================================*/

#ifndef __VEX_THREAD_H__
#define __VEX_THREAD_H__

#include <iostream>
#include <string>

class VexThread
{
public:
	int threadId;
	int startRecordChan;	// start record channel number, within datastream
	int nChan;		// number of channels in this thread
	double sampRate;	// [samples/sec]
	std::string linkName;
	unsigned int nBit;
	int dataBytes;		// size of one frame, excluding header

	VexThread(int id) : threadId(id) {}


private:
};

std::ostream& operator << (std::ostream &os, const VexThread &x);

bool operator < (const VexThread &T1, const VexThread &T2);

bool operator == (const VexThread &T, int t);

#endif
