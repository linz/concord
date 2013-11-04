#include "snapconfig.h"
/* Program concord - front end to coordinate conversion routines */
/* Version 2.0: Uses SNAP coordinate conversion routines....     */

#define PROGNAME "concord"
#define VERSION  "3.4"
#define PROGDATE __DATE__

/* Revision history:
   1.1) Changes code for latitude and longitude from GEOD to GEOG
        19/3/90

   1.2) Fixed reading of DMS to permit hemisphere in front of angle.

   1.3) Added user specified delimiter to command line options.

   1.4) Added DXF option

   1.5) Improved ease of use from keyboard - more descriptive of
        how data are to be entered.  Default if just concord is
        entered is now to do manual conversion.

   2.0) August 1995: Almost completely different!  Uses SNAP coordsys
        routines.

   2.1) May 1996: Added support for orthometric height using geoid model
        and for specifying coordsys and geoid files on the command line.
        */

/*
   $Log: concord2.c,v $
   Revision 1.12  2004/04/22 02:35:49  ccrook
   Setting up to support linux compilation (x86 architecture)

   Revision 1.11  2003/11/28 02:06:32  ccrook
   Updated to reflect changes to concord (grid based NZGD2000-NZGD49 transformation)

   Revision 1.10  1999/09/14 07:20:46  ccrook
   Updated version number

   Revision 1.9  1999/09/14 07:16:37  ccrook
   Modified default precision of heights, and fixed up missing separator
   after last ordinate.

   Revision 1.8  1999/05/20 10:59:34  ccrook
   Changed DOSLI to LINZ

   Revision 1.7  1998/02/16 07:50:43  CHRIS
   Added support for DDD MM.mmmm angle format

   Revision 1.6  1996/05/28 16:34:33  CHRIS
   Further minor fixes: last two checkins were too hasty!

   Revision 1.5  1996/05/28 16:06:44  CHRIS
   further minor fixes

   Revision 1.4  1996/05/28 16:01:29  CHRIS
   Minor fixes.  Changed to use -H insteak of -? for help (easier in unix)
   and supported "-" for input and output file names equivalent to stdin
   and stdout

   Revision 1.3  1996/05/21 20:56:22  CHRIS
   Fixed up handling of missing parameters and missing coordinate systems.

   Revision 1.2  1996/05/20 17:04:00  CHRIS
   Added support for orthometric heights, specifying coordsys.def and geoid.bin
   on the command line, and gcc compilation.

   Revision 1.1  1996/01/03 22:42:55  CHRIS
   Initial revision

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#if defined( __BORLANDC__ )
#include <conio.h>
#include <io.h>
#elif defined( _MSC_VER )
#include <conio.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include "coordsys/coordsys.h"
#include "geoid/geoid.h"
#include "util/fileutil.h"
#include "util/license.h"
#include "util/errdef.h"
#include "util/pi.h"

#define TRUE 1
#define FALSE 0

static char rcsid[]="$Id: concord2.c,v 1.12 2004/04/22 02:35:49 ccrook Exp $";

static FILE *crdin;
static FILE *crdout;
static FILE *crdcom;

static char *coordsys_file;
static char *geoid_file;

static char *crdin_fname;
static char *crdout_fname;
static char crdin_open;
static char crdout_open;

static char ask_params;        /* Prompt for program parameters?   */
static char ask_coords;        /* Prompt for coordinates           */
static char skip_errors;       /* True if errors are to be skipped */
static char verbose;           /* Verbose output of coordinates    */
static char point_ids;         /* Points have an id code           */

static int  id_length;         /* Number of columns allocated for output of point ids */

static coordsys *input_cs;     /* Input coordinate system */
static char input_ne;          /* Input order is N,E ? */
static char input_dms;         /* Input is to be converted from dms */
static char input_h;           /* Input heights */
static char input_ortho;       /* Input heights are orthometric */
static char input_latlong;
static int  input_prec;
static int  input_vprec;

static coordsys *output_cs;     /* Output coordinate system */
static char output_ne;          /* Output order is N,E ? */
static char output_dms;         /* Output is to be converted to dms */
static char output_h;           /* Output heights */
static char output_ortho;       /* Output heights are orthometric */
static char output_latlong;
static int  output_prec;        /* precision of output units */
static int  output_vprec;

static geoid_def *geoiddef = NULL; /* Geoid model */

/* Angle formats used for input_dms and output_dms */

#define AF_DEG 0
#define AF_DM  1
#define AF_DMS 2

static char transform_heights;

static coordsys *geoid_coordsys;

static double conv_epoch = 0;
static coord_conversion cnv;
static coord_conversion ingcnv;
static coord_conversion outgcnv;

/* Latitude and longitude hemisphere indicators */

static char *ns_string = "NS";
static char *ew_string = "EW";

/* Input and output coordinates */

static double inxyz[3];
static double outxyz[3];

/* Pointers to input and output coordinates and latitude strings for the
   first and second coordinates of input and output */

static double *in1, *in2, *in3;
static double *out1, *out2, *out3;
static char *indms1, *indms2;
static char *outdms1, *outdms2;

/* Input (echoed) and output field lengths */

static int infldlen, outfldlen;
static int invfldlen, outvfldlen;

/* Field separator - default is 0, implies tab or blank is delimiter */

#define DEFAULT_SEPARATOR ( (char) 0 )
static char separator = DEFAULT_SEPARATOR;

/* Pointer to string allocated for point id */

static char *id;

/* Character string for prompt for coordinates */

static char coord_prompt[80];
static int prompt_length;

/* Counters */

static int ncrdcnv;
static int ncrderr;

/* If no_seconds is set then the format holds degrees and decimal minutes -
   but with the minutes in the last seconds field! */

typedef struct
{
    int degrees;
    int minutes;
    double seconds;
    char neg;
    char no_seconds;
} DMS;

static void clear_screen(void)
{
#ifdef __BORLANDC__
    clrscr();
#else
    printf("\n\n=====================================================================\n");
#endif
}

/*------------------------------------------------------------------*/
/*  Angle format conversion routines - DMS to radians and           */
/*  vice-versa.                                                     */
/*                                                                  */
/*  dms_deg     Converts degrees, minutes, seconds to degrees       */
/*                                                                  */
/*  deg_dms     Converts degrees to degrees, minutes, seconds       */
/*              Can take an arguement prec, which defines the       */
/*              number of decimal places used for the output of     */
/*              seconds.  This is used to ensure numbers do not     */
/*              round to 60.0 seconds. The precision is ignored     */
/*              if less than 0 or greater than 10.                  */
/*                                                                  */
/*------------------------------------------------------------------*/

static double dms_deg( DMS dms )
{
    double deg;
    deg = dms.seconds/60.0;
    if( ! dms.no_seconds ) deg = (dms.minutes+deg)/60.0;
    deg += dms.degrees;
    if( dms.neg ) deg = -deg;
    return deg;
}

static DMS *deg_dms( double deg, DMS *dms, int prec, char no_seconds )
{
    double offset;
    if( prec < 0 || prec > 10 ) prec = 5;
    offset = 0.49;
    while (prec--) offset /= 10.0;

    if (dms->neg = deg<0.0) deg = -deg;
    deg += offset/3600.0;
    if( no_seconds ) deg += offset/60.0; else deg += offset/3600.0;
    deg -= (dms->degrees = floor(deg));
    if( ! no_seconds )
    {
        deg *= 60.0;
        deg -= (dms->minutes = floor(deg));
    }
    dms->seconds = deg*60.0 - offset;
    dms->no_seconds = no_seconds;
    return dms;
}

