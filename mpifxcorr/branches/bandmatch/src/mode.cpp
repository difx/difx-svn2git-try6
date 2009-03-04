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
//===========================================================================
// SVN properties (DO NOT CHANGE)
//
// $Id$
// $HeadURL$
// $LastChangedRevision$
// $Author$
// $LastChangedDate$
//
//============================================================================
#include <mpi.h>
#include <iomanip>
#include "mode.h"
#include "math.h"
#include "architecture.h"
#include "datastream.h"
#include "alert.h"

//using namespace std;
const float Mode::TINY = 0.000000001;


Mode::Mode(Configuration * conf, int confindex, int dsindex, int recordedbandchan, int bpersend, int gsamples, int nrecordedfreqs, double recordedbw, double * recordedfreqclkoffs, double * recordedfreqlooffs, int nrecordedbands, int nzoombands, int nbits, int unpacksamp, bool fbank, bool postffringe, bool quaddelayinterp, bool cacorrs, double bclock)
  : config(conf), configindex(confindex), datastreamindex(dsindex), recordedbandchannels(recordedbandchan), blockspersend(bpersend), guardsamples(gsamples), twicerecordedbandchannels(recordedbandchan*2), numrecordedfreqs(nrecordedfreqs), numrecordedbands(nrecordedbands), numzoombands(nzoombands), numbits(nbits), unpacksamples(unpacksamp), recordedbandwidth(recordedbw), blockclock(bclock), filterbank(fbank), calccrosspolautocorrs(cacorrs), postffringerot(postffringe), quadraticdelayinterp(quaddelayinterp), recordedfreqclockoffsets(recordedfreqclkoffs), recordedfreqlooffsets(recordedfreqlooffs)
{
  int status, localfreqindex;
  int decimationfactor = config->getDDecimationFactor(configindex, datastreamindex);

  initok = true;
  sampletime = 1.0/(2.0*recordedbandwidth); //microseconds
  processtime = twicerecordedbandchannels*sampletime; //microseconds
  fractionalLoFreq = false;
  for(int i=0;i<numrecordedfreqs;i++)
  {
    if(config->getDRecordedFreq(configindex, datastreamindex, i) - int(config->getDRecordedFreq(configindex, datastreamindex, i)) > TINY)
      fractionalLoFreq = true;
  }

  //cout << "Numzoombands is " << numzoombands << endl;

  //now do the rest of the initialising
  samplesperblock = int(recordedbandwidth*2/blockclock);
  if(samplesperblock == 0)
  {
    cfatal << startl << "Error!!! Samplesperblock is less than 1, current implementation cannot handle this situation.  Aborting!" << endl;
    initok = false;
  }
  else
  {
    bytesperblocknumerator = (numrecordedbands*samplesperblock*numbits*decimationfactor)/8;
    if(bytesperblocknumerator == 0)
    {
      bytesperblocknumerator = 1;
      bytesperblockdenominator = 8/(numrecordedbands*samplesperblock*numbits*decimationfactor);
      unpacksamples += bytesperblockdenominator*sizeof(u16)*samplesperblock;
    }
    else
    {
      bytesperblockdenominator = 1;
    }
    samplesperlookup = (numrecordedbands*sizeof(u16)*samplesperblock*bytesperblockdenominator)/bytesperblocknumerator;
    numlookups = (unpacksamples*bytesperblocknumerator)/(bytesperblockdenominator*sizeof(u16)*samplesperblock);
    if(samplesperblock > 1)
      numlookups++;
    unpackedarrays = new f32*[numrecordedbands];
    fftoutputs = new cf32*[numrecordedbands + numzoombands];
    conjfftoutputs = new cf32*[numrecordedbands + numzoombands];
    for(int i=0;i<numrecordedbands;i++)
    {
      unpackedarrays[i] = vectorAlloc_f32(unpacksamples);
      fftoutputs[i] = vectorAlloc_cf32(recordedbandchannels+1);
      conjfftoutputs[i] = vectorAlloc_cf32(recordedbandchannels+1);
    }
    for(int i=0;i<numzoombands;i++)
    {
      localfreqindex = config->getDLocalZoomFreqIndex(confindex, dsindex, i);
      //cout << "Setting the zoom band pointers for zoom band " << i << ", which has is from zoom freq " << localfreqindex << endl;
      //cout << "The parent freq index for this band is " << config->getDZoomFreqParentFreqIndex(confindex, dsindex, localfreqindex) << " and the channel offset is " << config->getDZoomFreqChannelOffset(confindex, dsindex, localfreqindex) << endl;
      fftoutputs[i+numrecordedbands] = &(fftoutputs[config->getDZoomFreqParentFreqIndex(confindex, dsindex, localfreqindex)][config->getDZoomFreqChannelOffset(confindex, dsindex, localfreqindex)]);
      conjfftoutputs[i+numrecordedbands] = &(conjfftoutputs[config->getDZoomFreqParentFreqIndex(confindex, dsindex, localfreqindex)][config->getDZoomFreqChannelOffset(confindex, dsindex, localfreqindex)]);
    }
    dataweight = 0.0;
    //cout << "fftoutputs[0] = " << fftoutputs[0] << ", fftoutputs[1] = " << fftoutputs[1] << ", fftoutputs[2] = " << fftoutputs[2] << endl;

    lookup = vectorAlloc_s16((MAX_U16+1)*samplesperlookup);
    linearunpacked = vectorAlloc_s16(numlookups*samplesperlookup);
    xval = vectorAlloc_f64(twicerecordedbandchannels);
    xoffset = vectorAlloc_f64(twicerecordedbandchannels);
    for(int i=0;i<twicerecordedbandchannels;i++)
      xoffset[i] = double(i)/double(recordedbandchannels*blockspersend);

    //initialise the fft info
    order = 0;
    while((twicerecordedbandchannels) >> order != 1)
      order++;
    flag = vecFFT_NoReNorm;
    hint = vecAlgHintFast;
  
    if(postffringerot) //initialise the specific structures
    {
      //cwarn << startl << "Warning - post-f fringe rotation not yet tested!!!" << endl;
      status = vectorInitFFTR_f32(&pFFTSpecR, order, flag, hint);
      if(status != vecNoErr)
        csevere << startl << "Error in FFT initialisation!!!" << status << endl;
      status = vectorGetFFTBufSizeR_f32(pFFTSpecR, &fftbuffersize);
      if(status != vecNoErr)
        csevere << startl << "Error in FFT buffer size calculation!!!" << status << endl;
    }
    else
    {
      rotateargument = vectorAlloc_f32(twicerecordedbandchannels);
      cosrotated = vectorAlloc_f32(twicerecordedbandchannels);
      cosrotatedoutput = vectorAlloc_f32(twicerecordedbandchannels);
      sinrotated = vectorAlloc_f32(twicerecordedbandchannels);
      sinrotatedoutput = vectorAlloc_f32(twicerecordedbandchannels);
      realfftd = vectorAlloc_f32(twicerecordedbandchannels);
      imagfftd = vectorAlloc_f32(twicerecordedbandchannels);
      fringedelayarray = vectorAlloc_f64(twicerecordedbandchannels);
      status = vectorInitFFTC_f32(&pFFTSpecC, order, flag, hint);
      if(status != vecNoErr)
        csevere << startl << "Error in FFT initialisation!!!" << status << endl;
      status = vectorGetFFTBufSizeC_f32(pFFTSpecC, &fftbuffersize);
      if(status != vecNoErr)
        csevere << startl << "Error in FFT buffer size calculation!!!" << status << endl;
      //zero the Nyquist channel for every band - that is where the weight will be stored on all
      //baselines (the imag part) so the datastream channel for it must be zeroed
      for(int i=0;i<numrecordedbands;i++)
      {
        if(config->getDRecordedLowerSideband(configindex, datastreamindex, config->getDLocalRecordedFreqIndex(configindex, datastreamindex, i))) {
          fftoutputs[i][0].re = 0.0;
          fftoutputs[i][0].im = 0.0;
        }
        else {
          fftoutputs[i][recordedbandchannels].re = 0.0;
          fftoutputs[i][recordedbandchannels].im = 0.0;
        }
      }
    }
    fftbuffer = vectorAlloc_u8(fftbuffersize);
  
    delaylength = blockspersend + guardsamples/twicerecordedbandchannels + 1;
    delays = vectorAlloc_f64(delaylength);
  
    fracmult = vectorAlloc_f32(recordedbandchannels + 1);
    fracmultcos = vectorAlloc_f32(recordedbandchannels + 1);
    fracmultsin = vectorAlloc_f32(recordedbandchannels + 1);
    complexfracmult = vectorAlloc_cf32(recordedbandchannels + 1);
  
    channelfreqs = vectorAlloc_f32(recordedbandchannels + 1);
    for(int i=0;i<recordedbandchannels + 1;i++)
      channelfreqs[i] = (float)((TWO_PI*i*recordedbandwidth)/recordedbandchannels);
    lsbchannelfreqs = vectorAlloc_f32(recordedbandchannels + 1);
    for(int i=0;i<recordedbandchannels + 1;i++)
      lsbchannelfreqs[i] = (float)((-TWO_PI*(recordedbandchannels-i)*recordedbandwidth)/recordedbandchannels);
  
    //space for the autocorrelations
    if(calccrosspolautocorrs)
      autocorrwidth = 2;
    else
      autocorrwidth = 1;
    autocorrelations = new cf32**[autocorrwidth];
    for(int i=0;i<autocorrwidth;i++)
    {
      autocorrelations[i] = new cf32*[numrecordedbands+numzoombands];
      for(int j=0;j<numrecordedbands;j++)
        autocorrelations[i][j] = vectorAlloc_cf32(recordedbandchannels+1);
      for(int j=0;j<numzoombands;j++)
      {
        localfreqindex = config->getDLocalZoomFreqIndex(confindex, dsindex, j);
        autocorrelations[i][j+numrecordedbands] = &(autocorrelations[i][config->getDZoomFreqParentFreqIndex(confindex, dsindex, localfreqindex)][config->getDZoomFreqChannelOffset(confindex, dsindex, localfreqindex)]);
      }
    }
  }
}

