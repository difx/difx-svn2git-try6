/***************************************************************************
 *  Copyright (C) 2018-2019 by Chris Phillips                              *
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
// $Author:$
// $LastChangedDate: $
//
//============================================================================

/**********************************************************************************************

  codif_write

  Read UDP packets from an ADE PAF with "Streaming" Firmware and write resulting data to local file
  Data is assumed to be 16bit complex and a lot of fixes are made to the data based on the PAF
  format. 

  Do not use on "generic" codif data stream  - this is unlikely to work


 **********************************************************************************************/


#ifdef __APPLE__
#define OSX
#define OPENOPTIONS O_WRONLY|O_CREAT|O_TRUNC
#ifdef SWAPENDIAN
#include <libkern/OSByteOrder.h>
#define byteswap_16(x) OSSwapInt16(x)
#define byteswap_32(x) OSSwapInt32(x)
#define byteswap_64(x) OSSwapInt64(x)
#else
#define byteswap_16(x) x
#define byteswap_32(x) x
#define byteswap_64(x) x
#endif
#else
#define LINUX
#define _LARGEFILE_SOURCE 
#define _LARGEFILE64_SOURCE
#define OPENOPTIONS O_WRONLY|O_CREAT|O_TRUNC|O_LARGEFILE|O_EXCL
#ifdef SWAPENDIAN
#include <byteswap.h>
#define byteswap_16(x) bswap_16(x)
#define byteswap_32(x) bswap_32(x)
#define byteswap_64(x) bswap_64(x)
#else
#define byteswap_16(x) x
#define byteswap_32(x) x
#define byteswap_64(x) x
#endif
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <sys/stat.h>  
#include <sys/time.h>  
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <time.h>
#include <netdb.h>
#include <limits.h>
#include <errno.h>
#include <math.h>
#include <fcntl.h>

#include <codifio.h>

#define MAXSTR              200 
#define MAXPACKETSIZE       9500
#define DEFAULT_PORT        52100
#define DEFAULT_TIME        60
#define DEFAULT_FILESIZE    60
#define DEFAULT_UPDATETIME  1.0
#define UPDATE              20
#define MAXTHREAD           20

#define DEBUG(x) 

double tim(void);
void alarm_signal (int sig);

int setup_net(unsigned short port, const char *ip, int *sock);
int openfile(char *fileprefix, char *timestr, int threadid);

volatile int time_to_quit = 0;
volatile int sig_received = 0;

#define INIT_STATS() { \
  npacket = 0; \
  maxpkt_size = 0; \
  minpkt_size = ULONG_MAX; \
  sum_pkts = 0; \
  pkt_drop = 0; \
  pkt_oo = 0; \
  skipped = 0; \
}

/* Globals needed by alarm handler */

unsigned long minpkt_size, maxpkt_size, pkt_drop, pkt_oo;
uint64_t sum_pkts, pkt_head, npacket, skipped;
double t0, t1, t2;

int lines = 0;

