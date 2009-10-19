/*---------------------------------------------------------------------------*
 |              PDFlib - A library for generating PDF on the fly             |
 +---------------------------------------------------------------------------+
 | Copyright (c) 1997-2004 Thomas Merz and PDFlib GmbH. All rights reserved. |
 +---------------------------------------------------------------------------+
 |                                                                           |
 |    This software is subject to the PDFlib license. It is NOT in the       |
 |    public domain. Extended versions and commercial licenses are           |
 |    available, please check http://www.pdflib.com.                         |
 |                                                                           |
 *---------------------------------------------------------------------------*/

/* $Id: p_image.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib image routines
 *
 */

#define P_IMAGE_C

#include "p_intern.h"
#include "p_image.h"
#include "p_font.h"

/* must be kept in sync with the pdf_compression enum in p_intern.h */
static const char *pdf_filter_names[] = {
    "", "LZWDecode", "RunLengthDecode", "CCITTFaxDecode",
    "DCTDecode", "FlateDecode", "JBIG2Decode"
};

static const char *pdf_short_filter_names[] = {
    "", "LZW", "RL", "CCF", "DCT", "Fl", ""
};

static void
pdf_init_image_struct(PDF *p, pdf_image *image)
{
    (void) p;

    /********** option variables *************/
    image->verbose      = pdc_true;
    image->bitreverse   = pdc_false;
    image->bpc          = pdc_undef;
    image->components   = pdc_undef;
    image->height_pixel = pdc_undef;
    image->ignoremask   = pdc_false;
    image->doinline     = pdc_false;
    image->interpolate  = pdc_false;
    image->invert       = pdc_false;
    image->K            = 0;
    image->imagemask    = pdc_false;
    image->mask         = pdc_undef;
    image->ri           = AutoIntent;
    image->page         = 1;
    image->reference    = pdf_ref_direct;
    image->width_pixel  = pdc_undef;
    /*****************************************/

    image->transparent  = pdc_false;
    image->compression	= none;
    image->predictor	= pred_default;
    image->in_use	= pdc_false;
    image->fp           = (pdc_file *) NULL;
    image->filename	= (char *) NULL;
    image->params	= (char *) NULL;
    image->dpi_x	= (float) 0;
    image->dpi_y	= (float) 0;
    image->strips	= 1;
    image->rowsperstrip	= 1;
    image->colorspace   = pdc_undef;
    image->dochandle    = pdc_undef;        /* this means "not a PDI page" */
    image->use_raw	= pdc_false;
    image->transval[0]	= 0;
    image->transval[1]	= 0;
    image->transval[2]	= 0;
    image->transval[3]	= 0;
}

void
pdf_init_images(PDF *p)
{
    int im;

    p->images_capacity = IMAGES_CHUNKSIZE;

    p->images = (pdf_image *)
    	pdc_malloc(p->pdc,
	    sizeof(pdf_image) * p->images_capacity, "pdf_init_images");

    for (im = 0; im < p->images_capacity; im++)
	pdf_init_image_struct(p, &(p->images[im]));
}

void
pdf_grow_images(PDF *p)
{
    int im;

    p->images = (pdf_image *) pdc_realloc(p->pdc, p->images,
	sizeof(pdf_image) * 2 * p->images_capacity, "pdf_grow_images");

    for (im = p->images_capacity; im < 2 * p->images_capacity; im++)
	pdf_init_image_struct(p, &(p->images[im]));

    p->images_capacity *= 2;
}

void
pdf_cleanup_image(PDF *p, int im)
{
    /* clean up parameter string if necessary */
    if (p->images[im].params) {
        pdc_free(p->pdc, p->images[im].params);
        p->images[im].params = NULL;
    }

    if (p->images[im].filename) {
        pdc_free(p->pdc, p->images[im].filename);
        p->images[im].filename = NULL;
    }

    if (p->images[im].fp) {
        pdc_fclose(p->images[im].fp);
        p->images[im].fp = NULL;
    }

    /* free the image slot and prepare for next use */
    pdf_init_image_struct(p, &(p->images[im]));
}

void
pdf_cleanup_images(PDF *p)
{
    int im;

    if (!p->images)
	return;

    /* Free images which the caller left open */

    /* When we think of inter-document survival of images,
    ** we MUST NOT FORGET that the current TIFF algorithm
    ** depends on contiguous image slots for the image strips!
    */
    for (im = 0; im < p->images_capacity; im++)
    {
	pdf_image *img = &p->images[im];

	if (img->in_use)		/* found used slot */
	    pdf_cleanup_image(p, im);	/* free image descriptor */
    }

    pdc_free(p->pdc, p->images);
    p->images = NULL;
}

void
pdf_init_xobjects(PDF *p)
{
    int idx;

    p->xobjects_number = 0;

    if (p->xobjects == (pdf_xobject *) 0)
    {
	p->xobjects_capacity = XOBJECTS_CHUNKSIZE;

	p->xobjects = (pdf_xobject *)
	    pdc_malloc(p->pdc, sizeof(pdf_xobject) * p->xobjects_capacity,
	    "pdf_init_xobjects");
    }

    for (idx = 0; idx < p->xobjects_capacity; idx++)
	p->xobjects[idx].flags = 0;
}

int
pdf_new_xobject(PDF *p, pdf_xobj_type type, pdc_id obj_id)
{
    static const char fn[] = "pdf_new_xobject";
    int i, slot = p->xobjects_number++;

    if (slot == p->xobjects_capacity)
    {
	p->xobjects = (pdf_xobject *) pdc_realloc(p->pdc, p->xobjects,
	    sizeof(pdf_xobject) * 2 * p->xobjects_capacity, fn);

	for (i = p->xobjects_capacity; i < 2 * p->xobjects_capacity; i++)
	    p->xobjects[i].flags = 0;

	p->xobjects_capacity *= 2;
    }

    if (obj_id == PDC_NEW_ID)
	obj_id = pdc_begin_obj(p->out, PDC_NEW_ID);

    p->xobjects[slot].obj_id = obj_id;
    p->xobjects[slot].type = type;
    p->xobjects[slot].flags = xobj_flag_used;

    return slot;
}

void
pdf_write_xobjects(PDF *p)
{
    if (p->xobjects_number > 0)
    {
	pdc_bool hit = pdc_false;
	int i;

	for (i = 0; i < p->xobjects_capacity; ++i)
	{
	    if (p->xobjects[i].flags & xobj_flag_write)
	    {
		if (!hit)
		{
		    pdc_puts(p->out, "/XObject");
		    pdc_begin_dict(p->out);
		    hit = pdc_true;
		}

		pdc_printf(p->out, "/I%d %ld 0 R\n", i, p->xobjects[i].obj_id);
		p->xobjects[i].flags &= ~xobj_flag_write;
	    }
	}

	if (hit)
	    pdc_end_dict(p->out);
    }
}

void
pdf_cleanup_xobjects(PDF *p)
{
    if (p->xobjects) {
	pdc_free(p->pdc, p->xobjects);
	p->xobjects = NULL;
    }
}

