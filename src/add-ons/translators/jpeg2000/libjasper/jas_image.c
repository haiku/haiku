/*
 * Copyright (c) 1999-2000 Image Power, Inc. and the University of
 *   British Columbia.
 * Copyright (c) 2001-2002 Michael David Adams.
 * All rights reserved.
 */

/* __START_OF_JASPER_LICENSE__
 * 
 * JasPer Software License
 * 
 * IMAGE POWER JPEG-2000 PUBLIC LICENSE
 * ************************************
 * 
 * GRANT:
 * 
 * Permission is hereby granted, free of charge, to any person (the "User")
 * obtaining a copy of this software and associated documentation, to deal
 * in the JasPer Software without restriction, including without limitation
 * the right to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the JasPer Software (in source and binary forms),
 * and to permit persons to whom the JasPer Software is furnished to do so,
 * provided further that the License Conditions below are met.
 * 
 * License Conditions
 * ******************
 * 
 * A.  Redistributions of source code must retain the above copyright notice,
 * and this list of conditions, and the following disclaimer.
 * 
 * B.  Redistributions in binary form must reproduce the above copyright
 * notice, and this list of conditions, and the following disclaimer in
 * the documentation and/or other materials provided with the distribution.
 * 
 * C.  Neither the name of Image Power, Inc. nor any other contributor
 * (including, but not limited to, the University of British Columbia and
 * Michael David Adams) may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * 
 * D.  User agrees that it shall not commence any action against Image Power,
 * Inc., the University of British Columbia, Michael David Adams, or any
 * other contributors (collectively "Licensors") for infringement of any
 * intellectual property rights ("IPR") held by the User in respect of any
 * technology that User owns or has a right to license or sublicense and
 * which is an element required in order to claim compliance with ISO/IEC
 * 15444-1 (i.e., JPEG-2000 Part 1).  "IPR" means all intellectual property
 * rights worldwide arising under statutory or common law, and whether
 * or not perfected, including, without limitation, all (i) patents and
 * patent applications owned or licensable by User; (ii) rights associated
 * with works of authorship including copyrights, copyright applications,
 * copyright registrations, mask work rights, mask work applications,
 * mask work registrations; (iii) rights relating to the protection of
 * trade secrets and confidential information; (iv) any right analogous
 * to those set forth in subsections (i), (ii), or (iii) and any other
 * proprietary rights relating to intangible property (other than trademark,
 * trade dress, or service mark rights); and (v) divisions, continuations,
 * renewals, reissues and extensions of the foregoing (as and to the extent
 * applicable) now existing, hereafter filed, issued or acquired.
 * 
 * E.  If User commences an infringement action against any Licensor(s) then
 * such Licensor(s) shall have the right to terminate User's license and
 * all sublicenses that have been granted hereunder by User to other parties.
 * 
 * F.  This software is for use only in hardware or software products that
 * are compliant with ISO/IEC 15444-1 (i.e., JPEG-2000 Part 1).  No license
 * or right to this Software is granted for products that do not comply
 * with ISO/IEC 15444-1.  The JPEG-2000 Part 1 standard can be purchased
 * from the ISO.
 * 
 * THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL PART OF THIS LICENSE.
 * NO USE OF THE JASPER SOFTWARE IS AUTHORIZED HEREUNDER EXCEPT UNDER
 * THIS DISCLAIMER.  THE JASPER SOFTWARE IS PROVIDED BY THE LICENSORS AND
 * CONTRIBUTORS UNDER THIS LICENSE ON AN ``AS-IS'' BASIS, WITHOUT WARRANTY
 * OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION,
 * WARRANTIES THAT THE JASPER SOFTWARE IS FREE OF DEFECTS, IS MERCHANTABLE,
 * IS FIT FOR A PARTICULAR PURPOSE OR IS NON-INFRINGING.  THOSE INTENDING
 * TO USE THE JASPER SOFTWARE OR MODIFICATIONS THEREOF FOR USE IN HARDWARE
 * OR SOFTWARE PRODUCTS ARE ADVISED THAT THEIR USE MAY INFRINGE EXISTING
 * PATENTS, COPYRIGHTS, TRADEMARKS, OR OTHER INTELLECTUAL PROPERTY RIGHTS.
 * THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE JASPER SOFTWARE
 * IS WITH THE USER.  SHOULD ANY PART OF THE JASPER SOFTWARE PROVE DEFECTIVE
 * IN ANY RESPECT, THE USER (AND NOT THE INITIAL DEVELOPERS, THE UNIVERSITY
 * OF BRITISH COLUMBIA, IMAGE POWER, INC., MICHAEL DAVID ADAMS, OR ANY
 * OTHER CONTRIBUTOR) SHALL ASSUME THE COST OF ANY NECESSARY SERVICING,
 * REPAIR OR CORRECTION.  UNDER NO CIRCUMSTANCES AND UNDER NO LEGAL THEORY,
 * WHETHER TORT (INCLUDING NEGLIGENCE), CONTRACT, OR OTHERWISE, SHALL THE
 * INITIAL DEVELOPER, THE UNIVERSITY OF BRITISH COLUMBIA, IMAGE POWER, INC.,
 * MICHAEL DAVID ADAMS, ANY OTHER CONTRIBUTOR, OR ANY DISTRIBUTOR OF THE
 * JASPER SOFTWARE, OR ANY SUPPLIER OF ANY OF SUCH PARTIES, BE LIABLE TO
 * THE USER OR ANY OTHER PERSON FOR ANY INDIRECT, SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES OF ANY CHARACTER INCLUDING, WITHOUT LIMITATION,
 * DAMAGES FOR LOSS OF GOODWILL, WORK STOPPAGE, COMPUTER FAILURE OR
 * MALFUNCTION, OR ANY AND ALL OTHER COMMERCIAL DAMAGES OR LOSSES, EVEN IF
 * SUCH PARTY HAD BEEN INFORMED, OR OUGHT TO HAVE KNOWN, OF THE POSSIBILITY
 * OF SUCH DAMAGES.  THE JASPER SOFTWARE AND UNDERLYING TECHNOLOGY ARE NOT
 * FAULT-TOLERANT AND ARE NOT DESIGNED, MANUFACTURED OR INTENDED FOR USE OR
 * RESALE AS ON-LINE CONTROL EQUIPMENT IN HAZARDOUS ENVIRONMENTS REQUIRING
 * FAIL-SAFE PERFORMANCE, SUCH AS IN THE OPERATION OF NUCLEAR FACILITIES,
 * AIRCRAFT NAVIGATION OR COMMUNICATION SYSTEMS, AIR TRAFFIC CONTROL, DIRECT
 * LIFE SUPPORT MACHINES, OR WEAPONS SYSTEMS, IN WHICH THE FAILURE OF THE
 * JASPER SOFTWARE OR UNDERLYING TECHNOLOGY OR PRODUCT COULD LEAD DIRECTLY
 * TO DEATH, PERSONAL INJURY, OR SEVERE PHYSICAL OR ENVIRONMENTAL DAMAGE
 * ("HIGH RISK ACTIVITIES").  LICENSOR SPECIFICALLY DISCLAIMS ANY EXPRESS
 * OR IMPLIED WARRANTY OF FITNESS FOR HIGH RISK ACTIVITIES.  USER WILL NOT
 * KNOWINGLY USE, DISTRIBUTE OR RESELL THE JASPER SOFTWARE OR UNDERLYING
 * TECHNOLOGY OR PRODUCTS FOR HIGH RISK ACTIVITIES AND WILL ENSURE THAT ITS
 * CUSTOMERS AND END-USERS OF ITS PRODUCTS ARE PROVIDED WITH A COPY OF THE
 * NOTICE SPECIFIED IN THIS SECTION.
 * 
 * __END_OF_JASPER_LICENSE__
 */

