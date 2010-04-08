/* write_psrfits.c */
#define _ISOC99_SOURCE          // For long double strtold
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "psrfits.h"

#define DEBUGOUT 0

// Define different obs modes
static const int search = SEARCH_MODE, fold = FOLD_MODE;
int psrfits_obs_mode(const char *obs_mode)
{
    if (strncmp("SEARCH", obs_mode, 6) == 0) {
        return (search);
    } else if (strncmp("FOLD", obs_mode, 4) == 0) {
        return (fold);
    } else if (strncmp("PSR", obs_mode, 3) == 0) {
        return (fold);
    } else if (strncmp("CAL", obs_mode, 3) == 0) {
        return (fold);
    } else {
        // TODO: what to do here? default to search for now
        printf("Warning: obs_mode '%s' not recognized, defaulting to SEARCH.\n",
               obs_mode);
        return (search);
    }
    return (search);
}

int psrfits_create(struct psrfits *pf)
{
    int itmp, *status;
    long double ldtmp;
    double dtmp;
    char ctmp[40];
    struct hdrinfo *hdr;

    hdr = &(pf->hdr);           // dereference the ptr to the header struct
    status = &(pf->status);     // dereference the ptr to the CFITSIO status

    // Figure out what mode this is 
    int mode = 0;
    mode = psrfits_obs_mode(hdr->obs_mode);
    if (mode == fold) {
        if (hdr->onlyI)
            printf("Warning!  In folding mode and ONLY_I is set!\n");
        if (hdr->ds_time_fact > 1)
            printf("Warning!  In folding mode and DS_TIME is > 1!\n");
        if (hdr->ds_freq_fact > 1)
            printf("Warning!  In folding mode and DS_FREQ is > 1!\n");
    }
    // Initialize the key variables if needed
    if (pf->filenum == 0) {     // first time writing to the file
        pf->status = 0;
        pf->tot_rows = 0;
        pf->N = 0L;
        pf->T = 0.0;
        hdr->offset_subint = 0;
        pf->mode = 'w';

        // Create the output directory if needed
        char datadir[1024];
        strncpy(datadir, pf->basefilename, 1023);
        char *last_slash = strrchr(datadir, '/');
        if (last_slash != NULL && last_slash != datadir) {
            *last_slash = '\0';
            printf("Using directory '%s' for output.\n", datadir);
            char cmd[1024];
            sprintf(cmd, "mkdir -m 1777 -p %s", datadir);
            system(cmd);
        }
    }
    pf->filenum++;
    pf->rownum = 1;
    hdr->offset_subint = pf->tot_rows;

    // Update the filename - don't include filenum for fold mode
    // TODO : use rf/cf extensions for psr/cals?
    if (mode == fold && pf->multifile != 1)
        sprintf(pf->filename, "%s.fits", pf->basefilename);
    else
        sprintf(pf->filename, "%s_%04d.fits", pf->basefilename, pf->filenum);

    // Create basic FITS file from our template
    char template_file[1024];
    printf("Opening file '%s' ", pf->filename);
    if (mode == search) {
        printf("in search mode.\n");
        sprintf(template_file, "%s/%s", SRCDIR, PSRFITS_SEARCH_TEMPLATE);
    }
    fits_create_template(&(pf->fptr), pf->filename, template_file, status);

    // Check to see if file was successfully created
    if (*status) {
        fprintf(stderr, "Error creating psrfits file from template.\n");
        fits_report_error(stderr, *status);
        exit(1);
    }
    // Go to the primary HDU
    fits_movabs_hdu(pf->fptr, 1, NULL, status);

    // Update the keywords that need it
    fits_get_system_time(ctmp, &itmp, status);
    // Note:  this is the date the file was _written_, not the obs start date
    fits_update_key(pf->fptr, TSTRING, "DATE", ctmp, NULL, status);
    fits_update_key(pf->fptr, TSTRING, "TELESCOP", hdr->telescope, NULL, status);
    fits_update_key(pf->fptr, TSTRING, "OBSERVER", hdr->observer, NULL, status);
    fits_update_key(pf->fptr, TSTRING, "PROJID", hdr->project_id, NULL, status);
    fits_update_key(pf->fptr, TSTRING, "FRONTEND", hdr->frontend, NULL, status);
    fits_update_key(pf->fptr, TSTRING, "BACKEND", hdr->backend, NULL, status);
    if (hdr->onlyI || hdr->summed_polns) {
        if (!hdr->onlyI && hdr->npol > 1) {
            printf("Warning!:  Can't have %d polarizations _and_ be summed!\n",
                   hdr->npol);
        }
        itmp = 2;
        fits_update_key(pf->fptr, TINT, "NRCVR", &itmp, NULL, status);
    } else {
        if (hdr->npol > 2) {    // Can't have more than 2 real polns (i.e. NRCVR)
            itmp = 2;
            fits_update_key(pf->fptr, TINT, "NRCVR", &itmp, NULL, status);
        } else {
            fits_update_key(pf->fptr, TINT, "NRCVR", &(hdr->npol), NULL, status);
        }
    }
    fits_update_key(pf->fptr, TSTRING, "FD_POLN", hdr->poln_type, NULL, status);
    fits_update_key(pf->fptr, TINT, "FD_HAND", &(hdr->fd_hand), NULL, status);
    fits_update_key(pf->fptr, TDOUBLE, "FD_SANG", &(hdr->fd_sang), NULL, status);
    fits_update_key(pf->fptr, TDOUBLE, "FD_XYPH", &(hdr->fd_xyph), NULL, status);
    fits_update_key(pf->fptr, TINT, "BE_PHASE", &(hdr->be_phase), NULL, status);
    fits_update_key(pf->fptr, TSTRING, "DATE-OBS", hdr->date_obs, NULL, status);
    if (mode == fold && !strcmp("CAL", hdr->obs_mode))
        fits_update_key(pf->fptr, TSTRING, "OBS_MODE", hdr->obs_mode, NULL, status);
    fits_update_key(pf->fptr, TDOUBLE, "OBSFREQ", &(hdr->fctr), NULL, status);
    fits_update_key(pf->fptr, TDOUBLE, "OBSBW", &(hdr->BW), NULL, status);
    fits_update_key(pf->fptr, TINT, "OBSNCHAN", &(hdr->orig_nchan), NULL, status);
    fits_update_key(pf->fptr, TDOUBLE, "CHAN_DM", &(hdr->chan_dm), NULL, status);
    fits_update_key(pf->fptr, TSTRING, "SRC_NAME", hdr->source, NULL, status);
    if (!strcmp("UNKNOWN", hdr->track_mode)) {
        printf("Warning!:  Unknown telescope tracking mode!\n");
    }
    fits_update_key(pf->fptr, TSTRING, "TRK_MODE", hdr->track_mode, NULL, status);
    // TODO: will need to change the following if we aren't tracking!
    fits_update_key(pf->fptr, TSTRING, "RA", hdr->ra_str, NULL, status);
    fits_update_key(pf->fptr, TSTRING, "DEC", hdr->dec_str, NULL, status);
    fits_update_key(pf->fptr, TSTRING, "STT_CRD1", hdr->ra_str, NULL, status);
    fits_update_key(pf->fptr, TSTRING, "STP_CRD1", hdr->ra_str, NULL, status);
    // TODO: update these at the end of the file or obs
    fits_update_key(pf->fptr, TSTRING, "STT_CRD2", hdr->dec_str, NULL, status);
    fits_update_key(pf->fptr, TSTRING, "STP_CRD2", hdr->dec_str, NULL, status);
    fits_update_key(pf->fptr, TDOUBLE, "BMAJ", &(hdr->beam_FWHM), NULL, status);
    fits_update_key(pf->fptr, TDOUBLE, "BMIN", &(hdr->beam_FWHM), NULL, status);
    if (strcmp("OFF", hdr->cal_mode)) {
        fits_update_key(pf->fptr, TDOUBLE, "CAL_FREQ", &(hdr->cal_freq), NULL,
                        status);
        fits_update_key(pf->fptr, TDOUBLE, "CAL_DCYC", &(hdr->cal_dcyc), NULL,
                        status);
        fits_update_key(pf->fptr, TDOUBLE, "CAL_PHS", &(hdr->cal_phs), NULL, status);
    }
    fits_update_key(pf->fptr, TDOUBLE, "SCANLEN", &(hdr->scanlen), NULL, status);
    itmp = (int) hdr->MJD_epoch;
    fits_update_key(pf->fptr, TINT, "STT_IMJD", &itmp, NULL, status);
    ldtmp = (hdr->MJD_epoch - (long double) itmp) * 86400.0L;   // in sec
    itmp = (int) ldtmp;
    fits_update_key(pf->fptr, TINT, "STT_SMJD", &itmp, NULL, status);
    ldtmp -= (long double) itmp;
    dtmp = (double) ldtmp;
    fits_update_key(pf->fptr, TDOUBLE, "STT_OFFS", &dtmp, NULL, status);
    fits_update_key(pf->fptr, TDOUBLE, "STT_LST", &(hdr->start_lst), NULL, status);

    // Go to the SUBINT HDU
    fits_movnam_hdu(pf->fptr, BINARY_TBL, "SUBINT", 0, status);

    // Update the keywords that need it
    if (hdr->onlyI) {
        itmp = 1;
        fits_update_key(pf->fptr, TINT, "NPOL", &itmp, NULL, status);
    } else {
        fits_update_key(pf->fptr, TINT, "NPOL", &(hdr->npol), NULL, status);
    }
    if (!hdr->onlyI && !hdr->summed_polns) {
        // TODO:  These need to be updated for the real machine.
        if (hdr->npol == 1)
            strcpy(ctmp, "AA");
        else if (hdr->npol == 2)
            strcpy(ctmp, "AABB");
        else if (hdr->npol == 4)
            strcpy(ctmp, "IQUV");
        fits_update_key(pf->fptr, TSTRING, "POL_TYPE", ctmp, NULL, status);
    } else {
        fits_update_key(pf->fptr, TSTRING, "POL_TYPE", "AA+BB", NULL, status);
    }
    // TODO what does TBIN mean in fold mode?
    dtmp = hdr->dt * hdr->ds_time_fact;
    fits_update_key(pf->fptr, TDOUBLE, "TBIN", &dtmp, NULL, status);
    fits_update_key(pf->fptr, TINT, "NSUBOFFS", &(hdr->offset_subint), NULL, status);
    itmp = hdr->nchan / hdr->ds_freq_fact;
    fits_update_key(pf->fptr, TINT, "NCHAN", &itmp, NULL, status);
    dtmp = hdr->df * hdr->ds_freq_fact;
    fits_update_key(pf->fptr, TDOUBLE, "CHAN_BW", &dtmp, NULL, status);
    if (mode == search) {
        int out_nsblk = hdr->nsblk / hdr->ds_time_fact;
        itmp = 1;
        fits_update_key(pf->fptr, TINT, "NSBLK", &out_nsblk, NULL, status);
        fits_update_key(pf->fptr, TINT, "NBITS", &(hdr->nbits), NULL, status);
        fits_update_key(pf->fptr, TINT, "NBIN", &itmp, NULL, status);
    } else if (mode == fold) {
        itmp = 1;
        fits_update_key(pf->fptr, TINT, "NSBLK", &itmp, NULL, status);
        fits_update_key(pf->fptr, TINT, "NBITS", &itmp, NULL, status);
        fits_update_key(pf->fptr, TINT, "NBIN", &(hdr->nbin), NULL, status);
        fits_update_key(pf->fptr, TSTRING, "EPOCHS", "MIDTIME", NULL, status);
    }
    // Update the column sizes for the colums containing arrays
    {
        int out_npol = hdr->npol;
        int out_nchan = hdr->nchan / hdr->ds_freq_fact;
        if (hdr->onlyI)
            out_npol = 1;
        int out_nsblk = hdr->nsblk / hdr->ds_time_fact;

        fits_modify_vector_len(pf->fptr, 13, out_nchan, status);        // DAT_FREQ
        fits_modify_vector_len(pf->fptr, 14, out_nchan, status);        // DAT_WTS
        itmp = out_nchan * out_npol;
        fits_modify_vector_len(pf->fptr, 15, itmp, status);     // DAT_OFFS
        fits_modify_vector_len(pf->fptr, 16, itmp, status);     // DAT_SCL

        if (mode == search)
            itmp = (hdr->nbits * out_nchan * out_npol * out_nsblk) / 8;
        else if (mode == fold)
            itmp = (hdr->nbin * out_nchan * out_npol);
        fits_modify_vector_len(pf->fptr, 17, itmp, status);     // DATA
        // Update the TDIM field for the data column
        if (mode == search)
            sprintf(ctmp, "(1,%d,%d,%d)", out_nchan, out_npol, out_nsblk);
        else if (mode == fold)
            sprintf(ctmp, "(%d,%d,%d,1)", hdr->nbin, out_nchan, out_npol);
        fits_update_key(pf->fptr, TSTRING, "TDIM17", ctmp, NULL, status);
    }

    fits_flush_file(pf->fptr, status);

    return *status;
}


