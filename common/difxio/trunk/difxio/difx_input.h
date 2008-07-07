/***************************************************************************
 *   Copyright (C) 2007 by Walter Brisken                                  *
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

#ifndef __DIFX_INPUT_H__
#define __DIFX_INPUT_H__

#define DIFX_SESSION_LEN	4
#define MAX_MODEL_ORDER		5

/* Straight from DiFX frequency table */
typedef struct
{
	double freq;		/* (MHz) */
	double bw;		/* (MHz) */
	char sideband;		/* U or L -- net sideband */
} DifxFreq;

/* To become a FITS IF */
typedef struct
{
	double freq;		/* (MHz) */
	double bw;		/* (MHz) */
	char sideband;		/* U or L -- net sideband */
	int nPol;
	char pol[2];		/* polarization codes : L R X or Y. */
} DifxIF;

typedef struct
{
	char fileName[256];	/* filename containing polyco data */
	double dm;		/* pc/cm^3 */
	double refFreq;		/* MHz */
	double mjd;
	int nCoef, nBlk;
	double p0, f0;
	double *coef;
} DifxPolyco;

typedef struct
{
	char fileName[256];	/* pulsar config filename */
	int nPolyco;		/* number of polyco structures in file */
	DifxPolyco *polyco;	/* individual polyco file contents */
	int nBin;		/* number of pulsar bins */
	double *binEnd;		/* [bin] end phase [0.0, 1.0) of bin */
	double *binWeight;	/* [bin] weight to apply to bin */
	int scrunch;		/* 1 = yes, 0 = no */
} DifxPulsar;

/* From DiFX config table, with additional derived information */
typedef struct
{
	double tInt;		/* integration time (sec) */
	int nChan;
	char name[32];
	int postFFringe;	/* 0 or 1 */
	int quadDelayInterp;	/* 0 or 1 */
	int pulsarId;		/* -1 if not pulsar */
	int nPol;		/* number of pols in datastreams (1 or 2) */
	char pol[2];		/* the polarizations */
	int doPolar;		/* >0 if cross hands to be correlated */
	int quantBits;		/* 1 or 2 */
	int nDatastream;	/* number of datastreams attached */
	int nBaseline;		/* number of baselines */
	int *datastreamId;	/* 0-based; datastream table indx */
				/* -1 terminated [ds # < nDatastream]  */
	int *baselineId;	/* baseline table indicies for this config */
				/* -1 terminated [bl # < nBaseline] */
	
	int nIF;		/* number of FITS IFs to create */
	DifxIF *IF;		/* FITS IF definitions */
	int freqId;		/* 0-based number -- uniq FITS IF[] index */
	int *freqId2IF;		/* map from freq table index to IF */
	int ***baselineFreq2IF;	/* [a1][a2][freqNum] -> IF */
	
} DifxConfig;

typedef struct
{
	int antennaId;
	char dataFormat[32];
	int quantBits;		/* quantization bits */
	int nFreq;		/* num freqs from this datastream */
	int nRecChan;		/* number of base band channels recorded */
	int *nPol;		/* [freq] */
	int *freqId;		/* [freq] index to DifxFreq table */
	double *clockOffset;	/* (us) [freq] */
	
	int *RCfreqId;		/* [recChan] index to DifxFreq table */
	char *RCpolName;	/* [recChan] Polarization name (R, L, X or Y) */
} DifxDatastream;

typedef struct
{
	int dsA, dsB;		/* indices to datastream table */
	int nFreq;
	int *nPolProd;		/* [freq] */
	int **recChanA;		/* [freq][productIndex] */
	int **recChanB;		/* [freq][productIndex] */
} DifxBaseline;

typedef struct
{
	char name[32];		/* null terminated */
	int origId;		/* antennaId before a sort */
	double delay;		/* (us) at start of job */
	double rate;		/* (us/s) */
	char mount[8];		/* azel, ... */
	double offset[3];	/* axis offset, (m) */
	double X, Y, Z;		/* telescope position, (m) */
	double dX, dY, dZ;	/* telescope position derivative, (m/s) */
	char vsn[12];		/* vsn for module */
	int spacecraftId;	/* -1 if not a spacecraft */
} DifxAntenna;

