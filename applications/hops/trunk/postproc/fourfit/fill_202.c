/************************************************************************/
/*                                                                      */
/*  Fills in a type_202 record, based on root information               */
/*                                                                      */
/*      Inputs:         root        scan_info struct                    */
/*                      param       lots of parameters                  */
/*                                                                      */
/*      Output:         t202        Filled in type_202 record           */
/*                                                                      */
/* Created 31 August 1999 by CJL                                        */
/*                                                                      */
/************************************************************************/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "vex.h"
#include "mk4_data.h"
#include "param_struct.h"

int
fill_202 (/* root, param, t202) */
struct vex *root,
struct type_param *param,
struct type_202 *t202)
    {
    char refst, remst;
    int i;
    double refepoch, remepoch, frt, refdiff, remdiff, time_to_double();
    struct station_struct *ref, *rem;
    struct station_log *lref, *lrem;
    struct date tempdate;

    clear_202 (t202);

    strncpy (t202->baseline, param->baseline, 2);
                                        /* Get station structs from root */
    refst = param->baseline[0];
    remst = param->baseline[1];
    ref = rem = NULL;
    for (i=0; i<root->ovex->nst; i++)
        {
        if (root->ovex->st[i].mk4_site_id == refst) ref = root->ovex->st + i;
        if (root->ovex->st[i].mk4_site_id == remst) rem = root->ovex->st + i;
        }
    if ((ref == NULL) || (rem == NULL))
        {
        if (ref == NULL)
            msg ("Failed to find station '%c' in ovex file", 2, refst);
        else
            msg ("Failed to find station '%c' in ovex file", 2, remst);
        return (-1);
        }
    strncpy (t202->ref_intl_id, ref->site_id, 2);
    strncpy (t202->rem_intl_id, rem->site_id, 2);
    strncpy (t202->ref_name, ref->site_name, 8);
    strncpy (t202->rem_name, rem->site_name, 8);
    t202->nlags = param->nlags;
    t202->ref_xpos = ref->coordinates[0];
    t202->rem_xpos = rem->coordinates[0];
    t202->ref_ypos = ref->coordinates[1];
    t202->rem_ypos = rem->coordinates[1];
    t202->ref_zpos = ref->coordinates[2];
    t202->rem_zpos = rem->coordinates[2];
                                        /* Baseline u,v,derivs here */

                                        /* Fourfit ref time is relative to start of year */
                                        /* So need to convert to secs since 1980 */
    tempdate.year = root->ovex->start_time.year;
    tempdate.day = 1;
    tempdate.hour = 0;
    tempdate.minute = 0;
    tempdate.second = 0.0;
    frt = time_to_double (tempdate) + param->reftime;
                                        /* Ref station clockrate ref time */
    if (ref->clockrate != 0.0)
        refepoch = time_to_double (ref->clockrate_epoch);
    else refepoch = frt;
    refdiff = frt - refepoch;
    if (fabs (refdiff) > 3.0e5)
        {
        msg ("Warning, ref station clockrate epoch highly discrepant from FRT", 2);
	msg ("frt = %12.2f, ref epoch = %12.2f", 2, frt, refepoch);
	}
                                        /* Rem station clockrate ref time */
    if (rem->clockrate != 0.0)
        remepoch = time_to_double (rem->clockrate_epoch);
    else remepoch = frt;
    remdiff = frt - remepoch;
    if (fabs (remdiff) > 3.0e5)
	{
        msg ("Warning, rem station clockrate epoch highly discrepant from FRT", 2);
	msg ("frt = %12.2f, rem epoch = %12.2f, diff = %12.2f", 2, frt, remepoch, fabs (remdiff));
	}
                                        /* Adjust clocks to frt for clockrate */
    t202->ref_clock = (ref->clock_early + (refdiff * ref->clockrate)) * 1.0e6;
    t202->rem_clock = (rem->clock_early + (remdiff * rem->clockrate)) * 1.0e6;
    t202->ref_clockrate = ref->clockrate;
    t202->rem_clockrate = rem->clockrate;
                                        /* Instrumental delays(?) here */

    t202->ref_zdelay = ref->zenith_atm * 1.0e6;
    t202->rem_zdelay = rem->zenith_atm * 1.0e6;
                                        /* elevations/azimuths here */

                                        /* Now find stations in lvex for VSNs */
    for (i=0; i<root->lvex->nstation; i++)
        {
        if (root->lvex->stn[i].station == refst)
            strncpy (t202->ref_tape, root->lvex->stn[i].vsn, 8);
        if (root->lvex->stn[i].station == remst)
            strncpy (t202->rem_tape, root->lvex->stn[i].vsn, 8);
        }
    return (0);
    }

