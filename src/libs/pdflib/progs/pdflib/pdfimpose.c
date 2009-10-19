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

/* $Id: pdfimpose.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Impose multiple PDF documents on a single sheet,
 * or concatenate multiple PDFs (if no -g option is supplied)
 * (requires the PDF import library PDI)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__CYGWIN32__)
#include <getopt.h>
#elif defined(WIN32)
int getopt(int argc, char * const argv[], const char *optstring);
extern char *optarg;
extern int optind;
#elif !defined(WIN32) && !defined(MAC)
#include <unistd.h>
#endif

#include "pdflib.h"

/* Array of known page sizes including name, width, and height */

typedef struct { const char *name; float width; float height; } PageSize_s;

PageSize_s PageSizes[] = {
    {"a0",      2380.0f, 3368.0f},
    {"a1",      1684.0f, 2380.0f},
    {"a2",      1190.0f, 1684.0f},
    {"a3",      842.0f, 1190.0f},
    {"a4",      595.0f, 842.0f},
    {"a5",      421.0f, 595.0f},
    {"a6",      297.0f, 421.0f},
    {"b5",      501.0f, 709.0f},
    {"letter",  612.0f, 792.0f},
    {"legal",   612.0f, 1008.0f},
    {"ledger",  1224.0f, 792.0f},
    {"p11x17",  792.0f, 1224.0f}
};

#define PAGESIZELISTLEN    (sizeof(PageSizes)/sizeof(PageSizes[0]))

static void
usage(void)
{
    fprintf(stderr,
	"\npdfimpose: impose multiple PDF documents on a single sheet.\n");
    fprintf(stderr, "(C) PDFlib GmbH and Thomas Merz 2001-2004\n");
    fprintf(stderr, "usage: pdfimpose [options] pdffiles(s)\n\n");
    fprintf(stderr, "Available options:\n");
    fprintf(stderr, "-b            print boxes around imposed pages\n");
    fprintf(stderr,
	"-g wxh        number of columns and rows per sheet (default: 1x1)\n");
    fprintf(stderr, "-l            landscape mode\n");
    fprintf(stderr, "-n            start each document on a new page\n");
    fprintf(stderr, "-o <file>     output file\n");
    fprintf(stderr, "-p <pagesize> page format (a0-a6, letter, legal, etc.)\n");
    fprintf(stderr, "-q            quiet mode: do not emit info messages\n");
    fprintf(stderr, "-v <version>  PDF output version: 1.3, 1.4, or 1.5\n");

    exit(1);
}

