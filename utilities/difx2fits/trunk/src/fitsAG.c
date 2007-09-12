#include <math.h>
#include <sys/types.h>
#include <strings.h>
#include "difx2fits.h"
#include "byteorder.h"


static double arrayGMST(int mjd)
{
    double mjd2000 = 51545.0;
    double gstmn;
    double convhs = 7.2722052166430399e-5;
    double cent;
    double daysj;
    double gmstc[4];
    double sidvel[3];
    int igstmn;
  
    gmstc[0] = 24110.548410;
    gmstc[1] = 8640184.8128660;
    gmstc[2] = 0.0931040;
    gmstc[3] = -6.2e-6;

    sidvel[0] = 1.0027379093507950;
    sidvel[1] = 5.9006e-11;
    sidvel[2] = -5.9e-15;
  
    daysj = mjd - mjd2000 + 0.5;
  
    cent = daysj / 36525;
  
    gstmn = (gmstc[0] + gmstc[1] * cent + gmstc[2] * cent * cent 
	     + gmstc[3] * cent * cent * cent) * convhs;
  
    igstmn = gstmn / (2.0*M_PI);
    gstmn = gstmn - (double)igstmn * (2.0*M_PI);
    if (gstmn < 0.0)
	gstmn += (2.0*M_PI);
  
    return gstmn / (2.0*M_PI);
}

struct __attribute__((packed)) AGrow
{
	char name[8];
	double x, y, z;
	float dx, dy, dz;
	int32_t antId;
	int32_t mountType;
	float offset[3];
};

const DifxInput *DifxInput2FitsAG(const DifxInput *D,
	struct fits_keywords *p_fits_keys, struct fitsPrivate *out)
{
	/* define the antenna geometry FITS table columns */
	static struct fitsBinTableColumn columns[] =
	{
		{"ANNAME", "8A", "station name"},
		{"STABXYZ", "3D", "station offset from array origin", "METERS"},
		{"DERXYZ", "3E", "first order derivs of STABXYZ", "M/SEC"},
		{"ORBPARM", "0D", "orbital parameters"},
		{"NOSTA", "1J", "station id number"},
		{"MNTSTA", "1J", "antenna mount type"},
		{"STAXOF", "3E", "axis offset, x, y, z", "METERS"}
	};

	char ref_date[12];
	int n_row_bytes;
	int i, a, e, mjd;
	struct AGrow row;
	int swap;

	if(D == 0)
	{
		return 0;
	}

	swap = (byteorder() == BO_LITTLE_ENDIAN);

	n_row_bytes = FitsBinTableSize(columns, NELEMENTS(columns));

	fitsWriteBinTable(out, NELEMENTS(columns), columns, n_row_bytes,
		"ARRAY_GEOMETRY");

	mjd = (int)(D->mjdStart);
	mjd2fits(mjd, ref_date);

	for(e = 0; e < D->nEOP; e++)
	{
		if(fabs(D->eop[e].mjd - mjd) < 0.01)
		{
			break;
		}
	}

	if(e >= D->nEOP)
	{
		fprintf(stderr, "EOP entry not found for mjd=%d\n", mjd);
		return 0;
	}

	fitsWriteFloat(out, "ARRAYX", 0.0, "");
	fitsWriteFloat(out, "ARRAYY", 0.0, "");
	fitsWriteFloat(out, "ARRAYZ", 0.0, "");
	fitsWriteString(out, "ARRNAM", "VLBA", "");
	fitsWriteInteger(out, "NUMORB", 0, "");
	fitsWriteString(out, "RDATE", ref_date, "");
	fitsWriteFloat(out, "FREQ", p_fits_keys->ref_freq, "");
	fitsWriteString(out, "FRAME", "GEOCENTRIC", "");
	fitsWriteString(out, "TIMSYS", "UTC", "");
	fitsWriteString(out, "TIMESYS", "UTC", "");
	fitsWriteFloat(out, "GSTIA0", 360.0*arrayGMST(mjd), "");
	fitsWriteFloat(out, "DEGPDY", 360.9856449733, "");
	fitsWriteFloat(out, "POLARX", D->eop[e].xPole, "");
	fitsWriteFloat(out, "POLARY", D->eop[e].yPole, "");
	fitsWriteFloat(out, "UT1UTC", D->eop[e].ut1_utc, "");
	fitsWriteFloat(out, "IATUTC", (double)(D->eop[e].tai_utc), "");

  	arrayWriteKeys(p_fits_keys, out);
	fitsWriteInteger(out, "TABREV", 1, "");
	fitsWriteEnd(out);

	for(a = 0; a < D->nAntenna; a++)
	{
		strcpypad(row.name, D->antenna[a].name, 8);
		row.x = D->antenna[a].X;
		row.y = D->antenna[a].Y;
		row.z = D->antenna[a].Z;
		row.dx = D->antenna[a].dX;
		row.dy = D->antenna[a].dY;
		row.dz = D->antenna[a].dZ;
		row.antId = a+1;
		if(strcasecmp(D->antenna[a].mount, "xyew") == 0)
		{
			row.mountType = 4;
		}
		else if(strcasecmp(D->antenna[a].mount, "xyns") == 0)
		{
			row.mountType = 3;
		}
		else if(strcasecmp(D->antenna[a].mount, "spac") == 0)
		{
			row.mountType = 2;
		}
		else if(strcasecmp(D->antenna[a].mount, "equa") == 0)
		{
			row.mountType = 1;
		}
		else
		{
			row.mountType = 0;
		}
		for(i = 0; i < 3; i++)
		{
			row.offset[i] = D->antenna[a].offset[i];
		}
		if(swap)
		{
			FitsBinRowByteSwap(columns, NELEMENTS(columns), &row);
		}
		fitsWriteBinRow(out, (char *)&row);
	}
	
	return D;
}