/*
 * Image Library
 *
 * $Id: jas_image.c 14449 2005-10-20 12:15:56Z stippi $
 */

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "jasper/jas_math.h"
#include "jasper/jas_image.h"
#include "jasper/jas_malloc.h"
#include "jasper/jas_string.h"

/******************************************************************************\
* Types.
\******************************************************************************/

/******************************************************************************\
* Local prototypes.
\******************************************************************************/

static void jas_image_cmpt_destroy(jas_image_cmpt_t *cmpt);
static jas_image_cmpt_t *jas_image_cmpt_create(uint_fast32_t tlx, uint_fast32_t tly,
  uint_fast32_t hstep, uint_fast32_t vstep, uint_fast32_t width, uint_fast32_t
  height, uint_fast16_t depth, JPR_BOOL sgnd, uint_fast32_t inmem);
static void jas_image_setbbox(jas_image_t *image);
static jas_image_cmpt_t *jas_image_cmpt_copy(jas_image_cmpt_t *cmpt);
static jas_image_cmpt_t *jas_image_cmpt_create0();
static int jas_image_growcmpts(jas_image_t *image, int maxcmpts);
static uint_fast32_t inttobits(jas_seqent_t v, int prec, JPR_BOOL sgnd);
static jas_seqent_t bitstoint(uint_fast32_t v, int prec, JPR_BOOL sgnd);