/*-------------------------------------------------------------------*/
/*                                                                   */
/*  read_string:  Reads a string from the specified input file.      */
/*                Leading tabs and spaces are discarded, and         */
/*                input terminates at a new line or EOF, or space    */
/*                character after the the string.  Only the first    */
/*                nch characters are copied to the string, the       */
/*                rest being discarded.  If the string is            */
/*                terminated by a newline, that character is         */
/*                restored to the input buffer. Returns the number   */
/*                of characters placed in the string, or -1 if       */
/*                EOF encountered                                    */
/*                                                                   */
/*-------------------------------------------------------------------*/


static int read_string( FILE *input, char separator, char *string, int nch )
{
    int i,c;

    /* Skip over leading spaces and tabs, to first character not one
       of these */

    while ((c=getc(input)) == ' ' || c=='\t');

    /* Read characters up to next space, storing first nch in string */

    i = 0;
    for(;;)
    {
        if( separator )
        {
            if( c==separator ) break;
        }
        else
        {
            if(c == ' ' || c == '\t' ) break;
        }
        if( c=='\r' || c == '\n' || c == EOF) break;
        if (i<nch) {*string++ = c; i++;}
        c = getc(input);
    }

    /* terminate string */

    *string = '\0';

    /* If terminating character was newline, restore to the input buffer */

    if (c=='\n') ungetc( c, input );

    /* Determine the return status */

    if (i==0 && c==EOF) i--;
    return i;
}



/*-------------------------------------------------------------------*/
/*                                                                   */
/*   copy_to_newline - copies input to output until a newline        */
/*                     character has been copied, or until the       */
/*                     end of the file is reached.  If the output    */
/*                     file pointer is NULL, simply discards input   */
/*                     until the newline has been read.              */
/*                     Returns '\n' or EOF.                          */
/*                                                                   */
/*-------------------------------------------------------------------*/

static int copy_to_newline( FILE *input, FILE *output, char *prefix )
{
    int c;
    do
    {
        c = getc(input);
        if (c==EOF) break;
        if (output)
        {
			if( c == '\r' ) continue;
            if( prefix && c != '\n' )
            {
                fputs( prefix, output );
                prefix = NULL;
            }
            putc( c, output );
        }
    }
    while (c != '\n');
    return c;
}


/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Initiallize concord parameters                                   */
/*                                                                   */
/*-------------------------------------------------------------------*/

static void concord_init( void )
{
    crdin = stdin;
    crdout = stdout;
    crdin_fname = NULL;
    crdout_fname = NULL;
    coordsys_file = NULL;
    geoid_file = NULL;
    crdin_open = FALSE;
    crdout_open = FALSE;
    ask_params = FALSE;
    ask_coords = FALSE;
    skip_errors = FALSE;
    verbose = FALSE;
    point_ids = FALSE;
    id_length = 10;

    input_cs = NULL;
    input_ne = FALSE;
    input_dms = AF_DEG;
    input_h = FALSE;
    input_ortho = FALSE;

    output_cs = NULL;
    output_ne = FALSE;
    output_dms = AF_DEG;
    output_h = FALSE;
    output_ortho = FALSE;

    transform_heights = FALSE;

    input_prec = -1;
    input_vprec = -1;
    output_prec = 3;
    output_vprec = -1;
}

/*-------------------------------------------------------------------*/
/*                                                                   */
/*   Summarise the concord command syntax if no command line         */
/*   parameters are entered.                                         */
/*                                                                   */
/*-------------------------------------------------------------------*/

static int pause_output( void )
{
    int c;
    int domore;
    return 1;
    /*
     * No longer interested in pausing output - almost any terminal window
     * these days will be scrollable...
     */
    /*
    if( ! _isatty(_fileno(stdout)) || ! _isatty(_fileno(stdin)) ) return 1;
    printf("Press return to continue: ");
    domore = 1;
    do
    {
        c = getchar();
        if( ! isspace(c) ) domore = 0;
    }
    while( c != '\n' && c != EOF );
    return domore;
    */
}

static void help( void )
{
    puts("");
    printf("%s - Converts coordinates between different coordinate systems\n",
           PROGNAME);
    puts("");
    printf("Syntax:  %s [switches] [input_file] [output_file]\n",PROGNAME);
    puts("");
    puts("Switches:  -A       Ask for program parameters");
    puts("           -K       Ask for coordinates at the keyboard");
    puts("           -L       List the coord system codes used by the program");
    puts("           -Ixxxx   Define input coord system, and order - e.g.NZMG:EN");
    puts("           -Oxxxx   Define output coord system, and order");
    puts("           -Nn      Specifies coordinates preceded by up to n character id");
    puts("           -Pn      Define the precision of output coordinates");
    puts("           -Sc      Defines input and output field separator is c");
    puts("           -E       Skip over input format errors");
    puts("           -Cfile   Specify the coordinate system definition file");
    puts("           -Gfile   Specify the geoid file");
    puts("           -Yyyyy   Specify the conversion epoch for conversions");
    puts("           -H       List this help information");
    puts("           -Z       List the program version");
    puts("");
    pause_output();
    puts("The -I and -O switches are required to specify the input and output");
    puts("coordinate systems.  These specify the coordinate system code. Use");
    puts("concord -L for a list of valid codes\n");
    puts("For non-geocentric coordinate systems the code may be followed by a");
    puts("colon and the order of the coordinates (one of EN, NE, ENH, NEH, ENO,");
    puts("NEO.  Here E and N are the east and north coordinates, H is ellipsoidal");
    puts("heights or O for orthometric heights.\n");
    puts("For lat/long systems this can be followed by \",H\", \",M\", or \",D\" for");
    puts("for degrees minutes and seconds, degrees and minutes or");
    puts("decimal degrees respectively\n");
    puts("The conversion epoch is the date at which conversions apply. It affects");
    puts("coordinate systems with a deformation model, and transformations between");
    puts("datums with a time dependent terms (eg between different ITRS realisations).");
    puts("The epoch can be entered as a decimal year, as YYYYMMDD, or \"now\" to use");
    puts("the current date.");
    puts("For coordinate systems such as NZGD2000 which include a deformation model");
    puts("the coordinate system code used in the -I and -O parameters can include a");
    puts("date, for example NZGD2000@20120512 defines coordinates in terms of the");
    puts("deformation model on 13 May 2012 rather than using the NZGD2000");
    puts("reference coordinates.");
}

/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Error exit - prints an error message and exits                   */
/*                                                                   */
/*-------------------------------------------------------------------*/

static void error_exit( char *errmsg1, char * errmsg2 )
{
    printf("%s: %s%s\n\n",PROGNAME,errmsg1,errmsg2);
    printf("Enter %s -H for a list of command line parameters\n", PROGNAME);
    exit(1);
}

/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Routines for interpreting the command line parameters.           */
/*    decode_proj_string - decodes a projection string (-I and -O)   */
/*    decode_number      - decodes an integer number                 */
/*    process_command_line - does everything!!                       */
/*                                                                   */
/*-------------------------------------------------------------------*/

