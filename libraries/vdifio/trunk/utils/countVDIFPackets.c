/***************************************************************************
 *   Copyright (C) 2010 by Adam Deller                                     *
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
 * $Id: stripVDIF.c 2006 2010-03-04 16:43:04Z AdamDeller $
 * $HeadURL:  $
 * $LastChangedRevision: 2006 $
 * $Author: AdamDeller $
 * $LastChangedDate: 2010-03-04 09:43:04 -0700 (Thu, 04 Mar 2010) $
 *
 *==========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include "vdifio.h"

const char program[] = "padVDIF";
const char author[]  = "Adam Deller <adeller@nrao.edu>";
const char version[] = "0.1";
const char verdate[] = "20100217";

int usage()
{
  fprintf(stderr, "\n%s ver. %s  %s  %s\n\n", program, version,
          author, verdate);
  fprintf(stderr, "A program to count the number of missing packets for a given thread\n");
  fprintf(stderr, "\nUsage: %s <VDIF input file> <Mbps> <theadId>\n", program);
  fprintf(stderr, "\n<VDIF input file> is the name of the VDIF file to read\n");
  fprintf(stderr, "\n<Mbps> is the data rate in Mbps expected for this file\n");
  fprintf(stderr, "\n<threadId> is the threadId to check for\n");

  return 0;
}

int main(int argc, char **argv)
{
  int VERBOSE = 0;
  char buffer[MAX_VDIF_FRAME_BYTES];
  FILE * input;
  FILE * output;
  int readbytes, framebytes, framemjd, framesecond, framenumber, frameinvalid, datambps, framespersecond, targetThreadId;
  int nextmjd, nextsecond, nextnumber, refmjd, refsecond, refnumber;
  long long framesread, framesmissed;

  if(argc != 4)
    return usage();

  datambps = atoi(argv[2]);
  targetThreadId = atoi(argv[3]);
  
  input = fopen(argv[1], "r");
  if(input == NULL)
  {
    fprintf(stderr, "Cannot open input file %s\n", argv[1]);
    exit(1);
  }

  readbytes = fread(buffer, 1, VDIF_HEADER_BYTES, input); //read the VDIF header
  framebytes = getVDIFFrameBytes(buffer);
  if(framebytes > MAX_VDIF_FRAME_BYTES) {
    fprintf(stderr, "Cannot read frame with %d bytes > max (%d)\n", framebytes, MAX_VDIF_FRAME_BYTES);
    exit(1);
  }
  framespersecond = (int)((((long long)datambps)*1000000)/(8*(framebytes-VDIF_HEADER_BYTES)));
  printf("Frames per second is %d\n", framespersecond);
 
  fseek(input, 0, SEEK_SET); //go back to the start

  framesread = 0;
  framesmissed = 0;
  while(!feof(input) && getVDIFThreadID(buffer) != targetThreadId) {
    readbytes = fread(buffer, 1, framebytes, input); //read the whole VDIF packet
    if (readbytes < framebytes) {
      fprintf(stderr, "Header read failed - probably at end of file.\n");
      break;
    }
  }

  refmjd = getVDIFFrameMJD(buffer);
  refsecond = getVDIFFrameSecond(buffer);
  refnumber = getVDIFFrameNumber(buffer);
  nextmjd = getVDIFFrameMJD(buffer);
  nextsecond = getVDIFFrameSecond(buffer);
  nextnumber = getVDIFFrameNumber(buffer);

  fseek(input, 0, SEEK_SET); //go back to the start again

  while(!feof(input)) {
    readbytes = fread(buffer, 1, framebytes, input); //read the whole VDIF packet
    if (readbytes < framebytes) {
      fprintf(stderr, "Header read failed - probably at end of file.\n");
      break;
    }
    if(getVDIFFrameBytes(buffer) != framebytes) {
      fprintf(stderr, "Framebytes has changed! Can't deal with this, aborting\n");
      break;
    }
    if(getVDIFThreadID(buffer) == targetThreadId) {
      framesread++;
      framemjd = getVDIFFrameMJD(buffer);
      framesecond = getVDIFFrameSecond(buffer);
      framenumber = getVDIFFrameNumber(buffer);
      //check for missing frames
      while(framemjd != nextmjd || framesecond != nextsecond || framenumber != nextnumber) {
        if(VERBOSE)
          fprintf(stderr, "Missed a packet! We were expecting MJD %d, sec %d, frame %d, and the next one came in MJD %d, sec %d, frame %d\n",
                  nextmjd, nextsecond, nextnumber, framemjd, framesecond, framenumber);
        framesmissed++;
        nextnumber++;
        if(nextnumber == framespersecond) {
          nextnumber = 0;
          nextsecond++;
          if(nextsecond == 86400) {
            nextsecond = 0;
            nextmjd++;
          }
        }
      }
      nextnumber++;
      if(nextnumber == framespersecond) {
        nextnumber = 0;
        nextsecond++;
        if(nextsecond == 86400) {
          nextsecond = 0;
          nextmjd++;
        }
      }
    }
  }

  printf("For thread %d, read %lld frames, spotted %lld missing frames\n", targetThreadId, framesread, framesmissed);
  fclose(input);

  return 0;
}
