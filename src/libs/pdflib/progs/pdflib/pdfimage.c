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

/* $Id: pdfimage.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Convert BMP/PNG/TIFF/GIF/JPEG images to PDF
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#if defined(__CYGWIN32__)
#include <getopt.h>
#elif defined(WIN32)
int getopt(int argc, char * const argv[], const char *optstring);
extern char *optarg;
extern int optind;
#elif !defined(WIN32) && !defined(MAC)
#include <unistd.h>
#endif

#ifdef WIN32
#include <process.h>
#endif

#ifdef NeXT
#include <libc.h>	/* for getopt(), optind, optarg */
#endif

#ifdef __CYGWIN32__
#include <getopt.h>	/* for getopt(), optind, optarg */
#endif

#include "pdflib.h"

#if defined WIN32 || defined __DJGPP__ || \
    defined __OS2__ || defined __IBMC__ || defined __IBMCPP__ || \
    defined __POWERPC__ || defined __CFM68K__ || defined __MC68K__

#define READMODE	"rb"

#else

#define READMODE	"r"

#endif	/* Mac, Windows, and OS/2 platforms */

static void
usage(void)
{
    fprintf(stderr, "pdfimage: convert images to PDF.\n");
    fprintf(stderr, "(C) PDFlib GmbH and Thomas Merz 1997-2004\n");
    fprintf(stderr, "usage: pdfimage [options] imagefile(s)\n");
    fprintf(stderr, "Available options:\n");
    fprintf(stderr, "-o <file>  output file (required)\n");
    fprintf(stderr, "-p <num>   bookmark page numbering starting from num\n");
    fprintf(stderr, "-r <res>   force resolution overriding image settings\n");

    exit(1);
}

int
main(int argc, char *argv[])
{
    char	*pdffilename = NULL;
    FILE	*imagefile;
    PDF		*p;
    int		image;
    int		opt;
    int		resolution = 0;
    int		page_numbering = 0;
    int		current_page = 1;
    char	optlist[128];

    while ((opt = getopt(argc, argv, "r:o:p:w")) != -1)
	switch (opt) {
	    case 'o':
		pdffilename = optarg;
		break;

	    case 'p':
		page_numbering = 1;
		if (optarg) {
		    current_page = atoi(optarg);
		}
		break;

	    case 'r':
		if (!optarg || (resolution = atoi(optarg)) <= 0) {
		    fprintf(stderr, "Error: non-positive resolution.\n");
		    usage();
		}

	    case '?':
	    default:
		usage();
	}

    if (optind == argc) {
	fprintf(stderr, "Error: no image files given.\n");
	usage();
    }

    if (pdffilename == NULL) {
	fprintf(stderr, "Error: no output file given.\n");
	usage();
    }

    p = PDF_new();

    if (PDF_open_file(p, pdffilename) == -1) {
	fprintf(stderr, "Error: cannot open output file %s.\n", pdffilename);
	exit(1);
    }

    PDF_set_info(p, "Creator", "pdfimage");

    while (optind++ < argc) {
	fprintf(stderr, "Processing image file '%s'...\n", argv[optind-1]);

	image = PDF_load_image(p, "auto", argv[optind-1], 0, "");

	if (image == -1) {
	    fprintf(stderr, "Error: %s (skipped).\n", PDF_get_errmsg(p));
	    continue;
	}

	/* dummy page size, will be adjusted later */
	PDF_begin_page(p, 20, 20);

	/* define outline with filename or page number */
	if (page_numbering) {
	    char buf[32];
	    sprintf(buf, "Page %d", current_page++);
	    PDF_add_bookmark(p, buf, 0, 0);
	} else {
	    PDF_add_bookmark(p, argv[optind-1], 0, 0);
	}

	if (resolution != 0)
	    sprintf(optlist, "dpi %d", resolution);
	else
	    sprintf(optlist, "adjustpage");

	PDF_fit_image(p, image, 0.0, 0.0, optlist);

	PDF_end_page(p);
    }

    PDF_close(p);
    PDF_delete(p);
    exit(0);
}
