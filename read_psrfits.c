/* read_psrfits.c 
 * Paul Demorest, 05/2008
 */
#include <stdio.h>
#include <string.h>
#include "psrfits.h"

/* This function is similar to psrfits_create, except it
 * deals with reading existing files.  It is assumed that
 * basename and filenum are filled in correctly to point to 
 * the first file in the set.
 */
int psrfits_open(struct psrfits *pf, int iomode) {

  int itmp;
  double dtmp;
  char ctmp[256];

  struct hdrinfo *hdr = &(pf->hdr);
  struct subint  *sub = &(pf->sub);

  int *status = &(pf->status);

  sprintf(pf->filename, "%s%0*d.fits", pf->basefilename,pf->fnamedigits,
	  pf->filenum);
  //fprintf(stderr,"%s\n",pf->filename);
  fits_open_file(&(pf->fptr), pf->filename, iomode, status);

  // If file does not exist, exit now
  if (*status) { return *status; }
  fprintf(stderr,"Opened file '%s'\n", pf->filename);

  // Move to main HDU
  fits_movabs_hdu(pf->fptr, 1, NULL, status);

  // Figure out obs mode
  fits_read_key(pf->fptr, TSTRING, "OBS_MODE", hdr->obs_mode, NULL, status);
  int mode = psrfits_obs_mode(hdr->obs_mode);

  // Read some stuff
  fits_read_key(pf->fptr, TSTRING, "TELESCOP", hdr->telescope, NULL, status);
  fits_read_key(pf->fptr, TSTRING, "OBSERVER", hdr->observer, NULL, status);
  fits_read_key(pf->fptr, TSTRING, "PROJID", hdr->project_id, NULL, status);
  fits_read_key(pf->fptr, TSTRING, "FRONTEND", hdr->frontend, NULL, status);
  fits_read_key(pf->fptr, TSTRING, "BACKEND", hdr->backend, NULL, status);
  fits_read_key(pf->fptr, TSTRING, "FD_POLN", hdr->poln_type, NULL, status);
  fits_read_key(pf->fptr, TSTRING, "DATE-OBS", hdr->date_obs, NULL, status);
  fits_read_key(pf->fptr, TDOUBLE, "OBSFREQ", &(hdr->fctr), NULL, status);
  fits_read_key(pf->fptr, TDOUBLE, "OBSBW", &(hdr->BW), NULL, status);
  fits_read_key(pf->fptr, TINT, "OBSNCHAN", &(hdr->orig_nchan), NULL, status);
  fits_read_key(pf->fptr, TSTRING, "SRC_NAME", hdr->source, NULL, status);
  fits_read_key(pf->fptr, TSTRING, "TRK_MODE", hdr->track_mode, NULL, status);
  // TODO warn if not TRACK?
  fits_read_key(pf->fptr, TSTRING, "RA", hdr->ra_str, NULL, status);
  fits_read_key(pf->fptr, TSTRING, "DEC", hdr->dec_str, NULL, status);
  fits_read_key(pf->fptr, TDOUBLE, "BMAJ", &(hdr->beam_FWHM), NULL, status);
  fits_read_key(pf->fptr, TSTRING, "CAL_MODE", hdr->cal_mode, NULL, status);
  fits_read_key(pf->fptr, TDOUBLE, "CAL_FREQ", &(hdr->cal_freq), NULL, 
		status);
  fits_read_key(pf->fptr, TDOUBLE, "CAL_DCYC", &(hdr->cal_dcyc), NULL, 
		status);
  fits_read_key(pf->fptr, TDOUBLE, "CAL_PHS", &(hdr->cal_phs), NULL, status);
  fits_read_key(pf->fptr, TSTRING, "FD_MODE", hdr->feed_mode, NULL, status);
  fits_read_key(pf->fptr, TDOUBLE, "FA_REQ", &(hdr->feed_angle), NULL, 
		status);
  fits_read_key(pf->fptr, TDOUBLE, "SCANLEN", &(hdr->scanlen), NULL, status);

  fits_read_key(pf->fptr, TINT, "STT_IMJD", &itmp, NULL, status);
  hdr->MJD_epoch = (long double)itmp;
  fits_read_key(pf->fptr, TDOUBLE, "STT_SMJD", &dtmp, NULL, status);
  hdr->MJD_epoch += dtmp/86400.0L;
  fits_read_key(pf->fptr, TDOUBLE, "STT_OFFS", &dtmp, NULL, status);
  hdr->MJD_epoch += dtmp/86400.0L;

  fits_read_key(pf->fptr, TDOUBLE, "STT_LST", &(hdr->start_lst), NULL, 
		status);

  // Move to pdev bintab to see if blanking enabled
  //
  int	blankSel,adcThr;
  fits_movnam_hdu(pf->fptr, BINARY_TBL, "PDEV", 0, status);
  fits_read_key(pf->fptr, TINT, "PHBLKSEL", &(blankSel), NULL, status);
  fits_read_key(pf->fptr, TINT, "PHADCTHR", &(adcThr), NULL, status);
  fits_read_key(pf->fptr, TINT, "PHFFTACC", &(hdr->fftAccum), NULL, status);
  hdr->blankingOn=(blankSel != 15) || ( adcThr != 65535);

  // Move to first subint
  fits_movnam_hdu(pf->fptr, BINARY_TBL, "SUBINT", 0, status);

  // Read some more stuff
  fits_read_key(pf->fptr, TINT, "NPOL", &(hdr->npol), NULL, status);
  fits_read_key(pf->fptr, TSTRING, "POL_TYPE", ctmp, NULL, status);
  if (strncmp(ctmp, "AA+BB", 6)==0) hdr->summed_polns=1;
  else hdr->summed_polns=0;
  fits_read_key(pf->fptr, TDOUBLE, "TBIN", &(hdr->dt), NULL, status);
  fits_read_key(pf->fptr, TINT, "NBIN", &(hdr->nbin), NULL, status);
  fits_read_key(pf->fptr, TINT, "NSUBOFFS", &(hdr->offset_subint), NULL, 
		status);
  fits_read_key(pf->fptr, TINT, "NCHAN", &(hdr->nchan), NULL, status);
  fits_read_key(pf->fptr, TDOUBLE, "CHAN_BW", &(hdr->df), NULL, status);
  fits_read_key(pf->fptr, TINT, "NSBLK", &(hdr->nsblk), NULL, status);
  fits_read_key(pf->fptr, TINT, "NBITS", &(hdr->nbits), NULL, status);

  if (mode==SEARCH_MODE) {
    sub->bytes_per_subint = 
      (hdr->nbits * hdr->nchan * hdr->npol * hdr->nsblk) / 8;
    sub->statbytes_per_subint=hdr->nsblk*AO_NUM_SH_STAT_1DMP*sizeof(short);
  } else if (mode==FOLD_MODE) {
    sub->bytes_per_subint = 
      (hdr->nbin * hdr->nchan * hdr->npol)*hdr->nbits/8;//XXX data type??
  }
  //  to tell us the end of the good data in file.
  pf->hdr.numBlksTot =(pf->hdr.scanlen/pf->hdr.dt + .5);
  // Init counters
  pf->rownum = 1;
      
  fits_read_key(pf->fptr, TINT, "NAXIS2", &(pf->rows_per_file), NULL, status);
  if (!pf->initialized) {
    fits_get_colnum(pf->fptr,1,"TSUBINT",&pf->subcols.tsubint,status);
    fits_get_colnum(pf->fptr,1,"OFFS_SUB",&pf->subcols.offs_sub,status);
    fits_get_colnum(pf->fptr,1,"LST_SUB",&pf->subcols.lst_sub,status);
    fits_get_colnum(pf->fptr,1,"RA_SUB",&pf->subcols.ra_sub,status);
    fits_get_colnum(pf->fptr,1,"DEC_SUB",&pf->subcols.dec_sub,status);
    fits_get_colnum(pf->fptr,1,"GLON_SUB",&pf->subcols.glon_sub,status);
    fits_get_colnum(pf->fptr,1,"GLAT_SUB",&pf->subcols.glat_sub,status);
    fits_get_colnum(pf->fptr,1,"FD_ANG",&pf->subcols.fd_ang,status);
    fits_get_colnum(pf->fptr,1,"POS_ANG",&pf->subcols.pos_ang,status);
    fits_get_colnum(pf->fptr,1,"PAR_ANG",&pf->subcols.par_ang,status);
    fits_get_colnum(pf->fptr,1,"TEL_AZ",&pf->subcols.tel_az,status);
    fits_get_colnum(pf->fptr,1,"TEL_ZEN",&pf->subcols.tel_zen,status);
    fits_get_colnum(pf->fptr,1,"DAT_FREQ",&pf->subcols.dat_freq,status);
    fits_get_colnum(pf->fptr,1,"DAT_WTS",&pf->subcols.dat_wts,status);
    fits_get_colnum(pf->fptr,1,"DAT_OFFS",&pf->subcols.dat_offs,status);
    fits_get_colnum(pf->fptr,1,"DAT_SCL",&pf->subcols.dat_scl,status);
    fits_get_colnum(pf->fptr,1,"DATA",&pf->subcols.data,status);
    fits_get_colnum(pf->fptr,1,"STAT",&pf->subcols.stat,status);
		
    long repeat;
    long width;
    fits_get_coltype(pf->fptr,pf->subcols.data,&sub->typecode,&repeat,
		     &width,status);
    sub->bytesPerDatum=width;
    if (sub->typecode == TINT32BIT) sub->typecode=TINT;
    sub->data=malloc(sizeof(char)*sub->bytes_per_subint);
    sub->dataBytesAlloced=sub->bytes_per_subint;
    sub->stat=(unsigned short *)malloc(sub->statbytes_per_subint);
    pf->initialized=1;
  }
  return *status;
}
/***********************************************************************************
 *  psrfits_subint(struct *psrfits *pf,int first)
 *  first: if true then first call, we need to read all the keys in the subint row.
 *         after the first we just read the data and the status.
 *
 * Read next subint from the set of files described
 * by the psrfits struct.  It is assumed that all files
 * form a consistent set.  Read automatically goes to the
 * next file when one ends.  Arrays should be allocated
 * outside this routine.
 */