typedef struct
{
	double ra, dec;		/* radians */
	char name[32];		/* source name */
	char calCode[4];	/* usually only 1 char long */
	int qual;		/* source qualifier */
	int configId;		/* to determine freqId */
	int spacecraftId;	/* -1 if not spacecraft */
	int fitsSourceId;	/* 0-based FITS source id */
} DifxSource;

typedef struct
{
	double u, v, w;		/* baseline (m) */
	double t;		/* delay (us) */
	double dt;		/* rate (us/s) */
	double dry;		/* dry atmosphere delay (us) */
	double wet;		/* wet atmosphere delay (us) */
	double ddry;		/* dry atmosphere rate (us/s) */
	double dwet;		/* wet atmosphere rate (us/s) */
} DifxModel;

typedef struct
{
	int mjd;		/* day of start of polynomial validity */
	int sec;		/* time (sec) of start of validity */
	int order;		/* order of polynomial -> order+1 terms! */
	int validDuration;	/* (seconds), from mjd, sec */
	double delay[MAX_MODEL_ORDER+1];	/* (us/sec^n); n=[0, order] */
	double dry[MAX_MODEL_ORDER+1];		/* (us/sec^n) */
	double wet[MAX_MODEL_ORDER+1];		/* (us/sec^n) */
	double u[MAX_MODEL_ORDER+1];		/* (m/sec^n) */
	double v[MAX_MODEL_ORDER+1];		/* (m/sec^n) */
	double w[MAX_MODEL_ORDER+1];		/* (m/sec^n) */
} DifxPolyModel;

typedef struct
{
	double mjdStart;	/* (day) */
	double mjdEnd;		/* (day) */
	double ra, dec;		/* (radians) */
	char name[32];		/* name of source */
	char calCode[4];	/* usually only 1 digit */
	int qual;		/* source qualifier */
	int sourceId;		/* 0, 1, ... nScan-1 */
	int configId;		/* 0, 1, ... nConfig-1 */
	int jobId;		/* 0, 1, ... nJob-1 */
	int nPoint;		/* number of points modeled for scan */
	int startPoint;		/* absolute "point number" of first model */
	int nAntenna;
	DifxModel **model;	/* indexed by [ant][point] */
				/* ant is index of DifxAntenna */
				/* NOTE : point is over [-1 .. nPoint+1] ! */
				/* NOTE : model[ant] can be zero -> no data */
	int nPoly;
	DifxPolyModel **im;	/* indexed by [ant][poly] */
				/* ant is index of DifxAntenna */
				/*   poly ranges over [0 .. nPoly-1] */
				/* NOTE : im[ant] can be zero -> no data */
} DifxScan;

typedef struct
{
	int mjd;		/* (day) */
	int tai_utc;		/* (sec) */
	double ut1_utc;		/* (sec) */
	double xPole, yPole;	/* (arcsec) */
} DifxEOP;

typedef struct
{
	int mjd;
	double fracDay;
	long double X, Y, Z;	/* (m) */
	long double dX, dY, dZ;	/* (m/sec) */
} sixVector;

typedef struct
{
	char name[32];		/* name of spacecraft */
	int nPoint;		/* number of entries in ephemeris */
	sixVector *pos;		/* array of positions and velocities */
} DifxSpacecraft;

typedef struct
{
	double mjd1, mjd2;	/* (day) */
	int antennaId;		/* antenna number (index to D->antenna) */
} DifxAntennaFlag;

typedef struct
{
	double jobStart;	/* cjobgen job start time (mjd) */
	double jobStop;		/* cjobgen job start time (mjd) */
	double mjdStart;	/* subjob start time (mjd) */
	double duration;	/* subjob observe duration (sec) */
	double modelInc;	/* model (delay, uvw) interval */
	int jobId;		/* correlator job number */
	int subjobId;		/* difx specific sub-job id */
	int subarrayId;		/* sub array number of the specified sub-job */
	char obsCode[8];	/* project name */
	char obsSession[8];	/* project session (e.g., A, B, C1) */
	char taperFunction[8];	/* usually "UNIFORM" */
	char calcServer[32];	/* name of calc server */
	int calcVersion;	/* version number of calc server */
	int calcProgram;	/* RPC program id of calc server */
	char fileBase[256];	/* base filename for this job table */
	int *antennaIdRemap;	/* remapping of antennas from this job to
                                   the full set of antennas, or 0 */
	int activeDatastreams;
	int activeBaselines;
	int polyOrder;		/* polynomial model order */
	int polyInterval;	/* (sec) length of valid polynomial */
} DifxJob;

