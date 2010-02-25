/* rescale.h */
/* Function Definitions for rescale.c */

int floatcmp(const void *v1, const void *v2);
int rescale(float *datav, int ndata, int nbits, float *offset, float *scale);
int basicscale(float *datav, int ndata, float *offset, float *scale);