void
pdf_put_inline_image(PDF *p, int im)
{
    pdf_image	*image;
    pdc_matrix m;
    PDF_data_source *src;
    int		i;

    image = &p->images[im];

    /* Image object */

    image->no = -1;

    pdf__save(p);

    pdc_scale_matrix(image->width, image->height, &m);

    pdf_concat_raw(p, &m);

    pdc_puts(p->out, "BI");

    pdc_printf(p->out, "/W %d", (int) image->width);
    pdc_printf(p->out, "/H %d", (int) image->height);

    if (image->imagemask == pdc_true) {
	pdc_puts(p->out, "/IM true");

    } else {

	pdc_printf(p->out, "/BPC %d", image->bpc);

	switch (p->colorspaces[image->colorspace].type) {
	    case DeviceGray:
		pdc_printf(p->out, "/CS/G");
		break;

	    case DeviceRGB:
		pdc_printf(p->out, "/CS/RGB");
		break;

	    case DeviceCMYK:
		pdc_printf(p->out, "/CS/CMYK");
		break;

	    default:
		pdc_error(p->pdc, PDF_E_INT_BADCS,
		    pdc_errprintf(p->pdc, "%d", image->colorspace), 0, 0, 0);
		break;
	}
    }

    if (image->compression != none) {
	pdc_printf(p->out, "/F/%s",
		pdf_short_filter_names[image->compression]);
    }

    /* prepare precompressed (raw) image data */
    if (image->use_raw &&
        (image->params ||
	 image->predictor != pred_default ||
	 image->compression == ccitt)) {

	pdc_printf(p->out, "/DP[<<");

        /* write EarlyChange */
        if (image->params)
            pdc_puts(p->out, image->params);

        if (image->compression == ccitt) {
            if (image->K != 0)
                pdc_printf(p->out, "/K %d", image->K);
        }

	if (image->compression == flate || image->compression == lzw) {
	    if (image->predictor != pred_default) {
		pdc_printf(p->out, "/Predictor %d", (int) image->predictor);
		pdc_printf(p->out, "/Columns %d", (int) image->width);
		if (image->bpc != 8)
		    pdc_printf(p->out, "/BitsPerComponent %d", image->bpc);

		if (image->components != 1)	/* 1 is default */
		    pdc_printf(p->out, "/Colors %d", image->components);
	    }
	}

	if (image->compression == ccitt) {
	    if ((int) image->width != 1728)	/* CCITT default width */
		pdc_printf(p->out, "/Columns %d", (int) image->width);

	    pdc_printf(p->out, "/Rows %d", (int) fabs(image->height));
	}
	pdc_puts(p->out, ">>]");		/* DecodeParms dict and array */
    }

    if (image->invert) {
	pdc_puts(p->out, "/D[1 0");
	for (i = 1; i < image->components; i++)
	    pdc_puts(p->out, " 1 0");
	pdc_puts(p->out, "]");
    }

    if (image->ri != AutoIntent) {
        pdc_printf(p->out, "/Intent/%s",
            pdc_get_keyword(image->ri, gs_renderingintents));
    }

    if (image->interpolate) {
        pdc_puts(p->out, "/I true");
    }

    pdc_puts(p->out, " ID\n");

    /* Write the actual image data to the content stream */

    src = &image->src;

    /* We can't use pdf_copy_stream() here because it automatically
     * generates a stream object, which is not correct for inline
     * image data.
     */
    if (src->init)
	src->init(p, src);

    while (src->fill(p, src))
	pdc_write(p->out, src->next_byte, src->bytes_available);

    if (src->terminate)
	src->terminate(p, src);

    pdc_puts(p->out, "EI\n");

    pdf__restore(p);

    /* Do the equivalent of PDF_close_image() since the image handle
     * cannot be re-used anyway.
     */
    pdf_cleanup_image(p, im);
}