typedef struct
{
	int inputFileVersion;	/* version of input file to parse. 0=current */
	double mjdStart;	/* start of combined dataset */
	double mjdStop;		/* end of combined dataset */
	double refFreq;		/* some sort of reference frequency, (MHz) */
	int startChan;		/* first (unaveraged) channel to write */
	int specAvg;		/* number of channels to average */
	int nInChan;		/* number of correlated channels */
	int nOutChan;		/* number of channels to write to FITS */
	int nFFT;		/* size of FFT used */

	int nIF;		/* maximum num IF across configs */
	int nPol;		/* maximum num pol across configs */
	int doPolar;		/* 0 if not, 1 if so */
	int nPolar;		/* nPol*(doPolar+1) */
				/* 1 for single pol obs */
				/* 2 for dual pol, parallel hands only */
				/* 4 for full pol */
	double chanBW;		/* MHz common channel bandwidth. 0 if differ */
	int quantBits;		/* 0 if if different in configs; or 1 or 2 */
	char polPair[4];	/* "  " if different in configs */
	
	int nAntenna, nConfig, nFreq, nScan, nSource, nEOP, nFlag;
	int nDatastream, nBaseline, nSpacecraft, nPulsar, nJob;
	DifxJob		*job;
	DifxConfig	*config;
	DifxFreq	*freq;
	DifxAntenna	*antenna;
	DifxScan	*scan;		/* assumed in time order */
	DifxSource	*source;
	DifxEOP		*eop;		/* assumed one per day, optional */
	DifxAntennaFlag *flag;		/* optional table */
	DifxDatastream	*datastream;
	DifxBaseline    *baseline;
	DifxSpacecraft	*spacecraft;	/* optional table */
	DifxPulsar	*pulsar;	/* optional table */
} DifxInput;

/* DifxJob functions */
DifxJob *newDifxJobArray(int nJob);
void deleteDifxJobArray(DifxJob *dj);
void printDifxJob(const DifxJob *dj);
void copyDifxJob(DifxJob *dest, const DifxJob *src, const int *antennaIdRemap);
DifxJob *mergeDifxJobArrays(const DifxJob *dj1, int ndj1,
	const DifxJob *dj2, int ndj2, int *jobIdRemap, 
	const int *antennaIdRemap, int *ndj);

/* DifxFreq functions */
DifxFreq *newDifxFreqArray(int nFreq);
void deleteDifxFreqArray(DifxFreq *df);
void printDifxFreq(const DifxFreq *df);
int isSameDifxFreq(const DifxFreq *df1, const DifxFreq *df2);
void copyDifxFreq(DifxFreq *dest, const DifxFreq *src);
DifxFreq *mergeDifxFreqArrays(const DifxFreq *df1, int ndf1,
	const DifxFreq *df2, int ndf2, int *freqIdRemap, int *ndf);

/* DifxAntenna functions */
DifxAntenna *newDifxAntennaArray(int nAntenna);
void deleteDifxAntennaArray(DifxAntenna *da);
void printDifxAntenna(const DifxAntenna *da);
int isSameDifxAntenna(const DifxAntenna *da1, const DifxAntenna *da2);
void copyDifxAntenna(DifxAntenna *dest, const DifxAntenna *src);
DifxAntenna *mergeDifxAntennaArrays(const DifxAntenna *da1, int nda1,
	const DifxAntenna *da2, int nda2, int *antennaIdRemap,
	int *nda);

/* DifxDatastream functions */
DifxDatastream *newDifxDatastreamArray(int nDatastream);
void deleteDifxDatastreamArray(DifxDatastream *ds, int nDatastream);
void printDifxDatastream(const DifxDatastream *ds);
int isSameDifxDatastream(const DifxDatastream *dd1, const DifxDatastream *dd2,
	const int *freqIdRemap, const int *antennaIdRemap);
