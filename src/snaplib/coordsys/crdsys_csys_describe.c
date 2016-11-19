#include "snapconfig.h"
/* Routines to print a description of the coordinate system to a file */

/*
   $Log: crdsysc4.c,v $
   Revision 1.2  2003/11/28 01:59:25  ccrook
   Updated to be able to use grid transformation for datum changes (ie to
   support official NZGD49-NZGD2000 conversion)

   Revision 1.1  1995/12/22 16:27:18  CHRIS
   Initial revision

*/

#include <stdio.h>
#include <math.h>

#include "util/errdef.h"
#include "coordsys/crdsys_prj.h"
#include "util/pi.h"


int  describe_ellipsoid( output_string_def *os, ellipsoid *el )
{
    char out[80];
    if( !el ) return OK;
    write_output_string( os, "Ellipsoid: ");
    write_output_string( os, el->name );
    sprintf(out,"\n  a = %.3lf  1/f = %.6lf\n",el->a,el->rf);
    write_output_string( os, out );
    return OK;
}

int  describe_ref_frame( output_string_def *os, ref_frame *rf )
{
    char translation;
    char scale;
    char rotation;
    const char *iers_components[]= {"Translation","IERS rotation","Scale"};
    const char *std_components[]= {"Translation","Rotation","Scale"};
    const char *ratestr="       rate";
    const char *iers_units[]= {"mm","mas","ppb"};
    const char *std_units[]= {"m","sec","ppm"};

    write_output_string( os, "Reference frame: " );
    write_output_string( os, rf->name );
    write_output_string( os, "\n" );

    scale = rf->scale != 0.0 || rf->dscale;

    translation = rf->txyz[0] != 0.0 ||
                  rf->txyz[1] != 0.0 ||
                  rf->txyz[2] != 0.0 ||
                  rf->dtxyz[0] != 0.0 ||
                  rf->dtxyz[1] != 0.0 ||
                  rf->dtxyz[2] != 0.0;

    rotation    = rf->rxyz[0] != 0.0 ||
                  rf->rxyz[1] != 0.0 ||
                  rf->rxyz[2] != 0.0 ||
                  rf->drxyz[0] != 0.0 ||
                  rf->drxyz[1] != 0.0 ||
                  rf->drxyz[2] != 0.0;

    if( rf->refcode && (scale || translation || rotation ))
    {
        char out[256];
        char unitstr[20];
        double uf=rf->use_iersunits ? 1000.0 : 1.0;
        double af=rf->use_iersunits ? -1000.0 : 1.0;
        const char **components=rf->use_iersunits ? iers_components : std_components;
        const char **units=rf->use_iersunits ? iers_units : std_units;
        const char *format1="    %-14s %8s  %10.5lf %10.5lf %10.5lf\n";
        const char *format2="    %-14s %8s  %10.5lf\n";
        const char *format3="    %-14s %8s  %7.2lf\n";
        int rates = rf->use_rates;

        write_output_string( os, "  Relative to ");
        write_output_string( os, rf->refcode);
        write_output_string( os, "\n" );
        if( rates )
        {
            sprintf(out,format3,"    Reference date","(year)",rf->refdate);
            write_output_string( os, out );
        }
        if( translation )
        {
            sprintf(unitstr,"(%s)",units[0]);
            sprintf(out,format1,components[0],unitstr,
                    uf*rf->txyz[0], uf*rf->txyz[1], uf*rf->txyz[2] );
            write_output_string( os, out );
            if( rates )
            {
                sprintf(unitstr,"(%s/yr)",units[0]);
                sprintf(out,format1,ratestr,unitstr,
                        uf*rf->dtxyz[0], uf*rf->dtxyz[1], uf*rf->dtxyz[2] );
                write_output_string( os, out );
            }
        }
        if( rotation )
        {
            sprintf(unitstr,"(%s)",units[1]);
            sprintf(out,format1,components[1],unitstr,
                    af*rf->rxyz[0], af*rf->rxyz[1], af*rf->rxyz[2] );
            write_output_string( os, out );
            if( rates )
            {
                sprintf(unitstr,"(%s/yr)",units[1]);
                sprintf(out,format1,ratestr,unitstr,
                        af*rf->drxyz[0], af*rf->drxyz[1], af*rf->drxyz[2] );
                write_output_string( os, out );
            }
        }
        if( scale )
        {
            sprintf(unitstr,"(%s)",units[2]);
            sprintf(out,format2,components[2],unitstr,uf*rf->scale);
            write_output_string( os, out );
            if( rates )
            {
                sprintf(unitstr,"(%s/yr)",units[2]);
                sprintf(out,format2,ratestr,unitstr,uf*rf->dscale);
                write_output_string( os, out );
            }
        }
    }
    if( rf->func && rf->func->describe_func ) (*(rf->func->describe_func))(rf, os);
    if( rf->def && rf->def->describe_func ) (*(rf->def->describe_func))(rf, os);
    describe_ellipsoid( os, rf->el );
    return OK;
}

int describe_projection( output_string_def *os, projection *prj )
{
    if( !prj ) return OK;
    if( prj->type->name )
    {
        write_output_string( os, "Projection: " );
        write_output_string( os, prj->type->name);
        write_output_string( os, "\n" );
    }

    if( prj->type->nparams )
    {
        print_param_list( os, prj->type->params, prj->type->nparams,
                          prj->data, "  ");
    }
    return OK;
}

int describe_vdatum( output_string_def *os, vdatum *hrs )
{
    if( !hrs ) return OK;
    if( hrs->name )
    {
        write_output_string( os, "Heights: " );
        write_output_string( os, hrs->name);
        write_output_string( os, "\n" );
    }
    return OK;
}

int  describe_coordsys(  output_string_def *os, coordsys *cs )
{
    write_output_string( os, "Coordinate system: " );
    write_output_string( os, cs->name );
    write_output_string( os, "\n" );
    describe_ref_frame( os, cs->rf );
    if( has_deformation_model(cs) && cs->rf->defepoch != 0.0)
    {
        char buf[32];
        write_output_string(os,"Calculated at epoch ");
        sprintf(buf,"%.1lf\n",cs->rf->defepoch);
        write_output_string(os,buf);
    }
    describe_projection( os, cs->prj );
    describe_vdatum( os, cs->hrs );
    return OK;
}

int  describe_deformation_model( output_string_def *os, ref_frame *rf )
{
    if( rf->def )
    {
        return rf->def->describe_func( rf, os );
    }
    return OK;
}

