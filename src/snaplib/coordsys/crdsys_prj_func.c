#include "snapconfig.h"
/*
   $Log: crdsysp2.c,v $
   Revision 1.1  1995/12/22 16:40:21  CHRIS
   Initial revision

*/

#include <stdio.h>
#include "coordsys/crdsys_prj.h"
#include "util/errdef.h"

/* Routines for convert to and from coordinate system projections */

/* Projection to and from geodetic coordinates                 */

int proj_to_geog( projection *prj, double e, double n, double *lon, double *lat )
{
    if( !prj || !prj->type || !prj->type->proj_to_geog  ) return INVALID_DATA;
    return (*prj->type->proj_to_geog)( prj->data, e, n, lon, lat );
}

int geog_to_proj( projection *prj, double lon, double lat, double *e, double *n )
{
    if( !prj || !prj->type || !prj->type->geog_to_proj) return INVALID_DATA;
    return (*prj->type->geog_to_proj)( prj->data, lon, lat, e, n );
}

