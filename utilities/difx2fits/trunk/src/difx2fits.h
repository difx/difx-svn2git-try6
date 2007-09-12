#ifndef __DIFX2FITS_H__
#define __DIFX2FITS_H__

#include <difxio/difx_input.h>
#include "fits.h"

#define array_MAX_BANDS 32

const DifxInput *DifxInput2FitsHeader(const DifxInput *D,
	struct fits_keywords *p_fits_keys, struct fitsPrivate *out);

const DifxInput *DifxInput2FitsAG(const DifxInput *D,
	struct fits_keywords *p_fits_keys, struct fitsPrivate *out);

const DifxInput *DifxInput2FitsSO(const DifxInput *D,
	struct fits_keywords *p_fits_keys, struct fitsPrivate *out);

const DifxInput *DifxInput2FitsAN(const DifxInput *D,
	struct fits_keywords *p_fits_keys, struct fitsPrivate *out);

const DifxInput *DifxInput2FitsFQ(const DifxInput *D,
	struct fits_keywords *p_fits_keys, struct fitsPrivate *out);

const DifxInput *DifxInput2FitsML(const DifxInput *D,
	struct fits_keywords *p_fits_keys, struct fitsPrivate *out);

const DifxInput *DifxInput2FitsCT(const DifxInput *D,
	struct fits_keywords *p_fits_keys, struct fitsPrivate *out);

const DifxInput *DifxInput2FitsMC(const DifxInput *D,
	struct fits_keywords *p_fits_keys, struct fitsPrivate *out);

const DifxInput *DifxInput2FitsUV(const DifxInput *D,
	struct fits_keywords *p_fits_keys, const char *filebase,
	struct fitsPrivate *out);


#endif