void copyDifxDatastream(DifxDatastream *dest, const DifxDatastream *src,
	const int *freqIdRemap, const int *antennaIdRemap);
DifxDatastream *mergeDifxDatastreamArrays(const DifxDatastream *dd1, int ndd1,
	const DifxDatastream *dd2, int ndd2, int *datastreamIdRemap,
	const int *freqIdRemap, const int *antennaIdRemap, int *ndd);

/* DifxBaseline functions */
DifxBaseline *newDifxBaselineArray(int nBaseline);
void deleteDifxBaselineArray(DifxBaseline *db, int nBaseline);
void printDifxBaseline(const DifxBaseline *db);
int isSameDifxBaseline(const DifxBaseline *db1, const DifxBaseline *db2,
	const int *datastreamIdRemap);
void copyDifxBaseline(DifxBaseline *dest, const DifxBaseline *src,
	const int *datastreamIdRemap);
DifxBaseline *mergeDifxBaselineArrays(const DifxBaseline *db1, int ndb1,
	const DifxBaseline *db2, int ndb2, int *baselineIdRemap,
	const int *datastreamIdRemap, int *ndb);

/* DifxPolyco functions */
DifxPolyco *newDifxPolycoArray(int nPolyco);
void deleteDifxPolycoArray(DifxPolyco *dp, int nPolyco);
void printDifxPolycoArray(const DifxPolyco *dp, int nPolyco);
void copyDifxPolyco(DifxPolyco *dest, const DifxPolyco *src);
DifxPolyco *dupDifxPolycoArray(const DifxPolyco *src, int nPolyco);
int loadPulsarPolycoFile(DifxPolyco *dp, const char *filename);
int DifxPolycoArrayGetMaxPolyOrder(const DifxPolyco *dp, int nPolyco);

/* DifxPulsar functions */
DifxPulsar *newDifxPulsarArray(int nPulsar);
DifxPulsar *growDifxPulsarArray(DifxPulsar *dp, int origSize);
void deleteDifxPulsarArray(DifxPulsar *dp, int nPulsar);
void printDifxPulsar(const DifxPulsar *dp);
int isSameDifxPulsar(const DifxPulsar *dp1, const DifxPulsar *dp2);
DifxPulsar *dupDifxPulsarArray(const DifxPulsar *src, int nPulsar);
DifxPulsar *mergeDifxPulsarArrays(const DifxPulsar *dp1, int ndp1,
	const DifxPulsar *dp2, int ndp2, int *pulsarIdRemap, int *ndp);
int DifxPulsarArrayGetMaxPolyOrder(const DifxPulsar *dp, int nPulsar);

/* DifxConfig functions */
DifxConfig *newDifxConfigArray(int nConfig);
void deleteDifxConfigArray(DifxConfig *dc);
void printDifxConfig(const DifxConfig *dc);
int DifxConfigGetPolId(const DifxConfig *dc, char polName);
int DifxConfigRecChan2IFPol(const DifxInput *D, int configId,
	int antennaId, int recChan, int *bandId, int *polId);
int isSameDifxConfig(const DifxConfig *dc1, const DifxConfig *dc2,
	const int *baselineIdRemap, const int *datastreamIdRemap,
	const int *pulsarIdRemap);
void copyDifxConfig(DifxConfig *dest, const DifxConfig *src,
	const int *baselineIdRemap, const int *datastreamIdRemap,
	const int *pulsarIdRemap);
DifxConfig *mergeDifxConfigArrays(const DifxConfig *dc1, int ndc1,
	const DifxConfig *dc2, int ndc2, int *configIdRemap,
	const int *baselineIdRemap, const int *datastreamIdRemap,
	const int *pulsarIdRemap, int *ndc);

/* DifxModel functions */
DifxModel **newDifxModelArray(int nAntenna, int nPoint);
DifxModel *dupDifxModelColumn(const DifxModel *src, int nPoint);
void deleteDifxModelArray(DifxModel **dm, int nAntenna);
void printDifxModel(const DifxModel *dm);

