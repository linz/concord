
/*
   $Log: fileutil.h,v $
   Revision 1.2  2004/04/22 02:35:25  ccrook
   Setting up to support linux compilation (x86 architecture)

   Revision 1.1  1995/12/22 19:01:26  CHRIS
   Initial revision

*/

#ifndef FILEUTIL_H_RCSID
#define FILEUTIL_H_RCSID "$Id: fileutil.h,v 1.2 2004/04/22 02:35:25 ccrook Exp $"
#endif
/* fileutil.h: routines to assist file management */

#ifndef _FILEUTIL_H
#define _FILEUTIL_H

int path_len( char *base, int want_name );
int file_exists( char *file );
char *build_filespec( char *spec, int nspec,
                      char *dir, char *name, char *dflt_ext );
char *find_image( char *argv0 );

#ifndef UNIX
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR2 '/'
#define DRIVE_SEPARATOR ':'
#define EXTENSION_SEPARATOR '.'
#else
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR2 '\\'
#define DRIVE_SEPARATOR '\0'
#define EXTENSION_SEPARATOR '.'
#endif

#define MAX_FILENAME_LEN 256
#define FF_TRYPROGDIR 1
#define FF_TRYHOMEDIR 2
#define FF_TRYBASEDIR 4
#define FF_TRYCURDIR  8
#define FF_TRYALL    15

void set_find_file_directories( char *progname, char *basedir, char *homeenv );
char *find_file( char *name, char *dflt_ext, int options );
char *find_file_from_base( char *base, char *name, char *dflt_ext );
FILE *snaptmpfile( void );



#endif
