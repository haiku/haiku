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

/* $Id: pc_output.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib output routines
 *
 */

#include "pc_util.h"
#include "pc_md5.h"

#if (defined(WIN32) || defined(OS2)) && !defined(WINCE)
#include <fcntl.h>
#include <io.h>
#endif

#if defined(MAC) || defined (MACOSX)

/*
 * Setting the file type requires either Carbon or InterfaceLib/Classic.
 * If we have neither (i.e. a Mach-O build without Carbon) we suppress
 * the code for setting the file type and creator.
 */

#if !defined(MACOSX) || defined(PDF_TARGET_API_MAC_CARBON)
#define PDF_FILETYPE_SUPPORTED
#endif

#ifdef PDF_FILETYPE_SUPPORTED
#include <Files.h>
#endif

#endif /* defined(MAC) || defined (MACOSX) */

#ifdef HAVE_LIBZ
#include "zlib.h"
#endif

#ifdef HAVE_LIBZ
#define PDF_DEFAULT_COMPRESSION	6		/* default zlib level */
#else
#define PDF_DEFAULT_COMPRESSION	0		/* no compression */
#endif

#define STREAM_CHUNKSIZE	65536		/* output buffer for in-core */
#define ID_CHUNKSIZE		2048		/* object ids */

#define PDF_LINEBUFLEN		256		/* len of pdc_printf() buffer */

/* generic output stuff */

typedef struct pdc_stream_s {
    pdc_byte	*basepos;		/* start of this chunk */
    pdc_byte	*curpos;		/* next write position */
    pdc_byte	*maxpos;		/* maximum position of chunk */
    size_t	base_offset;		/* base offset of this chunk */
    pdc_bool	compressing;		/* in compression state */
#ifdef HAVE_LIBZ
    z_stream	z;			/* zlib compression stream */
#endif

    pdc_bool	mustclose;		/* must close output file at end */
    FILE	*fp;			/* output file stream */
    /* client-supplied data sink procedure */
    size_t	(*writeproc)(pdc_output *out, void *data, size_t size);
} pdc_stream;

/* PDF-related output stuff */

struct pdc_output_s {
    pdc_core	*pdc;			/* core context */
    pdc_stream 	stream;			/* output stream buffer */
    int		compresslevel;		/* zlib compression level */
    pdc_bool	compr_changed;		/* compress level has been changed */
    long	length;			/* length of stream */

    long	*file_offset;		/* the objects' file offsets */
    int		file_offset_capacity;
    pdc_id	lastobj;		/* highest object id */

    long	start_pos;		/* stream start position */

    MD5_CTX	md5;			/* MD5 digest context for file ID */
    unsigned char id[2][MD5_DIGEST_LENGTH];
    void	*opaque;		/* this will be used to store PDF *p */
};

/* --------------------- PDFlib stream handling ----------------------- */

void *
pdc_get_opaque(pdc_output *out)
{
    return out->opaque;
}

#ifdef HAVE_LIBZ
/* wrapper for pdc_malloc for use in zlib */
static voidpf
pdc_zlib_alloc(voidpf pdc, uInt items, uInt size)
{
    return (voidpf) pdc_malloc((pdc_core *) pdc, items * size, "zlib");
}

#endif	/* HAVE_LIBZ */

pdc_bool
pdc_stream_not_empty(pdc_output *out)
{
    pdc_stream *stream = &out->stream;

    return(!stream->writeproc && stream->curpos != stream->basepos);
}

const char *
pdc_get_stream_contents(pdc_output *out, long *size)
{
    pdc_stream *stream = &out->stream;
    pdc_core *pdc = out->pdc;

    if (stream->writeproc)
	pdc_error(pdc, PDC_E_IO_NOBUFFER, 0, 0, 0, 0);

    *size = (long) (stream->curpos - stream->basepos);

    stream->base_offset += (size_t) (stream->curpos - stream->basepos);
    stream->curpos = stream->basepos;

    return (const char *) stream->basepos;
}

static void
pdc_boot_stream(pdc_output *out)
{
    pdc_stream *stream = &out->stream;

    /* curpos must be initialized here so that the check for empty
     * buffer in PDF_delete() also works in the degenerate case of
     * no output produced.
     */
    stream->basepos = stream->curpos = NULL;
}