/******************************************************************************\
* Global data.
\******************************************************************************/

static int jas_image_numfmts = 0;
static jas_image_fmtinfo_t jas_image_fmtinfos[JAS_IMAGE_MAXFMTS];

/******************************************************************************\
* Create and destroy operations.
\******************************************************************************/

jas_image_t *jas_image_create(uint_fast16_t numcmpts, jas_image_cmptparm_t *cmptparms,
  int colorspace)
{
	jas_image_t *image;
	uint_fast32_t rawsize;
	uint_fast32_t inmem;
	uint_fast16_t cmptno;
	jas_image_cmptparm_t *cmptparm;

	if (!(image = jas_image_create0())) {
		return 0;
	}

	image->colorspace_ = colorspace;
	image->maxcmpts_ = numcmpts;
	image->inmem_ = JPR_TRUE;

	/* Allocate memory for the per-component information. */
	if (!(image->cmpts_ = jas_malloc(image->maxcmpts_ *
	  sizeof(jas_image_cmpt_t *)))) {
		jas_image_destroy(image);
		return 0;
	}
	/* Initialize in case of failure. */
	for (cmptno = 0; cmptno < image->maxcmpts_; ++cmptno) {
		image->cmpts_[cmptno] = 0;
	}

	/* Compute the approximate raw size of the image. */
	rawsize = 0;
	for (cmptno = 0, cmptparm = cmptparms; cmptno < numcmpts; ++cmptno,
	  ++cmptparm) {
		rawsize += cmptparm->width * cmptparm->height *
		  (cmptparm->prec + 7) / 8;
	}
	/* Decide whether to buffer the image data in memory, based on the
	  raw size of the image. */
	inmem = (rawsize < JAS_IMAGE_INMEMTHRESH);

	/* Create the individual image components. */
	for (cmptno = 0, cmptparm = cmptparms; cmptno < numcmpts; ++cmptno,
	  ++cmptparm) {
		if (!(image->cmpts_[cmptno] = jas_image_cmpt_create(cmptparm->tlx,
		  cmptparm->tly, cmptparm->hstep, cmptparm->vstep,
		  cmptparm->width, cmptparm->height, cmptparm->prec,
		  cmptparm->sgnd, inmem))) {
			jas_image_destroy(image);
			return 0;
		}
		++image->numcmpts_;
	}

	/* Determine the bounding box for all of the components on the
	  reference grid (i.e., the image area) */
	jas_image_setbbox(image);

	return image;
}

jas_image_t *jas_image_create0()
{
	jas_image_t *image;

	if (!(image = jas_malloc(sizeof(jas_image_t)))) {
		return 0;
	}

	image->tlx_ = 0;
	image->tly_ = 0;
	image->brx_ = 0;
	image->bry_ = 0;
	image->colorspace_ = JAS_IMAGE_CS_UNKNOWN;
	image->numcmpts_ = 0;
	image->maxcmpts_ = 0;
	image->cmpts_ = 0;
	image->inmem_ = JPR_TRUE;
	image->iccp_ = 0;
	image->iccplen_ = 0;

	return image;
}

jas_image_t *jas_image_copy(jas_image_t *image)
{
	jas_image_t *newimage;
	int cmptno;

	newimage = jas_image_create0();
	if (jas_image_growcmpts(newimage, image->numcmpts_)) {
		goto error;
	}
	for (cmptno = 0; cmptno < image->numcmpts_; ++cmptno) {
		if (!(newimage->cmpts_[cmptno] = jas_image_cmpt_copy(image->cmpts_[cmptno]))) {
			goto error;
		}
		++newimage->numcmpts_;
	}

	jas_image_setbbox(newimage);

	return newimage;
error:
	if (newimage) {
		jas_image_destroy(newimage);
	}
	return 0;
}

static jas_image_cmpt_t *jas_image_cmpt_create0()
{
	jas_image_cmpt_t *cmpt;
	if (!(cmpt = jas_malloc(sizeof(jas_image_cmpt_t)))) {
		return 0;
	}
	memset(cmpt, 0, sizeof(jas_image_cmpt_t));
	cmpt->type_ = JAS_IMAGE_CT_UNKNOWN;
	return cmpt;
}