static void decode_proj_string( char *code, coordsys **proj,
                                char *dms, char *orderne, char *gothgt, char *ortho, char *iostring )
{
    char *s;
    char errmsg[80];

    s = strtok(code,",:");
    if (s==NULL) return;
    if (*s)
    {
        if ( (*proj=load_coordsys(s)) == NULL )
        {
            sprintf(errmsg,"Invalid coordinate code %s for %s coordinates",s,iostring);
            error_exit(errmsg,"");
        }
    }

    *dms=FALSE;
    if( is_geodetic(*proj)) *dms=AF_DMS;
    s = strtok(NULL,",:");
    if (s==NULL) return;
    if (*s)
    {
        _strupr(s);
        if( strcmp(s,"EN") == 0 ) {*orderne = FALSE; *gothgt = FALSE; *ortho = FALSE; }
        else if( strcmp(s,"ENH") == 0 ) {*orderne = FALSE; *gothgt = TRUE; *ortho = FALSE; }
        else if( strcmp(s,"ENO") == 0 ) {*orderne = FALSE; *gothgt = TRUE; *ortho = TRUE; }
        else if( strcmp(s,"NE") == 0 ) {*orderne = TRUE; *gothgt = FALSE; *ortho = FALSE; }
        else if( strcmp(s,"NEH") == 0 ) {*orderne = TRUE; *gothgt = TRUE; *ortho = FALSE; }
        else if( strcmp(s,"NEO") == 0 ) {*orderne = TRUE; *gothgt = TRUE; *ortho = TRUE; }
        else
        {
            sprintf(errmsg,"Invalid order code %s for %s coordinates",s,iostring);
            error_exit(errmsg,"");
        }
    }

    if( *dms )
    {
        s = strtok(NULL,":,");
        if (s==NULL || *s=='h' || *s=='H' || *s == 's' || *s == 'S' ) return;
        if( *s=='m' || *s=='M') { *dms = AF_DM; return;}
        if( *s=='d' || *s=='D') { *dms = AF_DEG; return;}
        sprintf(errmsg,"Invalid angle type  %s for %s coordinates",s,iostring);
        error_exit(errmsg,"");
    }

    s = strtok(NULL,"");
    if (s != NULL)
    {
        sprintf(errmsg,"Invalid extra data in %s coordinate definition",iostring);
        error_exit(errmsg,"");
    }
}

/*-------------------------------------------------------------------*/

static int decode_number( char *s, int min, int max, char *type )
{
    char checkend;
    int value;
    value = 0;
    if (!s || sscanf(s,"%d%c",&value,&checkend) != 1 || value<min || value>max)
        error_exit("Invalid data in ",type);
    return value;
}

/*-------------------------------------------------------------------*/

#pragma warning (disable : 4100)

static int printf_func( const char *s, void *dummy )
{
    printf("%s",s);
    return 0;
}

static void list_circuits_with_pause( void )
{
    int maxi;
    int i;
    int nl;
    maxi = coordsys_list_count();
    nl = 0;
    for( i = 0; i < maxi; i++ )
    {
        if( nl == 20 )
        {
            if( pause_output()) nl = 0; else break;
        }
        nl++;
        printf("  %-10s %s\n",coordsys_list_code(i), coordsys_list_desc(i));
    }
    printf("\n");
}

static void list_circuits_and_exit( int argc, char *argv[] )
{
    int ncs = 0;
    if( argc )
    {
        int i;
        coordsys *cs;
        output_string_def os;
        os.write = printf_func;
        for( i = 0; i < argc; i++ )
        {
            ncs++;
            cs = load_coordsys( argv[i] );
            if( ! cs ) continue;
            printf("\n");
            describe_coordsys( &os, cs );
            delete_coordsys( cs );
        }
    }
    if( ncs == 0 )
    {
        printf("\n%s: Valid coordinate systems are:\n",PROGNAME);
        list_circuits_with_pause();
    }
    exit(1);
}

static void list_program_details_and_exit( void )
{
    printf("\nProgram %s version %s, dated %s\n",PROGNAME,VERSION,PROGDATE);
    printf("Copyright: Land Information New Zealand\n");
    printf("Author: Chris Crook\n");
    /* printf("Licensed to: %s\n",decrypted_license()); */
    exit(1);
}

/*-------------------------------------------------------------------*/

static void get_definition_files( int argc, char *argv[] )
{
    char *s;
    while (--argc)
    {
        s = *++argv;
        if( _strnicmp(s,"-C",2) == 0 ) coordsys_file = s+2;
        else if( _strnicmp(s,"-G",2) == 0 ) geoid_file = s+2;
    }
}

static void process_command_line( int argc, char *argv[] )
{
    char *s;
    char c;
    int narg;

    narg = 0;
    while (--argc)
    {
        narg++;
        s = *++argv;
        if (s[0]=='-' && s[1])
        {
            c= *++s;
            if( islower(c) ) c = toupper(c);
            ++s;
            switch ( c )
            {
            case 'A': { ask_params = TRUE; break; }
            case 'C': { narg--; break; } /* Coord sys def file */
            case 'E': { skip_errors = TRUE; break; }
            case 'G': { narg--; break; } /* Geoid file definition */
            case 'I':
            {
                decode_proj_string(s,&input_cs,&input_dms,
                                   &input_ne,&input_h,&input_ortho,"input"); break;
            }
            case 'K': { ask_coords = TRUE; break; }
            case 'L': { list_circuits_and_exit( argc-1, argv+1 ); break; }
            case 'O':
            {
                decode_proj_string(s,&output_cs,&output_dms,
                                   &output_ne,&output_h,&output_ortho, "output"); break;
            }
            case 'N':
            {
                point_ids = TRUE;
                if (*s != '\0')
                    id_length = decode_number(s,0,80,"point id length");
                break;
            }
            case 'P':
            {
                char *s1, *s2;
                s1 = strtok(s,",");
                s2 = strtok(NULL,"");
                output_prec = decode_number(s1,0,20,"output precision");
                if( s2 )
                    output_vprec = decode_number(s2,0,20,"output precision");
                break;
            }
            case 'S': { separator = *s; break; }
            case 'V': { verbose = TRUE; break; }
            case 'Y':
            {
                if( ! parse_crdsys_epoch(s,&conv_epoch) )
                    error_exit("Invalid value for conversion epoch (-Y parameter)","");
                break;
            }
            case 'Z': { list_program_details_and_exit(); break;}
            case 'H':
            case '?': { help(); exit(0); break; }
            default : error_exit("Invalid switch ",--s);
            }
        }
        else
        {
            if (crdout_fname != NULL)
                error_exit("Too many parameters on command line","");
            else
            {
                if (crdin_fname==NULL) crdin_fname = s;
                else crdout_fname = s;
            }
        }
    }

    if( ! narg ) ask_params = ask_coords = TRUE;

    /* If asking for coordinates then there cannot be an input file */

    if (ask_coords)
    {
        if (crdout_fname) error_exit("Cannot have an input file and keyboard input","");
        crdout_fname = crdin_fname;
        crdin_fname = NULL;
    }

    if( ! ask_params && (!input_cs || !output_cs) )
    {
        error_exit("Input and output coordinate systems must be defined on the command line","");
    }

}

/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Routines to interactively obtain program parameters...           */
/*                                                                   */
/*-------------------------------------------------------------------*/