static size_t
pdc_writeproc_file(pdc_output *out, void *data, size_t size)
{
    pdc_stream *stream = &out->stream;

    return fwrite(data, 1, (size_t) size, stream->fp);
}

#if defined(_MSC_VER) && defined(_MANAGED)
#pragma managed
#endif
static pdc_bool
pdc_init_stream(
    pdc_core *pdc,
    pdc_output *out,
    const char *filename,
    FILE *fp,
    size_t (*writeproc)(pdc_output *out, void *data, size_t size))
{
    pdc_stream *stream = &out->stream;

#if defined(MAC) || defined(MACOSX)
#if !defined(TARGET_API_MAC_CARBON) || defined(__MWERKS__)
    FCBPBRec	fcbInfo;
    Str32	name;
#endif	/* TARGET_API_MAC_CARBON */
    FInfo	fInfo;
    FSSpec	fSpec;
#endif	/* defined(MAC) || defined(MACOSX) */

    /*
     * This may be left over from the previous run. We deliberately
     * don't reuse the previous buffer in order to avoid potentially
     * unwanted growth of the allocated buffer due to a single large
     * document in a longer series of documents.
     */
    if (stream->basepos)
	pdc_free(pdc, (void *) stream->basepos);

    stream->basepos	= (pdc_byte *)pdc_malloc(pdc, STREAM_CHUNKSIZE,
				    "pdc_init_stream");
    stream->curpos	= stream->basepos;
    stream->maxpos	= stream->basepos + STREAM_CHUNKSIZE;
    stream->base_offset	= 0L;
    stream->compressing	= pdc_false;

#ifdef HAVE_LIBZ
    /* zlib sometimes reads uninitialized memory where it shouldn't... */
    memset(&stream->z, 0, sizeof stream->z);

    stream->z.zalloc	= (alloc_func) pdc_zlib_alloc;
    stream->z.zfree	= (free_func) pdc_free;
    stream->z.opaque	= (voidpf) pdc;

    if (deflateInit(&stream->z, pdc_get_compresslevel(out)) != Z_OK)
	pdc_error(pdc, PDC_E_IO_COMPRESS, "deflateInit", 0, 0, 0);

    out->compr_changed = pdc_false;
#endif

    /* Defaults */
    stream->mustclose	= pdc_false;
    stream->fp		= (FILE *) NULL;
    stream->writeproc	= pdc_writeproc_file;

    if (fp) {
	/* PDF_open_fp */
	stream->fp	= fp;

    } else if (writeproc) {
	stream->writeproc	= writeproc;		/* PDF_open_mem */

    } else if (filename == NULL || *filename == '\0') {
	/* PDF_open_file with in-core output */
	stream->writeproc = NULL;

    } else {
	/* PDF_open_file with file output */
#if !((defined(MAC) || defined (MACOSX)) && defined(__MWERKS__))
	if (filename && !strcmp(filename, "-")) {
	    stream->fp = stdout;
#if !defined(__MWERKS__) && (defined(WIN32) || defined(OS2))
#if defined WINCE
	    _setmode(fileno(stdout), _O_BINARY);
#else
	    setmode(fileno(stdout), O_BINARY);
#endif /* !WINCE */
#endif
	} else {
#endif /* !MAC */

	    stream->fp = fopen(filename, WRITEMODE);

	    if (stream->fp == NULL)
		return pdc_false;

	    stream->mustclose = pdc_true;

#ifdef PDF_FILETYPE_SUPPORTED

#if defined(MAC) || defined(MACOSX)
	    /* set the proper type and creator for the output file */
#if TARGET_API_MAC_CARBON && !defined(__MWERKS__)

	    if (FSPathMakeFSSpec((const UInt8 *) filename, &fSpec) == noErr) {
		FSpGetFInfo(&fSpec, &fInfo);

		fInfo.fdType = 'PDF ';
		fInfo.fdCreator = 'CARO';
		FSpSetFInfo(&fSpec, &fInfo);
	    }

#else

	    memset(&fcbInfo, 0, sizeof(FCBPBRec));
	    fcbInfo.ioRefNum = (short) stream->fp->handle;
	    fcbInfo.ioNamePtr = name;

	    if (!PBGetFCBInfoSync(&fcbInfo) &&
		FSMakeFSSpec(fcbInfo.ioFCBVRefNum, fcbInfo.ioFCBParID,
		name, &fSpec) == noErr) {
		    FSpGetFInfo(&fSpec, &fInfo);
		    fInfo.fdType = 'PDF ';
		    fInfo.fdCreator = 'CARO';
		    FSpSetFInfo(&fSpec, &fInfo);
	    }
#endif  /* !defined(TARGET_API_MAC_CARBON) || defined(__MWERKS__) */
#endif	/* defined(MAC) || defined(MACOSX) */

#endif /* PDF_FILETYPE_SUPPORTED */

#if !((defined(MAC) || defined (MACOSX)) && defined(__MWERKS__))
	}
#endif /* !MAC */
    }

    return pdc_true;
}
#if defined(_MSC_VER) && defined(_MANAGED)
#pragma managed
#endif