static jas_image_cmpt_t *jas_image_cmpt_copy(jas_image_cmpt_t *cmpt)
{
	jas_image_cmpt_t *newcmpt;

	if (!(newcmpt = jas_image_cmpt_create0())) {
		return 0;
	}
	newcmpt->tlx_ = cmpt->tlx_;
	newcmpt->tly_ = cmpt->tly_;
	newcmpt->hstep_ = cmpt->hstep_;
	newcmpt->vstep_ = cmpt->vstep_;
	newcmpt->width_ = cmpt->width_;
	newcmpt->height_ = cmpt->height_;
	newcmpt->prec_ = cmpt->prec_;
	newcmpt->sgnd_ = cmpt->sgnd_;
	newcmpt->cps_ = cmpt->cps_;
	newcmpt->type_ = cmpt->type_;
	if (!(newcmpt->stream_ = jas_stream_memopen(0, 0))) {
		return 0;
	}
	if (jas_stream_seek(cmpt->stream_, 0, SEEK_SET)) {
		return 0;
	}
	if (jas_stream_copy(newcmpt->stream_, cmpt->stream_, -1)) {
		return 0;
	}
	if (jas_stream_seek(newcmpt->stream_, 0, SEEK_SET)) {
		return 0;
	}
	return newcmpt;
}

void jas_image_destroy(jas_image_t *image)
{
	int i;

	if (image->cmpts_) {
		for (i = 0; i < image->numcmpts_; ++i) {
			jas_image_cmpt_destroy(image->cmpts_[i]);
			image->cmpts_[i] = 0;
		}
		jas_free(image->cmpts_);
	}
	jas_free(image);
}

static jas_image_cmpt_t *jas_image_cmpt_create(uint_fast32_t tlx, uint_fast32_t tly,
  uint_fast32_t hstep, uint_fast32_t vstep, uint_fast32_t width, uint_fast32_t
  height, uint_fast16_t depth, JPR_BOOL sgnd, uint_fast32_t inmem)
{
	jas_image_cmpt_t *cmpt;
	long size;

	if (!(cmpt = jas_malloc(sizeof(jas_image_cmpt_t)))) {
		return 0;
	}

	cmpt->tlx_ = tlx;
	cmpt->tly_ = tly;
	cmpt->hstep_ = hstep;
	cmpt->vstep_ = vstep;
	cmpt->width_ = width;
	cmpt->height_ = height;
	cmpt->prec_ = depth;
	cmpt->sgnd_ = sgnd;
	cmpt->stream_ = 0;
	cmpt->cps_ = (depth + 7) / 8;

	size = cmpt->width_ * cmpt->height_ * cmpt->cps_;
	cmpt->stream_ = (inmem) ? jas_stream_memopen(0, size) : jas_stream_tmpfile();
	if (!cmpt->stream_) {
		jas_image_cmpt_destroy(cmpt);
		return 0;
	}

	/* Zero the component data.  This isn't necessary, but it is
	convenient for debugging purposes. */
	if (jas_stream_seek(cmpt->stream_, size - 1, SEEK_SET) < 0 ||
	  jas_stream_putc(cmpt->stream_, 0) == EOF ||
	  jas_stream_seek(cmpt->stream_, 0, SEEK_SET) < 0) {
		jas_image_cmpt_destroy(cmpt);
		return 0;
	}

	return cmpt;
}

static void jas_image_cmpt_destroy(jas_image_cmpt_t *cmpt)
{
	if (cmpt->stream_) {
		jas_stream_close(cmpt->stream_);
	}
	jas_free(cmpt);
}

/******************************************************************************\
* Load and save operations.
\******************************************************************************/

jas_image_t *jas_image_decode(jas_stream_t *in, int fmt, char *optstr)
{
	jas_image_fmtinfo_t *fmtinfo;

	/* If possible, try to determine the format of the input data. */
	if (fmt < 0) {
		if ((fmt = jas_image_getfmt(in)) < 0) {
			return 0;
		}
	}
	if (!(fmtinfo = jas_image_lookupfmtbyid(fmt))) {
		return 0;
	}
	return (fmtinfo->ops.decode) ? (*fmtinfo->ops.decode)(in, optstr) : 0;
}

int jas_image_encode(jas_image_t *image, jas_stream_t *out, int fmt, char *optstr)
{
	jas_image_fmtinfo_t *fmtinfo;
	if (!(fmtinfo = jas_image_lookupfmtbyid(fmt))) {
		return -1;
	}
	return (fmtinfo->ops.encode) ? (*fmtinfo->ops.encode)(image, out,
	  optstr) : (-1);
}

/******************************************************************************\
* Component read and write operations.
\******************************************************************************/