void
pdf_put_image(PDF *p, int im, pdc_bool firststrip)
{
    pdc_id	length_id;
    pdf_image	*image;
    int		i;

    image = &p->images[im];

    /* Images may also be written to the output before the first page */
    if (PDF_GET_STATE(p) == pdf_state_page)
	pdf_end_contents_section(p);

    /* Image object */

    image->no = pdf_new_xobject(p, image_xobject, PDC_NEW_ID);

    pdc_begin_dict(p->out); 		/* XObject */

    pdc_puts(p->out, "/Subtype/Image\n");

    pdc_printf(p->out, "/Width %d\n", (int) image->width);
    pdc_printf(p->out, "/Height %d\n", (int) fabs(image->height));

    /*
     * Transparency handling
     */

    /* Masking by color: single transparent color value */
    if (image->transparent) {
	pdf_colorspace *cs = &p->colorspaces[image->colorspace];

	switch (cs->type) {
	    case Indexed:
	    case DeviceGray:
	    pdc_printf(p->out,"/Mask[%d %d]\n",
		(int) image->transval[0], (int) image->transval[0]);
	    break;


	    case DeviceRGB:
	    pdc_printf(p->out,"/Mask[%d %d %d %d %d %d]\n",
		(int) image->transval[0], (int) image->transval[0],
		(int) image->transval[1], (int) image->transval[1],
		(int) image->transval[2], (int) image->transval[2]);
	    break;

	    case DeviceCMYK:
	    pdc_printf(p->out,"/Mask[%d %d %d %d %d %d %d %d]\n",
		(int) image->transval[0], (int) image->transval[0],
		(int) image->transval[1], (int) image->transval[1],
		(int) image->transval[2], (int) image->transval[2],
		(int) image->transval[3], (int) image->transval[3]);
	    break;

	    default:
	    pdc_error(p->pdc, PDF_E_INT_BADCS,
		pdc_errprintf(p->pdc, "%d",
		    (int) p->colorspaces[image->colorspace].type), 0, 0, 0);
	}

    /* Masking by position: separate bitmap mask */
    } else if (image->mask != pdc_undef && p->images[image->mask].bpc > 1) {
        pdc_printf(p->out, "/SMask %ld 0 R\n",
            p->xobjects[p->images[image->mask].no].obj_id);

    } else if (image->mask != pdc_undef) {
        pdc_printf(p->out, "/Mask %ld 0 R\n",
            p->xobjects[p->images[image->mask].no].obj_id);
    }

    /*
     * /BitsPerComponent is optional for image masks according to the
     * PDF reference, but some viewers require it nevertheless.
     * We must therefore always write it.
     */
    pdc_printf(p->out, "/BitsPerComponent %d\n", image->bpc);

    if (image->imagemask) {
	pdc_puts(p->out, "/ImageMask true\n");

    } else {

	switch (p->colorspaces[image->colorspace].type) {
	    case DeviceGray:
		break;

	    case DeviceRGB:
		break;

	    case DeviceCMYK:
		break;

	    case Indexed:
		break;



	    default:
		pdc_error(p->pdc, PDF_E_INT_BADCS,
		    pdc_errprintf(p->pdc, "%d", image->colorspace), 0, 0, 0);
        }

        pdc_puts(p->out, "/ColorSpace");
        pdf_write_colorspace(p, image->colorspace, pdc_false);
        pdc_puts(p->out, "\n");
    }

    if (image->invert) {
        pdc_puts(p->out, "/Decode[1 0");
        for (i = 1; i < image->components; i++)
            pdc_puts(p->out, " 1 0");
        pdc_puts(p->out, "]\n");
    }

    if (image->ri != AutoIntent) {
        pdc_printf(p->out, "/Intent/%s\n",
            pdc_get_keyword(image->ri, gs_renderingintents));
    }

    if (image->interpolate) {
        pdc_puts(p->out, "/Interpolate true\n");
    }

    /* special case: referenced image data instead of direct data */
    if (image->reference != pdf_ref_direct) {

	if (image->compression != none) {
	    pdc_printf(p->out, "/FFilter[/%s]\n",
		    pdf_filter_names[image->compression]);
	}

	if (image->compression == ccitt) {
	    pdc_puts(p->out, "/FDecodeParms[<<");

	    if ((int) image->width != 1728)	/* CCITT default width */
		pdc_printf(p->out, "/Columns %d", (int) image->width);

	    pdc_printf(p->out, "/Rows %d", (int) fabs(image->height));

            if (image->K != 0)
                pdc_printf(p->out, "/K %d", image->K);

	    pdc_puts(p->out, ">>]\n");

	}

	if (image->reference == pdf_ref_file) {

	    /* LATER: make image file name platform-neutral:
	     * Change : to / on the Mac
	     * Change \ to / on Windows
	     */
	    pdc_puts(p->out, "/F");
	    pdc_put_pdfstring(p->out, image->filename,
		(int) strlen(image->filename));
            pdc_puts(p->out, "/Length 0");

	} else if (image->reference == pdf_ref_url) {

	    pdc_puts(p->out, "/F<</FS/URL/F");
	    pdc_put_pdfstring(p->out, image->filename,
		(int) strlen(image->filename));
	    pdc_puts(p->out, ">>/Length 0");
	}

	pdc_end_dict(p->out);		/* XObject */

	/* We must avoid pdc_begin/end_pdfstream() here in order to
	 * generate a really empty stream.
	 */
	pdc_puts(p->out, "stream\n");	/* dummy image stream */
	pdc_puts(p->out, "endstream\n");

        pdc_end_obj(p->out);                    /* XObject */

	if (PDF_GET_STATE(p) == pdf_state_page)
	    pdf_begin_contents_section(p);

	return;
    }

    /*
     * Now the (more common) handling of actual image
     * data to be included in the PDF output.
     */
    /* do we need a filter (either ASCII or decompression)? */

    if (p->debug['a']) {
	pdc_puts(p->out, "/Filter[/ASCIIHexDecode");
	if (image->compression != none)
	    pdc_printf(p->out, "/%s", pdf_filter_names[image->compression]);
	pdc_puts(p->out, "]\n");

    } else {
	/* force compression if not a recognized precompressed image format */
	if (!image->use_raw && pdc_get_compresslevel(p->out))
	    image->compression = flate;

	if (image->compression != none)
	    pdc_printf(p->out, "/Filter/%s\n",
		    pdf_filter_names[image->compression]);
    }

    /* prepare precompressed (raw) image data; avoid empty DecodeParms */
    if (image->use_raw &&
        (image->params ||
	 image->predictor != pred_default ||
	 image->compression == ccitt)) {

	if (p->debug['a'])
	    pdc_printf(p->out, "/DecodeParms[%s<<", "null");
	else
	    pdc_printf(p->out, "/DecodeParms<<");

        /* write EarlyChange */
        if (image->params)
            pdc_puts(p->out, image->params);

        if (image->compression == ccitt) {
            if (image->K != 0)
                pdc_printf(p->out, "/K %d", image->K);
        }

	if (image->compression == flate || image->compression == lzw) {
	    if (image->predictor != pred_default) {
		pdc_printf(p->out, "/Predictor %d", (int) image->predictor);
		pdc_printf(p->out, "/Columns %d", (int) image->width);
		if (image->bpc != 8)
		    pdc_printf(p->out, "/BitsPerComponent %d", image->bpc);

		if (image->components != 1)	/* 1 is default */
		    pdc_printf(p->out, "/Colors %d", image->components);
	    }
	}

	if (image->compression == ccitt) {
	    if ((int) image->width != 1728)	/* CCITT default width */
		pdc_printf(p->out, "/Columns %d", (int) image->width);

	    pdc_printf(p->out, "/Rows %d", (int) fabs(image->height));
	}

	if (p->debug['a'])
	    pdc_puts(p->out, ">>]\n");		/* DecodeParms dict and array */
	else
	    pdc_puts(p->out, ">>\n");		/* DecodeParms dict */
    }

    /* Write the actual image data */
    length_id = pdc_alloc_id(p->out);

    pdc_printf(p->out,"/Length %ld 0 R\n", length_id);
    pdc_end_dict(p->out);		/* XObject */

    /* image data */

    if (p->debug['a'])
	pdf_ASCIIHexEncode(p, &image->src);
    else {
	pdf_copy_stream(p, &image->src, !image->use_raw);	/* image data */
    }

    pdc_end_obj(p->out);	/* XObject */

    pdc_put_pdfstreamlength(p->out, length_id);

    if (p->flush & pdf_flush_content)
	pdc_flush_stream(p->out);

    /*
     * Write colormap information for indexed color spaces
     */
    if (firststrip && p->colorspaces[image->colorspace].type == Indexed) {
	pdf_write_colormap(p, image->colorspace);
    }

    if (PDF_GET_STATE(p) == pdf_state_page)
	pdf_begin_contents_section(p);

    if (p->flush & pdf_flush_content)
	pdc_flush_stream(p->out);
}

void
pdf__fit_image(PDF *p, int im, float x, float y, const char *optlist)
{
    pdf_image *image;
    int legal_states;

    pdf_check_handle(p, im, pdc_imagehandle);

    image = &p->images[im];

    if (PDF_GET_STATE(p) == pdf_state_glyph && !pdf_get_t3colorized(p) &&
        image->imagemask == pdc_false)
        legal_states = pdf_state_page | pdf_state_pattern | pdf_state_template;
    else
        legal_states = pdf_state_content;
    PDF_CHECK_STATE(p, legal_states);

    if (PDF_GET_STATE(p) == pdf_state_template && im == p->templ)
        pdc_error(p->pdc, PDF_E_TEMPLATE_SELF,
            pdc_errprintf(p->pdc, "%d", im), 0, 0, 0);

    pdf_place_xobject(p, im, x, y, optlist);
}