/* close the output file, if opened with PDF_open_file();
 * close the output stream if opened
 */

static void
pdc_close_stream(pdc_output *out)
{
    pdc_stream *stream = &out->stream;

    pdc_flush_stream(out);

#ifdef HAVE_LIBZ
    /*
     * This is delicate: we must ignore the return value because of the
     * following reasoning: We are called in two situations:
     * - end of document
     * - exception
     * In the first case compression is inactive, so deflateEnd() will
     * fail only in the second case. However, when an exception occurs
     * the document is definitely unusable, so we avoid recursive exceptions
     * or an (unallowed) exception in PDF_delete().
     */
     
    (void) deflateEnd(&stream->z);
#endif

    /* close the output file if writing to file, but do not close the
     * in-core output stream since the caller will have to
     * fetch the buffer after PDF_close().
     */
    if (stream->fp == NULL || !stream->writeproc)
	return;

    if (stream->mustclose)
	fclose(stream->fp);

    /* mark fp as dead in case the error handler jumps in later */
    stream->fp = NULL;
}


static void
pdc_check_stream(pdc_output *out, size_t len)
{
    size_t max;
    int cur;
    pdc_stream *stream = &out->stream;
    pdc_core *pdc = out->pdc;

    if (stream->curpos + len <= stream->maxpos)
	return;

    pdc_flush_stream(out);

    if (stream->curpos + len <= stream->maxpos)
	return;

    max = (size_t) (2*(stream->maxpos - stream->basepos));
    cur = stream->curpos - stream->basepos;

    stream->basepos = (pdc_byte *)
	pdc_realloc(pdc, (void *) stream->basepos, max, "pdc_check_stream");
    stream->maxpos = stream->basepos + max;
    stream->curpos = stream->basepos + cur;

    pdc_check_stream(out, len);
}

void
pdc_flush_stream(pdc_output *out)
{
    size_t size;
    pdc_stream *stream = &out->stream;
    pdc_core *pdc = out->pdc;

    if (!stream->writeproc || stream->compressing)
	return;

    size = (size_t) (stream->curpos - stream->basepos);

    if (stream->writeproc(out, (void *) stream->basepos, size) != size) {
	pdc_free(pdc, stream->basepos);
	stream->basepos = NULL;
	pdc_error(pdc, PDC_E_IO_NOWRITE, 0, 0, 0, 0);
    }

    stream->base_offset += (size_t) (stream->curpos - stream->basepos);
    stream->curpos = stream->basepos;
}

void
pdc_cleanup_stream(pdc_output *out)
{
    pdc_stream *stream = &out->stream;
    pdc_core *pdc = out->pdc;

    /* this may happen in rare cases */
    if (!stream->basepos)
	return;

    if (stream->basepos) {
	pdc_free(pdc, (void *) stream->basepos);
	stream->basepos = NULL;
    }
}

long
pdc_tell_out(pdc_output *out)
{
    pdc_stream *stream = &out->stream;

    return(long)(stream->base_offset + (stream->curpos - stream->basepos));
}

/* --------------------- compression handling ----------------------- */

