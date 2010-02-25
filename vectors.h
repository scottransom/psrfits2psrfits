#include <stdlib.h>
#include <stdio.h>

/* The routines are in vectors.c                                    */

float *gen_fvect(long length);
/* Generate a floating point vector */

double *gen_dvect(long length);
/* Generate a double precision vector */

short *gen_svect(long length);
/* Generate an short integer vector */

int *gen_ivect(long length);
/* Generate an integer vector */

long *gen_lvect(long length);
/* Generate a long integer vector */

unsigned char *gen_bvect(long length);
/* Generate a 'byte' or unsigned character vector */

/*  Note:  To free memory allocated by these routines simply use  */
/*         the free() function.                                   */
/*                                                                */
/*  Example:                                                      */
/*                                                                */
/*  x = gen_fvect(100);   // Generate a 100 point float vector.   */
/*  free(x);              // Free the vector.                     */