static void prompt_for_proj(coordsys **proj,
                            char *dms, char *ne, char *gothgt,
                            char *orthohgt, char *iostring)
{
    char instring[21];
    int nstr;
    coordsys *nprj;

    /* Get projection code */

    nprj = NULL;
    for(;;)
    {
        *dms = AF_DEG;
        if( nprj ) delete_coordsys( nprj );
        nprj = NULL;
        printf("    Enter %s circuit code or ?: ",iostring);
        nstr = read_string(stdin,DEFAULT_SEPARATOR,instring,20);
        copy_to_newline(stdin,NULL, NULL);
        if (nstr<0) error_exit("Unexpected EOF in input","");
        if (nstr==0) continue;
        if (nstr==1 && instring[0]=='?')
        {
            printf("\n    Valid coordinate system codes are:\n");
            list_circuits_with_pause();
        }
        else
        {
            nprj = load_coordsys(instring);
            if (nprj != NULL )
            {
                *proj = nprj;
                if (is_geodetic(*proj)) *dms=AF_DMS;
                break;
            }
            printf("    **** Invalid coordinate system code ****\n");
        }
    }

    /* Enter order, NE or EN */

    if( !is_geocentric( *proj ) ) for (;;)
        {
            printf("    Coordinate order can be EN, ENH, ENO, NE, NEH, or NEO (O = ortho. height)\n");
            printf("    Enter %s coordinate order(default %s%s): ",
                   iostring, *ne ? "NE" : "EN",
                   *gothgt ? (*orthohgt ? "O" : "H") : "");
            nstr = read_string(stdin,DEFAULT_SEPARATOR,instring,4);
            copy_to_newline( stdin, NULL, NULL);
            if(nstr<0) error_exit("Unexpected EOF in input","");
            if(nstr==0) break;
            _strupr(instring);
            if(strcmp(instring,"NE")==0) {*ne = TRUE; *gothgt = FALSE; break;}
            if(strcmp(instring,"NEH")==0) {*ne = TRUE; *gothgt = TRUE; *orthohgt = FALSE; break;}
            if(strcmp(instring,"NEO")==0) {*ne = TRUE; *gothgt = TRUE; *orthohgt = TRUE; break;}
            if(strcmp(instring,"EN")==0) {*ne = FALSE; *gothgt = FALSE; break;}
            if(strcmp(instring,"ENH")==0) {*ne = FALSE; *gothgt = TRUE; *orthohgt = FALSE; break;}
            if(strcmp(instring,"ENO")==0) {*ne = FALSE; *gothgt = TRUE; *orthohgt = TRUE; break;}
            printf("    **** Invalid definition of coordinate order ****\n");
        }

    /* Enter unit code if DMS */

    if( *dms ) for(;;)
        {
            printf("    Enter angle format (D = deg, M = deg mins, H = deg min secs, default H): ");
            nstr = read_string(stdin,DEFAULT_SEPARATOR,instring,4);
            copy_to_newline(stdin,NULL,NULL);
            if (nstr<0) error_exit("Unexpected EOF in input","");
            if (nstr==0) break;
            if( instring[0] == 'd' || instring[0] == 'D' )
            {
                *dms = AF_DEG; break;
            }
            else if( instring[0] == 'm' || instring[0] == 'M' )
            {
                *dms = AF_DM; break;
            }
            else if( instring[0] == 'h' || instring[0] == 'H' ||
                     instring[0] == 's' || instring[0] == 'S' )
            {
                break;
            }
        }
}


static void prompt_for_number( char *prompt, int *value, int min, int max)
{
    int newval;
    char checkend;
    char instring[21];
    int nstr;

    for(;;)
    {
        printf("%s (default %d): ",prompt,*value);
        nstr = read_string( stdin, DEFAULT_SEPARATOR, instring, 20);
        copy_to_newline( stdin, NULL,NULL);
        if (nstr<0) error_exit("Unexpected EOF in input","");
        if (nstr==0) break;
        if (sscanf(instring,"%d%c",&newval,&checkend)==1 &&
                newval>=min && newval<=max )
        {
            *value = newval;
            break;
        }
        printf("    **** Invalid data or out of range ****\n");
    }
}

static void prompt_for_double( char *prompt, double *value)
{
    double newval;
    char checkend;
    char instring[21];
    int nstr;

    for(;;)
    {
        printf("%s: ",prompt);
        nstr = read_string( stdin, DEFAULT_SEPARATOR, instring, 20);
        copy_to_newline( stdin, NULL,NULL);
        if (nstr<0) error_exit("Unexpected EOF in input","");
        if (sscanf(instring,"%lf%c",&newval,&checkend)==1 )
        {
            *value = newval;
            break;
        }
        printf("    **** Invalid data ****\n");
    }
}

static void prompt_for_filename(char *prompt, FILE **file, char *mode, char *name,
                                char **nameptr, char *opened)
{
    FILE *newfile;
    int nstr;

    for(;;)
    {
        printf("%s",prompt);
        nstr = read_string( stdin, DEFAULT_SEPARATOR, name, 80 );
        copy_to_newline(stdin,NULL,NULL);
        if (nstr<0) error_exit("Unexpected EOF in input","");
        if (nstr==0) break;
        newfile = fopen(name,mode);
        if (newfile!=NULL)
        {
            *file = newfile;
            *nameptr = name;
            *opened = TRUE;
            break;
        }
        else
            printf("    **** Unable to open file ****\n");
    }
}


static void prompt_for_parameters( void )
{
    static char input_file[81];
    static char output_file[81];
    char ans[2];

    printf("\n%s:  Coordinate conversion program\n",PROGNAME);
    printf("\nInput coordinates:\n");
    prompt_for_proj(&input_cs,&input_dms,&input_ne,&input_h,&input_ortho,"input");
    printf("\nOutput coordinates:\n");
    prompt_for_proj(&output_cs,&output_dms,&output_ne,&output_h,&output_ortho,"output");
    prompt_for_number("    Enter number of decimal places for output",
                      &output_prec,0,15);
    printf("\nEach point you convert can have a name.  If you want to use names you must\n");
    printf("specify the length of the longest name you want to use\n\n");
    id_length = 0;
    prompt_for_number("Enter maximum name length, or 0 for no names",&id_length,
                      0,80);
    point_ids = id_length>0;
    printf("\nYou can enter the coordinates to convert yourself, or read them from a file\n");
    printf("If you want to read them from a file then specify the name of the file,\n");
    printf("otherwise just press Enter\n\n");
    prompt_for_filename("Enter coordinate input filename: ",&crdin,
                        "r",input_file,&crdin_fname,&crdin_open);
    if( crdin_fname != NULL )
    {
        printf("Enter field separator in input file (default is blank, use t for tab)");
        read_string(stdin,DEFAULT_SEPARATOR,ans,1);
        copy_to_newline(stdin,NULL,NULL);
        if( ans[0] != 0 ) separator = ans[0];
        if( separator == 't' || separator == 'T') separator = '\t';
    }
    printf("\nIf you want the output to go to a file, enter the file name\n\n");
    prompt_for_filename("Enter output file name: ",&crdout,"w",
                        output_file,&crdout_fname,&crdout_open);

    if (crdout_fname != NULL)
    {
        printf("\nDo you want the input coordinates copied to the output file?\n");
        printf("Enter yes or no: ");
        read_string(stdin,DEFAULT_SEPARATOR,ans,1);
        copy_to_newline(stdin,NULL,NULL);
        verbose = ans[0]=='y' || ans[0]=='Y';
    }
}