static void
pdc_begin_compress(pdc_output *out)
{
    pdc_stream *stream = &out->stream;
    pdc_core *pdc = out->pdc;

    if (!pdc_get_compresslevel(out)) {
	stream->compressing = pdc_false;
	return;
    }

#ifdef HAVE_LIBZ
    if (out->compr_changed)
    {
	if (deflateEnd(&stream->z) != Z_OK)
	    pdc_error(pdc, PDC_E_IO_COMPRESS, "deflateEnd", 0, 0, 0);

	if (deflateInit(&stream->z, pdc_get_compresslevel(out)) != Z_OK)
	    pdc_error(pdc, PDC_E_IO_COMPRESS, "deflateInit", 0, 0, 0);

	out->compr_changed = pdc_false;
    }
    else
    {
	if (deflateReset(&stream->z) != Z_OK)
	    pdc_error(pdc, PDC_E_IO_COMPRESS, "deflateReset", 0, 0, 0);
    }

    stream->z.avail_in = 0;
#endif /* HAVE_LIBZ */

    stream->compressing = pdc_true;
}


static void
pdc_end_compress(pdc_output *out)
{
    int status;
    pdc_stream *stream = &out->stream;
    pdc_core *pdc = out->pdc;

    /* this may happen during cleanup triggered by an exception handler */
    if (!stream->compressing)
	return;

    if (!pdc_get_compresslevel(out)) {
	stream->compressing = pdc_false;
	return;
    }


#ifdef HAVE_LIBZ
    /* Finish the stream */
    do {
	pdc_check_stream(out, 128);
	stream->z.next_out = (Bytef *) stream->curpos;
	stream->z.avail_out = (uInt) (stream->maxpos - stream->curpos);

	status = deflate(&(stream->z), Z_FINISH);
	stream->curpos = stream->z.next_out;

	if (status != Z_STREAM_END && status != Z_OK)
	    pdc_error(pdc, PDC_E_IO_COMPRESS, "Z_FINISH", 0, 0, 0);

    } while (status != Z_STREAM_END);

    stream->compressing = pdc_false;
#endif /* HAVE_LIBZ */
}

/* ---------------------- Low-level output function ---------------------- */
/*
 * Write binary data to the output without any modification,
 * and apply compression if we are currently in compression mode.
 */


void
pdc_write(pdc_output *out, const void *data, size_t size)
{
    pdc_stream *stream = &out->stream;
    int estimate = 0;
    pdc_core *pdc = out->pdc;

#ifdef HAVE_LIBZ
    if (stream->compressing) {
	stream->z.avail_in	= (uInt) size;
	stream->z.next_in	= (Bytef *) data;
	stream->z.avail_out	= 0;

	while (stream->z.avail_in > 0) {
	    if (stream->z.avail_out == 0) {
		/* estimate output buffer size */
		estimate = (int) (stream->z.avail_in/4 + 16);
		pdc_check_stream(out, (size_t) estimate);
		stream->z.next_out = (Bytef *) stream->curpos;
		stream->z.avail_out = (uInt) (stream->maxpos - stream->curpos);
	    }

	    if (deflate(&(stream->z), Z_NO_FLUSH) != Z_OK)
		pdc_error(pdc, PDC_E_IO_COMPRESS, "Z_NO_FLUSH", 0, 0, 0);

	    stream->curpos = stream->z.next_out;
	}
    } else {
#endif /* HAVE_LIBZ */

	pdc_check_stream(out, size);
	memcpy(stream->curpos, data, size);
	stream->curpos += size;

#ifdef HAVE_LIBZ
    }
#endif /* HAVE_LIBZ */
}

/* --------------------------- Setup --------------------------- */

void *
pdc_boot_output(pdc_core *pdc)
{
    static const char *fn = "pdc_boot_output";
    pdc_output *out;

    out = (pdc_output*)pdc_malloc(pdc, sizeof(pdc_output), fn);
    out->pdc = pdc;

    out->file_offset	= NULL;

    pdc_boot_stream(out);

    return (void *) out;
}

