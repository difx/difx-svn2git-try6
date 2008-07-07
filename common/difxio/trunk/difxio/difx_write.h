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

#ifndef __DIFX_WRITE_H__
#define __DIFX_WRITE_H__

#include <stdio.h>
#include "difxio/difx_input.h"

int writeDifxLine(FILE *out, const char *key, const char *value);

int writeDifxLineInt(FILE *out, const char *key, int value);

int writeDifxLineInt1(FILE *out, const char *key, int i1, int value);

int writeDifxLine1(FILE *out, const char *key, int i1, const char *value);

int writeDifxLine2(FILE *out, const char *key, int i1, int i2, 
	const char *value);

int writeDifxLine3(FILE *out, const char *key, int i1, int i2, int i3, 
	const char *value);

#endif