Mode::~Mode()
{
  int status;

  cdebug << startl << "Starting a mode destructor" << endl;

  for(int i=0;i<numrecordedbands;i++)
  {
    vectorFree(unpackedarrays[i]);
    vectorFree(fftoutputs[i]);
    vectorFree(conjfftoutputs[i]);
  }
  delete [] unpackedarrays;
  delete [] fftoutputs;
  delete [] conjfftoutputs;
  vectorFree(xoffset);
  vectorFree(xval);

  if(postffringerot)
  {
    status = vectorFreeFFTR_f32(pFFTSpecR);
    if(status != vecNoErr)
      csevere << startl << "Error in freeing FFT spec!!!" << status << endl;
  }
  else
  {
    vectorFree(rotateargument);
    vectorFree(realfftd);
    vectorFree(imagfftd);
    vectorFree(cosrotated);
    vectorFree(cosrotatedoutput);
    vectorFree(sinrotated);
    vectorFree(sinrotatedoutput);
    vectorFree(fringedelayarray);
    status = vectorFreeFFTC_f32(pFFTSpecC);
    if(status != vecNoErr)
      csevere << startl << "Error in freeing FFT spec!!!" << status << endl;
  }
  vectorFree(lookup);
  vectorFree(linearunpacked);
  vectorFree(fftbuffer);
  vectorFree(delays);
  vectorFree(fracmult);
  vectorFree(fracmultcos);
  vectorFree(fracmultsin);
  vectorFree(complexfracmult);
  vectorFree(channelfreqs);

  for(int i=0;i<autocorrwidth;i++)
  {
    for(int j=0;j<numrecordedbands;j++)
      vectorFree(autocorrelations[i][j]);
    delete [] autocorrelations[i];
  }
  delete [] autocorrelations;

  cdebug << startl << "Ending a mode destructor" << endl;
}