int main (int argc, char * const argv[]) {
  char *buf, timestr[MAXSTR], filename[MAXSTR];
  int tmp, opt, status, sock, ofile[MAXTHREAD], skip, i, nthread=0, threads[MAXTHREAD];
  int ithread, thisthread = -1;
  ssize_t nread, nwrote, nwrite;
  char msg[MAXSTR];
  float updatetime, ftmp;
  struct itimerval timeval;
  sigset_t set;
  time_t filetime=0;
  codif_header *cheader=NULL;
  int16_t *s16, *cdata;
  int8_t *s8;
  uint64_t *h64;

  unsigned int port = DEFAULT_PORT;
  char *ip = NULL;
  char *fileprefix = NULL;
  int time = DEFAULT_TIME;
  int filesize = DEFAULT_FILESIZE;
  int padding = 0;
  int threadid = -1;
  int scale = 0;
  int invert = 0;
  int splitthread = 0;
  
  struct option options[] = {
    {"port", 1, 0, 'p'},
    {"padding", 1, 0, 'P'},
    {"threadid", 1, 0, 'T'},
    {"ip", 1, 0, 'i'},
    {"outfile", 1, 0, 'o'},
    {"time", 1, 0, 't'},
    {"updatetime", 1, 0, 'u'},
    {"filesize", 1, 0, 'f'},
    {"scale", 1, 0, 's'},
    {"invert", 0, 0, 'I'},
    {"split", 0, 0, 'S'},
    {"help", 0, 0, 'h'},
    {0, 0, 0, 0}
  };

  for (i=0;i<MAXTHREAD;i++) ofile[i] = 0;
  
  updatetime = DEFAULT_UPDATETIME;

  while (1) {
    opt = getopt_long_only(argc, argv, "i:T:P:p:t:h", options, NULL);
    if (opt==EOF) break;

    switch (opt) {
     
     case 'p':
      status = sscanf(optarg, "%d", &tmp);
      if (status!=1 || tmp<=0 || tmp>USHRT_MAX)
	fprintf(stderr, "Bad port option %s\n", optarg);
      else 
	port = tmp;
     break;

    case 'P':
      status = sscanf(optarg, "%d", &tmp);
      if (status!=1 || tmp<=0)
	fprintf(stderr, "Bad padding option %s\n", optarg);
      else 
	padding = tmp;
     break;

    case 'T':
      status = sscanf(optarg, "%d", &tmp);
      if (status!=1 || tmp<0)
	fprintf(stderr, "Bad threadid option %s\n", optarg);
      else 
	threadid = tmp;
     break;

         case 's':
           status = sscanf(optarg, "%d", &tmp);
           if (status!=1 || tmp<=0 || tmp>15)
	     fprintf(stderr, "Bad scale option %s\n", optarg);
           else 
	     scale = tmp;
	   break;

    case 't':
      status = sscanf(optarg, "%d", &tmp);
      if (status!=1 || tmp<=0)
	fprintf(stderr, "Bad time option %s\n", optarg);
      else 
	time = tmp;
     break;

    case 'u':
      status = sscanf(optarg, "%f", &ftmp);
      if (status!=1 || ftmp<=0.0)
	fprintf(stderr, "Bad updatetime option %s\n", optarg);
      else 
	updatetime = ftmp;
     break;

    case 'f': // Size of file
      status = sscanf(optarg, "%d", &tmp);
      if (status!=1 || tmp<=0.0)
	fprintf(stderr, "Bad filesize option %s\n", optarg);
      else 
	filesize = tmp;
     break;

    case 'i':
      ip = strdup(optarg);
      break;

    case 'o':
      fileprefix = strdup(optarg);
      break;

    case 'I':
      invert = 1;
      break;

    case 'S':
      splitthread = 1;
      break;

    case 'h':
      printf("Usage: udp_write [options]\n");
      printf("  -p/-port <PORT>        Port to use\n");
      printf("  -P/-padding <N>        Discard N bytes at end of packet\n");
      printf("  -T/-threadid <N>       Only record threadid N\n");
      printf("  -i/-ip <IP>            Receive data from host IP\n");
      printf("  -o/-outfile <NAME>     output prefix NAME\n");
      printf("  -t/-time <N>           Record for N seconds\n");
      printf("  -f/-filesize <N>       Write files N seconds long\n");
      printf("  -s/-scale <S>          Reduce data to 8 bits, dividing by S\n");
      printf("  -S/-split              Write threads to separate files\n"); 
      printf("  -h/-help               This list\n");
      return(1);
    break;
    
    case '?':
    default:
      break;
    }
  }

  if (threadid>0 && splitthread) {
    fprintf(stderr, "Cannot split by thread and filter single thread. Quitting\n");
    exit(1);
  }
  
  if (padding>0) printf("Warning: Discarding %d bytes per packet\n", padding);
  
  if (!fileprefix) fileprefix=strdup("test");

  buf = malloc(MAXPACKETSIZE);
  if (buf==NULL) {
    sprintf(msg, "Trying to allocate %d bytes", MAXPACKETSIZE);
    perror(msg);
    return(1);
  }

  cheader = (codif_header*)buf;
  h64 = (uint64_t*)cheader; // Header as 64bit worlds
  cdata = (int16_t*) &buf[CODIF_HEADER_BYTES];

  status = setup_net(port, ip, &sock);
    
  if (status)  exit(1);

  INIT_STATS();
  pkt_head = 0;

  /* Install an alarm catcher */
  signal(SIGALRM, alarm_signal);
  sigemptyset(&set);
  sigaddset(&set, SIGALRM);

  timeval.it_interval.tv_sec = updatetime; 
  timeval.it_interval.tv_usec = 0;
  timeval.it_value.tv_sec = updatetime;
  timeval.it_value.tv_usec = 0;
  t0 = t1 = t2 = tim();
  setitimer(ITIMER_REAL, &timeval, NULL);

  int first = 1;
  while (t2-t0<time) {

    nread = recvfrom(sock, buf, MAXPACKETSIZE, MSG_WAITALL, 0, 0);
    if (nread==-1) {
      perror("Receiving packet");
      exit(1);
    } else if (nread==0) {
      fprintf(stderr, "Error: Did not read any bytes!\n");
      exit(1);
    } else if (nread==MAXPACKETSIZE) {
      fprintf(stderr, "Error: Packetsize larger than expected!\n");
      exit(1);
    } else if ((nread-padding) % 4) {
      fprintf(stderr, "Error: Needs to read multiple of 32 bytes\n");
      exit(1);
    }

    // Block alarm signal while we are updating these values
    status = sigprocmask(SIG_BLOCK, &set, NULL);
    if (status) {
      perror(": Trying to block SIGALRM\n");
      exit(1);
    }

    thisthread = cheader->threadid;

    npacket++;
    if (nread>maxpkt_size) maxpkt_size = nread;
    if (nread<minpkt_size) minpkt_size = nread;
    sum_pkts += nread;

    skip = 0;
    if (threadid>=0) {
      if (cheader->threadid!=threadid) {
	skip=1;
	skipped++;
      }
    }

    // Unblock the signal again
    status = sigprocmask(SIG_UNBLOCK, &set, NULL);
    if (status) {
      perror(": Trying to block SIGALRM\n");
      exit(1);
    }

    if (skip) continue;

    // Convert header then data to little endian - Note this is disabled above with #def
    for (i=0; i<8; i++) {
      h64[i] = byteswap_64(h64[i]);
    }
    if (invert) {
      for (i=0; i<(nread-CODIF_HEADER_BYTES)/sizeof(int16_t);i++) {
	cdata[i] = byteswap_16(cdata[i]);
	i++;
	cdata[i] = -byteswap_16(cdata[i]);
      }
    } else {
      for (i=0; i<(nread-CODIF_HEADER_BYTES)/sizeof(int16_t);i++) {
	cdata[i] = byteswap_16(cdata[i]);
      }
    }
    t2 = tim();

    if (first || t2-filetime>filesize) {
      if (first) {
	filetime = t0;
	first = 0;
      } else {
	while (t2-filetime>filesize) {
	  filetime += filesize;
	}
	if (splitthread) {
	  for (i=0;i<nthread;i++) {
	    close(ofile[i]);
	    ofile[i] = 0;
	    nthread = 0;
          }
	} else {
	  close(ofile[0]);
	  ofile[0] = 0;
	}
      }
      time_t itime = (time_t)floor(filetime);
      struct tm *date = gmtime(&itime); 

      strftime(timestr, MAXSTR-1, "%j_%H%M%S", date);

      if (splitthread) {
	ofile[0] = openfile(fileprefix, timestr, thisthread);
	threads[0] = thisthread;
      } else {
	ofile[0] = openfile(fileprefix, timestr, -1);
      }
      ithread = 0;
      nthread = 1;
      
      if (ofile[0]==-1) exit(1);

    } else if (splitthread) {
      ithread = -1;
      // Have we seen this thread this time
      for (i=0;i<nthread;i++) {
	if (threads[i] == thisthread) {
	  ithread = i;
	  break;
	}
      }
      if (ithread==-1) { // Not found
	if (nthread>=MAXTHREAD) {
	  fprintf(stderr, "Too many threads = increase MAXTHREAD (%d). Quitting\n", MAXTHREAD);
	  exit(1);
	}
	ofile[nthread] = openfile(fileprefix, timestr, thisthread);
	if (ofile[nthread]==-1) exit(1);
	ithread = nthread;
	threads[nthread] = thisthread;
	nthread++;
      }
    }
    if (scale>0) {
      s16 = cdata;
      s8 = (int8_t*)cdata;
      int i;
      for (i=0; i< (nread-padding-CODIF_HEADER_BYTES)/sizeof(int16_t); i++) {
	*s16 /= scale;
	if (*s16>127)
	  *s16 = 127;
	else if (*s16<-128)
	  *s16 = -128;
	*s8 = (*s16 & 0xFF);
	s16++;
	s8++;
      }
      nwrite = (nread-padding-CODIF_HEADER_BYTES)/sizeof(int16_t) + CODIF_HEADER_BYTES;
    } else {
      nwrite = nread - padding;
    }
    
    nwrote = write(ofile[ithread], buf, nwrite);
    if (nwrote==-1) {
      perror("Error writing outfile");
      for (i=0;i<nthread;i++) close(ofile[i]);
      exit(1);
    } else if (nwrote!=nwrite) {
      fprintf(stderr, "Warning: Did not write all bytes! (%zd/%zd)\n", nwrote, nwrite);
    } 
  }

  free(buf);

  for (i=0;i<nthread;i++) close(ofile[i]);

  return(0);
}
  