PDFLIB_API void PDFLIB_CALL
PDF_fit_image(PDF *p, int image, float x, float y, const char *optlist)
{
    static const char fn[] = "PDF_fit_image";

    /* precise scope diagnosis in pdf__fit_image */
    if (pdf_enter_api(p, fn, pdf_state_all,
        "(p[%p], %d, %g, %g, \"%s\")\n", (void *) p, image, x, y, optlist))
    {
        PDF_INPUT_HANDLE(p, image)
        pdf__fit_image(p, image, x, y, optlist);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_place_image(PDF *p, int image, float x, float y, float scale)
{
    static const char fn[] = "PDF_place_image";

    /* precise scope diagnosis in pdf__fit_image */
    if (pdf_enter_api(p, fn, pdf_state_all,
        "(p[%p], %d, %g, %g, %g)\n", (void *) p, image, x, y, scale))
    {
        char optlist[32];
        sprintf(optlist, "dpi none  scale %g", scale);
        PDF_INPUT_HANDLE(p, image)
        pdf__fit_image(p, image, x, y, optlist);
    }
}

/* definitions of place image options */
static const pdc_defopt pdf_place_xobject_options[] =
{
    {"dpi", pdc_floatlist, 0, 1, 2, 0.0, INT_MAX, pdf_dpi_keylist},

    {"scale", pdc_floatlist, PDC_OPT_NOZERO, 1, 2, PDC_FLOAT_MIN, PDC_FLOAT_MAX,
     NULL},

    {"orientate", pdc_keywordlist, 0, 1, 1, 0.0, 0.0, pdf_orientate_keylist},

    {"boxsize", pdc_floatlist, 0, 2, 2, PDC_FLOAT_MIN, PDC_FLOAT_MAX, NULL},

    {"rotate", pdc_floatlist, 0, 1, 1, PDC_FLOAT_MIN, PDC_FLOAT_MAX, NULL},

    {"position", pdc_floatlist, 0, 1, 2, PDC_FLOAT_MIN, PDC_FLOAT_MAX, NULL},

    {"fitmethod", pdc_keywordlist, 0, 1, 1, 0.0, 0.0, pdf_fitmethod_keylist},

    {"distortionlimit", pdc_floatlist, PDC_OPT_NONE, 1, 1, 0.0, 100.0, NULL},

    {"adjustpage", pdc_booleanlist, PDC_OPT_PDC_1_3, 1, 1, 0.0, 0.0, NULL},

    {"blind", pdc_booleanlist, 0, 1, 1, 0.0, 0.0, NULL},

    PDC_OPT_TERMINATE
};

void
pdf_place_xobject(PDF *p, int im, float x, float y, const char *optlist)
{
    pdf_image *image;
    pdc_resopt *results;
    pdc_clientdata data;
    pdc_matrix m;
    pdc_fitmethod method;
    pdc_vector imgscale, elemsize, elemscale, relpos, polyline[5];
    pdc_box fitbox, elembox;
    pdc_scalar ss, scaley, rowsize = 1, lastratio = 1, minfscale;
    pdc_bool adjustpage, hasresol;
    pdc_bool blindmode;
    pdc_bool negdir = pdc_false;
    float dpi[2], scale[2], boxsize[2], position[2], angle, distortionlimit;
    float dpi_x, dpi_y, tx = 0, ty = 0;
    int orientangle, indangle;
    int inum, num, is, ip, islast;
    int imageno;

    image = &p->images[im];

    /* has resolution (image_xobject) */
    hasresol = p->xobjects[image->no].type == image_xobject ?
                   pdc_true : pdc_false;

    /* defaults */
    orientangle = 0;
    dpi[0] = dpi[1] = (float) 0.0;
    scale[0] = scale[1] = (float) 1.0;
    boxsize[0] = boxsize[1] = (float) 0.0;
    position[0] = position[1] = (float) 0.0;
    angle = (float) 0.0;
    method = pdc_nofit;
    distortionlimit = (float) 75.0;
    adjustpage = pdc_false;
    blindmode = pdc_false;

    /* parsing optlist */
    if (optlist && strlen(optlist))
    {
        data.compatibility = p->compatibility;
        results = pdc_parse_optionlist(p->pdc, optlist,
                                       pdf_place_xobject_options,
                                       &data, pdc_true);

        /* save and check options */
        if (hasresol)
            dpi[0] = (float) dpi_internal;
        if ((num = pdc_get_optvalues(p->pdc, "dpi", results, dpi, NULL)) > 0)
        {
            if (!hasresol && dpi[0] != (float) dpi_none)
                pdc_warning(p->pdc, PDC_E_OPT_IGNORED, "dpi", 0, 0, 0);
            else if (num == 1)
                dpi[1] = dpi[0];
        }

        if (1 == pdc_get_optvalues(p->pdc, "scale", results, scale, NULL))
            scale[1] = scale[0];

        pdc_get_optvalues(p->pdc, "orientate", results, &orientangle, NULL);

        pdc_get_optvalues(p->pdc, "boxsize", results, boxsize, NULL);

        pdc_get_optvalues(p->pdc, "rotate", results, &angle, NULL);

        if (1 == pdc_get_optvalues(p->pdc, "position", results, position, NULL))
            position[1] = position[0];

        if (pdc_get_optvalues(p->pdc, "fitmethod", results, &inum, NULL))
            method = (pdc_fitmethod) inum;

        pdc_get_optvalues(p->pdc, "distortionlimit", results,
                          &distortionlimit, NULL);

        pdc_get_optvalues(p->pdc, "adjustpage", results, &adjustpage, NULL);
        pdc_get_optvalues(p->pdc, "blind", results, &blindmode, NULL);

        pdc_cleanup_optionlist(p->pdc, results);
    }

    /* calculation of image scale and size */
    imgscale.x = scale[0];
    imgscale.y = scale[1];
    if (hasresol)
    {
        if (dpi[0] == (float) dpi_internal)
        {
            dpi_x = image->dpi_x;
            dpi_y = image->dpi_y;
            if (dpi_x > 0 && dpi_y > 0)
            {
                imgscale.x *= 72.0 / dpi_x;
                imgscale.y *= 72.0 / dpi_y;
            }
            else if (dpi_x < 0 && dpi_y < 0)
            {
                imgscale.y *= dpi_y / dpi_x;
            }
        }
        else if (dpi[0] > 0)
        {
            imgscale.x *= 72.0 / dpi[0];
            imgscale.y *= 72.0 / dpi[1];
        }
        rowsize = imgscale.y * image->rowsperstrip;
        imgscale.x *= image->width;
        imgscale.y *= image->height;
        lastratio = (imgscale.y / rowsize) - (image->strips - 1);
        elemsize.x = imgscale.x;
        elemsize.y = imgscale.y;
    }
    else
    {
        elemsize.x = imgscale.x * image->width;
        elemsize.y = imgscale.y * image->height;
    }

    /* negative direction (e.g. BMP images) */
    if (image->height < 0)
    {
        elemsize.y = -elemsize.y;
        negdir = pdc_true;
    }

    /* minimal horizontal scaling factor */
    minfscale = distortionlimit / 100.0;

    /* orientation */
    indangle = orientangle / 90;
    if (indangle % 2)
    {
        ss = elemsize.x;
        elemsize.x = elemsize.y;
        elemsize.y = ss;
    }

    /* box for fitting */
    fitbox.ll.x = 0;
    fitbox.ll.y = 0;
    fitbox.ur.x = boxsize[0];
    fitbox.ur.y = boxsize[1];

    /* relative position */
    relpos.x = position[0] / 100.0;
    relpos.y = position[1] / 100.0;

    /* calculate image box */
    pdc_place_element(method, minfscale, &fitbox, &relpos,
                      &elemsize, &elembox, &elemscale);

    /* adjust page size */
    if (adjustpage && PDF_GET_STATE(p) == pdf_state_page)
    {
	float urx, ury, height;

	urx = (float) (2 * x + elembox.ur.x);
	ury = (float) (2 * y + elembox.ur.y);
	pdc_transform_point(&p->gstate[p->sl].ctm,
			    urx, ury, &p->width, &height);
	p->height = (p->ydirection > 0) ?
			height : p->height - p->ydirection * height;
	if (p->height < p->ydirection * height / 2.0)
	    pdc_error(p->pdc, PDF_E_IMAGE_NOADJUST, 0, 0, 0, 0);

	p->CropBox.llx = 0.0f;
	p->CropBox.lly = 0.0f;
	p->CropBox.urx = p->width;
	p->CropBox.ury = p->height;

	if ((p->width < PDF_ACRO4_MINPAGE || p->width > PDF_ACRO4_MAXPAGE ||
	     p->height < PDF_ACRO4_MINPAGE || p->height > PDF_ACRO4_MAXPAGE))
		pdc_warning(p->pdc, PDF_E_PAGE_SIZE_ACRO4, 0, 0, 0, 0);
    }

    if (!blindmode)
    {
        pdf_end_text(p);
        pdf_begin_contents_section(p);

        pdf__save(p);
    }

    /* reference point */
    pdc_translation_matrix(x, y, &m);
    pdf_concat_raw_ob(p, &m, blindmode);

    /* clipping */
    if (!blindmode && (method == pdc_clip || method == pdc_slice))
    {
        pdf__rect(p, 0, 0, boxsize[0], boxsize[1]);
        pdf__clip(p);
    }

    /* optional rotation */
    if (fabs((double)(angle)) > PDC_FLOAT_PREC)
    {
        pdc_rotation_matrix(p->ydirection * angle, &m);
        pdf_concat_raw_ob(p, &m, blindmode);
    }

    /* translation of element box */
    elembox.ll.y *= p->ydirection;
    elembox.ur.y *= p->ydirection;
    pdc_box2polyline(&elembox, polyline);
    ip = indangle;
    if (negdir)
    {
        ip = indangle - 1;
        if (ip < 0) ip = 3;
    }
    tx = (float) polyline[ip].x;
    ty = (float) polyline[ip].y;
    pdc_translation_matrix(tx, ty, &m);
    pdf_concat_raw_ob(p, &m, blindmode);

    /* orientation of image */
    if (orientangle != 0)
    {
        pdc_rotation_matrix(p->ydirection * orientangle, &m);
        pdf_concat_raw_ob(p, &m, blindmode);
        if (indangle % 2)
        {
            ss = elemscale.x;
            elemscale.x = elemscale.y;
            elemscale.y = ss;
        }
    }

    /* scaling of image */
    if (image->strips == 1)
        scaley = p->ydirection * imgscale.y * elemscale.y;
    else
        scaley = p->ydirection * rowsize * elemscale.y;
    pdc_scale_matrix((float)(imgscale.x * elemscale.x), (float) scaley, &m);
    pdf_concat_raw_ob(p, &m, blindmode);

    if (!hasresol && !blindmode)
    {
        pdf_reset_gstate(p);
        pdf_reset_tstate(p);
    }


    if (!blindmode)
    {
        /* last strip first */
        if (image->strips > 1 && lastratio != 1.0)
        {
	    pdc_scale_matrix((float) 1.0, (float) lastratio, &m);
	    pdf_concat_raw(p, &m);
	}

        /* put out image strips separately if available */
        islast = image->strips - 1;
        imageno = image->no + islast;
        for (is = islast; is >= 0; is--)
        {
            pdc_printf(p->out, "/I%d Do\n", imageno);
            p->xobjects[imageno].flags |= xobj_flag_write;
            if (image->strips > 1 && is > 0)
            {
                pdc_translation_matrix(0, 1, &m);
                pdf_concat_raw(p, &m);
                if (is == islast && lastratio != 1.0)
		{
		    pdc_scale_matrix((float) 1.0, (float) (1. / lastratio), &m);
		    pdf_concat_raw(p, &m);
		}
                imageno--;
            }
        }
        if (image->mask != pdc_undef)
            p->xobjects[p->images[image->mask].no].flags |= xobj_flag_write;

        pdf__restore(p);
    }
}

#define MAX_THUMBNAIL_SIZE	106

PDFLIB_API void PDFLIB_CALL
PDF_add_thumbnail(PDF *p, int im)
{
    static const char fn[] = "PDF_add_thumbnail";
    pdf_image *image;

    if (!pdf_enter_api(p, fn, pdf_state_page, "(p[%p], %d)\n", (void *) p, im))
	return;

    PDF_INPUT_HANDLE(p, im)
    pdf_check_handle(p, im, pdc_imagehandle);

    if (p->thumb_id != PDC_BAD_ID)
	pdc_error(p->pdc, PDF_E_IMAGE_THUMB, 0, 0, 0, 0);

    image = &p->images[im];

    if (image->strips > 1)
	pdc_error(p->pdc, PDF_E_IMAGE_THUMB_MULTISTRIP,
	    pdc_errprintf(p->pdc, "%d", im), 0, 0, 0);

    if (image->width > MAX_THUMBNAIL_SIZE || image->height > MAX_THUMBNAIL_SIZE)
	pdc_error(p->pdc, PDF_E_IMAGE_THUMB_SIZE,
	    pdc_errprintf(p->pdc, "%d", im),
	    pdc_errprintf(p->pdc, "%d", MAX_THUMBNAIL_SIZE), 0, 0);

    if (image->colorspace != (int) DeviceGray &&
        image->colorspace != (int) DeviceRGB &&
        image->colorspace != (int) Indexed)
	pdc_error(p->pdc, PDF_E_IMAGE_THUMB_CS,
	    pdc_errprintf(p->pdc, "%d", im), 0, 0, 0);

    /* Add the image to the thumbnail key of the current page.  */
    p->thumb_id = p->xobjects[image->no].obj_id;
}

PDFLIB_API void PDFLIB_CALL
PDF_close_image(PDF *p, int image)
{
    static const char fn[] = "PDF_close_image";

    if (!pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_document | pdf_state_page | pdf_state_font),
	"(p[%p], %d)\n", (void *) p, image))
    {
	return;
    }

    PDF_INPUT_HANDLE(p, image)
    pdf_check_handle(p, image, pdc_imagehandle);

    pdf_cleanup_image(p, image);
}

