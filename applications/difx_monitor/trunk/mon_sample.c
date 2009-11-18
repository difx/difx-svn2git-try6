/***************************************************************************
 *   Copyright (C) 2009 by Chris Phillips                                  *
 *                                                                         *
 *  This program is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU General Public License as         *
 *  published by the Free Software Foundation; version 2 of the License    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the        *
 *   GNU General Public License for more details.                          *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <cpgplot.h>
#include <complex>

#include "architecture.h"
#include "monserver.h"
#define MAX_PROD 4

float arraymax(float *array[], int nchan, int nprod);
float arraymin(float *a[], int nchan, int nprod);

int main(int argc, const char * argv[]) {
  int status, prod, i, ivis, nchan=0, nprod, cols[MAX_PROD] = {2,3,4,5};
  unsigned int iprod[MAX_PROD];
  struct monclient monserver;
  float *xval=NULL, *amp[MAX_PROD], *phase[MAX_PROD], *lags[MAX_PROD], *lagx=NULL;
  float delta, min, max, temp;
  cf32 *vis;
  IppsFFTSpec_R_32f* fftspec=NULL;

  if(argc < 3)  {
    fprintf(stderr, "Error - invoke with mon_sample <host> <product#>\n");
    return(EXIT_FAILURE);
  }

  if (argc-2>MAX_PROD) {
    fprintf(stderr, "Error - Too many products requested\n");
    return(EXIT_FAILURE);
  }

  for (i=2; i<argc; i++) iprod[i-2] = atoi(argv[i]);
  nprod = argc-2;

  for (i=0; i<nprod; i++) {
    amp[i] = NULL;
    phase[i] = NULL;
    lags[i] = NULL;
  }

  // Open PGPLOT first, as pgplot server seems to inherite monitor socket
  status = cpgbeg(0,"/xs",1,3);
  if (status != 1) {
    printf("Error opening pgplot device\n");
  }
  cpgask(0);

  status  = monserver_connect(&monserver, (char*)argv[1], -1);
  if (status) exit(1);

  printf("Opened connection to monitor server\n");

  status = monserver_requestproducts(monserver, iprod, nprod);
  if (status) exit(1);


  printf("Sent product request\n");

  status = 0;
  while (!status) {
    printf("Waiting for visibilities\n");
    status = monserver_readvis(&monserver);
    if (!status) {
      printf("Got visibility # %d\n", monserver.timestamp);

      ivis = 0;
      while (!monserver_nextvis(&monserver, &prod, &vis)) {
	//printf("Got visibility for product %d\n", prod);

	// (Re)allocate arrays if number of channels changes, including first time
	if (nchan!=monserver.numchannels) {
	  nchan = monserver.numchannels;

	  if (xval!=NULL) vectorFree(xval);
	  if (lagx!=NULL) vectorFree(lagx);
	  if (fftspec!=NULL) ippsFFTFree_R_32f(fftspec);
	  for (i=0; i<nprod; i++) {
	    if (amp[i]!=NULL) vectorFree(amp[i]);
	    if (phase[i]!=NULL) vectorFree(phase[i]);
	    if (lags[i]!=NULL) vectorFree(lags[i]);
	  }

	  xval = vectorAlloc_f32(nchan);
	  lagx = vectorAlloc_f32(nchan*2);
	  for (i=0; i<nprod; i++) {
	    amp[i] = vectorAlloc_f32(nchan);
	    phase[i] = vectorAlloc_f32(nchan);
	    lags[i] = vectorAlloc_f32(nchan*2);
	    if (amp[i]==NULL || phase[i]==NULL || lags[i]==NULL) {
	      fprintf(stderr, "Failed to allocate memory for plotting arrays\n");
	      exit(1);
	    }
	  }
	  if (xval==NULL || lagx==NULL) {
	    fprintf(stderr, "Failed to allocate memory for plotting arrays\n");
	    exit(1);
	  }
	  for (i=0; i<nchan; i++) {
	    xval[i] = i;
	  }
	  for (i=-nchan; i<nchan; i++) {
	    lagx[i+nchan] = i;
	  }

	  
	  printf("Initialise FFT\n");
	  int order = 0;
	  while(((nchan*2) >> order) > 1)
	    order++;
	  ippsFFTInitAlloc_R_32f(&fftspec, order, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast);
	}

	ippsMagnitude_32fc(vis, amp[ivis], nchan);
	ippsPhase_32fc(vis, phase[ivis], nchan);
	vectorMulC_f32_I(180/M_PI, phase[ivis], nchan);

	ippsFFTInv_CCSToR_32f((Ipp32f*)vis, lags[ivis], fftspec, 0);
	//rearrange the lags into order
	for(i=0;i<nchan;i++) {
	  temp = lags[ivis][i];
	  lags[ivis][i] = lags[ivis][i+nchan];
	  lags[ivis][i+nchan] = temp;
	}
	ivis++;
      }

      // Plot all the data
      cpgbbuf();

      max = arraymax(amp, nchan, nprod);
      min = arraymin(amp, nchan, nprod);
      delta = (max-min)*0.05;
      if (delta==0.0) delta = 0.5;
      min -= delta;
      max += delta;

      cpgsci(1);
      cpgenv(0,nchan,min,max,0,0);
      cpglab("Channel", "Amplitude", "");

      for (i=0; i<nprod; i++) {
	cpgsci(cols[i]);
	cpgline(nchan, xval, amp[i]);
      }

      max = arraymax(phase, nchan, nprod);
      min = arraymin(phase, nchan, nprod);
      delta = (max-min)*0.1;
      if (delta==0.0) delta = 0.5;
      min -= delta;
      max += delta;
      cpgsci(1);
      cpgenv(0,nchan,min,max,0,0);
      cpglab("Channel", "Phase", "");


      for (i=0; i<nprod; i++) {
	cpgsci(cols[i]);
	cpgpt(nchan, xval, phase[i], 17);
      }

      max = arraymax(lags, nchan*2, nprod);
      min = arraymin(lags, nchan*2, nprod);
      delta = (max-min)*0.1;
      if (delta==0.0) delta = 0.5;
      min -= delta;
      max += delta;
      cpgsci(1);
      cpgenv(-nchan,nchan,min,max,0,0);
      cpglab("Channel", "Delay", "");

      for (i=0; i<nprod; i++) {
	cpgsci(cols[i]);
	cpgline(nchan*2, lagx, lags[i]);
      }

      cpgebuf();

    }
  }

  monserver_close(&monserver);
}


float arraymax(float *a[], int nchan, int nprod) {
  float max, thismax;
  int i;

  max = a[0][0];
  for (i=0; i<nprod; i++)  {
    ippsMax_32f(a[i], nchan, &thismax);
    if (thismax>max) max = thismax;
  }
  return max;
}

float arraymin(float *a[], int nchan, int nprod) {
  float min, thismin;
  int i;

  min = a[0][0];
  for (i=0; i<nprod; i++)  {
    ippsMin_32f(a[i], nchan, &thismin);
    if (thismin<min) min = thismin;
  }
  return min;
}
