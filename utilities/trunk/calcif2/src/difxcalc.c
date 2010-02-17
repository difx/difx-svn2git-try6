#include <stdio.h>
#include <math.h>
#include "MATHCNST.H"
#include "difxcalc.h"
#include "poly.h"

#define MAX_EOPS 5

/* FIXME check mount type azel/altz */

static struct timeval TIMEOUT = {10, 0};

struct CalcResults
{
	int nRes;	/* 3 if UVW to be calculated via tau derivatives */
	double delta;
	struct getCALC_res res[3];
};

int difxCalcInit(const DifxInput *D, CalcParams *p)
{
	struct getCALC_arg *request;
	int i;

	request = &(p->request);

	memset(request, 0, sizeof(struct getCALC_arg));

	request->request_id = 150;
	for(i = 0; i < 64; i++)
	{
		request->kflags[i] = -1;
	}
	request->ref_frame = 0;
	
	request->pressure_a = 0.0;
	request->pressure_b = 0.0;

	request->station_a = "EC";
	request->a_x = 0.0;
	request->a_y = 0.0;
	request->a_z = 0.0;
	request->axis_type_a = "altz";
	request->axis_off_a = 0.0;

	if(D->nEOP >= MAX_EOPS)
	{
		for(i = 0; i < MAX_EOPS; i++)
		{
			request->EOP_time[i] = D->eop[i].mjd;
			request->tai_utc[i]  = D->eop[i].tai_utc;
			request->ut1_utc[i]  = D->eop[i].ut1_utc;
			request->xpole[i]    = D->eop[i].xPole;
			request->ypole[i]    = D->eop[i].yPole;
		}
	}
	else
	{
		fprintf(stderr, "Not enough eop values present (%d < %d)\n", 
			D->nEOP, MAX_EOPS);
		return -1;
	}

	/* check that eops bracket the observation */
	if(D->eop[MAX_EOPS-1].mjd < D->mjdStart ||
	   D->eop[0].mjd          > D->mjdStop)
	{
		fprintf(stderr, "EOPs don't bracket the observation.\n");
		return -2;
	}

	return 0;
}

static int calcSpacecraftPosition(const DifxInput *D,
	struct getCALC_arg *request, int spacecraftId)
{
	DifxSpacecraft *sc;
	sixVector pos;
	int r;
	long double r2, d;
	double muRA, muDec;
	
	sc = D->spacecraft + spacecraftId;

	r = evaluateDifxSpacecraft(sc, request->date, request->time, &pos);
	if(r < 0)
	{
		return -1;
	}

	r2 = pos.X*pos.X + pos.Y*pos.Y;
	d = sqrtl(r2 + pos.Z*pos.Z);

	/* proper motion in radians/day */
	muRA = (pos.X*pos.dY - pos.Y*pos.dX)/r2;
	muDec = (r2*pos.dZ - pos.X*pos.Z*pos.dX - pos.Y*pos.Z*pos.dY)/
		(d*d*sqrtl(r2));
	
	/* convert to arcsec/yr */
	/* FIXME -- use formal definition of year here! */
	muRA *= (180.0*3600.0/M_PI)*365.24;
	muDec *= (180.0*3600.0/M_PI)*365.24;

	request->ra  =  atan2(pos.Y, pos.X);
	request->dec =  atan2(pos.Z, sqrtl(r2));
	request->dra  = muRA;
	request->ddec = muDec;
	request->parallax = 3.08568025e16/d;
        request->depoch = pos.mjd + pos.fracDay;

	return 0;
}

static int callCalc(struct getCALC_arg *request, struct CalcResults *results,
	const CalcParams *p)
{
	double ra, dec;
	int i;
	enum clnt_stat clnt_stat;

	if(p->delta > 0.0)
	{
		results->nRes = 3;
		results->delta = p->delta;
	}
	else
	{
		results->nRes = 1;
		results->delta = 0;
	}

	for(i = 0; i < results->nRes; i++)
	{
		memset(results->res+i, 0, sizeof(struct getCALC_res));
	}
	clnt_stat = clnt_call(p->clnt, GETCALC,
		(xdrproc_t)xdr_getCALC_arg, 
		(caddr_t)request,
		(xdrproc_t)xdr_getCALC_res, 
		(caddr_t)(results->res),
		TIMEOUT);
	if(clnt_stat != RPC_SUCCESS)
	{
		fprintf(stderr, "clnt_call failed!\n");
		return -1;
	}
	if(results->res[0].error)
	{
		fprintf(stderr,"An error occured: %s\n",
			results->res[0].getCALC_res_u.errmsg);
		return -2;
	}

	if(results->nRes == 3)
	{
		ra  = request->ra;
		dec = request->dec;

		/* calculate delay offset in RA */
		request->ra  = ra - p->delta/cos(dec);
		request->dec = dec;
		clnt_stat = clnt_call(p->clnt, GETCALC,
			(xdrproc_t)xdr_getCALC_arg, 
			(caddr_t)request,
			(xdrproc_t)xdr_getCALC_res, 
			(caddr_t)(results->res + 1),
			TIMEOUT);
		if(clnt_stat != RPC_SUCCESS)
		{
			fprintf(stderr, "clnt_call failed!\n");
			return -1;
		}
		if(results->res[1].error)
		{
			fprintf(stderr,"An error occured: %s\n",
				results->res[1].getCALC_res_u.errmsg);
			return -2;
		}

		/* calculate delay offset in Dec */
		request->ra  = ra;
		request->dec = dec + p->delta;
		clnt_stat = clnt_call(p->clnt, GETCALC,
			(xdrproc_t)xdr_getCALC_arg, 
			(caddr_t)request,
			(xdrproc_t)xdr_getCALC_res, 
			(caddr_t)(results->res + 2),
			TIMEOUT);
		if(clnt_stat != RPC_SUCCESS)
		{
			fprintf(stderr, "clnt_call failed!\n");
			return -1;
		}
		if(results->res[2].error)
		{
			fprintf(stderr,"An error occured: %s\n",
				results->res[2].getCALC_res_u.errmsg);
			return -2;
		}
		
		request->ra  = ra;
		request->dec = dec;
	}
	
	return 0;
}