/*
 * Initialize the PDF output
 * only one of filename, fp, writeproc must be supplied,
 * the others must be NULL:
 * filename	use supplied file name to create a named output file
 *		filename == "" means generate output in-core
 * fp		use supplied FILE * to write to file
 * writeproc	use supplied procedure to write output data
 *
 * Note that the caller is responsible for supplying sensible arguments.
 */

#if defined(_MSC_VER) && defined(_MANAGED)
#pragma managed
#endif
pdc_bool
pdc_init_output(
    void *opaque,
    pdc_output *out,
    const char *filename,
    FILE *fp,
    size_t (*writeproc)(pdc_output *out, void *data, size_t size),
    int compatibility)
{
    static const char *fn = "pdc_init_output";
    pdc_core *pdc = out->pdc;
    int i;

    out->lastobj = 0;

    if (out->file_offset == NULL) {
	out->file_offset_capacity = + ID_CHUNKSIZE;

	out->file_offset = (long *) pdc_malloc(pdc,
		sizeof(long) * out->file_offset_capacity, fn);
    }

    for (i = 1; i < out->file_offset_capacity; ++i)
	out->file_offset[i] = PDC_BAD_ID;

    out->compresslevel	= PDF_DEFAULT_COMPRESSION;
    out->compr_changed	= pdc_false;

    out->opaque		= opaque;

    memcpy(out->id[0], out->id[1], MD5_DIGEST_LENGTH);


    if (!pdc_init_stream(pdc, out, filename, fp, writeproc))
	return pdc_false;

    /* Write the document header */
    if (compatibility == PDC_1_5)
	pdc_puts(out, "%PDF-1.5\n");
    else if (compatibility == PDC_1_4)
	pdc_puts(out, "%PDF-1.4\n");
    else
	pdc_puts(out, "%PDF-1.3\n");

    /* binary magic number */
#define PDC_MAGIC_BINARY "\045\344\343\317\322\012"
    pdc_write(out, PDC_MAGIC_BINARY, sizeof(PDC_MAGIC_BINARY) - 1);

    return pdc_true;
}
#if defined(_MSC_VER) && defined(_MANAGED)
#pragma managed
#endif

void
pdc_close_output(pdc_output *out)
{
    pdc_close_stream(out);

    if (out->file_offset)
    {
	pdc_free(out->pdc, out->file_offset);
	out->file_offset = 0;
    }

}

void
pdc_cleanup_output(pdc_output *out)
{
    pdc_core *pdc = out->pdc;

    if (out->file_offset) {
	pdc_free(pdc, out->file_offset);
	out->file_offset = NULL;
    }


    pdc_cleanup_stream(out);

}

/* --------------------------- Digest --------------------------- */

void
pdc_init_digest(pdc_output *out)
{
    MD5_Init(&out->md5);
}

void
pdc_update_digest(pdc_output *out, unsigned char *input,
    unsigned int len)
{
    MD5_Update(&out->md5, input, len);
}

void
pdc_finish_digest(pdc_output *out)
{
    MD5_Final(out->id[1], &out->md5);
}

/* --------------------------- Objects and ids --------------------------- */

pdc_id
pdc_begin_obj(pdc_output *out, pdc_id obj_id)
{
    if (obj_id == PDC_NEW_ID)
	obj_id = pdc_alloc_id(out);

    out->file_offset[obj_id] = pdc_tell_out(out);
    pdc_printf(out, "%ld 0 obj\n", obj_id);

    return obj_id;
}

pdc_id
pdc_alloc_id(pdc_output *out)
{
    pdc_core *pdc = out->pdc;

    out->lastobj++;

    if (out->lastobj >= out->file_offset_capacity) {
	out->file_offset_capacity *= 2;
	out->file_offset = (long *) pdc_realloc(pdc, out->file_offset,
		    sizeof(long) * out->file_offset_capacity, "pdc_alloc_id");
    }

    /* only needed for verifying obj table in PDF_close() */
    out->file_offset[out->lastobj] = PDC_BAD_ID;

    return out->lastobj;
}

/* --------------------------- Strings --------------------------- */