int psrfits_read_subint(struct psrfits *pf,int first) {

  struct hdrinfo *hdr = &(pf->hdr);
  struct subint  *sub = &(pf->sub);
  int *status = &(pf->status);

  // See if we need to move to next file
  /*
  if (pf->rownum > pf->rows_per_file) {
    fits_close_file(pf->fptr, status);
    fprintf(stderr,"Closed file '%s'\n", pf->filename);
    pf->filenum++;
    if (psrfits_open(pf) != 0) {
      return *status;
    }
  }
  */

  if (pf->rownum > pf->rows_per_file)
    return 1;
    

  int mode = psrfits_obs_mode(hdr->obs_mode);
  //   match the args for the cfitsio params
  LONGLONG nchan = hdr->nchan;
  LONGLONG nivals= hdr->nchan * hdr->npol;
  LONGLONG row   = pf->rownum;
  LONGLONG firstE=1;		// first element of row
  LONGLONG oneE  =1;   // one element in row
  SUBINT_COLS *pcol;
  pcol=&pf->subcols;

  //Need to read all columns at every subint in order to copy the fields to
  //converted psrfits data output
  //  if (first) { 
    // modified to use column names. pjp 10jan09 
    fits_read_col(pf->fptr, TDOUBLE, pcol->tsubint, row, firstE,oneE, NULL, &(sub->tsubint), NULL, status);
    fits_read_col(pf->fptr, TDOUBLE, pcol->offs_sub,row,firstE,oneE, NULL, &(sub->offs), NULL, status);
    fits_read_col(pf->fptr, TDOUBLE, pcol->lst_sub, row, firstE,oneE, NULL, &(sub->lst), NULL, status);
    fits_read_col(pf->fptr, TDOUBLE, pcol->ra_sub , row, firstE,oneE, NULL, &(sub->ra), NULL, status);
    fits_read_col(pf->fptr, TDOUBLE, pcol->dec_sub, row, firstE,oneE, NULL, &(sub->dec), NULL, status);
    fits_read_col(pf->fptr, TDOUBLE, pcol->glon_sub, row, firstE,oneE, NULL, &(sub->glon), NULL, status);
    fits_read_col(pf->fptr, TDOUBLE, pcol->glat_sub, row, firstE,oneE, NULL, &(sub->glat), NULL, status);
    fits_read_col(pf->fptr, TDOUBLE, pcol->fd_ang, row, firstE,oneE, NULL, &(sub->feed_ang), NULL, status);
    fits_read_col(pf->fptr, TDOUBLE, pcol->pos_ang, row, firstE,oneE, NULL, &(sub->pos_ang), NULL, status);
    fits_read_col(pf->fptr, TDOUBLE, pcol->par_ang, row, firstE,oneE, NULL, &(sub->par_ang), NULL, status);
    fits_read_col(pf->fptr, TDOUBLE, pcol->tel_az, row, firstE,oneE, NULL, &(sub->tel_az), NULL, status);
    fits_read_col(pf->fptr, TDOUBLE, pcol->tel_zen, row, firstE,oneE, NULL, &(sub->tel_zen), NULL, status);
    fits_read_col(pf->fptr, TFLOAT, pcol->dat_freq, row, firstE, nchan, NULL, sub->dat_freqs, NULL, status);
    fits_read_col(pf->fptr, TFLOAT, pcol->dat_wts, row, firstE, nchan, NULL, sub->dat_weights, NULL, status);
    fits_read_col(pf->fptr, TFLOAT, pcol->dat_offs, row, firstE, nivals, NULL, sub->dat_offsets, NULL, status);
    fits_read_col(pf->fptr, TFLOAT, pcol->dat_scl, row, firstE, nivals, NULL, sub->dat_scales, NULL, status);
    //  }

  if (mode==SEARCH_MODE) {
    switch (sub->typecode) {
    case TBYTE:
      fits_read_col(pf->fptr, sub->typecode, pcol->data, row, firstE, (LONGLONG)(sub->bytes_per_subint)/sub->bytesPerDatum,NULL, sub->data, NULL, status);
      break;
    case TSHORT:
      fits_read_col(pf->fptr, sub->typecode, pcol->data, row, firstE, (LONGLONG)(sub->bytes_per_subint)/sub->bytesPerDatum,NULL,(short*)sub->data, NULL, status);
      break;
    case TINT:
      fits_read_col(pf->fptr, sub->typecode, pcol->data, row, firstE, (LONGLONG)(sub->bytes_per_subint)/sub->bytesPerDatum,NULL,(int*)sub->data, NULL, status);
      break;
    case TFLOAT:
      fits_read_col(pf->fptr, sub->typecode, pcol->data, row, firstE, (LONGLONG)(sub->bytes_per_subint)/sub->bytesPerDatum,NULL,(float*)sub->data, NULL, status);
      break;
    case TDOUBLE:
      fits_read_col(pf->fptr, sub->typecode, pcol->data, row, firstE, (LONGLONG)(sub->bytes_per_subint)/sub->bytesPerDatum,NULL,(double*)sub->data, NULL, status);
      break;
    default:
      fprintf(stderr,"psrfits_read_subint:unsupported datatype code:%d\n",sub->typecode);
      fprintf(stderr,"Currently supported:TBYTE=11,TSHORT=21,TINT=31,TFLOAT=32,TDOUBLE=82\n");
      fprintf(stderr,"..see fitsio.h include file for the definitions\n");
      exit(-1);
      break;
    }
    fits_read_col(pf->fptr, TSHORT, pcol->stat, row, firstE, (LONGLONG)(sub->statbytes_per_subint)/sizeof(short), NULL, sub->stat, NULL, status);
  } else if (mode==FOLD_MODE) {
    fits_read_col(pf->fptr, sub->typecode, pcol->data, row, firstE, (LONGLONG)(sub->bytes_per_subint)/sub->bytesPerDatum, NULL, sub->data, NULL, status);
  }
  // Complain on error
  fits_report_error(stderr, *status);

  // Update counters
  if (!(*status)) {
    pf->rownum++;
    pf->tot_rows++;

    pf->N += hdr->nsblk;
    pf->T = pf->N * hdr->dt;
  }

  return *status;
}
