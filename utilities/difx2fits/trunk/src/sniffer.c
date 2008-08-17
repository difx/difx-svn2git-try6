#include <stdlib.h>
#include <string.h>
#include "sniffer.h"

typedef struct
{
	fftw_complex ***spectrum;	/* [IF][Time][Chan] */
	int a1, a2, sourceId;
	double mjdStart, mjdMax;
	int nTime, nChan;
	int nBBC;
	double *weightSum;
	int *nRec;
} Accumulator;

struct _Sniffer
{
	FILE *apd;
	FILE *wts;
	double solInt;			/* (sec) FFT interval */
	double bw;			/* (MHz) IF bandwidth */
	double deltaT;			/* (sec) grid spacing */
	double deltaF;			/* (MHz) grid spacing */
	const DifxInput *D;
	int nRec;			/* total records sniffed */
	int nPol, nStokes, nIF, nTime, nChan, nAntenna;
	int nComplex;
	int minInt;
	int configId;
	int fftOversample;
	fftw_plan plan;
	fftw_complex *fftbuffer;
	int fft_nx, fft_ny;
	Accumulator **accum;
};

void resetAccumulator(Accumulator *A)
{
	int i, t;

	for(i = 0; i < A->nBBC; i++)
	{
		for(t = 0; t < A->nTime; t++)
		{
			memset(A->spectrum[i][t], 0, 
				A->nChan*sizeof(fftw_complex));
		}
	}
	A->mjdStart = 0;
	memset(A->nRec, 0, A->nBBC*sizeof(int));
	memset(A->weightSum, 0, A->nBBC*sizeof(double));
}

Accumulator *newAccumulatorArray(Sniffer *S, int n)
{
	Accumulator *A;
	int i, t, a;
	int nBBC;

	nBBC = S->nIF*S->nPol;

	A = (Accumulator *)calloc(n, sizeof(Accumulator));
	for(a = 0; a < n; a++)
	{
		A[a].nBBC = nBBC;
		A[a].nChan = S->nChan;
		A[a].nTime = S->nTime;
		A[a].spectrum = (fftw_complex ***)malloc(
			nBBC*sizeof(fftw_complex **));
		for(i = 0; i < nBBC; i++)
		{
			A[a].spectrum[i] = (fftw_complex **)malloc(
				S->nTime*sizeof(fftw_complex *));
			for(t = 0; t < A[a].nTime; t++)
			{
				A[a].spectrum[i][t] = (fftw_complex *)calloc(
					1, S->nChan*sizeof(fftw_complex));
			}
		}
		A[a].nRec = (int *)calloc(nBBC, sizeof(int));
		A[a].weightSum = (double *)calloc(nBBC, sizeof(double));
		A[a].sourceId = -1;
	}

	return A;
}

void deleteAccumulatorArray(Accumulator *A, int n)
{
	int a, i, t;

	if(!A)
	{
		return;
	}

	for(a = 0; a < n; a++)
	{
		if(A[a].spectrum)
		{
			for(i = 0; i < A[a].nBBC; i++)
			{
				for(t = 0; t < A[a].nTime; t++)
				{
					free(A[a].spectrum[i][t]);
				}
				free(A[a].spectrum[i]);
			}
			free(A[a].spectrum);
			free(A[a].nRec);
			free(A[a].weightSum);
		}
	}

	free(A);
}