/* interface for using image data directly in memory */

PDFLIB_API int PDFLIB_CALL
PDF_open_image(
    PDF *p,
    const char *type,
    const char *source,
    const char *data,
    long length,
    int width,
    int height,
    int components,
    int bpc,
    const char *params)
{
    static const char fn[] = "PDF_open_image";
    const char *filename = data;
    char optlist[512];
    pdc_bool memory = pdc_false;
    int retval = -1;

    /* precise scope diagnosis in pdf__load_image */
    if (!pdf_enter_api(p, fn, pdf_state_all,
	"(p[%p], \"%s\", \"%s\", data[%p], %ld, %d, %d, %d, %d, \"%s\")",
    	(void *) p, type, source, (void *) data, length,
	width, height, components, bpc, params))
    {
        PDF_RETURN_HANDLE(p, retval)
    }

    if (type == NULL || *type == '\0')
	pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "type", 0, 0, 0);

    if (source == NULL || *source == '\0')
	pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "source", 0, 0, 0);

    if (!strcmp(type, "raw") && data == NULL)
	pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "data", 0, 0, 0);

    /* create optlist */
    optlist[0] = 0;
    sprintf(optlist,"width %d  height %d  components %d  bpc %d ",
            width, height, components, bpc);

    if (length < 0L)
    {
        strcat(optlist, "bitreverse true ");
        length = -length;
    }

    strcat(optlist, "reftype ");
    if (!strcmp(source, "fileref"))
        strcat(optlist, "fileref ");
    else if (!strcmp(source, "memory"))
    {
        memory = pdc_true;
        strcat(optlist, "direct ");
    }
    else if (!strcmp(source, "url"))
        strcat(optlist, "url ");

    if (params != NULL && *params != '\0')
    {
        char **items;
        int i, nitems;

        /* separator characters because of compatibility */
        nitems = pdc_split_stringlist(p->pdc, params, "\t :", &items);
        for (i = 0; i < nitems; i++)
        {
            if (!strcmp(items[i], "invert"))
                strcat(optlist, "invert true ");
            else if (!strcmp(items[i], "ignoremask"))
                strcat(optlist, "ignoremask true ");
            else if (!strcmp(items[i], "inline"))
                strcat(optlist, "inline true ");
            else if (!strcmp(items[i], "interpolate"))
                strcat(optlist, "interpolate true ");
            else if (!strcmp(items[i], "mask"))
                strcat(optlist, "mask true ");
            else if (!strcmp(items[i], "/K"))
                strcat(optlist, "K ");
            else if (!strcmp(items[i], "/BlackIs1"))
                strcat(optlist, "invert ");
            else
                strcat(optlist, items[i]);
        }
        pdc_cleanup_stringlist(p->pdc, items);
    }

    /* create virtual file */
    if (memory)
    {
        filename = "__raw__image__data__";
        pdf__create_pvf(p, filename, 0, data, (size_t) length, "");
    }

    retval = pdf__load_image(p, type, filename, (const char *) optlist);

    if (memory)
        (void) pdf__delete_pvf(p, filename, 0);

    PDF_RETURN_HANDLE(p, retval)
}