float Mode::unpack(int sampleoffset)
{
  int status, leftoversamples, stepin = 0;

  if(bytesperblockdenominator/bytesperblocknumerator == 0)
    leftoversamples = 0;
  else
    leftoversamples = sampleoffset%(bytesperblockdenominator/bytesperblocknumerator);
  unpackstartsamples = sampleoffset - leftoversamples;
  if(samplesperblock > 1)
    stepin = unpackstartsamples%(samplesperblock*bytesperblockdenominator);
  u16 * packed = (u16 *)(&(data[((unpackstartsamples/samplesperblock)*bytesperblocknumerator)/bytesperblockdenominator]));

  //copy from the lookup table to the linear unpacked array
  for(int i=0;i<numlookups;i++)
  {
    status = vectorCopy_s16(&lookup[packed[i]*samplesperlookup], &linearunpacked[i*samplesperlookup], samplesperlookup);
    if(status != vecNoErr) {
      csevere << startl << "Error in lookup for unpacking!!!" << status << endl;
      return 0;
    }
  }

  //split the linear unpacked array into the separate subbands
  status = vectorSplitScaled_s16f32(&(linearunpacked[stepin*numrecordedbands]), unpackedarrays, numrecordedbands, unpacksamples);
  if(status != vecNoErr) {
    csevere << startl << "Error in splitting linearunpacked!!!" << status << endl;
    return 0;
  }

  return 1.0;
}