int psrfits_write_subint(struct psrfits *pf)
{
    int row, *status, nchan, nivals, mode, out_nbytes, nstat, dummy;
    float ftmp;
    struct hdrinfo *hdr;
    struct subint *sub;

    hdr = &(pf->hdr);           // dereference the ptr to the header struct
    sub = &(pf->sub);           // dereference the ptr to the subint struct
    status = &(pf->status);     // dereference the ptr to the CFITSIO status
    nchan = hdr->nchan / hdr->ds_freq_fact;
    nstat = sub->statbytes_per_subint / 2;      //stat array is shorts
    if (hdr->onlyI)
        nivals = nchan;
    else
        nivals = nchan * hdr->npol;
    mode = psrfits_obs_mode(hdr->obs_mode);
    if (mode == fold)
        out_nbytes = sub->bytes_per_subint / hdr->ds_freq_fact;
    else {
        out_nbytes = sub->bytes_per_subint / (hdr->ds_freq_fact * hdr->ds_time_fact);
        if (hdr->onlyI)
            out_nbytes /= hdr->npol;
    }

    // Create the initial file or change to a new one if needed.
    // Stay with a single file for fold mode.

    //fprintf(stderr,"In psrfits write, pf->filenum: %d pf->multifile: %d\n",pf->filenum,pf->multifile);
    //fprintf(stderr,"In psrfits write, pf->rownum %d pf->rows_per_file: %d\n",pf->rownum,pf->rows_per_file);

    /*
       if (pf->filenum==0 || 
       ( (mode==search || pf->multifile==1) 
       && pf->rownum > pf->rows_per_file)) {
       if (pf->filenum) {
       printf("Closing file '%s'\n", pf->filename);
       fits_close_file(pf->fptr, status);
       return *status;
       }
       psrfits_create(pf);
       }
     */

    row = pf->rownum;
    //fprintf(stderr,"In psrfits write, row: %d\n",row);

    //fits_read_key(pf->fptr, TINT, "NAXIS2", &dummy, NULL, status);
    //fprintf(stderr,"In psrfits write, before col writing, NAXIS2: %d status; %d\n",dummy,*status);

    fprintf(stderr, "tsubint: %f  offs_sub: %f\n", sub->tsubint, sub->offs);

    fits_write_col(pf->fptr, TDOUBLE, 1, row, 1, 1, &(sub->tsubint), status);
    fits_write_col(pf->fptr, TDOUBLE, 2, row, 1, 1, &(sub->offs), status);
    fits_write_col(pf->fptr, TDOUBLE, 3, row, 1, 1, &(sub->lst), status);
    fits_write_col(pf->fptr, TDOUBLE, 4, row, 1, 1, &(sub->ra), status);
    fits_write_col(pf->fptr, TDOUBLE, 5, row, 1, 1, &(sub->dec), status);
    fits_write_col(pf->fptr, TDOUBLE, 6, row, 1, 1, &(sub->glon), status);
    fits_write_col(pf->fptr, TDOUBLE, 7, row, 1, 1, &(sub->glat), status);
    ftmp = (float) sub->feed_ang;
    fits_write_col(pf->fptr, TFLOAT, 8, row, 1, 1, &ftmp, status);
    ftmp = (float) sub->pos_ang;
    fits_write_col(pf->fptr, TFLOAT, 9, row, 1, 1, &ftmp, status);
    ftmp = (float) sub->par_ang;
    fits_write_col(pf->fptr, TFLOAT, 10, row, 1, 1, &ftmp, status);
    ftmp = (float) sub->tel_az;
    fits_write_col(pf->fptr, TFLOAT, 11, row, 1, 1, &ftmp, status);
    ftmp = (float) sub->tel_zen;
    fits_write_col(pf->fptr, TFLOAT, 12, row, 1, 1, &ftmp, status);
    fits_write_col(pf->fptr, TFLOAT, 13, row, 1, nchan, sub->dat_freqs, status);
    fits_write_col(pf->fptr, TFLOAT, 14, row, 1, nchan, sub->dat_weights, status);
    fits_write_col(pf->fptr, TFLOAT, 15, row, 1, nivals, sub->dat_offsets, status);
    fits_write_col(pf->fptr, TFLOAT, 16, row, 1, nivals, sub->dat_scales, status);

    if (mode == search) {
        // Need to change this for other data types...
        //fprintf(stderr,"In psrfits write, out_nbytes (data bytes per subint): %d\n",out_nbytes);
        fits_write_col(pf->fptr, TBYTE, 17, row, 1, out_nbytes, sub->data, status);
    } else if (mode == fold) {
        // Fold mode writes floats for now..
        fits_write_col(pf->fptr, TFLOAT, 17, row, 1, out_nbytes / sizeof(float),
                       sub->data, status);
    }

    /*
       fprintf(stderr,"In psrfits write, out_nbytes: %d, nchan: %d nivals: %d nstat: %d status: %d\n",out_nbytes,nchan,nivals,nstat,*status);
       fits_flush_file(pf->fptr, status);
       fits_read_key(pf->fptr, TINT, "NAXIS2", &dummy, NULL, status);
       fprintf(stderr,"In psrfits write after 17 column, NAXIS2: %d status; %d\n",dummy,*status);
     */

    fits_write_col(pf->fptr, TSHORT, 18, row, 1, nstat, sub->stat, status);

    // Flush the buffers if not finished with the file
    // Note:  this use is not entirely in keeping with the CFITSIO
    //        documentation recommendations.  However, manually 
    //        correcting NAXIS2 and using fits_flush_buffer()
    //        caused occasional hangs (and extrememly large
    //        files due to some infinite loop).
    fits_flush_file(pf->fptr, status);

    //NOTE: using flush_file resulted in an extra row for some reason

    //fits_flush_buffer(pf->fptr,0,status);

    //fits_read_key(pf->fptr, TINT, "NAXIS2", &dummy, NULL, status);
    //fprintf(stderr,"In psrfits write, after fits flush, NAXIS2: %d status; %d\n",dummy,*status);

    // Print status if bad
    if (*status) {
        fprintf(stderr, "Error writing subint %d:\n", pf->rownum);
        fits_report_error(stderr, *status);
        fflush(stderr);
    }
    // Now update some key values if no CFITSIO errors
    if (!(*status)) {
        pf->rownum++;
        pf->tot_rows++;
        pf->N += hdr->nsblk / hdr->ds_time_fact;
        pf->T += sub->tsubint;

        // For fold mode, print info each subint written
        if (mode == fold && pf->quiet != 1) {
            printf("Wrote subint %d (total time %.1fs)\n", pf->rownum - 1, pf->T);
            fflush(stdout);
        }

    }
    //fits_update_key(pf->fptr,TINT,"NAXIS2",&(pf->tot_rows),NULL,status);

    return *status;
}


int psrfits_close(struct psrfits *pf)
{
    if (!pf->status) {
        fits_close_file(pf->fptr, &(pf->status));
        printf("Closing file '%s'\n", pf->filename);
    }
    printf("Done.  %s %d subints (%f sec) in %d files (status = %d).\n",
           pf->mode == 'r' ? "Read" : "Wrote",
           pf->tot_rows, pf->T, pf->filenum, pf->status);
    return pf->status;
}