PDFLIB_API int PDFLIB_CALL
PDF_open_image_file(
    PDF *p,
    const char *type,
    const char *filename,
    const char *stringparam,
    int intparam)
{
    static const char fn[] = "PDF_open_image_file";
    char optlist[256];
    int retval = -1;

    /* precise scope diagnosis in pdf__load_image */
    if (!pdf_enter_api(p, fn, pdf_state_all,
	"(p[%p], \"%s\", \"%s\", \"%s\", %d)",
	(void *) p, type, filename, stringparam, intparam))
    {
        PDF_RETURN_HANDLE(p, retval)
    }

    optlist[0] = 0;
    if (stringparam != NULL && *stringparam != '\0')
    {
        if (!strcmp(stringparam, "invert"))
            strcpy(optlist, "invert true ");
        else if (!strcmp(stringparam, "inline"))
            strcpy(optlist, "inline true ");
        else if (!strcmp(stringparam, "ignoremask"))
            strcpy(optlist, "ignoremask true ");
        else if (!strcmp(stringparam, "mask"))
            strcpy(optlist, "mask true ");
        else if (!strcmp(stringparam, "masked"))
            sprintf(optlist, "masked %d ", intparam);
        else if (!strcmp(stringparam, "colorize"))
            sprintf(optlist, "colorize %d ", intparam);
        else if (!strcmp(stringparam, "page"))
            sprintf(optlist, "page %d ", intparam);
        else if (!strcmp(stringparam, "iccprofile"))
            sprintf(optlist, "iccprofile %d ", intparam);
    }

    retval = pdf__load_image(p, type, filename, (const char *) optlist);

    PDF_RETURN_HANDLE(p, retval)
}

PDFLIB_API int PDFLIB_CALL
PDF_open_CCITT(PDF *p, const char *filename, int width, int height,
               int BitReverse, int K, int BlackIs1)
{
    static const char fn[] = "PDF_open_CCITT";
    int retval = -1;

    if (pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_document | pdf_state_page | pdf_state_font),
        "(p[%p], \"%s\", %d, %d, %d, %d, %d)",
        (void *) p, filename, width, height, BitReverse, K, BlackIs1))
    {
        /* create optlist */
        char optlist[256];
        sprintf(optlist,"width %d  height %d  bitreverse %s  K %d  invert %s",
            width, height, PDC_BOOLSTR(BitReverse), K, PDC_BOOLSTR(BlackIs1));

        retval = pdf__load_image(p, "CCITT", filename, (const char *) optlist);
    }

    PDF_RETURN_HANDLE(p, retval)
}

PDFLIB_API int PDFLIB_CALL
PDF_load_image(
    PDF *p,
    const char *type,
    const char *filename,
    int reserved,
    const char *optlist)
{
    static const char fn[] = "PDF_load_image";
    int retval = -1;

    /* precise scope diagnosis in pdf__load_image */
    if (pdf_enter_api(p, fn, pdf_state_all,
        "(p[%p], \"%s\", \"%s\", %d ,\"%s\")",
        (void *) p, type, filename, reserved, optlist))
    {
        retval = pdf__load_image(p, type, filename, optlist);
    }

    PDF_RETURN_HANDLE(p, retval)
}


/* keywords for options 'reftype' */
static const pdc_keyconn pdf_reftype_keys[] =
{
    {"direct",  pdf_ref_direct},
    {"fileref", pdf_ref_file},
    {"url",     pdf_ref_url},
    {NULL, 0}
};

/* allowed values for options 'bcp' */
static const pdc_keyconn pdf_bpcvalues[] =
{
    {"1", 1}, {"2", 2}, {"4", 4}, {"8", 8}, {NULL, 0}
};

/* allowed values for options 'components' */
static const pdc_keyconn pdf_compvalues[] =
{
    {"1", 1}, {"3", 3}, {"4", 4}, {NULL, 0}
};

#define PDF_ICCOPT_FLAG PDC_OPT_UNSUPP