float Mode::process(int index)  //frac sample error, fringedelay and wholemicroseconds are in microseconds 
{
  double phaserotation, averagedelay, nearestsampletime, starttime, finaloffset, lofreq, distance, walltimesecs;
  f32 phaserotationfloat, fracsampleerror;
  int status, count, nearestsample, integerdelay, sidebandoffset;
  cf32* fftptr;
  f32* currentchannelfreqptr;
  int indices[10];
  
  if(datalengthbytes == 0 || !(delays[index] > MAX_NEGATIVE_DELAY) || !(delays[index+1] > MAX_NEGATIVE_DELAY))
  {
    for(int i=0;i<numrecordedbands;i++)
    {
      status = vectorZero_cf32(fftoutputs[i], recordedbandchannels+1);
      if(status != vecNoErr)
        csevere << startl << "Error trying to zero fftoutputs when data is bad!" << endl;
      status = vectorZero_cf32(conjfftoutputs[i], recordedbandchannels+1);
      if(status != vecNoErr)
        csevere << startl << "Error trying to zero fftoutputs when data is bad!" << endl;
    }
    return 0.0; //don't process crap data
  }

  averagedelay = (delays[index] + delays[index+1])/2.0;
  starttime = (offsetseconds-bufferseconds)*1000000.0 + (double(offsetns)/1000.0 + index*twicerecordedbandchannels*sampletime - buffermicroseconds) - averagedelay;
  nearestsample = int(starttime/sampletime + 0.5);
  walltimesecs = offsetseconds + ((double)offsetns)/1000000000.0 + index*twicerecordedbandchannels*sampletime;

  //if we need to, unpack some more data - first check to make sure the pos is valid at all
  if(nearestsample < -1 || (((nearestsample + twicerecordedbandchannels)/samplesperblock)*bytesperblocknumerator)/bytesperblockdenominator > datalengthbytes)
  {
    cerror << startl << "MODE error for datastream " << datastreamindex << " - trying to process data outside range - aborting!!! nearest sample was " << nearestsample << ", the max bytes should be " << datalengthbytes << ".  bufferseconds was " << bufferseconds << ", offsetseconds was " << offsetseconds << ", buffermicroseconds was " << buffermicroseconds << ", offsetns was " << offsetns << ", index was " << index << ", average delay was " << averagedelay << " composed of previous delay " << delays[index] << " and next delay " << delays[index+1] << endl;
    return 0.0;
  }
  if(nearestsample == -1)
  {
    nearestsample = 0;
    dataweight = unpack(nearestsample);
  }
  else if(nearestsample < unpackstartsamples || nearestsample > unpackstartsamples + unpacksamples - twicerecordedbandchannels)
    //need to unpack more data
    dataweight = unpack(nearestsample);

  if(!(dataweight > 0.0))
    return 0.0;

  nearestsampletime = nearestsample*sampletime;
  fracsampleerror = float(starttime - nearestsampletime);

  if(postffringerot)
  {
    integerdelay = int(averagedelay);
  }
  else //doing pre-f so need to work out the delay arrays
  {
    if(dolinearinterp)
    {
      integerdelay = int(delays[index]);
      double rate = blockspersend*(delays[index+1]-delays[index])/2.0;
      //multiply the offset by the rate
      status = vectorMulC_f64(xoffset, rate, xval, twicerecordedbandchannels);
      if(status != vecNoErr)
        csevere << startl << "Error in linearinterpolate, multiplication" << endl;
      //add the starting delay
      status = vectorAddC_f64_I(delays[index] - integerdelay, xval, twicerecordedbandchannels);
      if(status != vecNoErr)
        csevere << startl << "Error in linearinterpolate, final offset add!!!" << endl;
    }
    else //we're doing quadratic interpolation
    {
      integerdelay = int(centredelay);
      finaloffset = toaddlast - integerdelay;
      distance = double(index*2)/double(blockspersend) - 1.0;

      //change x to x + b/a
      status = vectorAddC_f64(xoffset, distance + toaddfirst, xval, twicerecordedbandchannels);
      if(status != vecNoErr)
        csevere << startl << "Error in quadinterpolate, offset add!!!" << endl;
      //square
      status = vectorSquare_f64_I(xval, twicerecordedbandchannels);
      if(status != vecNoErr)
        csevere << startl << "Error in quadinterpolate, xval squaring" << endl;
      //multiply by a
      status = vectorMulC_f64_I(a, xval, twicerecordedbandchannels);
      if(status != vecNoErr)
        csevere << startl << "Error in quadinterpolate, multiplication" << endl;
      //add c - b^2/4a
      status = vectorAddC_f64_I(finaloffset, xval, twicerecordedbandchannels);
      if(status != vecNoErr)
        csevere << startl << "Error in quadinterpolate, final offset add!!!" << endl;
    }
  }

  for(int i=0;i<numrecordedfreqs;i++)
  {
    count = 0;
    //updated so that Nyquist channel is not accumulated for either USB or LSB data
    sidebandoffset = 0;
    if(config->getDRecordedLowerSideband(configindex, datastreamindex, i))
      sidebandoffset = 1;

    //create the array for fractional sample error correction - including the post-f fringe rotation
    currentchannelfreqptr = (config->getDRecordedLowerSideband(configindex, datastreamindex, i))?lsbchannelfreqs:channelfreqs;
    status = vectorMulC_f32(currentchannelfreqptr, fracsampleerror - recordedfreqclockoffsets[i], fracmult, recordedbandchannels + 1);
    if(status != vecNoErr)
      csevere << startl << "Error in frac sample correction!!!" << status << endl;

    lofreq = config->getDRecordedFreq(configindex, datastreamindex, i);
    if(postffringerot)
    {
      //work out the phase rotation to apply
      phaserotation = (averagedelay-integerdelay)*lofreq;
      if(fractionalLoFreq)
        phaserotation += integerdelay*(lofreq-int(lofreq));
      phaserotation -= walltimesecs*recordedfreqlooffsets[i];
      phaserotationfloat = (f32)(-TWO_PI*(phaserotation-int(phaserotation)));

      status = vectorAddC_f32_I(phaserotationfloat, fracmult, recordedbandchannels+1);
      if(status != vecNoErr)
        csevere << startl << "Error in post-f phase rotation addition (and maybe LO offset correction)!!!" << status << endl;
    }
    else //need to work out the time domain modulation
    {
      status = vectorMulC_f64(xval, lofreq, fringedelayarray, twicerecordedbandchannels);
      if(status != vecNoErr)
        csevere << startl << "Error in delay multiplication!!!" << status << endl;
      if(fractionalLoFreq)
      {
          status = vectorAddC_f64_I((lofreq-int(lofreq))*double(integerdelay), fringedelayarray, twicerecordedbandchannels);
          if(status != vecNoErr)
            csevere << startl << "Error in addition of fractional LO contribution to fringe rotation!!!" << status << endl;
      }

      //convert to angle in range 0->2pi
      for(int j=0;j<twicerecordedbandchannels;j++)
        rotateargument[j] = -TWO_PI*(fringedelayarray[j] - int(fringedelayarray[j]));

      //do the sin/cos
      status = vectorSinCos_f32(rotateargument, sinrotated, cosrotated, twicerecordedbandchannels);
      if(status != vecNoErr)
        csevere << startl << "Error in sin/cos of rotate argument!!! Status = " << status << endl;

      //take care of an LO offset if present\
      if(recordedfreqlooffsets[i] > 0.0 || recordedfreqlooffsets[i] < 0.0)
      {
        phaserotation = -walltimesecs*recordedfreqlooffsets[i];
        phaserotationfloat = (f32)(-TWO_PI*(phaserotation-int(phaserotation)));
        status = vectorAddC_f32_I(phaserotationfloat, fracmult, recordedbandchannels+1);
        if(status != vecNoErr)
          csevere << startl << "Error in LO offset correction!!!" << status << endl;
      }
    }

    status = vectorSinCos_f32(fracmult, fracmultsin, fracmultcos, recordedbandchannels + 1);
    if(status != vecNoErr)
      csevere << startl << "Error in frac sample correction!!!" << status << endl; 

    status = vectorRealToComplex_f32(fracmultcos, fracmultsin, complexfracmult, recordedbandchannels + 1);
    if(status != vecNoErr)
      csevere << startl << "Error in frac sample correction!!!" << status << endl; 

    for(int j=0;j<numrecordedbands;j++)
    {
      if(config->matchingRecordedBand(configindex, datastreamindex, i, j))
      {
        indices[count++] = j;
        if(postffringerot)
        {
          fftptr = (config->getDRecordedLowerSideband(configindex, datastreamindex, i))?conjfftoutputs[j]:fftoutputs[j];
  
          //do the fft
          status = vectorFFT_RtoC_f32(&(unpackedarrays[j][nearestsample - unpackstartsamples]), (f32*)fftptr, pFFTSpecR, fftbuffer);
          if(status != vecNoErr)
            csevere << startl << "Error in FFT!!!" << status << endl;
  
          //fix the lower sideband if required
          if(config->getDRecordedLowerSideband(configindex, datastreamindex, i))
          {
            status = vectorConjFlip_cf32(fftptr, fftoutputs[j], recordedbandchannels + 1);
            if(status != vecNoErr)
              csevere << startl << "Error in conjugate!!!" << status << endl;
          }
        }
        else //doing pre-f fringe rot
        {
          //do the fringe rotation
          status = vectorMul_f32(sinrotated, &(unpackedarrays[j][nearestsample - unpackstartsamples]), sinrotatedoutput, twicerecordedbandchannels);
          if(status != vecNoErr)
            csevere << startl << "Error in sine fringe rotation!!!" << status << endl;
          status = vectorMul_f32(cosrotated, &(unpackedarrays[j][nearestsample - unpackstartsamples]), cosrotatedoutput, twicerecordedbandchannels);
          if(status != vecNoErr)
            csevere << startl << "Error in cosine fringe rotation!!!" << status << endl;

          //do the fft
          status = vectorFFT_CtoC_f32(cosrotatedoutput, sinrotatedoutput, realfftd, imagfftd, pFFTSpecC, fftbuffer);
          if(status != vecNoErr)
            csevere << startl << "Error in FFT!!!" << status << endl;

          //assemble complex from the real and imaginary
          if(config->getDRecordedLowerSideband(configindex, datastreamindex, i)) {
            //updated to include "DC" channel at upper end of LSB band
            status = vectorRealToComplex_f32(&realfftd[recordedbandchannels+1], &imagfftd[recordedbandchannels+1], &(fftoutputs[j][1]), recordedbandchannels-1);
            fftoutputs[j][recordedbandchannels].re = realfftd[0];
            fftoutputs[j][recordedbandchannels].im = imagfftd[0];
          }
          else {
            //updated to include "Nyquist" channel
            status = vectorRealToComplex_f32(realfftd, imagfftd, fftoutputs[j], recordedbandchannels);
          }
          if(status != vecNoErr)
            csevere << startl << "Error assembling complex fft result" << endl;
        }

        //do the frac sample correct (+ fringe rotate if its post-f)
        status = vectorMul_cf32_I(complexfracmult, fftoutputs[j], recordedbandchannels + 1);
        if(status != vecNoErr)
          csevere << startl << "Error in frac sample correction!!!" << status << endl;

        //do the conjugation
        status = vectorConj_cf32(fftoutputs[j], conjfftoutputs[j], recordedbandchannels + 1);
        if(status != vecNoErr)
          csevere << startl << "Error in conjugate!!!" << status << endl;

        //do the autocorrelation (skipping Nyquist channel)
        status = vectorAddProduct_cf32(fftoutputs[j]+sidebandoffset, conjfftoutputs[j]+sidebandoffset, autocorrelations[0][j]+sidebandoffset, recordedbandchannels);
        if(status != vecNoErr)
          csevere << startl << "Error in autocorrelation!!!" << status << endl;

        //Add the weight in magic location (imaginary part of Nyquist channel)
        autocorrelations[0][j][recordedbandchannels*(1-sidebandoffset)].im += dataweight;
      }
    }

    //if we need to, do the cross-polar autocorrelations
    if(calccrosspolautocorrs && count > 1)
    {
      //cinfo << startl << "For frequency " << i << ", datastream " << datastreamindex << " has chosen bands " << indices[0] << " and " << indices[1] << endl; 
      status = vectorAddProduct_cf32(fftoutputs[indices[0]]+sidebandoffset, conjfftoutputs[indices[1]]+sidebandoffset, autocorrelations[1][indices[0]]+sidebandoffset, recordedbandchannels);
      if(status != vecNoErr)
        csevere << startl << "Error in cross-polar autocorrelation!!!" << status << endl;
      status = vectorAddProduct_cf32(fftoutputs[indices[1]]+sidebandoffset, conjfftoutputs[indices[0]]+sidebandoffset, autocorrelations[1][indices[1]]+sidebandoffset, recordedbandchannels);
      if(status != vecNoErr)
        csevere << startl << "Error in cross-polar autocorrelation!!!" << status << endl;
      //add the weight in magic location (imaginary part of Nyquist channel)
      autocorrelations[1][indices[0]][recordedbandchannels*(1-sidebandoffset)].im += dataweight;
      autocorrelations[1][indices[1]][recordedbandchannels*(1-sidebandoffset)].im += dataweight;
    }
  }

  return dataweight;
}