/* DifxScan functions */
DifxScan *newDifxScanArray(int nScan);
void deleteDifxScanArray(DifxScan *ds, int nScan);
void printDifxScan(const DifxScan *ds);
void copyDifxScan(DifxScan *dest, const DifxScan *src,
	const int *jobIdRemap, const int *antennaIdRemap, 
	const int *configIdRemap);
DifxScan *mergeDifxScanArrays(const DifxScan *ds1, int nds1,
	const DifxScan *ds2, int nds2, const int *jobIdRemap,
	const int *antennaIdRemap, const int *configIdRemap, int *nds);

/* DifxEOP functions */
DifxEOP *newDifxEOPArray(int nEOP);
void deleteDifxEOPArray(DifxEOP *de);
void printDifxEOP(const DifxEOP *de);
void copyDifxEOP(DifxEOP *dest, const DifxEOP *src);
DifxEOP *mergeDifxEOPArrays(const DifxEOP *de1, int nde1,
	const DifxEOP *de2, int nde2, int *nde);

/* DifxSpacecraft functions */
DifxSpacecraft *newDifxSpacecraftArray(int nSpacecraft);
DifxSpacecraft *dupDifxSpacecraftArray(const DifxSpacecraft *src, int n);
void deleteDifxSpacecraft(DifxSpacecraft *ds, int nSpacecraft);
void printDifxSpacecraft(const DifxSpacecraft *ds);
DifxSpacecraft *mergeDifxSpacecraft(const DifxSpacecraft *ds1, int nds1,
	const DifxSpacecraft *ds2, int nds2, int *spacecraftIdRemap, int *nds);

/* DifxSource functions */
DifxSource *newDifxSourceArray(int nSource);
void deleteDifxSourceArray(DifxSource *ds);
void printDifxSource(const DifxSource *ds);

/* DifxIF functions */
DifxIF *newDifxIFArray(int nIF);
void deleteDifxIFArray(DifxIF *di);
void printDifxIF(const DifxIF *di);
void deleteBaselineFreq2IF(int ***map);
void printBaselineFreq2IF(int ***map, int nAnt, int nChan);
int makeBaselineFreq2IF(DifxInput *D, int configId);

/* DifxAntennaFlag functions */
DifxAntennaFlag *newDifxAntennaFlagArray(int nFlag);
void deleteDifxAntennaFlagArray(DifxAntennaFlag *df);
void printDifxAntennaFlagArray(const DifxAntennaFlag *df, int nf);
void copyDifxAntennaFlag(DifxAntennaFlag *dest, const DifxAntennaFlag *src,
	const int *antennaIdRemap);
DifxAntennaFlag *mergeDifxAntennaFlagArrays(const DifxAntennaFlag *df1, 
	int ndf1, const DifxAntennaFlag *df2, int ndf2, 
	const int *antennaIdRemap, int *ndf);

/* DifxInput functions */
DifxInput *newDifxInput();
void deleteDifxInput(DifxInput *D);
void printDifxInput(const DifxInput *D);
DifxInput *loadDifxInput(const char *filePrefix);
DifxInput *loadDifxCalc(const char *filePrefix);
DifxInput *updateDifxInput(DifxInput *D);
int areDifxInputsMergable(const DifxInput *D1, const DifxInput *D2);
int areDifxInputsCompatible(const DifxInput *D1, const DifxInput *D2);
DifxInput *mergeDifxInputs(const DifxInput *D1, const DifxInput *D2,
	int verbose);
int DifxInputGetSourceIdByJobId(const DifxInput *D, double mjd, int jobId);
int DifxInputGetSourceIdByAntennaId(const DifxInput *D, double mjd, 
	int antennaId);
int DifxInputGetScanIdByJobId(const DifxInput *D, double mjd, int jobId);
int DifxInputGetScanIdByAntennaId(const DifxInput *D, double mjd, 
	int antennaId);
int DifxInputGetAntennaId(const DifxInput *D, const char *antennaName);
int DifxInputSortAntennas(DifxInput *D, int verbose);

/* Writing functions */
int writeDifxDelay(const DifxInput *D, const char *filename);
int writeDifxUVW(const DifxInput *D, const char *filename);
int writeDifxRate(const DifxInput *D, const char *filename);
int writeDifxIM(const DifxInput *D, const char *filename);

#endif