Sniffer *newSniffer(const DifxInput *D, int nComplex, 
	const char *filebase, double solInt)
{
	Sniffer *S;
	char filename[256];
	int a1, a2, i, c;
	double tMax = 0.0;

	for(c = 0; c < D->nConfig; c++)
	{
		if(D->config[c].tInt > tMax)
		{
			tMax = D->config[c].tInt;
		}
	}

	S = (Sniffer *)calloc(1, sizeof(Sniffer));

	S->deltaT = tMax;
	S->deltaF = D->chanBW;
	S->bw = D->chanBW;
	S->fftOversample = 6;

	S->nAntenna = D->nAntenna;
	S->D = D;
	S->nIF = D->nIF;
	S->nPol = D->nPol;
	S->nStokes = D->nPolar;
	S->nComplex = nComplex;
	S->configId = -1;
	S->nChan = D->nOutChan;
	S->nTime = solInt/tMax;
	S->solInt = tMax * S->nTime;
	
	/* Open fringe fit file */
	sprintf(filename, "%s.apd", filebase);
	S->apd = fopen(filename, "w");
	if(!S->apd)
	{
		fprintf(stderr, "Cannot open %s for write\n", filename);
		deleteSniffer(S);
		return 0;
	}
	fprintf(S->apd, "%s\n", D->job->obsCode);

	/* Open weights file */
	sprintf(filename, "%s.wts", filebase);
	S->wts = fopen(filename, "w");
	if(!S->wts)
	{
		fprintf(stderr, "Cannot open %s for write\n", filename);
		deleteSniffer(S);
		return 0;
	}
	fprintf(S->wts, "%s\n", D->job->obsCode);
	
	S->accum = (Accumulator **)malloc(
		S->nAntenna*sizeof(Accumulator *));
	for(a1 = 0; a1 < S->nAntenna; a1++)
	{
		S->accum[a1] = newAccumulatorArray(S, S->nAntenna);
		for(a2 = 0; a2 < S->nAntenna; a2++)
		{
			S->accum[a1][a2].a1 = a1;
			S->accum[a1][a2].a2 = a2;
		}
	}

	/* Prepare FFT stuff */
	S->fft_nx = S->fftOversample*S->nChan;
	S->fft_ny = S->fftOversample*S->nTime;
	S->fftbuffer = (fftw_complex*)fftw_malloc(S->fft_nx*S->fft_ny*
		sizeof(fftw_complex));
	S->plan = fftw_plan_dft_2d(S->fft_ny, S->fft_nx, S->fftbuffer,
		S->fftbuffer, FFTW_FORWARD, FFTW_MEASURE);

	printf("bw = %f fft=%dx%d solInt=%f\n", S->bw, S->fft_nx, S->fft_ny, S->solInt);

	return S;
}

void deleteSniffer(Sniffer *S)
{
	int a, i;
	printf("bw = %f fft=%dx%d solInt=%f\n", S->bw, S->fft_nx, S->fft_ny, S->solInt);

	if(S)
	{
		if(S->apd)
		{
			fclose(S->apd);
			S->apd = 0;
		}
		if(S->wts)
		{
			fclose(S->wts);
			S->wts = 0;
		}
		if(S->accum)
		{
			for(a = 0; a < S->nAntenna; a++)
			{
				deleteAccumulatorArray(S->accum[a], 
					S->nAntenna);
			}
			free(S->accum);
		}
		if(S->fftbuffer)
		{
			free(S->fftbuffer);
		}
		if(S->plan)
		{
			fftw_destroy_plan(S->plan);
		}
		printf("Sniffer stopping.  %d records processed\n", S->nRec);
		free(S);
	}
}

double peakup(double peak[3], int i, int n, double w)
{
	double d, f;

	if(i >= n/2)
	{
		i -= n;
	}

	d = 2.0*peak[1]-peak[0]-peak[2];
	if(d <= 0.0)
	{
		f = i;
	}
	else
	{
		f = i + (peak[2]-peak[0])/(2.0*d);
	}

	return f/w;
}

