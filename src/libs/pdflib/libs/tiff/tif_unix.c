/* PDFlib GmbH cvsid: $Id: tif_unix.c 14574 2005-10-29 16:27:43Z bonefish $ */

/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

/*
 * TIFF Library UNIX-specific Routines.
 */
#include "tiffiop.h"
#include "pc_config.h"
#include <stdlib.h>

static tsize_t
_tiffReadProc(void* fd, tdata_t buf, tsize_t size)
{
	return ((tsize_t) fread(buf, 1, (size_t) size, (FILE *)fd));
}

static tsize_t
_tiffWriteProc(void* fd, tdata_t buf, tsize_t size)
{
	return ((tsize_t) fwrite(buf, 1, (size_t) size, (FILE *)fd));
}

static toff_t
_tiffSeekProc(void* fd, toff_t off, int whence)
{
        return ((toff_t) fseek((FILE *)fd, (long) off, whence));
}

static int
_tiffCloseProc(void* fd)
{
        return (fclose((FILE *)fd));
}

static toff_t
_tiffSizeProc(void* fd)
{
#if defined(MVS) && defined(I370)
#define PDF_FTELL_BUSIZE        32000

	char buf[PDF_FTELL_BUSIZE];
	size_t filelen = 0;

        fseek((FILE *)fd, 0, SEEK_SET);

        while (!feof((FILE *)fd))
                filelen += fread((void *) buf, 1, PDF_FTELL_BUSIZE, (FILE *)fd);

	return filelen;
#else
        fseek((FILE *)fd, 0, SEEK_END);
        return (toff_t) ftell((FILE *)fd);
#endif
}

#ifdef HAVE_MMAP
#include <sys/mman.h>

static int
_tiffMapProc(void* fd, tdata_t* pbase, toff_t* psize)
{
        toff_t size = _tiffSizeProc((FILE *)fd);
	if (size != (toff_t) -1) {
		*pbase = (tdata_t)
                    mmap(0, size, PROT_READ, MAP_SHARED, (FILE *) fd, 0);
		if (*pbase != (tdata_t) -1) {
			*psize = size;
			return (1);
		}
	}
	return (0);
}

static void
_tiffUnmapProc(void* fd, tdata_t base, toff_t size)
{
	(void) fd;
	(void) munmap(base, (off_t) size);
}
#else /* !HAVE_MMAP */
static int
_tiffMapProc(void* fd, tdata_t* pbase, toff_t* psize)
{
	(void) fd; (void) pbase; (void) psize;
	return (0);
}

static void
_tiffUnmapProc(void* fd, tdata_t base, toff_t size)
{
	(void) fd; (void) base; (void) size;
}
#endif /* !HAVE_MMAP */

/*
 * Open a TIFF file descriptor for read/writing.
 */
TIFF*
TIFFFdOpen(void* fd, const char* name, const char* mode, void* pdflib_opaque,
        TIFFmallocHandler malloc_h, TIFFreallocHandler realloc_h,
        TIFFfreeHandler free_h, TIFFErrorHandler error_h,
        TIFFErrorHandler warn_h)
{
        TIFF* tif;

        tif = TIFFClientOpen(name, mode, fd,
            _tiffReadProc, _tiffWriteProc,
            _tiffSeekProc, _tiffCloseProc, _tiffSizeProc,
            _tiffMapProc, _tiffUnmapProc, pdflib_opaque,
            malloc_h, realloc_h, free_h, error_h, warn_h);
        if (tif)
                tif->tif_fd = (FILE *)fd;
        return (tif);
}

/*
 * Open a TIFF file for read/writing.
 */
TIFF*
TIFFOpen(const char* name, const char* mode, void* pdflib_opaque,
        TIFFmallocHandler malloc_h, TIFFreallocHandler realloc_h,
        TIFFfreeHandler free_h, TIFFErrorHandler error_h,
        TIFFErrorHandler warn_h)
{
        static const char module[] = "TIFFOpen";
        FILE *fd;

        if ((fd = fopen(name, READBMODE)) == NULL) {
                TIFFError(module, "%s: Cannot open", name);
                return ((TIFF *)0);
        }
        return (TIFFFdOpen(fd, name, mode, pdflib_opaque,
                        malloc_h, realloc_h, free_h, error_h, warn_h));
}

void*
_TIFFmalloc(TIFF* tif, tsize_t s)
{
        if (tif->pdflib_malloc != NULL)
            return (tif->pdflib_malloc(tif, (size_t) s));
        else
            return (malloc((size_t) s));
}

void
_TIFFfree(TIFF* tif, tdata_t p)
{
        if (tif->pdflib_free != NULL)
            tif->pdflib_free(tif, p);
        else
            free(p);
}

void*
_TIFFrealloc(TIFF* tif, tdata_t p, tsize_t s)
{
        if (tif->pdflib_realloc != NULL)
            return (tif->pdflib_realloc(tif, p, (size_t) s));
        else
            return (realloc(p, (size_t) s));
}

void
_TIFFmemset(tdata_t p, int v, tsize_t c)
{
	memset(p, v, (size_t) c);
}

void
_TIFFmemcpy(tdata_t d, const tdata_t s, tsize_t c)
{
	memcpy(d, s, (size_t) c);
}

int
_TIFFmemcmp(const tdata_t p1, const tdata_t p2, tsize_t c)
{
	return (memcmp(p1, p2, (size_t) c));
}

static void
unixWarningHandler(const char* module, const char* fmt, va_list ap)
{
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	fprintf(stderr, "Warning, ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
}
TIFFErrorHandler _TIFFwarningHandler = unixWarningHandler;

static void
unixErrorHandler(const char* module, const char* fmt, va_list ap)
{
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
}
TIFFErrorHandler _TIFFerrorHandler = unixErrorHandler;