int jas_image_readcmpt(jas_image_t *image, uint_fast16_t cmptno, uint_fast32_t x, uint_fast32_t y, uint_fast32_t width,
  uint_fast32_t height, jas_matrix_t *data)
{
	jas_image_cmpt_t *cmpt;
	uint_fast32_t i;
	uint_fast32_t j;
	int k;
	jas_seqent_t v;
	int c;
	jas_seqent_t *dr;
	jas_seqent_t *d;
	int drs;

	if (cmptno < 0 || cmptno >= image->numcmpts_) {
		return -1;
	}

	cmpt = image->cmpts_[cmptno];
	if (x >= cmpt->width_ || y >= cmpt->height_ ||
	  x + width > cmpt->width_ ||
	  y + height > cmpt->height_) {
		return -1;
	}

	if (jas_matrix_numrows(data) != height || jas_matrix_numcols(data) != width) {
		if (jas_matrix_resize(data, height, width)) {
			return -1;
		}
	}

	dr = jas_matrix_getref(data, 0, 0);
	drs = jas_matrix_rowstep(data);
	for (i = 0; i < height; ++i, dr += drs) {
		d = dr;
		if (jas_stream_seek(cmpt->stream_, (cmpt->width_ * (y + i) + x)
		  * cmpt->cps_, SEEK_SET) < 0) {
			return -1;
		}
		for (j = width; j > 0; --j, ++d) {
			v = 0;
			for (k = cmpt->cps_; k > 0; --k) {
				if ((c = jas_stream_getc(cmpt->stream_)) == EOF) {
					return -1;
				}
				v = (v << 8) | (c & 0xff);
			}
			*d = bitstoint(v, cmpt->prec_, cmpt->sgnd_);
		}
	}

	return 0;
}

#if 0
int_fast64_t jas_image_readcmpt1(jas_image_t *image, uint_fast16_t cmptno,
  uint_fast32_t x, uint_fast32_t y)
{
	jas_image_cmpt_t *cmpt;
	int k;
	int c;
	int_fast64_t v;
	cmpt = image->cmpts_[cmptno];
	if (jas_stream_seek(cmpt->stream_, (cmpt->width_ * y + x) * cmpt->cps_,
	  SEEK_SET) < 0) {
		goto error;
	}
	v = 0;
	for (k = cmpt->cps_; k > 0; --k) {
		if ((c = jas_stream_getc(cmpt->stream_)) == EOF) {
			goto error;
		}
		v = (v << 8) | (c & 0xff);
	}
if (cmpt->sgnd_) {
	abort();
}

	return v;

error:
	return 0;
}
#endif

int jas_image_writecmpt(jas_image_t *image, uint_fast16_t cmptno, uint_fast32_t x, uint_fast32_t y, uint_fast32_t width,
  uint_fast32_t height, jas_matrix_t *data)
{
	jas_image_cmpt_t *cmpt;
	uint_fast32_t i;
	uint_fast32_t j;
	jas_seqent_t *d;
	jas_seqent_t *dr;
	int drs;
	jas_seqent_t v;
	int k;
	int c;

	if (cmptno < 0 || cmptno >= image->numcmpts_) {
		return -1;
	}

	cmpt = image->cmpts_[cmptno];
	if (x >= cmpt->width_ || y >= cmpt->height_ ||
	  x + width > cmpt->width_ ||
	  y + height > cmpt->height_) {
		return -1;
	}

	if (jas_matrix_numrows(data) != height || jas_matrix_numcols(data) != width) {
		return -1;
	}

	dr = jas_matrix_getref(data, 0, 0);
	drs = jas_matrix_rowstep(data);
	for (i = 0; i < height; ++i, dr += drs) {
		d = dr;
		if (jas_stream_seek(cmpt->stream_, (cmpt->width_ * (y + i) + x)
		  * cmpt->cps_, SEEK_SET) < 0) {
			return -1;
		}
		for (j = width; j > 0; --j, ++d) {
			v = inttobits(*d, cmpt->prec_, cmpt->sgnd_);
			for (k = cmpt->cps_; k > 0; --k) {
				c = (v >> (8 * (cmpt->cps_ - 1))) & 0xff;
				if (jas_stream_putc(cmpt->stream_,
				  (unsigned char) c) == EOF) {
					return -1;
				}
				v <<= 8;
			}
		}
	}

	return 0;
}

/******************************************************************************\
* File format operations.
\******************************************************************************/

void jas_image_clearfmts()
{
	int i;
	jas_image_fmtinfo_t *fmtinfo;
	for (i = 0; i < jas_image_numfmts; ++i) {
		fmtinfo = &jas_image_fmtinfos[i];
		if (fmtinfo->name) {
			jas_free(fmtinfo->name);
			fmtinfo->name = 0;
		}
		if (fmtinfo->ext) {
			jas_free(fmtinfo->ext);
			fmtinfo->ext = 0;
		}
		if (fmtinfo->desc) {
			jas_free(fmtinfo->desc);
			fmtinfo->desc = 0;
		}
	}
	jas_image_numfmts = 0;
}