void
pdc_put_pdfstring(pdc_output *out, const char *text, int len)
{
    const unsigned char *goal, *s;

    pdc_putc(out, PDF_PARENLEFT);

    goal = (const unsigned char *) text + len;

    for (s = (const unsigned char *) text; s < goal; s++) {
	switch (*s) {
	    case PDF_RETURN:
		pdc_putc(out, PDF_BACKSLASH);
		pdc_putc(out, PDF_r);
		break;

	    case PDF_NEWLINE:
		pdc_putc(out, PDF_BACKSLASH);
		pdc_putc(out, PDF_n);
		break;

	    default:
		if (*s == PDF_PARENLEFT || *s == PDF_PARENRIGHT ||
		    *s == PDF_BACKSLASH)
		    pdc_putc(out, PDF_BACKSLASH);
		pdc_putc(out, (char) *s);
	}
    }

    pdc_putc(out, PDF_PARENRIGHT);
}

void
pdc_put_pdfunistring(pdc_output *out, const char *text)
{
    int len;

    len = (int) pdc_strlen(text) - 1;	/* subtract a null byte... */

    /* ...and possibly another one */
    if (pdc_is_unicode(text))
	len--;

    pdc_put_pdfstring(out, text, len);
}

/* --------------------------- Streams --------------------------- */

void
pdc_begin_pdfstream(pdc_output *out)
{
    pdc_puts(out, "stream\n");

    out->start_pos = pdc_tell_out(out);

    if (out->compresslevel)
	pdc_begin_compress(out);
}

void
pdc_end_pdfstream(pdc_output *out)
{
    if (out->compresslevel)
	pdc_end_compress(out);

    out->length = pdc_tell_out(out) - out->start_pos;

    /* some PDF consumers seem to need the additional "\n" before "endstream",
    ** the PDF reference allows it, and Acrobat's "repair" feature relies on it.
    */
    pdc_puts(out, "\nendstream\n");
}

void
pdc_put_pdfstreamlength(pdc_output *out, pdc_id length_id)
{

    pdc_begin_obj(out, length_id);	/* Length object */
    pdc_printf(out, "%ld\n", out->length);
    pdc_end_obj(out);
}

void
pdc_set_compresslevel(pdc_output *out, int compresslevel)
{
    out->compresslevel = compresslevel;
    out->compr_changed = pdc_true;
}

int
pdc_get_compresslevel(pdc_output *out)
{
    return out->compresslevel;
}

/* --------------------------- Names --------------------------- */

/* characters illegal in PDF names: "()<>[]{}/%#" */
#define PDF_ILL_IN_NAMES "\050\051\074\076\133\135\173\175\057\045\043"

#define PDF_NEEDS_QUOTE(c) \
	((c) < 33 || (c) > 126 || strchr(PDF_ILL_IN_NAMES, (c)) != (char *) 0)

void
pdc_put_pdfname(pdc_output *out, const char *text, size_t len)
{
    const unsigned char *goal, *s;
    static const char BinToHex[] = PDF_STRING_0123456789ABCDEF;

    goal = (const unsigned char *) text + len;

    pdc_putc(out, PDF_SLASH);

    for (s = (const unsigned char *) text; s < goal; s++) {
	if (PDF_NEEDS_QUOTE(*s)) {
	    pdc_putc(out, PDF_HASH);
	    pdc_putc(out, BinToHex[*s >> 4]);	/* first nibble  */
	    pdc_putc(out, BinToHex[*s & 0x0F]);	/* second nibble  */
	} else
	    pdc_putc(out, (char) *s);
    }
}


char *
pdc_make_quoted_pdfname(pdc_output *out, const char *text,
    size_t len, char *buf)
{
    const unsigned char *goal, *s;
    unsigned char *cur = (unsigned char *) buf;
    static const unsigned char BinToHex[] = PDF_STRING_0123456789ABCDEF;

    (void) out;

    goal = (const unsigned char *) text + len;

    for (s = (const unsigned char *) text; s < goal; s++) {
	if (PDF_NEEDS_QUOTE(*s)) {
	    *cur++ = PDF_HASH;
	    *cur++ = BinToHex[*s >> 4];		/* first nibble */
	    *cur++ = BinToHex[*s & 0x0F];	/* second nibble */
	} else
	    *cur++ = *s;
    }

    *cur = 0;

    return buf;
}

/* --------------------------- Document sections  --------------------------- */