int
main(int argc, char *argv[])
{
    char	*pdffilename = NULL;
    char	*pdfversion = NULL;
    PDF		*p;
    int		opt;
    int		doc, page;
    int		pageno, docpages;
    char	*filename;
    int		quiet = 0, landscape = 0, boxes = 0, newpage = 0;
    int		cols = 1, rows = 1;
    int		c = 0, r = 0;
    float	sheetwidth = 595.0f, sheetheight = 842.0f;
    float	width, height, scale = 1.0f;
    float	rowheight = 0.0f, colwidth = 0.0f;
    
    while ((opt = getopt(argc, argv, "bg:lnp:o:qv:")) != -1)
	switch (opt) {
	    case 'b':
		boxes = 1;
		break;

	    case 'g':
		if (sscanf(optarg, "%dx%d", &rows, &cols) != 2) {
		    fprintf(stderr, "Error: Couldn't parse -g option.\n");
		    usage();
		}
		if (rows <= 0 || cols <= 0) {
		    fprintf(stderr, "Bad row or column number.\n");
		    usage();
		}
		break;

	    case 'l':
		landscape = 1;
		break;

	    case 'n':
		newpage = 1;
		break;

	    case 'p':
		for(c = 0; c < PAGESIZELISTLEN; c++)
		if (!strcmp((const char *) optarg, PageSizes[c].name)) {
		    sheetheight = PageSizes[c].height;
		    sheetwidth = PageSizes[c].width;
		    break;
		}
		if (c == PAGESIZELISTLEN) {  /* page size name not found */
		    fprintf(stderr, "Error: Unknown page size '%s'.\n", optarg);
		    usage();
		}
		break;

	    case 'o':
		pdffilename = optarg;
		break;

	    case 'v':
		pdfversion = optarg;
		if (strcmp(pdfversion, "1.3") && strcmp(pdfversion, "1.4") &&
		    strcmp(pdfversion, "1.5")) {
		    fprintf(stderr, "Error: bad PDF version number '%s'.\n",
		    	optarg);
		    usage();
		}

		break;

	    case 'q':
		quiet = 1;
		break;

	    case '?':
	    default:
		usage();
	}

    if (optind == argc) {
	fprintf(stderr, "Error: no PDF files given.\n");
	usage();
    }

    if (pdffilename == NULL) {
	fprintf(stderr, "Error: no PDF output file given.\n");
	usage();
    }

    p = PDF_new();

    if (pdfversion)
	PDF_set_parameter(p, "compatibility", pdfversion);

    if (PDF_open_file(p, pdffilename) == -1) {
	fprintf(stderr, "Error: %s.\n", PDF_get_errmsg(p));
	exit(1);
    }

    PDF_set_info(p, "Creator", "pdfimpose by PDFlib GmbH");

    PDF_set_parameter(p, "openaction", "fitpage");

    if (!quiet)
	PDF_set_parameter(p, "pdiwarning", "true"); /* report PDI problems */

    /* multi-page imposition: calculate scaling factor and cell dimensions */
    if (rows != 1 || cols != 1) {
	if (landscape) {
	    height = sheetheight;
	    sheetheight = sheetwidth;
	    sheetwidth = height;
	}

	if (rows > cols)
	    scale = 1.0f / rows;
	else
	    scale = 1.0f / cols;

	rowheight = sheetheight * scale;
	colwidth = sheetwidth * scale;
    }

    /* process all PDF documents */
    while (optind++ < argc) {
	filename = argv[optind-1];

	if (!quiet)
	    fprintf(stderr, "Imposing '%s'...\n", filename);

	if ((doc = PDF_open_pdi(p, filename, "", 0)) == -1) {
	    if (quiet)
		fprintf(stderr, "Error: %s.\n", PDF_get_errmsg(p));
	    continue;
	}

	/* query number of pages in the document */
	docpages = (int) PDF_get_pdi_value(p, "/Root/Pages/Count", doc, -1, 0);

	/* single cell only: concatenate, using original page dimensions */
	if (rows == 1 && cols == 1) {
	    /* open all pages and add to the output file */
	    for (pageno = 1; pageno <= docpages ; pageno++) {

		page = PDF_open_pdi_page(p, doc, pageno, "");

		if (page == -1) {
		    /* we'll get an exception in verbose mode anyway */
		    if (quiet)
			fprintf(stderr,
			    "Couldn't open page %d of PDF file '%s' (%s)\n",
			    pageno, filename, PDF_get_errmsg(p));
		    break;
		}

		sheetwidth = PDF_get_pdi_value(p, "width", doc, page, 0);
		sheetheight = PDF_get_pdi_value(p, "height", doc, page, 0);

		PDF_begin_page(p, sheetwidth, sheetheight);

		/* define bookmark with filename */
		if (pageno == 1)
		    PDF_add_bookmark(p, argv[optind-1], 0, 0);

		PDF_place_pdi_page(p, page, 0.0f, 0.0f, 1.0f, 1.0f);
		PDF_close_pdi_page(p, page);
		PDF_end_page(p);
	    }

	} else {		/* impose multiple pages */

	    if (newpage)
		r = c = 0;

	    /* open all pages and add to the output file */
	    for (pageno = 1; pageno <= docpages ; pageno++) {

		page = PDF_open_pdi_page(p, doc, pageno, "");

		if (page == -1) {
		    /* we'll get an exception in verbose mode anyway */
		    if (quiet)
			fprintf(stderr,
			    "Couldn't open page %d of PDF file '%s' (%s)\n",
			    pageno, filename, PDF_get_errmsg(p));
		    break;
		}

		/* start a new page */
		if (r == 0 && c == 0)
		    PDF_begin_page(p, sheetwidth, sheetheight);
		
		/* define bookmark with filename */
		if (pageno == 1)
		    PDF_add_bookmark(p, argv[optind-1], 0, 0);

		width = PDF_get_pdi_value(p, "width", doc, page, 0);
		height = PDF_get_pdi_value(p, "height", doc, page, 0);

		/*
		 * The save/restore pair is required to get the clipping right,
		 * and helps PostScript printing manage its memory efficiently.
		 */
		PDF_save(p);
		PDF_rect(p, c * colwidth, sheetheight - (r + 1) * rowheight,
		    colwidth, rowheight);
		PDF_clip(p);

		PDF_setcolor(p, "stroke", "gray", 0.0f, 0.0f, 0.0f, 0.0f);

		/* TODO: adjust scaling factor if page doesn't fit into  cell */
		PDF_place_pdi_page(p, page,
		    c * colwidth, sheetheight - (r + 1) * rowheight,
		    scale, scale);

		PDF_close_pdi_page(p, page);

		/* only half of the linewidth will be drawn due to clip() */
		if (boxes) {
		    PDF_setlinewidth(p, 1.0f * scale);
		    PDF_rect(p, c * colwidth,
			sheetheight - (r + 1) * rowheight,
			colwidth, rowheight);
		    PDF_stroke(p);
		}

		PDF_restore(p);

		c++;
		if (c == cols) {
		    c = 0;
		    r++;
		}
		if (r == rows) {
		    r = 0;
		    PDF_end_page(p);
		}
	    }
	}

	PDF_close_pdi(p, doc);
    }

    /* finish last page if multi-page imposition */
    if ((rows != 1 || cols != 1) && (r != 0 || c != 0))
	PDF_end_page(p);

    PDF_close(p);
    PDF_delete(p);
    exit(0);
}