int jas_image_addfmt(int id, char *name, char *ext, char *desc,
  jas_image_fmtops_t *ops)
{
	jas_image_fmtinfo_t *fmtinfo;
	assert(id >= 0 && name && ext && ops);
	if (jas_image_numfmts >= JAS_IMAGE_MAXFMTS) {
		return -1;
	}
	fmtinfo = &jas_image_fmtinfos[jas_image_numfmts];
	fmtinfo->id = id;
	if (!(fmtinfo->name = jas_strdup(name))) {
		return -1;
	}
	if (!(fmtinfo->ext = jas_strdup(ext))) {
		jas_free(fmtinfo->name);
		return -1;
	}
	if (!(fmtinfo->desc = jas_strdup(desc))) {
		jas_free(fmtinfo->name);
		jas_free(fmtinfo->ext);
		return -1;
	}
	fmtinfo->ops = *ops;
	++jas_image_numfmts;
	return 0;
}

int jas_image_strtofmt(char *name)
{
	jas_image_fmtinfo_t *fmtinfo;
	if (!(fmtinfo = jas_image_lookupfmtbyname(name))) {
		return -1;
	}
	return fmtinfo->id;
}

char *jas_image_fmttostr(int fmt)
{
	jas_image_fmtinfo_t *fmtinfo;
	if (!(fmtinfo = jas_image_lookupfmtbyid(fmt))) {
		return 0;
	}
	return fmtinfo->name;
}

int jas_image_getfmt(jas_stream_t *in)
{
	jas_image_fmtinfo_t *fmtinfo;
	int found;
	int i;

	/* Check for data in each of the supported formats. */
	found = 0;
	for (i = 0, fmtinfo = jas_image_fmtinfos; i < jas_image_numfmts; ++i,
	  ++fmtinfo) {
		if (fmtinfo->ops.validate) {
			/* Is the input data valid for this format? */
			if (!(*fmtinfo->ops.validate)(in)) {
				found = 1;
				break;
			}
		}
	}
	return found ? fmtinfo->id : (-1);
}

int jas_image_fmtfromname(char *name)
{
	int i;
	char *ext;
	jas_image_fmtinfo_t *fmtinfo;
	/* Get the file name extension. */
	if (!(ext = strrchr(name, '.'))) {
		return -1;
	}
	++ext;
	/* Try to find a format that uses this extension. */	
	for (i = 0, fmtinfo = jas_image_fmtinfos; i < jas_image_numfmts; ++i,
	  ++fmtinfo) {
		/* Do we have a match? */
		if (!strcmp(ext, fmtinfo->ext)) {
			return fmtinfo->id;
		}
	}
	return -1;
}

/******************************************************************************\
* Miscellaneous operations.
\******************************************************************************/

uint_fast32_t jas_image_rawsize(jas_image_t *image)
{
	uint_fast32_t rawsize;
	uint_fast32_t cmptno;
	jas_image_cmpt_t *cmpt;

	rawsize = 0;
	for (cmptno = 0; cmptno < image->numcmpts_; ++cmptno) {
		cmpt = image->cmpts_[cmptno];
		rawsize += (cmpt->width_ * cmpt->height_ * cmpt->prec_ +
		  7) / 8;
	}
	return rawsize;
}

void jas_image_delcmpt(jas_image_t *image, uint_fast16_t cmptno)
{
	if (cmptno >= image->numcmpts_) {
		return;
	}
	jas_image_cmpt_destroy(image->cmpts_[cmptno]);
	if (cmptno < image->numcmpts_) {
		memmove(&image->cmpts_[cmptno], &image->cmpts_[cmptno + 1],
		  (image->numcmpts_ - 1 - cmptno) * sizeof(jas_image_cmpt_t *));
	}
	--image->numcmpts_;

	jas_image_setbbox(image);
}

int jas_image_addcmpt(jas_image_t *image, uint_fast16_t cmptno,
  jas_image_cmptparm_t *cmptparm)
{
	jas_image_cmpt_t *newcmpt;
	assert(cmptno >= 0 && cmptno <= image->numcmpts_);
	if (image->numcmpts_ >= image->maxcmpts_) {
		if (jas_image_growcmpts(image, image->maxcmpts_ + 128)) {
			return -1;
		}
	}
	if (!(newcmpt = jas_image_cmpt_create(cmptparm->tlx,
	  cmptparm->tly, cmptparm->hstep, cmptparm->vstep,
	  cmptparm->width, cmptparm->height, cmptparm->prec,
	  cmptparm->sgnd, 1))) {
		return -1;
	}
	if (cmptno < image->numcmpts_) {
		memmove(&image->cmpts_[cmptno + 1], &image->cmpts_[cmptno],
		  (image->numcmpts_ - cmptno) * sizeof(jas_image_cmpt_t *));
	}
	image->cmpts_[cmptno] = newcmpt;
	++image->numcmpts_;

	jas_image_setbbox(image);

	return 0;
}