void
pdc_mark_free(pdc_output *out, pdc_id obj_id)
{
    out->file_offset[obj_id] = PDC_FREE_ID;
}


void
pdc_write_xref_and_trailer(pdc_output *out, pdc_id info_id, pdc_id root_id)
{
    static const char bin2hex[] = PDF_STRING_0123456789ABCDEF;
    long	pos;
    pdc_id	i;
    pdc_id	free_id;
    pdc_core *pdc = out->pdc;


    /* Don't write any object after this check! */

    for (i = 1; i <= out->lastobj; i++) {
	if (out->file_offset[i] == PDC_BAD_ID) {
	    pdc_warning(pdc, PDC_E_INT_UNUSEDOBJ,
		pdc_errprintf(pdc, "%ld", i), 0, 0, 0);
	    /* write a dummy object */
	    pdc_begin_obj(out, i);
	    pdc_printf(out, "null %% unused object\n");
	    pdc_end_obj(out);
	}
    }

    pos = pdc_tell_out(out);			/* xref table */
    pdc_puts(out, "xref\n");
    pdc_printf(out, "0 %ld\n", out->lastobj + 1);

    /* find the last free entry in the xref table.
    */
    out->file_offset[0] = PDC_FREE_ID;
    for (free_id = out->lastobj;
	out->file_offset[free_id] != PDC_FREE_ID;
	--free_id)
	;

    pdc_printf(out, "%010ld 65535 f \n", free_id);
    free_id = 0;

#define PDF_FLUSH_AFTER_MANY_OBJS	3000    /* ca. 60 KB */
    for (i = 1; i <= out->lastobj; i++) {
	/* Avoid spike in memory usage at the end of the document */
	if (i % PDF_FLUSH_AFTER_MANY_OBJS)
	    pdc_flush_stream(out);

	if (out->file_offset[i] == PDC_FREE_ID)
	{
	    pdc_printf(out, "%010ld 00001 f \n", free_id);
	    free_id = i;
	}
	else
	{
	    pdc_printf(out, "%010ld 00000 n \n", out->file_offset[i]);
	}
    }

    pdc_puts(out, "trailer\n");

    pdc_begin_dict(out);				/* trailer */
    pdc_printf(out, "/Size %ld\n", out->lastobj + 1);
    pdc_printf(out, "/Root %ld 0 R\n", root_id);

    if (info_id != PDC_BAD_ID)
	pdc_printf(out, "/Info %ld 0 R\n", info_id);

    /* write the digest to the ID array */
    pdc_puts(out, "/ID[<");
    for (i = 0; i < MD5_DIGEST_LENGTH; ++i)
    {
	pdc_putc(out, bin2hex[out->id[0][i] >> 4]);
	pdc_putc(out, bin2hex[out->id[0][i] & 0x0F]);
    }
    pdc_puts(out, "><");
    for (i = 0; i < MD5_DIGEST_LENGTH; ++i)
    {
	pdc_putc(out, bin2hex[out->id[1][i] >> 4]);
	pdc_putc(out, bin2hex[out->id[1][i] & 0x0F]);
    }
    pdc_puts(out, ">]\n");

    pdc_end_dict(out);				/* trailer */

    pdc_puts(out, "startxref\n");
    pdc_printf(out, "%ld\n", pos);
    pdc_puts(out, "%%EOF\n");
}

/* ---------------------- High-level output functions ---------------------- */


/*
 * Write a string to the output.
 */

void
pdc_puts(pdc_output *out, const char *s)
{
    pdc_write(out, (void *) s, strlen(s));
}


/* Write a character to the output without any modification. */

void
pdc_putc(pdc_output *out, const char c)
{
    pdc_write(out, (void *) &c, (size_t) 1);
}

/*
 * Write a formatted string to the output, converting from native
 * encoding to ASCII if necessary.
 */

void
pdc_printf(pdc_output *out, const char *fmt, ...)
{
    char	buf[PDF_LINEBUFLEN];	/* formatting buffer */
    va_list ap;

    va_start(ap, fmt);

    pdc_vsprintf(out->pdc, buf, fmt, ap);
    pdc_puts(out, buf);

    va_end(ap);
}