static int dump(Sniffer *S, Accumulator *A, double mjd)
{
	int bbc, i, j, besti, bestj;
	fftw_complex **array;
	fftw_complex z;
	double a2, max2, x, y;
	double amp, phase, delay, rate;
	double peak[3];
	double w;

	if(A->sourceId < 0)
	{
		return 0;
	}

	if(A->a1 == A->a2) /* Autocorrelation? */
	{
		/* weights file */
		fprintf(S->wts, "%d %f %d %s %d",
			(int)mjd, 24.0*(mjd-(int)mjd), A->a1+1,
			S->D->antenna[A->a1].name,
			A->nBBC);

		for(bbc = 0; bbc < A->nBBC; bbc++)
		{
			if(A->nRec[bbc] == 0)
			{
				w = 0.0;
			}
			else
			{
				w = A->weightSum[bbc]/A->nRec[bbc];
			}
			fprintf(S->wts, " %5.3f", w);
		}
		fprintf(S->wts, "\n");
	}

	else
	{
		/* fringe fit */

		/* FIXME -- choose refant here? */
		if(A->a1 != 4 && A->a2 != 4)
		{
			/* Note Return here! */
			return 0;
		}

		fprintf(S->apd, "%d %f %d %s %d %d %s %s %d",
			(int)mjd, 24.0*(mjd-(int)mjd), A->sourceId+1,
			S->D->source[A->sourceId].name, A->a1+1, A->a2+1,
			S->D->antenna[A->a1].name,
			S->D->antenna[A->a2].name,
			A->nBBC);

		for(bbc = 0; bbc < A->nBBC; bbc++)
		{
			if(A->nRec[bbc] < S->nTime/2 || 
			   A->weightSum[bbc] == 0.0)
			{
				fprintf(S->apd, " 0 0 0 0");
				continue;
			}
			array = A->spectrum[bbc];
			memset(S->fftbuffer, 0, 
				S->fft_nx*S->fft_ny*sizeof(fftw_complex));
			for(j = 0; j < A->nTime; j++)
			{
				for(i = 0; i < A->nChan; i++)
				{
					S->fftbuffer[j*S->fft_nx + i] = 
						array[j][i];
				}
			}
			fftw_execute(S->plan);

			max2 = 0.0;
			besti = bestj = 0;
			for(j = 0; j < S->fft_ny; j++)
			{
				for(i = 0; i < S->fft_nx; i++)
				{
					z = S->fftbuffer[j*S->fft_nx + i];
					a2 = z*~z;
					if(a2 > max2)
					{
						besti = i;
						bestj = j;
						max2 = a2;
					}
				}
			}
			z = S->fftbuffer[bestj*S->fft_nx + besti];
			phase = (180.0/M_PI)*atan2(cimag(z), creal(z));
			amp = sqrt(max2);
			peak[1] = amp;
			if(besti == 0)
			{
				z = S->fftbuffer[(bestj+1)*S->fft_nx - 1];
			}
			else
			{
				z = S->fftbuffer[bestj*S->fft_nx + besti - 1];
			}
			peak[0] = sqrt(z*~z);
			if(besti == S->fft_nx-1)
			{
				z = S->fftbuffer[bestj*S->fft_nx];
			}
			else
			{
				z = S->fftbuffer[bestj*S->fft_nx + besti + 1];
			}
			peak[2] = sqrt(z*~z);
			delay = peakup(peak, besti, 
				S->fft_nx, S->bw*S->fftOversample/1000.0);
			if(bestj == 0)
			{
				z = S->fftbuffer[(S->fft_ny-1)*S->fft_nx+besti];
			}
			else
			{
				z = S->fftbuffer[(bestj-1)*S->fft_nx + besti];
			}
			peak[0] = sqrt(z*~z);
			if(bestj == S->fft_ny-1)
			{
				z = S->fftbuffer[besti];
			}
			else
			{
				z = S->fftbuffer[(bestj+1)*S->fft_nx + besti];
			}
			peak[2] = sqrt(z*~z);
			rate = peakup(peak, bestj, 
				S->fft_ny, S->solInt*S->fftOversample);

			fprintf(S->apd, " %f %f %f %f", delay, 
				amp/A->weightSum[bbc], phase, rate);
		}
		fprintf(S->apd, "\n");
	}

	return 0;
}

static int add(Accumulator *A, int bbc, int index, float weight, 
	const float *data, int stride)
{
	fftw_complex *array;
	complex float *z;
	int c;

	array = A->spectrum[bbc][index];
	for(c = 0; c < A->nChan; c++)
	{
		z = (complex float *)(data + c*stride);
		array[c] += (*z)*weight;
	}

	A->nRec[bbc]++;
	A->weightSum[bbc] += weight;

	return A->nRec[bbc];
}

int feedSnifferFITS(Sniffer *S, const struct UVrow *data)
{
	double mjd;
	Accumulator *A;
	int a1, a2;
	int a, b, i, p;
	int configId, sourceId;
	float weight;
	int stride, offset, bbc, index;

	sourceId = data->sourceId1-1;

	configId = S->D->source[data->sourceId1-1].configId;

	if(configId != S->configId)
	{
		S->nIF = S->D->config[configId].nIF;
		S->nPol = S->D->config[configId].nPol;
		S->configId = configId;
	}

	mjd = data->jd - 2400000.5;
	mjd += data->iat;
	a1 = data->baseline/256 - 1;
	a2 = data->baseline%256 - 1;

	A = &(S->accum[a1][a2]);

	if(mjd > A->mjdMax || A->sourceId != sourceId)
	{
		dump(S, A, mjd);
		resetAccumulator(A);
		A->sourceId = sourceId;
	}
	if(A->mjdStart < 50000.0)
	{
		A->mjdStart = mjd - 0.5*S->deltaT/86400.0;
		A->mjdMax = A->mjdStart + S->solInt/86400.0;
	}

	index = (mjd - A->mjdStart)/(S->deltaT/86400.0);

	if(index < 0 || index >= A->nTime)
	{
		fprintf(stderr, "Bad index = %d\n", index);
		return -1;
	}

	stride = S->nComplex*S->nStokes;

	for(i = 0; i < S->nIF; i++)
	{
		for(p = 0; p < S->nPol; p++)
		{
			bbc = i*S->nPol + p;
			weight = data->data[p + S->nStokes*i];
			offset = S->nStokes*S->nIF + stride*S->nChan*i + p*S->nComplex;
			add(A, bbc, index, weight, data->data+offset, stride);
		}
	}

	S->nRec++;

	return S->nRec;
}
