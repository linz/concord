#ifndef IOSTRING_H
#define IOSTRING_H

/*
   $Log: iostring.h,v $
   Revision 1.2  2004/04/22 02:35:26  ccrook
   Setting up to support linux compilation (x86 architecture)

   Revision 1.1  1995/12/22 19:51:15  CHRIS
   Initial revision

*/

#ifndef IOSTRING_H_RCSID
#define IOSTRING_H_RCSID "$Id: iostring.h,v 1.2 2004/04/22 02:35:26 ccrook Exp $"
#endif

/* Definitions of an input and output string structure */

typedef struct
{
    char *buffer;   /* The input string data */
    char *ptr;      /* Pointer into the data for the next read operation */
    char *sourcename; /* Name of the source - file name for a file source */
    void *source;   /* Used for the error handler */
    int (*report_error)( void *source, int status, char *message );
} input_string_def;

/* Output string def - defines a way of sending strings to some form
   of output device */

typedef struct
{
    void *sink;
    int (*write)( char *string, void *sink );
} output_string_def;

/* Input string functions.  Return status values are as defined in
   errdef.h, ie 0 = OK, non-zero represent errors.

   The end_of_string function returns 1 if there is no more data available
   in the string, 0 otherwise.  */

void set_input_string_def( input_string_def *s, char *string );
int next_string_field( input_string_def *is, char *buf, int nbuf );
int skip_string_field( input_string_def *is );
int double_from_string( input_string_def *is, double *value );
int float_from_string( input_string_def *is, float *value );
int long_from_string( input_string_def *is, long *value );
int int_from_string( input_string_def *is, int *value );
int short_from_string( input_string_def *is, short *value );
int character_from_string( input_string_def *is, char *c );
long get_string_loc( input_string_def *is );
void set_string_loc( input_string_def *is, long loc );
int end_of_string( input_string_def *is );
char *unread_string( input_string_def *is );
void report_string_error( input_string_def *is, int status, char *message );

int write_output_string( output_string_def *os, char *s );

#endif