int extractCalcResults(DifxPolyModel *im, int index, 
	struct CalcResults *results)
{
	struct getCALC_res *res0, *res1, *res2;
	double d, dx, dy;
	int rv=0;

	res0 = &results->res[0];
	res1 = &results->res[1];
	res2 = &results->res[2];

	
	im->delay[index] = -res0->getCALC_res_u.record.delay[0]*1e6;
	im->dry[index] = res0->getCALC_res_u.record.dry_atmos[0]*1e6;
	im->wet[index] = res0->getCALC_res_u.record.wet_atmos[0]*1e6;
	if(results->nRes == 3)
	{
		/* compute u, v, w by taking angular derivative of geometric delay */
		d =  res0->getCALC_res_u.record.delay[0] -
		     res0->getCALC_res_u.record.wet_atmos[0] -
		     res0->getCALC_res_u.record.dry_atmos[0];
		dx = res1->getCALC_res_u.record.delay[0] -
		     res1->getCALC_res_u.record.wet_atmos[0] -
		     res1->getCALC_res_u.record.dry_atmos[0];
		dy = res2->getCALC_res_u.record.delay[0] -
		     res2->getCALC_res_u.record.wet_atmos[0] -
		     res2->getCALC_res_u.record.dry_atmos[0];

		im->u[index] = (C_LIGHT/results->delta)*(d-dx);
		im->v[index] = (C_LIGHT/results->delta)*(dy-d);
		im->w[index] = C_LIGHT*d;
	
		if( isnan(d)  || isinf(d)  ||
		    isnan(dx) || isinf(dx) ||
		    isnan(dy) || isinf(dy) )
		{
			rv = 1;
		}
	}
	else
	{
		im->u[index] = res0->getCALC_res_u.record.UV[0];
		im->v[index] = res0->getCALC_res_u.record.UV[1];
		im->w[index] = res0->getCALC_res_u.record.UV[2];
		
		if(isnan(im->delay[index]) || isinf(im->delay[index]))
		{
			rv = 1;
		}
	}

	return rv;
}

void computePolyModel(DifxPolyModel *im, double deltaT)
{
	computePoly(im->delay, im->order+1, deltaT);
	computePoly(im->dry,   im->order+1, deltaT);
	computePoly(im->wet,   im->order+1, deltaT);
	computePoly(im->u,     im->order+1, deltaT);
	computePoly(im->v,     im->order+1, deltaT);
	computePoly(im->w,     im->order+1, deltaT);
}

