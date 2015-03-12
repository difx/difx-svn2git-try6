/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

/* 2012 Jul 17  James M Anderson  This file has been heavily hand-edited,
                    as I do not have access to the original IDL file. */
/* 2015 Mar 06  JMA  edit for change to CALC_9_1_RA_Server */

#include "CALC_9_1_RA_Server.h"
#include <stdio.h>






static bool_t
xdr_getCALC_9_1_RA_arg_5_0_0(xdrs, struct_code, objp)
	register XDR *xdrs;
        long struct_code;
	getCALC_9_1_RA_arg *objp;
{
        int i;

        if(xdrs->x_op == XDR_DECODE) {
            objp->struct_code = struct_code;
            objp->request_id = struct_code;
        }
        /* for encode, struct_code already sent instead of request_id */
	if (!xdr_long(xdrs, &objp->date))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->time))
		return (FALSE);
	if (!xdr_long(xdrs, &objp->ref_frame))
		return (FALSE);
        for (i = 0; i < 64; i++)
            if (!xdr_short(xdrs, &objp->kflags[i]))
                    return (FALSE);
	if (!xdr_double(xdrs, &objp->a_x))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->a_y))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->a_z))
		return (FALSE);
        if(xdrs->x_op == XDR_DECODE) {
            objp->a_dx = 0.0;
            objp->a_dy = 0.0;
            objp->a_dz = 0.0;
            objp->a_ddx = 0.0;
            objp->a_ddy = 0.0;
            objp->a_ddz = 0.0;
        }
	if (!xdr_double(xdrs, &objp->axis_off_a))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->b_x))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->b_y))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->b_z))
		return (FALSE);
        if(xdrs->x_op == XDR_DECODE) {
            objp->b_dx = 0.0;
            objp->b_dy = 0.0;
            objp->b_dz = 0.0;
            objp->b_ddx = 0.0;
            objp->b_ddy = 0.0;
            objp->b_ddz = 0.0;
        }
	if (!xdr_double(xdrs, &objp->axis_off_b))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->ra))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->dec))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->dra))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->ddec))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->depoch))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->parallax))
		return (FALSE);
        if(xdrs->x_op == XDR_DECODE) {
            objp->source_epoch = 0.0;
            objp->source_parallax = 0.0;
            objp->pointing_epoch_a = 0.0;
            objp->pointing_epoch_b = 0.0;
            objp->pointing_parallax = 0.0;
        }
	if (!xdr_double(xdrs, &objp->pressure_a))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->pressure_b))
		return (FALSE);

	if (!xdr_string(xdrs, &objp->station_a, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, &objp->axis_type_a, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, &objp->station_b, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, &objp->axis_type_b, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, &objp->source, ~0))
		return (FALSE);

        if(xdrs->x_op == XDR_DECODE) {
            for (i = 0; i < 3; i++)
            {
                objp->source_pos[i] = 0.0;
                objp->source_vel[i] = 0.0;
                objp->pointing_pos_a_x[i] = 0.0;
                objp->pointing_vel_a_x[i] = 0.0;
                objp->pointing_pos_a_y[i] = 0.0;
                objp->pointing_vel_a_y[i] = 0.0;
                objp->pointing_pos_a_z[i] = 0.0;
                objp->pointing_vel_a_z[i] = 0.0;
                objp->pointing_pos_b_x[i] = 0.0;
                objp->pointing_vel_b_x[i] = 0.0;
                objp->pointing_pos_b_y[i] = 0.0;
                objp->pointing_vel_b_y[i] = 0.0;
                objp->pointing_pos_b_z[i] = 0.0;
                objp->pointing_vel_b_z[i] = 0.0;
            }
        }

        for (i = 0; i < 5; i++)
	{
   	   if (!xdr_double(xdrs, &objp->EOP_time[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->tai_utc[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->ut1_utc[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->xpole[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->ypole[i]))
		   return (FALSE);
        }
	return (TRUE);
}

static bool_t
xdr_getCALC_9_1_RA_arg_5_1_0(xdrs, struct_code, objp)
	register XDR *xdrs;
        long struct_code;
	getCALC_9_1_RA_arg *objp;
{
        int i;

        if(xdrs->x_op == XDR_DECODE) {
            objp->struct_code = struct_code;
        }
	if (!xdr_long(xdrs, &objp->request_id))
		return (FALSE);
	if (!xdr_long(xdrs, &objp->date))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->time))
		return (FALSE);
	if (!xdr_long(xdrs, &objp->ref_frame))
		return (FALSE);
        for (i = 0; i < 64; i++)
            if (!xdr_short(xdrs, &objp->kflags[i]))
                    return (FALSE);
	if (!xdr_double(xdrs, &objp->a_x))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->a_y))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->a_z))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->a_dx))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->a_dy))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->a_dz))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->a_ddx))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->a_ddy))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->a_ddz))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->axis_off_a))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->b_x))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->b_y))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->b_z))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->b_dx))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->b_dy))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->b_dz))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->b_ddx))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->b_ddy))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->b_ddz))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->axis_off_b))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->ra))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->dec))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->dra))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->ddec))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->depoch))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->parallax))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->source_epoch))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->source_parallax))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->pointing_epoch_a))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->pointing_epoch_b))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->pointing_parallax))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->pressure_a))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->pressure_b))
		return (FALSE);

	if (!xdr_string(xdrs, &objp->station_a, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, &objp->axis_type_a, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, &objp->station_b, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, &objp->axis_type_b, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, &objp->source, ~0))
		return (FALSE);

        for (i = 0; i < 3; i++)
	{
   	   if (!xdr_double(xdrs, &objp->source_pos[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->source_vel[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->pointing_pos_a_x[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->pointing_vel_a_x[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->pointing_pos_a_y[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->pointing_vel_a_y[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->pointing_pos_a_z[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->pointing_vel_a_z[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->pointing_pos_b_x[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->pointing_vel_b_x[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->pointing_pos_b_y[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->pointing_vel_b_y[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->pointing_pos_b_z[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->pointing_vel_b_z[i]))
		   return (FALSE);
        }

        for (i = 0; i < 5; i++)
	{
   	   if (!xdr_double(xdrs, &objp->EOP_time[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->tai_utc[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->ut1_utc[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->xpole[i]))
		   return (FALSE);
	   if (!xdr_double(xdrs, &objp->ypole[i]))
		   return (FALSE);
        }
	return (TRUE);
}









bool_t
xdr_getCALC_9_1_RA_arg(xdrs, objp)
	register XDR *xdrs;
	getCALC_9_1_RA_arg *objp;
{
    if(xdrs->x_op == XDR_DECODE) {
        long struct_code;
        if (!xdr_long(xdrs, &struct_code))
            return (FALSE);
        switch(struct_code) {
        case CALC_9_1_RA_SERVER_STRUCT_CODE_0:
        case CALC_9_1_RA_SERVER_STRUCT_CODE_5_0_0:
            return xdr_getCALC_9_1_RA_arg_5_0_0(xdrs, struct_code, objp);
        case CALC_9_1_RA_SERVER_STRUCT_CODE_5_1_0:
            return xdr_getCALC_9_1_RA_arg_5_1_0(xdrs, struct_code, objp);
        }
        return (FALSE);
    }
    else if((xdrs->x_op == XDR_ENCODE) || (xdrs->x_op == XDR_FREE)) {
        if (!xdr_long(xdrs, &(objp->struct_code)))
            return (FALSE);
        switch(objp->struct_code) {
        case CALC_9_1_RA_SERVER_STRUCT_CODE_0:
        case CALC_9_1_RA_SERVER_STRUCT_CODE_5_0_0:
            return xdr_getCALC_9_1_RA_arg_5_0_0(xdrs, objp->struct_code, objp);
        case CALC_9_1_RA_SERVER_STRUCT_CODE_5_1_0:
            return xdr_getCALC_9_1_RA_arg_5_1_0(xdrs, objp->struct_code, objp);
        }
        return (FALSE);
    }
    else {
        fprintf(stderr, "Have xdr_getCALC_9_1_RA_arg call with xdrs->x_op == %d\n", (int)xdrs->x_op);
        return (FALSE);
    }
    return (FALSE);
}

bool_t
xdr_CALC_9_1_RARecord_5_0_0(xdrs, struct_code, objp)
	register XDR *xdrs;
        long struct_code;
	CALC_9_1_RARecord *objp;
{
        int i;

        if(xdrs->x_op == XDR_DECODE) {
            objp->struct_code = struct_code;
            objp->request_id = struct_code;
        }
	if (!xdr_long(xdrs, &objp->date))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->time))
		return (FALSE);
        for (i = 0; i < 2; i++)
	{ 
            if (!xdr_double(xdrs, &objp->delay[i]))
		   return (FALSE);
	    if (!xdr_double(xdrs, &objp->UV[i]))
		   return (FALSE);
	    if (!xdr_double(xdrs, &objp->riseset[i]))
		   return (FALSE);
        }
        if (!xdr_double(xdrs, &objp->UV[2]))
	       return (FALSE);
        for (i = 0; i < 4; i++)
	{
	    if (!xdr_double(xdrs, &objp->dry_atmos[i]))
		   return (FALSE);
	    if (!xdr_double(xdrs, &objp->wet_atmos[i]))
		   return (FALSE);
	    if (!xdr_double(xdrs, &objp->el[i]))
		   return (FALSE);
	    if (!xdr_double(xdrs, &objp->az[i]))
		   return (FALSE);
        }
        if(xdrs->x_op == XDR_DECODE) {
            for (i = 0; i < 3; i++) {
                objp->baselineP2000[i] = 0.0;
                objp->baselineV2000[i] = 0.0;
                objp->baselineA2000[i] = 0.0;
            }
            for (i = 0; i < 4; i++) {
                objp->msa[i] = 0.0;
            }
        }
	return (TRUE);
}

static bool_t
xdr_CALC_9_1_RARecord_5_1_0(xdrs, struct_code, objp)
	register XDR *xdrs;
        long struct_code;
	CALC_9_1_RARecord *objp;
{
        int i;

        if(xdrs->x_op == XDR_DECODE) {
            objp->struct_code = struct_code;
        }
	if (!xdr_long(xdrs, &objp->request_id))
		return (FALSE);
	if (!xdr_long(xdrs, &objp->date))
		return (FALSE);
	if (!xdr_double(xdrs, &objp->time))
		return (FALSE);
        for (i = 0; i < 2; i++)
	{ 
            if (!xdr_double(xdrs, &objp->delay[i]))
		   return (FALSE);
	    if (!xdr_double(xdrs, &objp->riseset[i]))
		   return (FALSE);
        }
        for (i = 0; i < 3; i++)
	{
	    if (!xdr_double(xdrs, &objp->UV[i]))
		   return (FALSE);
	    if (!xdr_double(xdrs, &objp->baselineP2000[i]))
		   return (FALSE);
	    if (!xdr_double(xdrs, &objp->baselineV2000[i]))
		   return (FALSE);
	    if (!xdr_double(xdrs, &objp->baselineA2000[i]))
		   return (FALSE);
        }
        for (i = 0; i < 4; i++)
	{
	    if (!xdr_double(xdrs, &objp->dry_atmos[i]))
		   return (FALSE);
	    if (!xdr_double(xdrs, &objp->wet_atmos[i]))
		   return (FALSE);
	    if (!xdr_double(xdrs, &objp->el[i]))
		   return (FALSE);
	    if (!xdr_double(xdrs, &objp->az[i]))
		   return (FALSE);
	    if (!xdr_double(xdrs, &objp->msa[i]))
		   return (FALSE);
        }
	return (TRUE);
}

bool_t
xdr_CALC_9_1_RARecord(xdrs, objp)
	register XDR *xdrs;
	CALC_9_1_RARecord *objp;
{
    if(xdrs->x_op == XDR_DECODE) {
        long struct_code;
        if (!xdr_long(xdrs, &struct_code))
            return (FALSE);
        switch(struct_code) {
        case CALC_9_1_RA_SERVER_STRUCT_CODE_0:
        case CALC_9_1_RA_SERVER_STRUCT_CODE_5_0_0:
            return xdr_CALC_9_1_RARecord_5_0_0(xdrs, struct_code, objp);
        case CALC_9_1_RA_SERVER_STRUCT_CODE_5_1_0:
            return xdr_CALC_9_1_RARecord_5_1_0(xdrs, struct_code, objp);
        }
        return (FALSE);
    }
    else if((xdrs->x_op == XDR_ENCODE) || (xdrs->x_op == XDR_FREE)) {
        if (!xdr_long(xdrs, &(objp->struct_code)))
            return (FALSE);
        switch(objp->struct_code) {
        case CALC_9_1_RA_SERVER_STRUCT_CODE_0:
        case CALC_9_1_RA_SERVER_STRUCT_CODE_5_0_0:
            return xdr_CALC_9_1_RARecord_5_0_0(xdrs, objp->struct_code, objp);
        case CALC_9_1_RA_SERVER_STRUCT_CODE_5_1_0:
            return xdr_CALC_9_1_RARecord_5_1_0(xdrs, objp->struct_code, objp);
        }
        return (FALSE);
    }
    else {
        fprintf(stderr, "Have xdr_CALC_9_1_RARecord call with xdrs->x_op == %d\n", (int)xdrs->x_op);
        return (FALSE);
    }
    return (FALSE);
}






bool_t
xdr_getCALC_9_1_RA_res(xdrs, objp)
	register XDR *xdrs;
	getCALC_9_1_RA_res *objp;
{
	if (!xdr_int(xdrs, &objp->error))
		return (FALSE);
	switch (objp->error) {
	case 0:
		if (!xdr_CALC_9_1_RARecord(xdrs, &objp->getCALC_9_1_RA_res_u.record))
			return (FALSE);
		break;
	default:
		if (!xdr_string(xdrs, &objp->getCALC_9_1_RA_res_u.errmsg, ~0))
			return (FALSE);
		break;
	}
	return (TRUE);
}