void Mode::zeroAutocorrelations()
{
  for(int i=0;i<autocorrwidth;i++)
  {
    for(int j=0;j<numrecordedbands;j++)
      vectorZero_cf32(autocorrelations[i][j], recordedbandchannels+1);
  }
}

void Mode::setDelays(f64 * d)
{
  //cout << "Mode about to copy delays, length " << delaylength << endl;
  int status = vectorCopy_f64(d, delays, delaylength);
  if(status != vecNoErr)
    csevere << startl << "Error trying to copy delays!!!" << endl;
    
  dolinearinterp = !quadraticdelayinterp;
    
  //work out the twiddle factors used later to interpolate
  if(quadraticdelayinterp)
  {
    if(!(delays[0] > MAX_NEGATIVE_DELAY) || !(delays[blockspersend/2] > MAX_NEGATIVE_DELAY) || !(delays[blockspersend/2+1] > MAX_NEGATIVE_DELAY) || !(delays[blockspersend] > MAX_NEGATIVE_DELAY))
    {
      dolinearinterp = true; //no quadratic cause we don't have the info everywhere
    }
    else
    {
      centredelay = (blockspersend%2==1)?(delays[blockspersend/2]+delays[blockspersend/2+1])/2.0:delays[blockspersend/2];
      a = (delays[blockspersend]+delays[0])/2.0 - centredelay;
      b = (delays[blockspersend]-delays[0])/2.0;
      c = centredelay;
      toaddfirst = b/(2*a);
      toaddlast = c - (b*b)/(4*a);
      if(a == 0)
        dolinearinterp = true; //can't use the quadratic since that would involve divide by 0
    }
  }
}