/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Introduce manual coordinate entry - show an example of what      */
/*  the input line should look like.                                 */
/*                                                                   */
/*-------------------------------------------------------------------*/

static void head_output( FILE *out );

static void show_example_input( void  )
{
    static char *east[] = {"315378.28","2571312.90","171.14238","171 30.21 E","171 41 53.55 E"};
    static char *north[] = {"728910.43","6025519.64","-41.25531","41 22.05 S","41 22 03.26 S"};
    char *c1, *c2;
    int ncd;

    clear_screen();
    head_output( stdout );

    printf("\nYou can now enter coordinates you want converted.  Your input should look\n");
    printf("something like this: \n\n");
    if( point_ids ) printf("STN_3  ");
    ncd = 0;
    if( is_geodetic(input_cs))
    {
        switch(input_dms)
        {
        case AF_DEG: ncd = 2; break;
        case AF_DM:  ncd = 3; break;
        case AF_DMS: ncd = 4; break;
        }
    }
    if( input_ne )
    {
        c1 = north[ncd]; c2 = east[ncd];
    }
    else
    {
        c1 = east[ncd]; c2 = north[ncd];
    }
    printf("%s  %s%s\n\n",c1,c2,input_h ? "  1532.40" : "");
    printf("Each item should be separated by at least one blank space.\n");
    if( point_ids ) printf("The name cannot include blanks.\n");
    printf("When you have finished enter a blank line to quit the program\n\n");
}


/*-------------------------------------------------------------------*/
/*                                                                   */
/*  tidy_up_parameters - post processing of program options to       */
/*           ensure that they are all consistent.                    */
/*                                                                   */
/*-------------------------------------------------------------------*/

static void tidy_up_parameters( void )
{

    /* Keyboard input of coordinates not possible if input file
       specified */

    if (crdin_fname) ask_coords = FALSE;

    /* If no input file, and asking for parameters, then coordinate
       entry must be from terminal stdin, so ask for coords */

    if (ask_params && !crdin_fname) ask_coords = TRUE;

    /* If prompting for coordinates then don't skip errors */

    if (ask_coords) skip_errors = FALSE;

    /* Determine the input coordinate precision to be used for
       echoing.  Attempt to get similar accuracy to output
       coordinates */

    input_prec = output_prec;
    if (is_geodetic(output_cs))
    {
        switch(output_dms)
        {
        case AF_DEG:  input_prec -= 5; break;
        case AF_DM:   input_prec -= 3; break;
        case AF_DMS:  input_prec -= 1; break;
        }
    }
    if( output_vprec < 0 ) output_vprec = input_prec;
    input_vprec = output_vprec;
    if (is_geodetic(input_cs))
    {
        switch( input_dms )
        {
        case AF_DEG: input_prec += 5; break;
        case AF_DM:  input_prec += 3; break;
        case AF_DMS: input_prec += 1; break;
        }
    }
    if (input_prec < 0) input_prec = 0;
    if (input_vprec < 0) input_vprec = 0;
    if (output_vprec < 0) output_vprec = 0;

    /* Don't need orthometric if haven't got height */

    if (! input_h ) input_ortho = FALSE;
    if (! output_h ) output_ortho = FALSE;

    /* Geocentric systems require a Z coordinate */

    if( is_geocentric(input_cs) ) {input_h = TRUE; input_ortho = FALSE; }
    if( is_geocentric(output_cs) ) {output_h = TRUE; output_ortho = FALSE; }

    transform_heights = input_ortho ^ output_ortho;

    /* Point id length */

    if (id_length <= 0) point_ids = FALSE;

    /* Verbose valid only if writing to a file */

    verbose = verbose && (!ask_coords || crdout_fname!=NULL);

}


/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Set up parameters of transformation and initiallize the          */
/*  projection routine (using define_conv).                          */
/*                                                                   */
/*-------------------------------------------------------------------*/


static void setup_transformation( void )
{

    /* Set up the conversion */

    if( !input_cs || !output_cs )
    {
        error_exit("Input and output coordinate systems must be specified","");
    }

    if( define_coord_conversion_epoch( &cnv, input_cs, output_cs, conv_epoch ) != OK )
    {
        if( strlen(cnv.errmsg) > 0 )
        {
            fprintf(stderr,"%s\n",cnv.errmsg);
        }
        else
        {
            fprintf(stderr,"Cannot convert coordinates from %s to %s\n",
                input_cs->code, output_cs->code );
        }
        exit(1);
    }

    /* Set up pointers to input coordinates and parameters */


    input_latlong = 0;
    if (is_projection(input_cs))
    {
        if (input_ne) { in1 = inxyz+CRD_NORTH; in2 = inxyz+CRD_EAST;}
        else          { in1 = inxyz+CRD_EAST; in2 = inxyz+CRD_NORTH;}
        if( input_h ) in3 = inxyz+CRD_HGT; else in3 = NULL;
    }
    else if(is_geodetic(input_cs))
    {
        input_latlong = 1;
        if (input_ne)
        {
            in1 = inxyz+CRD_LAT; in2 = inxyz+CRD_LON;
            indms1 = ns_string; indms2 = ew_string;
        }
        else
        {
            in1 = inxyz+CRD_LON; in2 = inxyz+CRD_LAT;
            indms1 = ew_string; indms2 = ns_string;
        }
        if( input_h ) in3 = inxyz+CRD_HGT; else in3 = NULL;
    }
    else
    {
        in1 = inxyz+CRD_X;
        in2 = inxyz+CRD_Y;
        in3 = inxyz+CRD_Z;
    }

    /* Set up pointers to output coordinates and parameters */


    output_latlong = 0;
    if (is_projection(output_cs))
    {
        if (output_ne) { out1 = outxyz+CRD_NORTH; out2 = outxyz+CRD_EAST;}
        else          { out1 = outxyz+CRD_EAST; out2 = outxyz+CRD_NORTH;}
        if( output_h ) out3 = outxyz+CRD_HGT; else out3 = NULL;
    }
    else if(is_geodetic(output_cs))
    {
        output_latlong = 1;
        if (output_ne)
        {
            out1 = outxyz+CRD_LAT; out2 = outxyz+CRD_LON;
            outdms1 = ns_string; outdms2 = ew_string;
        }
        else
        {
            out1 = outxyz+CRD_LON; out2 = outxyz+CRD_LAT;
            outdms1 = ew_string; outdms2 = ns_string;
        }
        if( output_h ) out3 = outxyz+CRD_HGT; else out3 = NULL;
    }
    else
    {
        out1 = outxyz+CRD_X;
        out2 = outxyz+CRD_Y;
        out3 = outxyz+CRD_Z;
    }

    /* Allocate space for point id */

    if (point_ids)
    {
        id = (char *) malloc( id_length+1 );
        if (id==NULL) error_exit("Memory allocation error","");
    }

    /* Create the coordinate prompt */

    strcpy(coord_prompt,"Enter");
    if (point_ids) strcat(coord_prompt," name,");
    if (is_geodetic(input_cs))
    {
        if (input_ne) strcat(coord_prompt," latitude, longitude: ");
        else          strcat(coord_prompt," longitude, latitude: ");
    }
    else if(is_projection(input_cs))
    {
        if (input_ne) strcat(coord_prompt," northing, easting: ");
        else          strcat(coord_prompt," easting, northing: ");
    }
    else
    {
        strcat(coord_prompt," X, Y, Z: ");
    }
    prompt_length = strlen(coord_prompt);

    /* Define the input and output field lengths */

    if (is_geodetic(input_cs))
    {
        infldlen = input_prec;
        switch(input_dms)
        {
        case AF_DEG: infldlen += 5; break;
        case AF_DM:  infldlen += 10; break;
        case AF_DMS: infldlen += 13; break;
        }
    }
    else                       infldlen = input_prec + 9;
    if( is_geocentric(input_cs)) invfldlen = input_vprec + 9;
    else                         invfldlen = input_vprec + 6;

    if (is_geodetic(output_cs))
    {
        outfldlen = output_prec;
        switch(output_dms)
        {
        case AF_DEG: outfldlen += 5; break;
        case AF_DM:  outfldlen += 10; break;
        case AF_DMS: outfldlen += 13; break;
        }
    }
    else                       outfldlen = output_prec + 9;
    if( is_geocentric(output_cs)) outvfldlen = output_vprec + 9;
    else                          outvfldlen = output_vprec + 6;


    /* Initiallize counters */

    ncrdcnv = 0;
    ncrderr = 0;
}