/* definitions of open image options */
static const pdc_defopt pdf_open_image_options[] =
{
    {"bitreverse", pdc_booleanlist, 0, 1, 1, 0.0, 0.0, NULL},

    {"bpc", pdc_integerlist, PDC_OPT_INTLIST, 1, 1, 1.0, 8.0, pdf_bpcvalues},

    {"components", pdc_integerlist, PDC_OPT_INTLIST, 1, 1, 1.0, 4.0,
      pdf_compvalues},

    {"height", pdc_integerlist, 0, 1, 1, 1.0, INT_MAX,  NULL},

    {"honoriccprofile", pdc_booleanlist, PDF_ICCOPT_FLAG, 1, 1, 0.0, 0.0, NULL},

    /* ordering of the next three options is significant */

    {"iccprofile", pdc_iccprofilehandle, PDF_ICCOPT_FLAG, 1, 1, 1.0, 0.0, NULL},

    {"colorize", pdc_colorhandle, PDC_OPT_IGNOREIF1, 1, 1, 0.0, 0.0, NULL},

    {"mask", pdc_booleanlist, PDC_OPT_IGNOREIF2, 1, 1, 0.0, 0.0, NULL},

    {"ignoremask", pdc_booleanlist, 0, 1, 1, 0.0, 0.0, NULL},

    {"imagewarning", pdc_booleanlist, 0, 1, 1, 0.0, 0.0, NULL},

    {"inline", pdc_booleanlist, 0, 1, 1, 0.0, 0.0, NULL},

    {"interpolate", pdc_booleanlist, 0, 1, 1, 0.0, 0.0, NULL},

    {"invert", pdc_booleanlist, 0, 1, 1, 0.0, 0.0, NULL},

    {"K", pdc_integerlist, 0, 1, 1, -1.0, 1.0, NULL},

    {"masked", pdc_imagehandle, 0, 1, 1, 0.0, 0.0, NULL},

    {"page", pdc_integerlist, 0, 1, 1, 1.0, INT_MAX, NULL},

    {"renderingintent", pdc_keywordlist, 0, 1, 1, 0.0, 0.0,
      gs_renderingintents},

    {"reftype", pdc_keywordlist, 0, 1, 1, 0.0, 0.0, pdf_reftype_keys},

    {"width", pdc_integerlist, 0, 1, 1, 1.0, INT_MAX, NULL},

    PDC_OPT_TERMINATE
};