/* antenna here is a pointer to a particular antenna object */
static int antennaCalc(int scanId, int antId, const DifxInput *D, CalcParams *p, int phasecentre)
{
	struct getCALC_arg *request;
	struct CalcResults results;
	int i, j, v;
	int mjd;
	int jobStart;
	double sec, subInc;
	double lastsec = -1000;
	DifxPolyModel **im;
	DifxScan *scan;
	DifxSource *source;
	DifxAntenna *antenna;
	DifxJob *job;
	int nInt;
	int spacecraftId = -1;
	int sourceId;
	int nError = 0;

	job = D->job;
	antenna = D->antenna + antId;
	scan = D->scan + scanId;
	im = scan->im[antId];
	nInt = scan->nPoly;
	mjd = (int)(job->mjdStart);
	jobStart = (int)(86400.0*(job->mjdStart - mjd) + 0.5);
        if(phasecentre == 0) // this is the pointing centre
		sourceId = scan->pointingCentreSrc;
	else
		sourceId = scan->phsCentreSrcs[phasecentre-1];
	source = D->source + sourceId;
	subInc = p->increment/(double)(p->order);
	request = &(p->request);
	spacecraftId = source->spacecraftId;

	request->station_b = antenna->name;
	request->b_x = antenna->X;
	request->b_y = antenna->Y;
	request->b_z = antenna->Z;
	request->axis_type_b = antenna->mount;
	request->axis_off_b = antenna->offset[0];

	request->source = source->name;
	if(spacecraftId < 0)
	{
	        request->ra       = source->ra;
	        request->dec      = source->dec;
	        request->dra      = source->pmRA;	
	        request->ddec     = source->pmDec;
	        request->parallax = source->parallax;
	        request->depoch   = source->pmEpoch;
	}
	for(i = 0; i < nInt; i++)
	{
		request->date = im[phasecentre][i].mjd;
		sec = im[phasecentre][i].sec;
		for(j = 0; j <= p->order; j++)
		{
			request->time = sec/86400.0;

			/* call calc if we didn't just for this time */
			if(fabs(lastsec - sec) > 1.0e-6)
			{
				if(spacecraftId >= 0)
				{
					v = calcSpacecraftPosition(D, 
						request, spacecraftId);
					if(v < 0)
					{
						printf("Spacecraft %d table out of time range\n", spacecraftId);
						return -1;
					}
				}
				v = callCalc(request, &results, p);
				if(v < 0)
				{
					printf("Bad: callCalc = %d\n", v);
					/* oops -- an error! */
					return -1;
				}
			}
			/* use result to populate tabulated values */

			nError += extractCalcResults(&im[phasecentre][i], j, &results);
			lastsec = sec;
			sec += subInc;
			if(sec >= 86400.0)
			{
				sec -= 86400.0;
				request->date += 1;
			}
		}
		computePolyModel(&im[phasecentre][i], subInc);
	}

	if(nError > 0)
	{
		fprintf(stderr, "Error: Antenna %s had %d invalid delays\n", D->antenna[antId].name, nError);
	}

	return nError;
}

static int scanCalc(int scanId, const DifxInput *D, CalcParams *p, int isLast)
{
	DifxPolyModel *im;
	int antId;
	int mjd, sec;
	int sec1, sec2;
	int jobStart;	/* seconds since last midnight */
	int int1, int2;	/* polynomial intervals */
	int nInt;
	int i, v, k;
	DifxJob *job;
	DifxAntenna *antenna;
	DifxScan *scan;

	job = D->job;
	antenna = D->antenna;
	scan = D->scan + scanId;

	scan->nAntenna = D->nAntenna;

	scan->im = (DifxPolyModel ***)calloc(scan->nAntenna, 
		sizeof(DifxPolyModel **));

	mjd = (int)(job->mjdStart);
	jobStart = (int)(86400.0*(job->mjdStart - mjd) + 0.5);

	sec1 = jobStart + scan->startSeconds; 
	sec2 = sec1 + scan->durSeconds;
	int1 = sec1/p->increment;
	int2 = (sec2 + p->increment - 1)/p->increment;
	nInt = int2 - int1;
	if(isLast)
	{
		nInt++;
	}
	scan->nPoly = nInt;

	for(antId = 0; antId < scan->nAntenna; antId++)
	{
		scan->im[antId] = (DifxPolyModel **)calloc(scan->nPhaseCentres+1,
				sizeof(DifxPolyModel*));
		for(k=0;k<scan->nPhaseCentres+1;k++)
		{
			scan->im[antId][k] = (DifxPolyModel *)calloc(nInt, 
				sizeof(DifxPolyModel));
			im = scan->im[antId][k];
			sec = int1*p->increment;
			mjd = (int)(job->mjdStart);
		
			for(i = 0; i < nInt; i++)
			{
				if(sec >= 86400)
				{
					sec -= 86400;
					mjd++;
				}
	
				/* set up the intervals to calc polys over */
				im[i].mjd = mjd;
				im[i].sec = sec;
				im[i].order = p->order;
				im[i].validDuration = p->increment;
				sec += p->increment;
			}

			/* call calc to derive delay, etc... polys */
			v = antennaCalc(scanId, antId, D, p, k);
			if(v < 0)
			{
				return -1;
			}
		}
	}

	return 0;
}

int difxCalc(DifxInput *D, CalcParams *p)
{
	int scanId;
	int v;
	int isLast;
	DifxScan *scan;
	DifxJob *job;

	if(!D)
	{
		return -1;
	}

	for(scanId = 0; scanId < D->nScan; scanId++)
	{
		scan = D->scan + scanId;
		job = D->job;

		job->polyOrder = p->order;
		job->polyInterval = p->increment;
		if(p->delta > 0.0)
		{
			job->aberCorr = AberCorrExact;
		}
		else
		{
			job->aberCorr = AberCorrUncorrected;
		}
		if(scan->im)
		{
			fprintf(stderr, "Error: scan %d: model already "
				"exists\n", scanId);
			return -2;
		}
		if(scanId == D->nScan - 1)
		{
			isLast = 1;
		}
		else
		{
			isLast = 0;
		}
		v = scanCalc(scanId, D, p, isLast);
		if(v < 0)
		{
			return -1;
		}
	}

	return 0;
}