static void setup_geoid_calculations( void )
{

    /* Set up the conversion */

    geoid_coordsys = get_geoid_coordsys( geoiddef );
    if( !geoid_coordsys )
    {
        char errmess[120];
        sprintf(errmess,"Geoid reference frame %s not defined in COORDSYS.DEF file",
                geoid_coordsys->code );
        error_exit(errmess,"");
    }

    if( input_ortho && define_coord_conversion_epoch( &ingcnv, input_cs, geoid_coordsys, DEFAULT_GEOID_CRDSYS_EPOCH ) != OK )
    {
        char errmsg[256];
        sprintf(errmsg,"Cannot convert between %.20s and geoid coordinate system %.20s",
            input_cs->code, geoid_coordsys->code );
        error_exit(errmsg,"");
    }

    if( output_ortho && define_coord_conversion_epoch( &outgcnv, output_cs, geoid_coordsys, DEFAULT_GEOID_CRDSYS_EPOCH ) != OK )
    {
        char errmsg[256];
        sprintf(errmsg,"Cannot convert between %.20s and geoid coordinate system %.20s",
            output_cs->code, geoid_coordsys->code );
        error_exit(errmsg,"");
    }
}

/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Open input and output files, checking that they have not         */
/*  already been opened.  (If filenames have been obtained by        */
/*  prompts then the files will have been opened in the process      */
/*  of checking that the names are valid.                            */
/*                                                                   */
/*  Also here are routines for writing header information on the     */
/*  output file and for closing the files.                           */
/*                                                                   */
/*-------------------------------------------------------------------*/

static void open_files( void )
{
    if (crdin_fname && !crdin_open)
    {
        if( strcmp(crdin_fname,"-") == 0 )
        {
            crdin = stdin;
        }
        else
        {
            crdin = fopen(crdin_fname,"rb");
        }
        if (crdin == NULL)
            error_exit("Cannot open input file ",crdin_fname);
        crdin_open = TRUE;
    }

    if (crdout_fname && !crdout_open)
    {
        if( strcmp(crdout_fname,"-") == 0 )
        {
            crdout = stdout;
        }
        else
        {
            crdout = fopen(crdout_fname,"w");
        }
        if (crdout == NULL)
            error_exit("Cannot open output file ",crdout_fname);
        crdout_open = TRUE;
    }
    crdcom = !ask_coords || crdout_fname ? crdout : NULL;

}


static void head_output( FILE * out )
{
    fprintf(out,"\n%s - coordinate conversion program (version %s dated %s)\n",
         PROGNAME,VERSION,PROGDATE);
    fprintf(out,"\nInput coordinates:  %s", input_cs->name);
    /* if( use_deformation ) fprintf(out," at epoch %.2lf",cnv.epochfrom); */
    fprintf(out,"\n");
    if( input_ortho ) fprintf(out,"                    Input heights are orthometric\n");
    fprintf(out,  "\nOutput coordinates: %s", output_cs->name );
    /* if( use_deformation ) fprintf(out," at epoch %.2lf",cnv.epochto); */
    fprintf(out,"\n");
    if( output_ortho ) fprintf(out,"                    Output heights are orthometric\n");
    if( cnv.needsepoch )
    {
        fprintf(out,"\nDatum conversion epoch %.2lf\n",cnv.epochconv);
    }
    if( transform_heights ) fprintf(out,"\nGeoid heights from %s\n",get_geoid_model( geoiddef ));
}


static void head_columns( FILE *out )
{
    char *prjcol[3] = { "Easting", "Northing", "Height" };
    char *geocol[3] = { "Longitude", "Latitude", "Height" };
    char *xyzcol[3] = { "X", "Y", "Z" };
    int  enorder[3] = { 0, 1, 2 };
    int  neorder[3] = { 1, 0, 2 };
    char **cols;
    int  *order;
    int  ncol;
    int  icol;

    fprintf(out,"\n");
    if (point_ids) fprintf(out,"%-*s  ",id_length,"ID");
    if (verbose)
    {
        if( is_projection(input_cs)) cols = prjcol;
        else if( is_geodetic(input_cs)) cols = geocol;
        else cols = xyzcol;
        if( input_ne ) order = neorder; else order = enorder;
        if( input_h ) ncol = 3; else ncol = 2;
        for( icol = 0; icol < ncol; icol++ )
        {
            fprintf(out,"%*s ",icol == 2 ? invfldlen : infldlen,cols[order[icol]]);
        }
        fprintf(out," ");
    }

    if( is_projection(output_cs)) cols = prjcol;
    else if( is_geodetic(output_cs)) cols = geocol;
    else cols = xyzcol;
    if( output_ne ) order = neorder; else order = enorder;
    if( output_h ) ncol = 3; else ncol = 2;
    for( icol = 0; icol < ncol; icol++ )
    {
        fprintf(out, "%*s ", icol == 2 ? outvfldlen : outfldlen,cols[order[icol]]);
    }
    fprintf(out,"\n");
}

static void close_files( void )
{
    if (crdin_fname && crdin != stdin ) fclose(crdin);
    if (crdout_fname && crdout != stdout ) fclose(crdout);
}

/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Routines for data input:                                         */
/*                                                                   */
/*  find_next_data_line() - skips over blank lines and comment       */
/*            lines to find a line containing coordinate data.       */
/*            The comments and blanks will be echoed to the          */
/*            output file.  Returns EOF when either the end          */
/*            of file is reached, or when a blank line is entered    */
/*            at the keyboard.                                       */
/*                                                                   */
/*  read_point_id() - reads the point id from the beginning of       */
/*            the line                                               */
/*                                                                   */
/*  read_dms() - reads an angle in degrees, minutes, seconds         */
/*            notations                                              */
/*                                                                   */
/*  read_number() - reads a number                                   */
/*                                                                   */
/*  read_coordinates() - reads the coordinate pair, either two       */
/*            numbers or two dms strings (which are converted to     */
/*            degrees.                                               */
/*                                                                   */
/*-------------------------------------------------------------------*/