int
pdf__load_image(
    PDF *p,
    const char *type,
    const char *filename,
    const char *optlist)
{
    const char *keyword = NULL;
    char qualname[32];
    pdc_clientdata data;
    pdc_resopt *results;
    pdf_image_type imgtype;
    int colorize = pdc_undef;
    pdf_image *image;
    pdc_bool indjpeg = pdc_false;
    int legal_states = 0;
    int k, inum, imageslot, retval = -1;

    if (type == NULL || *type == '\0')
        pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "type", 0, 0, 0);

    if (filename == NULL || *filename == '\0')
        pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "filename", 0, 0, 0);

    /* parsing image type */
    k = pdc_get_keycode_ci(type, pdf_image_keylist);
    if (k == PDC_KEY_NOTFOUND)
        pdc_error(p->pdc, PDC_E_ILLARG_STRING, "type", type, 0, 0);
    imgtype = (pdf_image_type) k;
    type = pdc_get_keyword(imgtype, pdf_image_keylist);

    /* automatic check */
    if (imgtype == pdf_img_auto)
    {
        pdc_file *fp;
        pdf_jpeg_info jpeg_info;
        pdf_tiff_info tiff_info;

        if ((fp = pdf_fopen(p, filename, "", PDC_FILE_BINARY)) == NULL)
	{
	    if (p->debug['i'])
		pdc_error(p->pdc, -1, 0, 0, 0, 0);

            return -1;
	}

        if (pdf_is_BMP_file(p, fp))
            imgtype = pdf_img_bmp;
        else if (pdf_is_GIF_file(p, fp))
            imgtype = pdf_img_gif;
        else if (pdf_is_PNG_file(p, fp))
            imgtype = pdf_img_png;
        else if (pdf_is_TIFF_file(p, fp, &tiff_info, pdc_true))
            imgtype = pdf_img_tiff;
        else if (pdf_is_JPEG_file(p, fp, &jpeg_info))
            imgtype = pdf_img_jpeg;
        else
        {
            pdc_fclose(fp);

	    pdc_set_errmsg(p->pdc, PDF_E_IMAGE_UNKNOWN, filename, 0, 0, 0);

            if (p->debug['i'])
                pdc_error(p->pdc, -1, 0, 0, 0, 0);

            return -1;
        }
        pdc_fclose(fp);
        type = pdc_get_keyword(imgtype, pdf_image_keylist);
    }

    /* find free slot */
    for (imageslot = 0; imageslot < p->images_capacity; imageslot++)
        if (!p->images[imageslot].in_use)
            break;

    if (imageslot == p->images_capacity)
        pdf_grow_images(p);
    image = &p->images[imageslot];

    /* inherit global flags */
    image->verbose = p->debug['i'];
    image->ri = p->rendintent;

    /* parsing optlist */
    if (optlist && strlen(optlist))
    {
        data.compatibility = p->compatibility;
        data.maxcolor = p->colorspaces_number - 1;
        data.maximage = p->images_capacity - 1;
        data.hastobepos = p->hastobepos;
        results = pdc_parse_optionlist(p->pdc, optlist, pdf_open_image_options,
                                       &data, image->verbose);
        if (!results)
            return -1;

        /* save and check options */
        keyword = "imagewarning";
        pdc_get_optvalues(p->pdc, keyword, results,
                          &image->verbose, NULL);
        keyword = "reftype";
        if (pdc_get_optvalues(p->pdc, keyword, results, &inum, NULL))
        {
            image->reference = (pdf_ref_type) inum;
            if (image->reference != pdf_ref_direct &&
                imgtype != pdf_img_ccitt &&
                imgtype != pdf_img_jpeg &&
                imgtype != pdf_img_raw)
            {
                if (image->verbose)
                    pdc_warning(p->pdc, PDF_E_IMAGE_OPTUNSUPP, keyword, type,
                                0, 0);
                image->reference = pdf_ref_direct;
            }
        }
        indjpeg = (imgtype == pdf_img_jpeg &&
                   image->reference != pdf_ref_direct) ? pdc_true : pdc_false;

        keyword = "bpc";
        if (pdc_get_optvalues(p->pdc, keyword, results,
                              &image->bpc, NULL))
        {
            if (image->verbose && imgtype != pdf_img_raw && !indjpeg)
                pdc_warning(p->pdc, PDF_E_IMAGE_OPTUNREAS, keyword, type, 0, 0);
        }

        keyword = "components";
        if (pdc_get_optvalues(p->pdc, keyword, results,
                              &image->components, NULL))
        {
            if (image->verbose && imgtype != pdf_img_raw && !indjpeg)
                pdc_warning(p->pdc, PDF_E_IMAGE_OPTUNREAS, keyword, type, 0, 0);
        }

        keyword = "height";
        if (pdc_get_optvalues(p->pdc, keyword, results,
                              &image->height_pixel, NULL))
        {
            if (image->verbose && imgtype != pdf_img_ccitt &&
                                  imgtype != pdf_img_raw && !indjpeg)
                pdc_warning(p->pdc, PDF_E_IMAGE_OPTUNREAS, keyword, type, 0, 0);
        }

        keyword = "width";
        if (pdc_get_optvalues(p->pdc, keyword, results,
                              &image->width_pixel, NULL))
        {
            if (image->verbose && imgtype != pdf_img_raw &&
                                  imgtype != pdf_img_ccitt && !indjpeg)
                pdc_warning(p->pdc, PDF_E_IMAGE_OPTUNREAS, keyword, type, 0, 0);
        }

        keyword = "bitreverse";
        if (pdc_get_optvalues(p->pdc, keyword, results,
                              &image->bitreverse, NULL))
        {
            if (image->verbose && image->bitreverse &&
               (imgtype != pdf_img_ccitt || image->reference != pdf_ref_direct))
                pdc_warning(p->pdc, PDF_E_IMAGE_OPTUNREAS, keyword, type, 0, 0);
        }

        keyword = "colorize";
        pdc_get_optvalues(p->pdc, keyword, results, &colorize, NULL);


        keyword = "ignoremask";
        if (pdc_get_optvalues(p->pdc, keyword, results,
                              &image->ignoremask, NULL))
        {
            if (image->verbose && (imgtype == pdf_img_bmp ||
                                   imgtype == pdf_img_ccitt ||
                                   imgtype == pdf_img_raw))
                pdc_warning(p->pdc, PDF_E_IMAGE_OPTUNSUPP, keyword, type, 0, 0);
        }

        keyword = "inline";
        if (pdc_get_optvalues(p->pdc, keyword, results,
                              &image->doinline, NULL) && image->doinline)
        {
            if (imgtype != pdf_img_ccitt &&
                imgtype != pdf_img_jpeg &&
                imgtype != pdf_img_raw)
            {
                if (image->verbose)
                    pdc_warning(p->pdc,
                    PDF_E_IMAGE_OPTUNSUPP, keyword, type, 0, 0);
                image->doinline = pdc_false;
            }
            else if (image->verbose && image->reference != pdf_ref_direct)
            {
                pdc_warning(p->pdc, PDF_E_IMAGE_OPTUNREAS, keyword, type, 0, 0);
                image->doinline = pdc_false;
            }
        }

        keyword = "interpolate";
        pdc_get_optvalues(p->pdc, keyword, results,
                          &image->interpolate, NULL);

        keyword = "invert";
        pdc_get_optvalues(p->pdc, keyword, results,
                              &image->invert, NULL);

        keyword = "K";
        if (pdc_get_optvalues(p->pdc, keyword, results,
                              &image->K, NULL))
        {
            if (image->verbose && imgtype != pdf_img_ccitt)
                pdc_warning(p->pdc, PDF_E_IMAGE_OPTUNREAS, keyword, type, 0, 0);
        }

        keyword = "mask";
        pdc_get_optvalues(p->pdc, keyword, results,
                              &image->imagemask, NULL);

        keyword = "masked";
        if (pdc_get_optvalues(p->pdc, keyword, results,
                              &image->mask, NULL))
        {
            if (!p->images[image->mask].in_use ||
                 p->images[image->mask].strips != 1 ||
                (p->compatibility <= PDC_1_3 &&
                (p->images[image->mask].imagemask != pdc_true ||
                 p->images[image->mask].bpc != 1)))
            {
                pdc_set_errmsg(p->pdc, PDF_E_IMAGE_OPTBADMASK, keyword,
                          pdc_errprintf(p->pdc, "%d", image->mask), 0, 0);

                if (image->verbose)
                    pdc_error(p->pdc, -1, 0, 0, 0, 0);

                pdc_cleanup_optionlist(p->pdc, results);
                pdf_cleanup_image(p, imageslot);
                return -1;
            }
        }

        keyword = "renderingintent";
        if (pdc_get_optvalues(p->pdc, keyword, results, &inum, NULL))
            image->ri = (pdf_renderingintent) inum;

        keyword = "page";
        if (pdc_get_optvalues(p->pdc, keyword, results,
                              &image->page, NULL))
        {
            if (imgtype != pdf_img_gif && imgtype != pdf_img_tiff)
            {
                if (image->page == 1)
                {
                    if (image->verbose)
                        pdc_warning(p->pdc, PDF_E_IMAGE_OPTUNSUPP, keyword,
                                    type, 0, 0);
                }
                else
                {
                    pdc_set_errmsg(p->pdc, PDF_E_IMAGE_NOPAGE,
                              pdc_errprintf(p->pdc, "%d", image->page), type,
                              pdc_errprintf(p->pdc, "%s", filename), 0);

                    if (image->verbose)
                        pdc_error(p->pdc, -1, 0, 0, 0, 0);

                    pdc_cleanup_optionlist(p->pdc, results);
                    pdf_cleanup_image(p, imageslot);
                    return -1;
                }
            }
        }

        pdc_cleanup_optionlist(p->pdc, results);
    }

    /* precise scope diagnosis */
    if (image->doinline)
        legal_states = pdf_state_content;
    else
        legal_states = pdf_state_document | pdf_state_page | pdf_state_font;
    PDF_CHECK_STATE(p, legal_states);

    /* required options */
    if (imgtype == pdf_img_raw || imgtype == pdf_img_ccitt || indjpeg)
    {
        keyword = "";
        if (image->height_pixel == pdc_undef)
            keyword = "height";
        else if (image->width_pixel == pdc_undef)
            keyword = "width";
        else
        {
            image->width = (float) image->width_pixel;
            image->height = (float) image->height_pixel;
        }

        if (imgtype == pdf_img_ccitt)
        {
            image->components = 1;
            image->bpc = 1;
        }
        if (image->bpc == pdc_undef)
            keyword = "bpc";
        else if (image->components == pdc_undef)
            keyword = "components";

        if (*keyword)
        {
	    pdc_set_errmsg(p->pdc, PDC_E_OPT_NOTFOUND, keyword, 0, 0, 0);

            if (image->verbose)
		pdc_error(p->pdc, -1, 0, 0, 0, 0);

            pdf_cleanup_image(p, imageslot);
            return -1;
        }
    }

    /* set colorspace */
    if (colorize != pdc_undef)
    {
        image->colorspace = colorize;
    }
    else if (image->imagemask == pdc_true)
    {
        image->colorspace = (int) DeviceGray;
    }
    else
    {
        switch(image->components)
        {
            case 1:
            image->colorspace = DeviceGray;
            break;

            case 3:
            image->colorspace = DeviceRGB;
            break;

            case 4:
            image->colorspace = DeviceCMYK;
            break;

            default:
            break;
        }
    }

    /* try to open image file */
    if (image->reference == pdf_ref_direct)
    {
	strcpy(qualname, type);
	strcat(qualname, " ");
        image->fp = pdf_fopen(p, filename, qualname, PDC_FILE_BINARY);

        if (image->fp == NULL)
        {
	    if (image->verbose)
		pdc_error(p->pdc, -1, 0, 0, 0, 0);

	    pdf_cleanup_image(p, imageslot);
            return -1;
        }
    }

    /* copy filename */
    image->filename = pdc_strdup(p->pdc, filename);


    /* call working function */
    switch (imgtype)
    {
        case pdf_img_bmp:
        retval = pdf_process_BMP_data(p, imageslot);
        break;

        case pdf_img_ccitt:
        retval = pdf_process_CCITT_data(p, imageslot);
        break;

        case pdf_img_gif:
        retval = pdf_process_GIF_data(p, imageslot);
        break;

        case pdf_img_jpeg:
        retval = pdf_process_JPEG_data(p, imageslot);
        break;

        case pdf_img_png:
        retval = pdf_process_PNG_data(p, imageslot);
        break;

        default:
        case pdf_img_raw:
        retval = pdf_process_RAW_data(p, imageslot);
        break;

        case pdf_img_tiff:
        retval = pdf_process_TIFF_data(p, imageslot);
        break;
    }

    /* cleanup */
    if (retval == -1)
    {
        pdf_cleanup_image(p, imageslot);
    }
    else
    {
        if (image->fp)
            pdc_fclose(image->fp);
        image->fp = NULL;
    }

    return retval;
}
