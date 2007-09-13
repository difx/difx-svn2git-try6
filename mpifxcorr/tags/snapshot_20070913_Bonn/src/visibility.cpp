/***************************************************************************
 *   Copyright (C) 2006 by Adam Deller                                     *
 *                                                                         *
 *   This program is free for non-commercial use: see the license file     *
 *   at http://astronomy.swin.edu.au:~adeller/software/difx/ for more      *
 *   details.                                                              *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 ***************************************************************************/
#include "visibility.h"
#include "core.h"
#include "datastream.h"
#include <dirent.h>
#include <cmath>
#include <string>
#include <string.h>
#include <stdio.h>
#include <RPFITS.h>
#include <iomanip>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

Visibility::Visibility(Configuration * conf, int id, int numvis, int eseconds, int skipseconds, const string * pnames, bool mon, int port, char * hname, int * sock, int monskip)
  : config(conf), visID(id), numvisibilities(numvis), executeseconds(eseconds), polnames(pnames), monitor(mon), portnum(port), hostname(hname), mon_socket(sock), monitor_skip(monskip)
{
  int status;

  cout << "About to create visibility " << id << "/" << numvis << endl;

  if(visID == 0)
    *mon_socket = -1;
  maxproducts = config->getMaxProducts();
  first = true;
  currentblocks = 0;
  numdatastreams = config->getNumDataStreams();
  resultlength = config->getMaxResultLength();
  results = vectorAlloc_cf32(resultlength);
  status = vectorZero_cf32(results, resultlength);
  if(status != vecNoErr)
    cerr << "Error trying to zero when incrementing visibility " << visID << endl;
  numbaselines = config->getNumBaselines();
  currentconfigindex = config->getConfigIndex(skipseconds);
  expermjd = config->getStartMJD();
  experseconds = config->getStartSeconds();
  changeConfig(currentconfigindex);

  //set up the initial time period this Visibility will be responsible for
  offset = 0;
  currentstartsamples = 0;
  currentstartseconds = skipseconds;
  offset = offset+offsetperintegration;
  blocksthisintegration = integrationsamples/blocksamples;
  if(offset >= blocksamples/2)
  {
    offset -= blocksamples;
    blocksthisintegration++;
  }
  for(int i=0;i<visID;i++)
    updateTime();
}


Visibility::~Visibility()
{
  /*for(int i=0;i<<numbaselines+numtelescopes;i++)
  {
    for(int j=0;j<numfreqs*numproducts;j++)
      vectorFree(results[i][j]);
    delete [] results[i];
  }
  delete [] results;*/
  vectorFree(results);
  for(int i=0;i<numdatastreams;i++)
    delete [] autocorrcalibs[i];
  delete [] autocorrcalibs;
  vectorFree(rpfitsarray);
}

bool Visibility::addData(cf32* blockresult)
{
  int status;

  status = vectorAdd_cf32_I(blockresult, results, resultlength);
  if(status != vecNoErr)
    cerr << "Error copying results in visibility ID " << visID << endl;
  currentblocks++;

  return (currentblocks==blocksthisintegration); //are we finished integrating?
}

void Visibility::increment()
{
  int status;
  cout << "VISIBILITY " << visID << " IS INCREMENTING, SINCE CURRENTBLOCKS = " << currentblocks << endl;

  currentblocks = 0;
  for(int i=0;i<numvisibilities;i++) //adjust the start time and offset
    updateTime();

  status = vectorZero_cf32(results, resultlength);
  if(status != vecNoErr)
    cerr << "Error trying to zero when incrementing visibility " << visID << endl;
}

void Visibility::updateTime()
{
  int configindex;
  offset = offset+offsetperintegration;
  blocksthisintegration = integrationsamples/blocksamples;
  if(offset >= blocksamples/2)
  {
    offset -= blocksamples;
    blocksthisintegration++;
  }
  currentstartsamples += integrationsamples;
  currentstartseconds += currentstartsamples/samplespersecond;
  currentstartsamples %= samplespersecond;
  configindex = config->getConfigIndex(currentstartseconds);
  while(configindex < 0 && currentstartseconds < executeseconds)
  {
    configindex = config->getConfigIndex(++currentstartseconds);
    currentstartsamples = 0;
    offset = offsetperintegration;
    blocksthisintegration = integrationsamples/blocksamples;
    if(offset >= blocksamples/2)
    {
      offset -= blocksamples;
      blocksthisintegration++;
    }
  }
  if(configindex != currentconfigindex && currentstartseconds < executeseconds)
  {
    currentconfigindex = configindex;
    changeConfig(currentconfigindex);
  }
}