static int find_next_data_line( void )
{
    int c;
    for(;;)
    {
        if(ask_coords) printf("\n%s",coord_prompt);
        c = getc(crdin);
        while (c == ' ' || c == '\t') c=getc(crdin);
        if (c=='\n' && ask_coords) return EOF;
        ungetc(c,crdin);
        if (c!='\n' && c!='!') break;
        copy_to_newline(crdin,crdcom,NULL);
    }
    return c;
}


static void read_point_id( void )
{
    read_string( crdin, separator, id, id_length );
}

/*-------------------------------------------------------------------*/
/*                                                                   */
/*  read_dms - reads an angle in degrees, minutes, seconds format    */
/*             optionally followed by a hemisphere indicator         */
/*             character.  The hemisphere may immediately follow     */
/*             the seconds, or may be followed by blank space.       */
/*             Returns values   0   read OK                          */
/*                              1   read OK, but no hemisphere       */
/*                              2   invalid data in string           */
/*                              3   end of line or file before       */
/*                                  file read OK.                    */
/*             The parameter hem should contain the positive and     */
/*             negative hemisphere characters, e.g. "NS".            */
/*-------------------------------------------------------------------*/

static int read_dms( FILE *input, DMS *dms, char *hem )
{
    char string[21];
    char checkend;
    char hemdef;
    int nc,c;

    if (read_string(input,separator,string,20)<=0) return 3;

    hemdef=0;
    dms->neg = 0;

    if(strlen(string)==1 && hem )
    {
        c = string[0];
        dms->neg = toupper(c)==hem[1];
        if (dms->neg || toupper(c)==hem[0])
        {
            hemdef=1;
            if(read_string(input,separator,string,20)<=0) return 3;
        }
    }

    if (sscanf(string,"%d%c",&dms->degrees,&checkend)!=1) return 2;
    if (dms->degrees < 0) return 2;


    if( ! dms->no_seconds )
    {
        if (read_string(input,separator,string,20)<=0) return 3;
        if (sscanf(string,"%d%c",&dms->minutes,&checkend)!=1) return 2;
        if (dms->minutes<0 || dms->minutes>60) return 2;
    }

    nc=read_string(input,separator,string,20);
    if (nc<=0) return 3;

    /* No hemisphere indicator required */

    if (hem && !hemdef)
    {

        /* Is the hemisphere indicator at the end of the seconds */

        c = toupper( (int) string[nc-1]);
        dms->neg = c==hem[1];
        if ( dms->neg || c==hem[0] )
        {
            string[nc-1] = '\0';
            hemdef=1;
        }

        /* Otherwise scan input for the hemisphere indicator */

        else
        {
            while( (c=getc(input)) == ' ' || c == '\t' );
            dms->neg = toupper(c)==hem[1];
            if (dms->neg || toupper(c)==hem[0]) hemdef=1;
            else { ungetc(c,input); }
        }
    }

    if (sscanf(string,"%lf%c",&dms->seconds,&checkend)!=1) return 2;
    if (dms->seconds<0.0 || dms->seconds>60.0) return 2;
    return hemdef ? 0 : 1;
}

/*-------------------------------------------------------------------*/
/*                                                                   */
/*  read_number - Reads a number from the input string, returns      */
/*                one of the following error codes                   */
/*                    0   Number read OK                             */
/*                    2   Invalid data in the input                  */
/*                    3   End of line or eof encountered             */
/*-------------------------------------------------------------------*/

static int read_number( FILE *input, double *value )
{
    char numbstr[33];
    char checkend;
    if (read_string( input, separator, numbstr, 32 )<=0) return 3;
    if (sscanf( numbstr, "%lf%c", value, &checkend) != 1) return 2;
    return 0;
}


static int read_coordinates( void )
{
    DMS dms;
    int sts;
    char hemdef;
    if (input_dms)
    {
        dms.no_seconds = (input_dms == AF_DM);
        sts = read_dms( crdin, &dms, indms1 );
        if (sts>1) return sts;
        hemdef = sts==0;
        *in1 = dms_deg( dms );
        sts = read_dms( crdin, &dms, hemdef ? indms2 : NULL );
        if (sts>1) return sts; else sts=0;
        *in2 = dms_deg( dms );
    }
    else
    {
        hemdef = FALSE;
        sts = read_number( crdin, in1 );
        if (sts != 0) return sts;
        sts = read_number( crdin, in2 );
        if (sts != 0) return sts;
    }
    if( input_h ) sts = read_number( crdin, in3 );
    if( input_latlong ) { *in1 *= DTOR; *in2 *= DTOR; }
    return sts;
}



/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Output routines. Output may be directed either to crdout, or     */
/*  if prompting for coordinates and writing to a file to both       */
/*  crdout and to stdout.  The latter case is tested for using       */
/*  the expression (ask_coords && crdout_fname), i.e. asking for     */
/*  coordinates and having a named output file.                      */
/*                                                                   */
/*-------------------------------------------------------------------*/

static void write_point_id( FILE * out )
{
    if( !separator )
    {
        fprintf(out,"%-*s  ",id_length,id);
    }
    else
    {
        fprintf(out,"%s%c",id,separator);
    }
}

static void write_dms( FILE *out, double deg, int prec, char no_secs, char *hem )
{
    DMS dms;
    deg_dms( deg, &dms, prec, no_secs );
    if( separator )
    {
        fprintf(out,"%d%c",dms.degrees,separator);
        if( ! no_secs ) fprintf(out,"%02d%c",dms.minutes,separator);
        fprintf(out,"%0*.*lf%c%c",prec+3,prec,
                dms.seconds<0.0 ? 0.0 : dms.seconds, separator,
                dms.neg ? hem[1] : hem[0] );
    }
    else
    {
        fprintf(out,"%4d ",dms.degrees);
        if( ! no_secs ) fprintf(out,"%02d ",dms.minutes);
        fprintf(out,"%0*.*lf %c",prec+3,prec,
                dms.seconds<0.0 ? 0.0 : dms.seconds, dms.neg ? hem[1] : hem[0]);
    }
}



static void write_coords( FILE *out, double *c1, double *c2, double *c3,
                          char *h1, char *h2, int len, int prec,
                          int vlen, int vprec, char dms, char radians )
{

    char sep;
    double mult;
    sep = separator;
    if( !sep ) sep = ' ';
    if( separator ) len = vlen = 0;
    mult = 1.0;
    if( radians ) mult = RTOD;
    if (dms)
    {
        write_dms(out,*c1*mult,prec,dms==AF_DM,h1);
        fprintf(out,"%c",sep);
        write_dms(out,*c2*mult,prec,dms==AF_DM,h2);
    }
    else
    {
        fprintf(out,"%*.*lf%c%*.*lf",len, prec,*c1 * mult,sep,len, prec,*c2 * mult);
    }
    if( c3 ) fprintf( out, "%c%*.*lf", sep,vlen,vprec,*c3);
}



static void write_results( void )
{
    if (ask_coords)
    {
        printf("%*s",prompt_length," ");
        write_coords(stdout,out1,out2,out3,outdms1,outdms2,outfldlen,
                     output_prec, outvfldlen, output_vprec, output_dms,output_latlong);
    }
    if ( !ask_coords || crdout_fname != NULL )
    {
        if (point_ids) write_point_id( crdout );
        if (verbose)
        {
            write_coords(crdout,in1,in2,in3,indms1,indms2,infldlen,
                         input_prec, invfldlen, input_vprec, input_dms,input_latlong);
            if( separator ) fputc( separator, crdout );
            else            fputs( "  ", crdout );
        }
        write_coords(crdout,out1,out2,out3,outdms1,outdms2,outfldlen,
                     output_prec, outvfldlen, output_vprec, output_dms,output_latlong);
    }
}