jas_image_fmtinfo_t *jas_image_lookupfmtbyid(int id)
{
	int i;
	jas_image_fmtinfo_t *fmtinfo;

	for (i = 0, fmtinfo = jas_image_fmtinfos; i < jas_image_numfmts; ++i, ++fmtinfo) {
		if (fmtinfo->id == id) {
			return fmtinfo;
		}
	}
	return 0;
}

jas_image_fmtinfo_t *jas_image_lookupfmtbyname(const char *name)
{
	int i;
	jas_image_fmtinfo_t *fmtinfo;

	for (i = 0, fmtinfo = jas_image_fmtinfos; i < jas_image_numfmts; ++i, ++fmtinfo) {
		if (!strcmp(fmtinfo->name, name)) {
			return fmtinfo;
		}
	}
	return 0;
}





static uint_fast32_t inttobits(jas_seqent_t v, int prec, JPR_BOOL sgnd)
{
	uint_fast32_t ret;
	ret = ((sgnd && v < 0) ? ((1 << prec) + v) : v) & JAS_ONES(prec);
	return ret;
}

static jas_seqent_t bitstoint(uint_fast32_t v, int prec, JPR_BOOL sgnd)
{
	jas_seqent_t ret;
	v &= JAS_ONES(prec);
	ret = (sgnd && (v & (1 << (prec - 1)))) ? (v - (1 << prec)) : v;
	return ret;
}

static void jas_image_setbbox(jas_image_t *image)
{
	jas_image_cmpt_t *cmpt;
	int cmptno;
	int_fast32_t x;
	int_fast32_t y;

	if (image->numcmpts_ > 0) {
		/* Determine the bounding box for all of the components on the
		  reference grid (i.e., the image area) */
		cmpt = image->cmpts_[0];
		image->tlx_ = cmpt->tlx_;
		image->tly_ = cmpt->tly_;
		image->brx_ = cmpt->tlx_ + cmpt->hstep_ * (cmpt->width_ - 1) + 1;
		image->bry_ = cmpt->tly_ + cmpt->vstep_ * (cmpt->height_ - 1) + 1;
		for (cmptno = 1; cmptno < image->numcmpts_; ++cmptno) {
			cmpt = image->cmpts_[cmptno];
			if (image->tlx_ > cmpt->tlx_) {
				image->tlx_ = cmpt->tlx_;
			}
			if (image->tly_ > cmpt->tly_) {
				image->tly_ = cmpt->tly_;
			}
			x = cmpt->tlx_ + cmpt->hstep_ * (cmpt->width_ - 1) + 1;
			if (image->brx_ < x) {
				image->brx_ = x;
			}
			y = cmpt->tly_ + cmpt->vstep_ * (cmpt->height_ - 1) + 1;
			if (image->bry_ < y) {
				image->bry_ = y;
			}
		}
	} else {
		image->tlx_ = 0;
		image->tly_ = 0;
		image->brx_ = 0;
		image->bry_ = 0;
	}
}

static int jas_image_growcmpts(jas_image_t *image, int maxcmpts)
{
	jas_image_cmpt_t **newcmpts;
	int cmptno;

	newcmpts = (!image->cmpts_) ? jas_malloc(maxcmpts * sizeof(jas_image_cmpt_t *)) :
	  jas_realloc(image->cmpts_, maxcmpts * sizeof(jas_image_cmpt_t *));
	if (!newcmpts) {
		return -1;
	}
	image->cmpts_ = newcmpts;
	image->maxcmpts_ = maxcmpts;
	for (cmptno = image->numcmpts_; cmptno < image->maxcmpts_; ++cmptno) {
		image->cmpts_[cmptno] = 0;
	}
	return 0;
}

int jas_image_copycmpt(jas_image_t *dstimage, int dstcmptno, jas_image_t *srcimage,
  int srccmptno)
{
	jas_image_cmpt_t *newcmpt;
	if (dstimage->numcmpts_ >= dstimage->maxcmpts_) {
		if (jas_image_growcmpts(dstimage, dstimage->maxcmpts_ + 128)) {
			return -1;
		}
	}
	if (!(newcmpt = jas_image_cmpt_copy(srcimage->cmpts_[srccmptno]))) {
		return -1;
	}
	if (dstcmptno < dstimage->numcmpts_) {
		memmove(&dstimage->cmpts_[dstcmptno + 1], &dstimage->cmpts_[dstcmptno],
		  (dstimage->numcmpts_ - dstcmptno) * sizeof(jas_image_cmpt_t *));
	}
	dstimage->cmpts_[dstcmptno] = newcmpt;
	++dstimage->numcmpts_;

	jas_image_setbbox(dstimage);
	return 0;
}

