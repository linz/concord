#ifndef LAMBERTR_H
#define LAMBERTR_H

/*
   $Log: lambertr.h,v $
   Revision 1.1  1995/12/22 16:56:33  CHRIS
   Initial revision

*/

void register_lcc_projection( void );
projection *create_lcc_projection(  double sp1, double sp2,
                                    double lt0, double ln0, double e, double n );

#endif