int setup_net(unsigned short port, const char *ip, int *sock) {
  int status;
  struct sockaddr_in server; 

  /* Initialise server's address */
  memset((char *)&server,0,sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(port);

  if (ip==NULL) 
    server.sin_addr.s_addr = htonl(INADDR_ANY); /* Anyone can connect */
  else {
    server.sin_addr.s_addr = inet_addr(ip);
    if (server.sin_addr.s_addr==INADDR_NONE) {
      fprintf(stderr, "Error parsing IP address %s. Exitting\n", ip);
      return(1);
    }
  }

  /* Create socket */
  *sock = socket(AF_INET,SOCK_DGRAM, IPPROTO_UDP); 
  if (*sock==-1) {
    perror("Error creating socket");
    return(1);
  }

  int udpbufbytes = 10*1024*1024;
  status = setsockopt(*sock, SOL_SOCKET, SO_RCVBUF,
		      (char *) &udpbufbytes, sizeof(udpbufbytes));
    
  if (status!=0) {
    fprintf(stderr, "Warning: Could not set socket RCVBUF\n");
  } 

  status = bind(*sock, (struct sockaddr *)&server, sizeof(server));
  if (status!=0) {
    perror("Error binding socket");
    close(*sock);
    return(1);
  }

  return(0);
}
  
double tim(void) {
  struct timeval tv;
  double t;

  gettimeofday(&tv, NULL);
  t = (double)tv.tv_sec + (double)tv.tv_usec/1000000.0;

  return t;
}

void alarm_signal (int sig) {
  long avgpkt;
  float delta, rate, percent;
  t2 = tim();
  delta = t2-t1;

  if (npacket==0) {
    avgpkt = 0;
    minpkt_size = 0;
  } else 
    avgpkt = sum_pkts/npacket;
  rate = sum_pkts*8/delta/1e6;

  if (lines % UPDATE ==0) {
    printf("  npkt   min  max  avg sec Rate Mbps  drop  %%    oo skipped\n");
  }
  lines++;

  if (npacket==0)
    percent = 0; 
  else 
    percent = (double)pkt_drop/(npacket+pkt_drop)*100;

  printf("%6" PRIu64"  %4lu %4lu %4ld %3.1f  %7.3f %5lu %5.2f %3lu  %2.0f%%\n", 
	 npacket, minpkt_size, maxpkt_size, avgpkt, delta, rate, 
	 pkt_drop, percent, pkt_oo, skipped/(float)npacket*100.0);	
  INIT_STATS();
  t1 = t2;
  return;
}  

int openfile(char *fileprefix, char *timestr, int threadid) {
  char filename[MAXSTR], msg[MAXSTR];

  if (threadid>=0) 
    snprintf(filename, MAXSTR-1, "%s_%s-%d.cdf", fileprefix, timestr, threadid);
  else
    snprintf(filename, MAXSTR-1, "%s_%s.cdf", fileprefix, timestr);

  // File name contained in buffer
  int ofile = open(filename, OPENOPTIONS,S_IRWXU|S_IRWXG|S_IRWXO); 
  if (ofile==-1) {
    sprintf(msg, "Failed to open output file (%s)", filename);
    perror(msg);
  }
  return ofile;
}