void Mode::setData(u8 * d, int dbytes, double btime)
{
  data = d;
  datalengthbytes = dbytes;
  bufferseconds = int(btime);
  buffermicroseconds = (btime - int(btime))*1000000.0;
  unpackstartsamples = -999999999;
}

const float Mode::decorrelationpercentage[] = {0.63662, 0.88, 0.94, 0.96, 0.98, 0.99, 0.996, 0.998}; //note these are just approximate!!!


LBAMode::LBAMode(Configuration * conf, int confindex, int dsindex, int recordedbandchan, int bpersend, int gsamples, int nrecordedfreqs, double recordedbw, double * recordedfreqclkoffs, double * recordedfreqlooffs, int nrecordedbands, int nzoombands, int nbits, bool fbank, bool postffringe, bool quaddelayinterp, bool cacorrs, const s16* unpackvalues)
    : Mode(conf,confindex,dsindex,recordedbandchan,bpersend,gsamples,nrecordedfreqs,recordedbw,recordedfreqclkoffs,recordedfreqlooffs,nrecordedbands,nzoombands,nbits,recordedbandchan*2,fbank,postffringe,quaddelayinterp,cacorrs,(recordedbw<16.0)?recordedbw*2.0:32.0)
{
  int shift, outputshift;
  int count = 0;
  int numtimeshifts = (sizeof(u16)*bytesperblockdenominator)/bytesperblocknumerator;

  //build the lookup table - NOTE ASSUMPTION THAT THE BYTE ORDER IS **LITTLE-ENDIAN**!!!
  for(u16 i=0;i<MAX_U16;i++)
  {
    shift = 0;
    for(int j=0;j<numtimeshifts;j++)
    {
      for(int k=0;k<numrecordedbands;k++)
      {
        for(int l=0;l<samplesperblock;l++)
        {
          if(samplesperblock > 1 && numrecordedbands > 1) //32 MHz or 64 MHz dual pol
            if(samplesperblock == 4) //64 MHz
              outputshift = 3*(2-l) - 3*k;
            else
              outputshift = -k*samplesperblock + k + l;
          else
            outputshift = 0;

          //if(samplesperblock > 1 && numinputbands > 1) //32 MHz or 64 MHz dual pol
          //  outputshift = (2 - (k + l))*(samplesperblock-1);
          //else
          //  outputshift = 0;

          //littleendian means that the shift, starting from 0, will go through the MS byte, then the LS byte, just as we'd like
          lookup[count + outputshift] = unpackvalues[(i >> shift) & 0x03];
          shift += 2;
          count++;
        }
      }
    }
  }

  //get the last values, i = 1111111111111111
  for (int i=0;i<samplesperlookup;i++)
  {
    lookup[count + i] = unpackvalues[3]; //every sample is 11 = 3
  }
}

const s16 LBAMode::stdunpackvalues[] = {MAX_S16/4, -MAX_S16/4 - 1, 3*MAX_S16/4, -3*MAX_S16/4 - 1};
const s16 LBAMode::vsopunpackvalues[] = {-3*MAX_S16/4 - 1, MAX_S16/4, -MAX_S16/4 - 1, 3*MAX_S16/4};