void jas_image_dump(jas_image_t *image, FILE *out)
{
	int cmptno;
	jas_seq2d_t *data;
	jas_image_cmpt_t *cmpt;
	if (!(data = jas_seq2d_create(0, 0, 1, 1))) {
		abort();
	}
	for (cmptno = 0; cmptno < image->numcmpts_; ++cmptno) {
		cmpt = image->cmpts_[cmptno];
		fprintf(out, "prec=%d sgnd=%d\n", cmpt->prec_, cmpt->sgnd_);
		if (jas_image_readcmpt(image, cmptno, 0, 0, 1, 1, data)) {
			abort();
		}
		fprintf(out, "tlsample %ld\n", (long) jas_seq2d_get(data, 0, 0));
	}
	jas_seq2d_destroy(data);
}

int jas_image_depalettize(jas_image_t *image, int cmptno, int numlutents,
  int_fast32_t *lutents, int dtype, int newcmptno)
{
	jas_image_cmptparm_t cmptparms;
	int_fast32_t v;
	int i;
	int j;
	jas_image_cmpt_t *cmpt;

	cmpt = image->cmpts_[cmptno];
	cmptparms.tlx = cmpt->tlx_;
	cmptparms.tly = cmpt->tly_;
	cmptparms.hstep = cmpt->hstep_;
	cmptparms.vstep = cmpt->vstep_;
	cmptparms.width = cmpt->width_;
	cmptparms.height = cmpt->height_;
	cmptparms.prec = JAS_IMAGE_CDT_GETPREC(dtype);
	cmptparms.sgnd = JAS_IMAGE_CDT_GETSGND(dtype);

	if (jas_image_addcmpt(image, newcmptno, &cmptparms)) {
		return -1;
	}
	if (newcmptno <= cmptno) {
		++cmptno;
		cmpt = image->cmpts_[cmptno];
	}

	for (j = 0; j < cmpt->height_; ++j) {
		for (i = 0; i < cmpt->width_; ++i) {
			v = jas_image_readcmptsample(image, cmptno, i, j);
			if (v < 0) {
				v = 0;
			} else if (v >= numlutents) {
				v = numlutents - 1;
			}
			jas_image_writecmptsample(image, newcmptno, i, j,
			  lutents[v]);
		}
	}
	return 0;
}

int jas_image_readcmptsample(jas_image_t *image, int cmptno, int x, int y)
{
	jas_image_cmpt_t *cmpt;
	uint_fast32_t v;
	int k;
	int c;

	cmpt = image->cmpts_[cmptno];

	if (jas_stream_seek(cmpt->stream_, (cmpt->width_ * y + x) * cmpt->cps_,
	  SEEK_SET) < 0) {
		return -1;
	}
	v = 0;
	for (k = cmpt->cps_; k > 0; --k) {
		if ((c = jas_stream_getc(cmpt->stream_)) == EOF) {
			return -1;
		}
		v = (v << 8) | (c & 0xff);
	}
	return bitstoint(v, cmpt->prec_, cmpt->sgnd_);
}

void jas_image_writecmptsample(jas_image_t *image, int cmptno, int x, int y,
  int_fast32_t v)
{
	jas_image_cmpt_t *cmpt;
	uint_fast32_t t;
	int k;
	int c;

	cmpt = image->cmpts_[cmptno];

	if (jas_stream_seek(cmpt->stream_, (cmpt->width_ * y + x) * cmpt->cps_,
	  SEEK_SET) < 0) {
		return;
	}
	t = inttobits(v, cmpt->prec_, cmpt->sgnd_);
	for (k = cmpt->cps_; k > 0; --k) {
		c = (t >> (8 * (cmpt->cps_ - 1))) & 0xff;
		if (jas_stream_putc(cmpt->stream_, (unsigned char) c) == EOF) {
			return;
		}
		t <<= 8;
	}
}

int jas_image_getcmptbytype(jas_image_t *image, int ctype)
{
	int cmptno;

	for (cmptno = 0; cmptno < image->numcmpts_; ++cmptno) {
		if (image->cmpts_[cmptno]->type_ == ctype) {
			return cmptno;
		}
	}
}