/*-------------------------------------------------------------------*/
/*                                                                   */
/*   Error reporting - error in coordinate processing occur either   */
/*   reading the data or in processing the coordinates.  The         */
/*   latter errors will only be range errors - either the input      */
/*   or output coordinates are outside their valid range.            */
/*                                                                   */
/*-------------------------------------------------------------------*/


static void report_read_error( void )
{
    char *s1, *s2;
    char *fp = " for point ";
    char *bl = "";

    if (point_ids) { s1 = fp; s2 = id; }
    else s1 = s2 = bl;
    if (ask_coords) printf("%*s**** Error reading data ****",prompt_length,"");
    if (!ask_coords || crdout_fname != NULL)
        fprintf(crdout,"**** Error reading data%s%s **** ",s1,s2);
}

static void report_conv_error( int sts )
{
    char *s1, *s2;
    char *fp = " for point ";
    char *bl = "";
    char *msg1 = "Coordinate range error";
    char *msg2 = "Conversion error";
    const char *em;

    em = sts == INCONSISTENT_DATA ? msg1 : msg2;
    if( strlen(cnv.errmsg) > 0 ) em=cnv.errmsg;
    if (point_ids) { s1 = fp; s2 = id; }
    else s1 = s2 = bl;
    if (ask_coords) printf("%*s**** %s ****",
                               prompt_length,"",em);
    if (!ask_coords || crdout_fname != NULL)
        fprintf(crdout,"**** %s%s%s **** ",em,s1,s2);
}

static void report_geoid_error( void )
{
    char *s1, *s2;
    char *fp = " for point ";
    char *bl = "";

    if (point_ids) { s1 = fp; s2 = id; }
    else s1 = s2 = bl;
    if (ask_coords) printf("%*s**** Cannot calculate geoid height ****",
                               prompt_length,"");
    if (!ask_coords || crdout_fname != NULL)
        fprintf(crdout,"**** Cannot calculate geoid height%s%s **** ",s1,s2);
}

/*-------------------------------------------------------------------*/
/*                                                                   */
/* Main coordinate processing loop ... processes coordinates until   */
/* EOF occurs in input (also generated by find_next_data_line if     */
/* a blank line is entered when prompting for coordinates).          */
/*                                                                   */
/*-------------------------------------------------------------------*/

static int calc_geoid_height( coord_conversion *gcnv, double xyz[3], double *h )
{
    double gxyz[3];
    int sts;
    *h = 0.0;
    sts = convert_coords(gcnv,xyz,NULL,gxyz,NULL );
    if( sts != OK ) return sts;
    sts = calculate_geoid_undulation( geoiddef, gxyz[CRD_LAT], gxyz[CRD_LON], h );
    if( sts != OK ) *h = 0.0; else *h += xyz[2] - gxyz[2];
    return sts;
}


static void process_coordinates( void )
{
    long start_loc;
    double inhgt;
    int sts;
    double gh;
    char sep[2];
    sep[0] = separator ? separator : ' ';
    sep[1] = 0;
    for(;;)
    {

        if(find_next_data_line()==EOF) break;
        start_loc = ftell(crdin);
        if(point_ids) read_point_id();
        inxyz[2] = 0.0;  /* In case no height info */
        sts = read_coordinates();
        if( sts != OK )
        {
            if(!skip_errors) {report_read_error(); ncrderr++;}
        }
        inhgt = inxyz[2];
        if( sts == OK )
        {
            if( transform_heights && input_ortho )
            {
                sts = calc_geoid_height( &ingcnv, inxyz, &gh );
                if( sts != OK && !skip_errors) report_geoid_error();
                inxyz[2] += gh;
            }
            if( sts == OK )
            {
                sts = convert_coords(&cnv,inxyz,NULL,outxyz,NULL );
                if( sts != OK && ! skip_errors ) report_conv_error(sts);
            }
            if( sts == OK && transform_heights && output_ortho )
            {
                sts = calc_geoid_height( &outgcnv, outxyz, &gh );
                if( sts != OK && ! skip_errors ) report_geoid_error();
                outxyz[2] -= gh;
            }

            if( input_ortho && output_ortho ) outxyz[2] = inhgt;
            if( sts != OK && !skip_errors ) ncrderr++;
        }
        if( sts == OK )
        {
            ncrdcnv++;
            write_results();
        }
        else
        {
            if( start_loc != EOF && ! ask_coords )
                fseek( crdin, start_loc, SEEK_SET );
        }
        copy_to_newline(crdin,crdcom,sep);
    }
}


static void summarise_job( FILE *out )
{
    fprintf(out,"\n%d coordinates converted\n",ncrdcnv);
    fprintf(out,"%d errors occurred reading or converting coordinates\n",
            ncrderr);
}


/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Main program - self explanatory!                                 */
/*                                                                   */
/*-------------------------------------------------------------------*/

/* TODO:  Cludge with set_find_file_directories to pick up grid file needs attention */

int main( int argc, char *argv[] )
{
    int rplen;
    int sts;
    char *exefile;
    exefile = find_image( argv[0] );

    concord_init();
    get_definition_files( argc, argv );
    if( coordsys_file )
    {
        install_default_projections();
        sts = install_crdsys_file( coordsys_file );
        set_find_file_directories( coordsys_file, NULL, NULL );
    }
    else
    {
        rplen = path_len( exefile, 0 );
        exefile[rplen] = 0;
        sts = install_default_crdsys_file( exefile );
        /* set_application_name( "concord" ); */
        set_find_file_directories( exefile, NULL, NULL );
    }
    if( sts != OK )
    {
        printf("Cannot read COORDSYS.DEF file\n");
        return 0;
    }
    process_command_line(argc,argv);
    if(ask_params) prompt_for_parameters();
    tidy_up_parameters();
    setup_transformation();
    if( transform_heights )
    {
        const char *gfile = geoid_file;
        const char *title = NULL;
        geoiddef = 0;
        if( ! file_exists(gfile) &&input_cs && input_cs->source && strncmp(input_cs->source,"file:",5) == 0 )
        {
            set_find_file_directories((input_cs->source + 5),NULL,NULL);
            gfile = create_geoid_filename(geoid_file);
        }
        set_find_file_directories(exefile,NULL,NULL);
        if( ! file_exists(gfile))
        {
            gfile = create_geoid_filename(geoid_file);
        }
        if( ! file_exists(gfile) )
        {
            printf("Cannot find geoid file %s\n",geoid_file ? geoid_file : "");
            return 0;
        }
        else
        {
            geoiddef = create_geoid_grid( gfile );
            if( ! geoiddef )
            {
                printf("Cannot read geoid file %s\n",gfile ? gfile : "");
                return 0;
            }
        }
        title = get_geoid_model( geoiddef );
        printf("Using geoid: %s\n", title ? title : "Unnamed");

        setup_geoid_calculations();
    }
    open_files();
    if(verbose) { head_output(crdout); head_columns(crdout); }
    if( ask_coords ) show_example_input();
    process_coordinates();
    if(verbose) summarise_job(crdout);
    close_files();

    if(crdout_fname && crdout != stdout) summarise_job(stdout);
    return 0;
}

