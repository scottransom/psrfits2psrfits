/******
 * testrsc.c SC 20091123 
 *  Driver to test rescaling
 * gcc rescale.c testrsc.c -o testrsc -lm
 ******/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "rescale.h"

#define VERBOSE 1
#define CASE 3
#define NDATA 15625

float randg(void);

float dataval[NDATA], outval[NDATA], outval2[NDATA];

int runtest(void);              // Only uses globals

/******************************/
int main(int argc, char *argv[])
{
    int i, j, Nval;

#if(CASE==1)
  /***** Fake Gaussian data *****/
    for (i = 0; i < NDATA; i++) {
        dataval[i] = (int) (32000.0 + randg() * 10240.0) % 65536;
        /* Poor random values, but who cares? */
    }
    if (runtest() != 0) {
        printf("Error!?\n");
        return (-1);
    }
#endif

#if(CASE==2)
  /***** Fake Gaussian data, bad bias *****/
    for (i = 0; i < NDATA; i++) {
        dataval[i] = roundf(1000.0 + randg() * 3000.0);
        if (dataval[i] < 0.0) {
            dataval[i] = rand() % 100;
        }
        if (dataval[i] > 65536.0) {
            dataval[i] = (65536 - rand() % 100);
        }
    }
    if (runtest() != 0) {
        printf("Error!?\n");
        return (-1);
    }
#endif

#if(CASE==3)
  /***** Long tail case *****/
    for (i = 0; i < NDATA; i++) {
        j = rand() % 256;
        dataval[i] = (j * j) % 65536;
    }
    if (runtest() != 0) {
        printf("Error!?\n");
        return (-1);
    }
#endif

#if(CASE==4)
  /***** Bad range case *****/
    for (i = 0; i < NDATA; i++) {
        dataval[i] = ((int) (randg() * 1000.0 + 20000 + rand() % 20000)) % 65536;
    }
    if (runtest() != 0) {
        printf("Error!?\n");
        return (-1);
    }
#endif

    return (0);
}


/******************************/
int runtest(void)
{
    int i;
    float offset, scale;

    // Obtain scale parameters:
    if (rescale(dataval, NDATA, &offset, &scale) != 0) {
        printf("Rescale routine failed!\n");
        return (-1);
    }
#if(VERBOSE)
    printf("# Calculated offset and scale: %.3f, %.3f\n", offset, scale);
#endif

    // Generate rescaled data:
    for (i = 0; i < NDATA; i++) {
        // outval reflects results of rescale()
        outval[i] = roundf((dataval[i] - offset) / scale);
        if (outval[i] < 0.0) {
            outval[i] = 0.0;
        }
        // outval2 reflects basicscale(), always 2048.0. 4096.0:
        outval2[i] = roundf((dataval[i] - 2048.0) / 4096.0);
    }

    // Now print it out for plotting:
    for (i = 0; i < NDATA; i++) {
        printf("%8.1f %8.1f %8.1f\n", dataval[i], outval2[i], outval[i]);
    }

    return (0);
}


/******************************/
float randg(void)
{
    float x1, x2, w;
    /* WARNING: Uses system rand()! */
    do {
        x1 = 2.0 * ((float) rand() / RAND_MAX) - 1.0;
        x2 = 2.0 * ((float) rand() / RAND_MAX) - 1.0;
        w = x1 * x1 + x2 * x2;
    } while (w >= 1.0);

    w = sqrt((-2.0 * log(w)) / w);
    // x1*w and x2*w are two Gaussian random variables
    // (mu=0,sigma=1)
    // Return one: 
    return (x1 * w);
}

/******************************/