//setup monitoring socket
int Visibility::openMonitorSocket(char *hostname, int port, int window_size, int *sock) {
  int status;
  int * err;
  unsigned long ip_addr;
  struct hostent     *hostptr;
  struct linger      linger = {1, 1};
  struct sockaddr_in server;    /* Socket address */
  int saveflags,ret,back_err;
  fd_set fd_w;
  struct timeval timeout;

  timeout.tv_sec = 0;
  timeout.tv_usec = 100000;

  hostptr = gethostbyname(hostname);
  if (hostptr==NULL) {
    printf("Failed to look up hostname %s\n", hostname);
    return(1);
  }
  
  memcpy(&ip_addr, (char *)hostptr->h_addr, sizeof(ip_addr));
  memset((char *) &server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons((unsigned short)port); 
  server.sin_addr.s_addr = ip_addr;
  
  printf("Connecting to %s\n",inet_ntoa(server.sin_addr));
    
  *sock = socket(AF_INET, SOCK_STREAM, 0);
  if (*sock==-1) {
    perror("Failed to allocate socket");
    return(1);
  }

  /* Set the linger option so that if we need to send a message and
     close the socket, the message shouldn't get lost */
  status = setsockopt(*sock, SOL_SOCKET, SO_LINGER, (char *)&linger,
                      sizeof(struct linger)); 
  if (status!=0) {
    close(*sock);
    perror("Setting socket options");
    return(1);
  }

  /* Set the window size to TCP actually works */
  status = setsockopt(*sock, SOL_SOCKET, SO_SNDBUF,
                      (char *) &window_size, sizeof(window_size));
  if (status!=0) {
    close(*sock);
    perror("Setting socket options");
    return(1);
  }
  status = setsockopt(*sock, SOL_SOCKET, SO_RCVBUF,
                      (char *) &window_size, sizeof(window_size));
  if (status!=0) {
    close(*sock);
    perror("Setting socket options");
    return(1);
  }
  saveflags=fcntl(*sock,F_GETFL,0);
  if(saveflags<0) {
    perror("fcntl1");
    *err=errno;
    return 1;
  }
  /* Set non blocking */
  if(fcntl(*sock,F_SETFL,saveflags|O_NONBLOCK)<0) {
    perror("fcntl2");
    *err=errno;
    return 1;
  }

  // try to connect    
  status = connect(*sock, (struct sockaddr *) &server, sizeof(server));
  back_err=errno;

  /* restore flags */
  if(fcntl(*sock,F_SETFL,saveflags)<0) {
    perror("fcntl3");
    *err=errno;
    return 1;
  }

  /* return unless the connection was successful or the connect is
           still in progress. */
  if(*err<0 && back_err!=EINPROGRESS) {
    perror("connect");
    *err=errno;
    return 1;
  }

  FD_ZERO(&fd_w);
  FD_SET(*sock,&fd_w);

  status = select(FD_SETSIZE,NULL,&fd_w,NULL,&timeout);
  if(status < 0) {
    perror("select");
    *err=errno;
    return 1;
  }

  /* 0 means it timeout out & no fds changed */
  if(status==0) {
    close(*sock);
    status=ETIMEDOUT;
    return 1;
  }

  /* Get the return code from the connect */
  socklen_t len=sizeof(ret);
  status=getsockopt(*sock,SOL_SOCKET,SO_ERROR,&ret,&len);
  if(status<0) {
    perror("getsockopt");
    status = errno;
    return 1;
  }

  /* ret=0 means success, otherwise it contains the errno */
  if(ret) {
    status=ret;
    return 1;
  }

  return 0;
} /* Setup Net */

int Visibility::sendMonitorData(bool tofollow) {
  char *ptr;
  int ntowrite, nwrote, atsec;

  //ensure the socket is open
  if(checkSocketStatus())
  {
    if(!tofollow)
      atsec = -1;
    else
      atsec = currentstartseconds;

    ptr = (char*)(&atsec);
    nwrote = send(*mon_socket, ptr, 4, 0);
    if (nwrote < 4)
    {
      cerr << "Error writing to network - will try to reconnect next Visibility 0 integration!" << endl;
      return 1;
    }

    cout << "Timestamp sent ok" << endl;

    if(tofollow)
    {
      ptr = (char*)results;
      ntowrite = resultlength*sizeof(cf32);

      while (ntowrite>0) {
        nwrote = send(*mon_socket, ptr, ntowrite, 0);
        if(errno == EPIPE)
        {
          printf("Network seems to have dropped out!  Will try to reconnect shortly...!\n");
          return(1);
        }
        if (nwrote==-1) {
          if (errno == EINTR) continue;
          perror("Error writing to network");

          return(1);
        } else if (nwrote==0) {
          printf("Warning: Did not write any bytes!\n");
          return(1);
        } else {
          ntowrite -= nwrote;
          ptr += nwrote;
        }
      }
      cout << "Finished writing monitoring data to socket!" << endl;
    }
  }
  return(0);
}

bool Visibility::checkSocketStatus()
{
  if(*mon_socket < 0)
  {
    if(visID != 0)
    {
      //don't even try to connect, unless you're the first visibility.  Saves trying to reconnect too often
      cerr << "Visibility " << visID << " won't try to reconnect monitor - waiting for vis 0..." << endl;
      return false;
    }
    if(openMonitorSocket(hostname, portnum, Configuration::MONITOR_TCP_WINDOWBYTES, mon_socket) != 0)
    {
      *mon_socket = -1;
      cerr << "WARNING: Monitor socket could not be opened - monitoring not proceeding! Will try again after " << numvisibilities << " integrations..." << endl;
      return false;
    }
  }
  return true;
}

void Visibility::writedata()
{
  f32 scale, divisor;
  int status, ds1, ds2, ds1bandindex, ds2bandindex, binloop;

  cout << "Visibility " << visID << " is starting to write out data" << endl;

  if (monitor && currentblocks == 0) //nothing to write out
  {
    //send a message to the monitor not to expect any data this integration - if we can't get through to the monitor, close the socket
    if(sendMonitorData(false) != 0) {
      cerr << "tried to send a header only to monitor and it failed - closing socket!" << endl;
      close(*mon_socket);
      *mon_socket = -1;
    }
    return; //NOTE EXIT HERE!!!
  }

  int skip = 0;
  int count = 0;
  autocorrincrement = (maxproducts>1)?2:1;
  if(config->pulsarBinOn(currentconfigindex) && !config->scrunchOutputOn(currentconfigindex))
    binloop = config->getNumPulsarBins(currentconfigindex);
  else
    binloop = 1;

  for(int i=0;i<numbaselines;i++)
  {
    //skip past the actual baseline visibilities to get to the autocorrelations, so we can calculate amplitude calibration coefficients
    for(int j=0;j<config->getBNumFreqs(currentconfigindex,i);j++)
      skip += config->getBNumPolProducts(currentconfigindex, i, j)*(numchannels+1)*binloop;
  }
  for(int i=0;i<numdatastreams;i++)
  {
    for(int j=0;j<config->getDNumOutputBands(currentconfigindex, i); j++)
    {
      //work out the band average, for use in calibration (allows us to calculate fractional correlation)
      status = vectorMean_cf32(&results[skip + count], numchannels, &autocorrcalibs[i][j], vecAlgHintFast);
      if(status != vecNoErr)
        cerr << "Error in getting average of autocorrelation!!!" << status << endl;
      count += numchannels + 1;
    }
    if(config->writeAutoCorrs(currentconfigindex) && autocorrincrement > 1) //need to skip the cross autocorrs that are also present in the array
      count += config->getDNumOutputBands(currentconfigindex, i)*(numchannels + 1);
  }
  count = 0;
  for(int i=0;i<numbaselines;i++) //do each baseline
  {
    ds1 = config->getBOrderedDataStream1Index(currentconfigindex, i);
    ds2 = config->getBOrderedDataStream2Index(currentconfigindex, i);
    for(int j=0;j<config->getBNumFreqs(currentconfigindex,i);j++) //do each frequency
    {
      for(int k=0;k<config->getBNumPolProducts(currentconfigindex, i, j);k++) //do each product of this frequency eg RR,LL,RL,LR
      {
        ds1bandindex = config->getBDataStream1BandIndex(currentconfigindex, i, j, k);
        ds2bandindex = config->getBDataStream2BandIndex(currentconfigindex, i, j, k);
        divisor = (Mode::getDecorrelationPercentage(config->getDNumBits(currentconfigindex, ds1)))*(Mode::getDecorrelationPercentage(config->getDNumBits(currentconfigindex, ds2)))*autocorrcalibs[ds1][ds1bandindex].re*autocorrcalibs[ds2][ds2bandindex].re;
        if(divisor > 0.0) //only do it if there is something to calibrate with
          scale = sqrt(config->getDTsys(currentconfigindex, ds1)*config->getDTsys(currentconfigindex, ds2)/divisor);
        else
          scale = 0.0;
        for(int b=0;b<binloop;b++)
        {
          //amplitude calibrate the data
          status = vectorMulC_f32_I(scale, (f32*)(&(results[count])), 2*(numchannels+1));
          if(status != vecNoErr)
            cerr << "Error trying to amplitude calibrate the baseline data!!!" << endl;
          count += numchannels+1;
        }
      }
    }
  }

  if(config->writeAutoCorrs(currentconfigindex)) //if we need to, calibrate the autocorrs
  {
    for(int i=0;i<numdatastreams;i++) //do each datastream
    {
      for(int j=0;j<2;j++) //the parallel, and the cross product for which this band is the first
      {
        for(int k=0;k<config->getDNumOutputBands(currentconfigindex, i); k++) //for each band
        {
          //calibrate the data
          divisor = (Mode::getDecorrelationPercentage(config->getDNumBits(currentconfigindex, i))*sqrt(autocorrcalibs[i][k].re*autocorrcalibs[i][(j==0)?k:config->getDMatchingBand(currentconfigindex, i, k)].re));
          if(divisor > 0.0)
            scale = config->getDTsys(currentconfigindex, i)/divisor;
          else
            scale = 0.0;
          status = vectorMulC_f32_I(scale, (f32*)(&(results[count])), 2*(numchannels+1));
          if(status != vecNoErr)
            cerr << "Error trying to amplitude calibrate the datastream data!!!" << endl;
          count += numchannels+1;
        }
      }
    }
  }
  
  //all calibrated, now just need to write out
  if(config->getOutputFormat() == Configuration::RPFITS)
    writerpfits();
  else if(config->getOutputFormat() == Configuration::DIFX)
    writedifx();
  else
    writeascii();

  //send monitoring data, if we don't have to skip this one
  if(monitor && visID % monitor_skip == 0) {
    if (sendMonitorData(true) != 0){ 
      cerr << "Error sending monitoring data - closing socket!" << endl;
      close(*mon_socket);
      *mon_socket = -1;
    }
  }

  cout << "Visibility has finished writing data" << endl;
}

void Visibility::writeascii()
{
  ofstream output;
  int binloop;
  char datetimestring[26];

  int count = 0;
  int samples = currentstartsamples + integrationsamples/2;
  int seconds = experseconds + currentstartseconds + samples/samplespersecond;
  int microseconds = int((double(samples%samplespersecond)/double(samplespersecond))*1000000 + 0.5);
  int hours = seconds/3600;
  int minutes = (seconds-hours*3600)/60;
  int mjd = expermjd;
  seconds = seconds - (hours*3600 + minutes*60);
  while(hours >= 24)
  {
     hours -= 24;
     mjd++;
  }
  sprintf(datetimestring, "%05u_%02u%02u%02u_%06u", mjd, hours, minutes, seconds, microseconds);
  cout << "Mjd is " << mjd << ", hours is " << hours << ", minutes is " << minutes << ", seconds is " << seconds << endl;
  
  if(config->pulsarBinOn(currentconfigindex) && !config->scrunchOutputOn(currentconfigindex))
    binloop = config->getNumPulsarBins(currentconfigindex);
  else
    binloop = 1;

  for(int i=0;i<numbaselines;i++)
  {
    for(int j=0;j<config->getBNumFreqs(currentconfigindex,i);j++)
    {
      for(int b=0;b<binloop;b++)
      {
        for(int k=0;k<config->getBNumPolProducts(currentconfigindex, i, j);k++)
        {
          //write out to a naive filename
          output.open(string(string("baseline_")+char('0' + i)+"_freq_"+char('0' + j)+"_product_"+char('0'+k)+"_"+datetimestring+"_bin_"+char('0'+b)+".output").c_str(), ios::out|ios::trunc);
          for(int l=0;l<numchannels+1;l++)
              output << l << " " << sqrt(results[count + l].re*results[count + l].re + results[count + l].im*results[count + l].im) << " " << atan2(results[count + l].im, results[count + l].re) << endl;
          output.close();
          count += numchannels+1;
        }
      }
    }
  }

  if(config->writeAutoCorrs(currentconfigindex)) //if we need to, write out the autocorrs
  {
    for(int i=0;i<numdatastreams;i++)
    {
      for(int j=0;j<autocorrincrement;j++)
      {
        for(int k=0;k<config->getDNumOutputBands(currentconfigindex, i); k++)
        {
          //write out to naive filename
          output.open(string(string("datastream_")+char('0' + i)+"_crosspolar_"+char('0' + j)+"_product_"+char('0'+k)+"_"+datetimestring+"_bin_"+char('0'+0)+".output").c_str(), ios::out|ios::trunc);
          for(int l=0;l<numchannels+1;l++)
            output << l << " " << sqrt(results[count + l].re*results[count + l].re + results[count + l].im*results[count + l].im) << " " << atan2(results[count + l].im, results[count + l].re) << endl;
          output.close();
          count += numchannels+1;
        }
      }
    }
  }
}

void Visibility::writerpfits()
{
  int baselinenumber, freqnumber, sourcenumber, numpolproducts, binloop;
  int firstpolindex;
  int count = 0;
  int status = 0;
  int flag = 0;
  int bin = 0;
  float buvw[3]; //the u,v and w for this baseline at this time
  f32 * visibilities = (f32*)rpfitsarray;
  float offsetstartdaysec = currentstartseconds + experseconds + float((currentstartsamples+integrationsamples/2)/(2000000.0*config->getDBandwidth(currentconfigindex,0,0)));
  
  if(config->pulsarBinOn(currentconfigindex) && !config->scrunchOutputOn(currentconfigindex))
    binloop = config->getNumPulsarBins(currentconfigindex);
  else
    binloop = 1;
  sourcenumber = config->getSourceIndex(expermjd, experseconds + currentstartseconds) + 1;

  //ensure the intbase parameter is set correctly
  param_.intbase = config->getIntTime(currentconfigindex);

  for(int i=0;i<numbaselines;i++)
  {
    baselinenumber = config->getBNumber(currentconfigindex, i);

    //interpolate the uvw
    (config->getUVW())->interpolateUvw(config->getDStationName(currentconfigindex, config->getBOrderedDataStream1Index(currentconfigindex, i)), config->getDStationName(currentconfigindex, config->getBOrderedDataStream2Index(currentconfigindex, i)), expermjd, offsetstartdaysec, buvw);
    for(int j=0;j<config->getBNumFreqs(currentconfigindex,i);j++)
    {
      for(int b=0;b<binloop;b++)
      {
        //clear the rpfits array, which is a specially ordered array of all products for this frequency
        status = vectorZero_cf32(rpfitsarray, maxproducts*(numchannels+1));
        if(status != vecNoErr)
          cerr << "Error trying to zero the rpfitsarray!!!" << endl;
        freqnumber = config->getBFreqIndex(currentconfigindex, i, j) + 1;
        numpolproducts = config->getBNumPolProducts(currentconfigindex, i, j);

        //put the stuff in the rpfitsarray in order
        for(int k=0;k<numpolproducts;k++)
        {
          for(int l=0;l<numchannels+1;l++)
            rpfitsarray[l*maxproducts + baselinepoloffsets[i][j][k]] = results[count + l];
          count += numchannels + 1;
        }

        //write out the rpfits data
        rpfitsout_(&status/*should be 0 = writing data*/, visibilities, 0/*not using weights*/, &baselinenumber, &offsetstartdaysec, &buvw[0], &buvw[1], &buvw[2], &flag/*not flagged*/, &b, &freqnumber, &sourcenumber);
      }
    }
  }
  //zero the uvw for autocorrelations
  buvw[0] = 0.0;
  buvw[1] = 0.0;
  buvw[2] = 0.0;
  if(config->writeAutoCorrs(currentconfigindex)) //if we need to, write out the autocorrs
  {
    for(int i=0;i<numdatastreams;i++)
    {
      baselinenumber = 257*(config->getDTelescopeIndex(currentconfigindex, i)+1);
      for(int j=0;j<config->getDNumFreqs(currentconfigindex, i); j++)
      {
        firstpolindex = -1;
        //find a product that is active, so we know where to look to work out which frequency this is
        for(int k=maxproducts-1;k>=0;k--) {
          if(datastreampolbandoffsets[i][j][k] >= 0)
            firstpolindex = k;
        }
        if(firstpolindex >= 0)
        {
          //zero the rpfitsarray
          status = vectorZero_cf32(rpfitsarray, maxproducts*(numchannels+1));
          if(status != vecNoErr)
            cerr << "Error trying to zero the rpfitsarray!!!" << endl;
          freqnumber = config->getDFreqIndex(currentconfigindex, i, datastreampolbandoffsets[i][j][firstpolindex]%config->getDNumOutputBands(currentconfigindex, i)) + config->getIndependentChannelIndex(currentconfigindex)*config->getFreqTableLength() + 1;

          for(int k=0;k<maxproducts; k++)
          {
            if(datastreampolbandoffsets[i][j][k] >= 0)
            {
              //put the stuff in the rpfitsarray in order
              for(int l=0;l<numchannels+1;l++)
                rpfitsarray[l*maxproducts + k] = results[count + l + (numchannels+1)*datastreampolbandoffsets[i][j][k]];
            }
          }

          //write out the rpfits data
          rpfitsout_(&status/*should be 0 = writing data*/, visibilities, 0/*not using weights*/, &baselinenumber, &offsetstartdaysec, &buvw[0], &buvw[1], &buvw[2], &flag/*not flagged*/, &bin, &freqnumber, &sourcenumber);
        }
        else
          cerr << "WARNING - did not find any bands for frequency " << j << " of datastream " << i << endl;
      }
      count += autocorrincrement*(numchannels+1)*config->getDNumOutputBands(currentconfigindex, i);
    }
  }
  if(status != 0)
    cerr << "Error trying to write visibilities for " << currentstartseconds << " seconds plus " << currentstartsamples << " samples" << endl;
}

void Visibility::writedifx()
{
  DIR * difxdir;
  struct dirent * difxfile;
  string currentfile, tempfile;
  ofstream output;
  char newfilename[256];
  int intmjd, intseconds, numvispoints, activevispoints, dumpmjd, binloop, sourceindex, freqindex, numpolproducts, firstpolindex, baselinenumber;
  double dumpseconds, filemjd, latestmjd = -1.0;
  int status = 0;
  int flag = 0;
  int bin = 0;
  int count = 0;
  float buvw[3]; //the u,v and w for this baseline at this time
  char polpair[3]; //the polarisation eg RR, LL

  if(config->pulsarBinOn(currentconfigindex) && !config->scrunchOutputOn(currentconfigindex))
    binloop = config->getNumPulsarBins(currentconfigindex);
  else
    binloop = 1;
  sourceindex = config->getSourceIndex(expermjd, experseconds + currentstartseconds);

  //work out the time of this integration
  dumpmjd = expermjd + (experseconds + currentstartseconds)/86400;
  dumpseconds = double((experseconds + currentstartseconds)%86400) + double((currentstartsamples+integrationsamples/2)/(2000000.0*config->getDBandwidth(currentconfigindex,0,0)));

  //get the name of the last DiFX format file written with the correct prefix
  difxdir = opendir(config->getOutputFilename().c_str());
  while (difxfile = readdir(difxdir)) {
    tempfile = difxfile->d_name;
    if(strstr(difxfile->d_name, "DIFX") != NULL) //it is a DiFX binary output file
    {
      strtok(difxfile->d_name, "_."); //skip the leading DIFX_
      intmjd = atoi(strtok(NULL, "_."));
      intseconds = atoi(strtok(NULL, "_."));
      numvispoints = atoi(strtok(NULL, "_"));
      filemjd = double(intmjd) + double(intseconds)/86400.0;
      if(filemjd>latestmjd) {
        latestmjd = filemjd;
        currentfile = config->getOutputFilename() + "/" + tempfile;
        activevispoints = numvispoints;
      }
    }
  }
  closedir(difxdir);

  //if none exist or this file already is large enough, create a new file
  if(latestmjd < 0 || activevispoints > 49999) {
    sprintf(newfilename, "%s/DIFX_%05d_%06d.00000", config->getOutputFilename().c_str(), dumpmjd, int(dumpseconds));
    currentfile = newfilename;
    output.open(newfilename);
    output.close();
    activevispoints = 0;
  }

  //work through each baseline visibility point
  for(int i=0;i<numbaselines;i++)
  {
    baselinenumber = config->getBNumber(currentconfigindex, i);

    //interpolate the uvw
    (config->getUVW())->interpolateUvw(config->getDStationName(currentconfigindex, config->getBOrderedDataStream1Index(currentconfigindex, i)), config->getDStationName(currentconfigindex, config->getBOrderedDataStream2Index(currentconfigindex, i)), expermjd, dumpseconds + (dumpmjd-expermjd)*86400.0, buvw);
    for(int j=0;j<config->getBNumFreqs(currentconfigindex,i);j++)
    {
      freqindex = config->getBFreqIndex(currentconfigindex, i, j);
      numpolproducts = config->getBNumPolProducts(currentconfigindex, i, j);

      for(int b=0;b<binloop;b++)
      {
        for(int k=0;k<numpolproducts;k++) 
        {
          config->getBPolPair(currentconfigindex, i, j, k, polpair);

          //open the file for appending in ascii and write the ascii header
          output.open(currentfile.c_str(), ios::app);
          writeDiFXHeader(&output, baselinenumber, dumpmjd, dumpseconds, currentconfigindex, sourceindex, freqindex, polpair, b, 0, 0, buvw);

          //close, reopen in binary and write the binary data, then close again
          output.close();
          output.open(currentfile.c_str(), ios::app|ios::binary);
          output.write((char*)(results + (count*(numchannels+1))), numchannels*sizeof(cf32));
          output.close();

          count++;
        }
      }
    }
  }

  //now each autocorrelation visibility point if necessary
  if(config->writeAutoCorrs(currentconfigindex))
  {
    buvw[0] = 0.0;
    buvw[1] = 0.0;
    buvw[2] = 0.0;
    for(int i=0;i<numdatastreams;i++)
    {
      baselinenumber = 257*(config->getDTelescopeIndex(currentconfigindex, i)+1);
      for(int j=0;j<config->getDNumFreqs(currentconfigindex, i); j++)
      {
        firstpolindex = -1;
        //find a product that is active, so we know where to look to work out which frequency this is
        for(int k=maxproducts-1;k>=0;k--) {
          if(datastreampolbandoffsets[i][j][k] >= 0)
            firstpolindex = k;
        }
        if(firstpolindex >= 0)
        {
          freqindex = config->getDFreqIndex(currentconfigindex, i, datastreampolbandoffsets[i][j][firstpolindex]%config->getDNumOutputBands(currentconfigindex, i));

          for(int k=0;k<maxproducts; k++)
          {
            if(datastreampolbandoffsets[i][j][k] >= 0)
            {
              //open, write the header and close
              output.open(currentfile.c_str(), ios::app);
              writeDiFXHeader(&output, baselinenumber, dumpmjd, dumpseconds, currentconfigindex, sourceindex, freqindex, polnames[k].c_str(), 0, 0, 0, buvw);
              output.close();

              //open, write the binary data and close
              output.open(currentfile.c_str(), ios::app|ios::binary);
              output.write((char*)(results + (count*(numchannels+1)*datastreampolbandoffsets[i][j][k])), numchannels*sizeof(cf32));
              output.close();
            }
          }
        }
        else
          cerr << "WARNING - did not find any bands for frequency " << j << " of datastream " << i << endl;
      }
      count += autocorrincrement*config->getDNumOutputBands(currentconfigindex, i);
    }
  }

  //update the filename
  sprintf(newfilename, "%s/DIFX_%05d_%06d.%05d", config->getOutputFilename().c_str(), dumpmjd, int(dumpseconds), activevispoints+count);
  rename(currentfile.c_str(), newfilename);
}

void Visibility::writeDiFXHeader(ofstream * output, int baselinenum, int dumpmjd, double dumpseconds, int configindex, int sourceindex, int freqindex, const char polproduct[3], int pulsarbin, int flag, int writeweights, float buvw[3])
{
  *output << setprecision(15);
  *output << "BASELINE NUM:       " << baselinenum << endl;
  *output << "MJD:                " << dumpmjd << endl;
  *output << "SECONDS:            " << dumpseconds << endl;
  *output << "CONFIG INDEX:       " << configindex << endl;
  *output << "SOURCE INDEX:       " << sourceindex << endl;
  *output << "FREQ INDEX:         " << freqindex << endl;
  *output << "POLARISATION PAIR:  " << polproduct[0] << polproduct[1] << endl;
  *output << "PULSAR BIN:         " << pulsarbin << endl;
  *output << "FLAGGED:            " << flag << endl;
  *output << "WEIGHTS WRITTEN:    " << writeweights << endl;
  *output << "U (METRES):         " << buvw[0] << endl;
  *output << "V (METRES):         " << buvw[1] << endl;
  *output << "W (METRES):         " << buvw[2] << endl;
}

void Visibility::changeConfig(int configindex)
{
  char polpair[3];
  bool found;
  polpair[2] = 0;
  
  if(first) 
  {
    //can just allocate without freeing all the old stuff
    first = false;
    autocorrcalibs = new cf32*[numdatastreams];
    baselinepoloffsets = new int**[numbaselines];
    datastreampolbandoffsets = new int**[numdatastreams];
  }
  else
  {
    //need to delete the old arrays before allocating the new ones
    for(int i=0;i<numdatastreams;i++)
      delete [] autocorrcalibs[i];
    for(int i=0;i<numbaselines;i++)
    {
      for(int j=0;j<config->getBNumFreqs(currentconfigindex, i);j++)
        delete [] baselinepoloffsets[i][j];
      delete [] baselinepoloffsets[i];
    }
    for(int i=0;i<numdatastreams;i++)
    {
      for(int j=0;j<config->getDNumFreqs(currentconfigindex, i);j++)
        delete [] datastreampolbandoffsets[i][j];
      delete [] datastreampolbandoffsets[i];
    }
    vectorFree(rpfitsarray);
  }

  //get the new parameters for this configuration from the config object
  currentconfigindex = configindex;
  numchannels = config->getNumChannels(configindex);
  samplespersecond = int(2000000*config->getDBandwidth(configindex, 0, 0) + 0.5);
  integrationsamples = int(2000000*config->getDBandwidth(configindex, 0, 0)*config->getIntTime(configindex) + 0.5);
  blocksamples = config->getBlocksPerSend(configindex)*numchannels*2;
  offsetperintegration = integrationsamples%blocksamples;
  cout << "For Visibility " << visID << ", offsetperintegration is " << offsetperintegration << ", blocksamples is " << blocksamples << ", and configindex is " << configindex << endl;
  resultlength = config->getResultLength(configindex);
  for(int i=0;i<numdatastreams;i++)
    autocorrcalibs[i] = new cf32[config->getDNumOutputBands(configindex, i)];

  //work out the offsets we need to use to put things in the rpfits array in the right order
  for(int i=0;i<numbaselines;i++)
  {
    baselinepoloffsets[i] = new int*[config->getBNumFreqs(configindex, i)];
    for(int j=0;j<config->getBNumFreqs(configindex, i);j++)
    {
      baselinepoloffsets[i][j] = new int[config->getBNumPolProducts(configindex, i, j)];
      for(int k=0;k<config->getBNumPolProducts(configindex, i, j);k++)
      {
        config->getBPolPair(configindex, i, j, k, polpair);
        found = false;
        for(int l=0;l<maxproducts;l++)
        {
          if(polnames[l] == string(polpair))
          {
            baselinepoloffsets[i][j][k] = l;
            found = true;
          }
        }
        if(!found)
        {
          cerr << "Error - could not find a polarisation pair, will be put in position " << maxproducts << "!!!" << endl;
          baselinepoloffsets[i][j][k] = maxproducts-1;
        }
      }   
    }
  }

  //do the same for the datastreams, so we can order things properly in the rpfits array
  for(int i=0;i<numdatastreams;i++)
  {
    cout << "Creating datastreampolbandoffsets, length " << config->getDNumFreqs(configindex,i) << endl;
    datastreampolbandoffsets[i] = new int*[config->getDNumFreqs(configindex,i)];
    for(int j=0;j<config->getDNumFreqs(configindex, i);j++)
    {
      datastreampolbandoffsets[i][j] = new int[maxproducts];
      for(int k=0;k<maxproducts;k++)
        datastreampolbandoffsets[i][j][k] = -1;
      for(int k=0;k<config->getDNumOutputBands(configindex, i);k++)
      {
        if(config->getDLocalFreqIndex(configindex, i, k) == j)
        {
          //work out the index in the polnames array
          for(int l=0;l<maxproducts;l++)
          {
            if(config->getDBandPol(configindex, i, k) == polnames[l].data()[0])
            {
              if(config->getDBandPol(configindex, i, k) == polnames[l].data()[1])
                datastreampolbandoffsets[i][j][l] = k;
              else
                datastreampolbandoffsets[i][j][l] = k+config->getDNumOutputBands(configindex, i);
            }
          }
        }
      }
    }
  }
  //allocate the rpfits array
  rpfitsarray = vectorAlloc_cf32(maxproducts*(numchannels+1));
}
