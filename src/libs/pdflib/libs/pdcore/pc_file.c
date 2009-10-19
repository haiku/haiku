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

/* $Id: pc_file.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Various file routines
 *
 */

#include <errno.h>

#include "pc_config.h"
#include "pc_util.h"
#include "pc_md5.h"
#include "pc_file.h"

/* headers for getpid() or _getpid().
*/
#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#else
#if !defined(MAC)
	#include <sys/types.h>
	#include <unistd.h>
#endif
#endif

#ifndef WINCE
#include <time.h>
#else
#include <winbase.h>
#endif

struct pdc_file_s
{
    pdc_core       *pdc;       /* pdcore struct */
    char           *filename;  /* file name */
    FILE           *fp;        /* file struct or NULL. Then data != NULL: */
    const pdc_byte *data;      /* file data or NULL. Then fp != NULL */
    const pdc_byte *end;       /* first byte above data buffer */
    const pdc_byte *pos;       /* current file position in data buffer */
    pdc_byte       *tdata;     /* temporary buffer filled up by ungetc */
    int             number;    /* next available entry in tdata buffer */
    int             capacity;  /* currently allocated size of tdata buffer */
};

#if defined(_MSC_VER) && defined(_MANAGED)
#pragma unmanaged
#endif
int
pdc_get_fopen_errnum(pdc_core *pdc, int errnum)
{
    int outnum = errnum, isread;

    (void) pdc;

    isread = (errnum == PDC_E_IO_RDOPEN);

#if defined(MVS)

    switch (errno)
    {
        case 49:
        outnum = isread ? PDC_E_IO_RDOPEN_NF : PDC_E_IO_WROPEN_NF;
    }
    return outnum;

#elif defined(WIN32)
    {
	DWORD lasterror = GetLastError();
        switch (lasterror)
        {
            case ERROR_ACCESS_DENIED:
            outnum = isread ? PDC_E_IO_RDOPEN_PD : PDC_E_IO_WROPEN_PD;
            break;

            case ERROR_FILE_NOT_FOUND:
            case ERROR_INVALID_DRIVE:
            case ERROR_BAD_UNIT:
            outnum = isread ? PDC_E_IO_RDOPEN_NF : PDC_E_IO_WROPEN_NF;
            break;

            case ERROR_INVALID_NAME:
            outnum = isread ? PDC_E_IO_RDOPEN_NF : PDC_E_IO_WROPEN_IS;
            break;

            case ERROR_PATH_NOT_FOUND:
            outnum = isread ? PDC_E_IO_RDOPEN_NF : PDC_E_IO_WROPEN_NP;
            break;

            case ERROR_TOO_MANY_OPEN_FILES:
            case ERROR_SHARING_BUFFER_EXCEEDED:
            outnum = isread ? PDC_E_IO_RDOPEN_TM : PDC_E_IO_WROPEN_TM;
            break;

            case ERROR_FILE_EXISTS:
            outnum = PDC_E_IO_WROPEN_AE;
            break;

            case ERROR_BUFFER_OVERFLOW:
            outnum = PDC_E_IO_WROPEN_TL;
            break;

            case ERROR_WRITE_FAULT:
            case ERROR_CANNOT_MAKE:
            outnum = PDC_E_IO_WROPEN_NC;
            break;

            case ERROR_HANDLE_DISK_FULL:
            outnum = PDC_E_IO_WROPEN_NS;
            break;
        }

        if (lasterror)
            return outnum;

        /* if lasterror == 0 we must look for errno (see .NET) */
    }

#endif /* WIN32 */

    switch (errno)
    {
#ifdef EACCES
        case EACCES:
        outnum = isread ? PDC_E_IO_RDOPEN_PD : PDC_E_IO_WROPEN_PD;
        break;
#endif
#ifdef ENOENT
        case ENOENT:
        outnum = isread ? PDC_E_IO_RDOPEN_NF : PDC_E_IO_WROPEN_NF;
        break;
#endif
#ifdef EMFILE
        case EMFILE:
        outnum = isread ? PDC_E_IO_RDOPEN_TM : PDC_E_IO_WROPEN_TM;
        break;
#endif
#ifdef ENFILE
        case ENFILE:
        outnum = isread ? PDC_E_IO_RDOPEN_TM : PDC_E_IO_WROPEN_TM;
        break;
#endif
#ifdef EISDIR
        case EISDIR:
        outnum = isread ? PDC_E_IO_RDOPEN_ID : PDC_E_IO_WROPEN_ID;
        break;
#endif
#ifdef EEXIST
        case EEXIST:
        outnum = PDC_E_IO_WROPEN_AE;
        break;
#endif
#ifdef ENAMETOOLONG
        case ENAMETOOLONG:
        outnum = PDC_E_IO_WROPEN_TL;
        break;
#endif
#ifdef ENOSPC
        case ENOSPC:
        outnum = PDC_E_IO_WROPEN_NS;
        break;
#endif
        default:
        outnum = errnum;
        break;
    }

    return outnum;
}
#if defined(_MSC_VER) && defined(_MANAGED)
#pragma managed
#endif

void *
pdc_read_file(pdc_core *pdc, FILE *fp, size_t *o_filelen, int incore)
{
    static const char *fn = "pdc_read_file";
    size_t filelen, len;
    char *content = NULL;

#if defined(MVS) && defined(I370)
#define PDC_READ_CHUNKSIZE 64000

    while (!feof(fp))
    {
        if (!content)
        {
            len = 0;
            filelen = PDC_READ_CHUNKSIZE;
            content = (char *) pdc_malloc(pdc, filelen + 1, fn);
        }
        else if (incore)
        {
            len = filelen;
            filelen += PDC_READ_CHUNKSIZE;
            content = (char *) pdc_realloc(pdc, content, filelen + 1, fn);
        }
        len = fread(&content[len], 1, PDC_READ_CHUNKSIZE, fp);
    }
    filelen += len - PDC_READ_CHUNKSIZE;
    if (incore && filelen)
    {
        content = (char *) pdc_realloc(pdc, content, filelen + 1, fn);
    }
    else
    {
        pdc_free(pdc, content);
        content = NULL;
    }

#else

    fseek(fp, 0L, SEEK_END);
    filelen = (size_t) ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    if (incore && filelen)
    {
        content = (char *) pdc_malloc(pdc, filelen + 1, fn);
        len = fread(content, 1, filelen, fp);
        if (len != filelen)
        {
            pdc_free(pdc, content);
            filelen = 0;
            content = NULL;
        }
    }

#endif

    if (content) content[filelen] = 0;
    *o_filelen = filelen;
    return (void *) content;
}

pdc_file *
pdc_fopen(pdc_core *pdc, const char *filename, const char *qualifier,
          const pdc_byte *data, size_t size, int flags)
{
    static const char fn[] = "pdc_fopen";
    pdc_file *sfile;

    sfile = (pdc_file *) pdc_calloc(pdc, sizeof(pdc_file), fn);
    if (data)
    {
        sfile->data = data;
        sfile->pos = sfile->data;
        sfile->end = sfile->data + size;
    }
    else
    {
        sfile->fp = fopen(filename,
                          (flags & PDC_FILE_BINARY) ? READBMODE : READTMODE);
        if (sfile->fp == NULL)
        {
            pdc_free(pdc, sfile);
            if (qualifier)
            {
                pdc_set_errmsg(pdc, pdc_get_fopen_errnum(pdc, PDC_E_IO_RDOPEN),
                          qualifier, filename, 0, 0);
            }
            return NULL;
        }
    }

    sfile->pdc = pdc;
    sfile->filename = pdc_strdup(pdc, filename);


    return sfile;
}


pdc_bool
pdc_file_isvirtual(pdc_file *sfp)
{
    return sfp->fp ? pdc_false : pdc_true;
}

char *
pdc_file_name(pdc_file *sfp)
{
    return sfp->filename;
}

pdc_core *
pdc_file_getpdc(pdc_file *sfp)
{
    return sfp->pdc;
}

size_t
pdc_file_size(pdc_file *sfp)
{
    size_t filelen;

    if (sfp->fp)
    {
        long pos = ftell(sfp->fp);
        pdc_read_file(sfp->pdc, sfp->fp, &filelen, 0);
        fseek(sfp->fp, pos, SEEK_SET);
    }
    else
        filelen = (size_t) (sfp->end - sfp->data);
    return filelen;
}

const void *
pdc_freadall(pdc_file *sfp, size_t *filelen, pdc_bool *ismem)
{
    if (sfp->fp)
    {
        if (ismem) *ismem = pdc_false;
        return (const void *) pdc_read_file(sfp->pdc, sfp->fp, filelen, 1);
    }

    if (ismem) *ismem = pdc_true;
    *filelen = (size_t) (sfp->end - sfp->data);
    return sfp->data;
}

static int
pdc_fgetc_e(pdc_file *sfp)
{
    int c = pdc_fgetc(sfp);
    return c;
}

char *
pdc_fgets_comp(char *s, int size, pdc_file *sfp)
{
    int i, c;

    c = pdc_fgetc_e(sfp);
    if (c == EOF)
        return NULL;

    size--;
    for (i = 0; i < size; i++)
    {
        if (c == '\n' || c == '\r' || c == EOF) break;
        s[i] = (char) c;
        c = pdc_fgetc_e(sfp);
    }
    s[i] = 0;

    /* Skip windows line end \r\n */
    if (c == '\r')
    {
        c = pdc_fgetc_e(sfp);
        if (c != '\n')
        {
            pdc_ungetc(c, sfp);
        }
    }
    return s;
}

/*
 * Emulation of C file functions - relevant for PDFlib
 */

long
pdc_ftell(pdc_file *sfp)
{
    if (sfp->fp)
        return ftell(sfp->fp);

    return (long) (sfp->pos - sfp->data);
}

int
pdc_fseek(pdc_file *sfp, long offset, int whence)
{
    if (sfp->fp)
        return fseek(sfp->fp, offset, whence);

    sfp->number = 0;

    switch (whence)
    {
        case SEEK_SET:
        if (sfp->data + offset > sfp->end)
            return -1;
        sfp->pos = sfp->data + offset;
        break;

        case SEEK_CUR:
        if (sfp->pos + offset > sfp->end)
            return -1;
        sfp->pos += offset;
        break;

        case SEEK_END:
        if (sfp->end + offset > sfp->end)
            return -1;
        sfp->pos = sfp->end + offset;
        break;
    }
    return 0;
}

size_t
pdc_fread(void *ptr, size_t size, size_t nmemb, pdc_file *sfp)
{
    size_t nbytes = 0;

    if (sfp->fp)
        return fread(ptr, size, nmemb, sfp->fp);

    nbytes = size * nmemb;
    if (sfp->pos + nbytes > sfp->end)
    {
        nbytes = (size_t) (sfp->end - sfp->pos);
        nmemb = nbytes / size;
        nbytes = nmemb *size;
    }
    memcpy(ptr, sfp->pos, nbytes);
    sfp->pos += nbytes;

    return nmemb;
}

#define UNGETC_CHUNKSIZE  16

int
pdc_ungetc(int c, pdc_file *sfp)
{
    static const char fn[] = "pdc_ungetc";

    if (sfp->fp)
        return ungetc(c, sfp->fp);

    if (!sfp->capacity)
    {
        sfp->capacity = UNGETC_CHUNKSIZE;
        sfp->tdata = (pdc_byte *) pdc_malloc(sfp->pdc,
                         sfp->capacity * sizeof(pdc_byte), fn);
    }
    else if (sfp->number == sfp->capacity)
    {
        sfp->capacity += UNGETC_CHUNKSIZE;
        sfp->tdata = (pdc_byte *) pdc_realloc(sfp->pdc, sfp->tdata,
                         sfp->capacity * sizeof(pdc_byte), fn);
    }

    sfp->tdata[sfp->number] = (pdc_byte) c;
    sfp->number++;
    return c;
}

int
pdc_fgetc(pdc_file *sfp)
{
    int ch = 0;

    if (sfp->fp)
        return fgetc(sfp->fp);

    if (sfp->tdata && sfp->number > 0)
    {
        sfp->number--;
        ch = (int) sfp->tdata[sfp->number];
    }
    else
    {
        if (sfp->pos < sfp->end)
        {
            ch = (int) *sfp->pos;
            sfp->pos++;
        }
        else
        {
            ch = EOF;
        }
    }
    return ch;
}

char *
pdc_fgets(char *s, int size, pdc_file *sfp)
{
    int i;
    int ch, chp;

    if (sfp->fp)
        return fgets(s, size, sfp->fp);

    chp = 0;
    for (i = 0; i < size ; i++)
    {
        ch = 0;
        if (i < size - 1 && chp != '\n')
        {
            if (sfp->number > 0)
            {
                sfp->number--;
                ch = (int) sfp->tdata[sfp->number];
            }
            else if (sfp->pos < sfp->end)
            {
                ch = (int) *sfp->pos;
                sfp->pos++;
            }
            if (ch == EOF) ch = 0;
        }
        s[i] = (char) ch;
        if (!ch)
            break;
        chp = (int) s[i];
    }

    return (i > 1) ? s : NULL;
}

int
pdc_feof(pdc_file *sfp)
{
    if (sfp->fp)
        return feof(sfp->fp);

    return (sfp->pos >= sfp->end) ? 1 : 0;
}

void
pdc_fclose(pdc_file *sfp)
{
    if (sfp)
    {
        if (sfp->fp)
        {
            fclose(sfp->fp);
            sfp->fp = NULL;
        }
        if (sfp->tdata)
        {
            pdc_free(sfp->pdc, sfp->tdata);
            sfp->tdata = NULL;
        }
        if (sfp->filename)
        {
            pdc_free(sfp->pdc, sfp->filename);
            sfp->filename = NULL;
        }
        pdc_free(sfp->pdc, sfp);
    }
}

/*
 * Concatenating a directory name with a file base name to a full valid
 * file name. On MVS platforms an extension at the end of basename
 * will be discarded.
 */
void
pdc_file_fullname(const char *dirname, const char *basename, char *fullname)
{
    int ib;
    size_t len = strlen(basename);
#ifdef MVS
    pdc_bool lastterm = pdc_false;
#endif

    for (ib = 0; ib < (int) len; ib++)
        if (!isspace(basename[ib]))
            break;

    if (!dirname || !dirname[0])
    {
        strcpy(fullname, &basename[ib]);
    }
    else
    {
        fullname[0] = 0;
#ifdef MVS
        if (strncmp(dirname, PDC_FILEQUOT, 1))
            strcat(fullname, PDC_FILEQUOT);
#endif
        strcat(fullname, dirname);
        strcat(fullname, PDC_PATHSEP);
        strcat(fullname, &basename[ib]);
#ifdef MVS
        lastterm = pdc_true;
#endif
    }

#ifdef MVS
    {
        int ie;

        len = strlen(fullname);
        for (ie = len; ie >= 0; ie--)
        {
            if (!strncmp(&fullname[ie], PDC_PATHSEP, 1))
                break;
            if (fullname[ie] == '.')
            {
                fullname[ie] = 0;
                break;
            }
        }
        if (lastterm)
        {
            strcat(fullname, PDC_PATHTERM);
            strcat(fullname, PDC_FILEQUOT);
        }
    }
#endif
}

/*
 * Function reads a text file and creates a string list
 * of all no-empty and no-comment lines. The strings are stripped
 * by leading and trailing white space characters.
 *
 * The caller is responsible for freeing the resultated string list
 * by calling the function pdc_cleanup_stringlist.
 *
 * Not for unicode strings.
 *
 * Return value: Number of strings
 */

#define PDC_BUFSIZE 1024
#define PDC_ARGV_CHUNKSIZE 256

int
pdc_read_textfile(pdc_core *pdc, pdc_file *sfp, char ***linelist)
{
    static const char *fn = "pdc_read_textfile";
    char   buf[PDC_BUFSIZE];
    char  *content = NULL;
    char **argv = NULL;
    int    nlines = 0;
    size_t filelen;
    size_t len, maxl = 0;
    int    tocont, incont = 0;
    int    i, is = 0;

    /* Get file length and allocate content array */
    filelen = pdc_file_size(sfp);
    content = (char *) pdc_malloc(pdc, filelen, fn);

    /* Read loop */
    while (pdc_fgets_comp(buf, PDC_BUFSIZE, sfp) != NULL)
    {
        /* Strip blank and comment lines */
        pdc_str2trim(buf);
        if (buf[0] == 0 || buf[0] == '%')
            continue;

        /* Strip inline comments */
        len = strlen(buf);
        for (i = 1; i < (int) len; i++)
        {
            if (buf[i] == '%' && buf[i-1] != '\\')
            {
                buf[i] = 0;
                pdc_strtrim(buf);
                len = strlen(buf);
                break;
            }
        }

        /* Continuation line */
        tocont = (buf[len-1] == '\\') ? 1:0;
        if (tocont)
        {
            buf[len-1] = '\0';
            len--;
        }

        /* Copy line */
        strcpy(&content[is], buf);

        /* Save whole line */
        if (!incont)
        {
            if (nlines >= (int) maxl)
            {
                maxl += PDC_ARGV_CHUNKSIZE;
                argv = (argv == NULL) ?
                        (char **)pdc_malloc(pdc, maxl * sizeof(char *), fn):
                        (char **)pdc_realloc(pdc, argv, maxl *
                                             sizeof(char *), fn);
            }
            argv[nlines] = &content[is];
            nlines++;
        }

        /* Next index */
        incont = tocont;
        is += (int) (len + 1 - incont);
    }

    if (!argv) pdc_free(pdc, content);
    *linelist = argv;
    return nlines;
}


/* generate a temporary file name from the current time, pid, and the
** data in inbuf using MD5.
*/
void
pdc_tempname(char *outbuf, int outlen, const char *inbuf, int inlen)
{
    MD5_CTX             md5;
    time_t              timer;
    unsigned char       digest[MD5_DIGEST_LENGTH];
    int                 i;

#if defined(WIN32)
    int pid = _getpid();
#else
#if !defined(MAC)
    pid_t pid = getpid();
#endif
#endif

    time(&timer);

    MD5_Init(&md5);
#if !defined(MAC)
    MD5_Update(&md5, (unsigned char *) &pid, sizeof pid);
#endif
    MD5_Update(&md5, (unsigned char *) &timer, sizeof timer);
    MD5_Update(&md5, (unsigned char *) inbuf, (unsigned int) inlen);
    MD5_Final(digest, &md5);

    for (i = 0; i < outlen - 1; ++i)
        outbuf[i] = (char) ('A' + digest[i % MD5_DIGEST_LENGTH] % 26);

    outbuf[i] = 0;
}

#ifdef PDF_TARGET_API_MAC_CARBON

/* Construct an FSSpec from a Posix path name. Only required for
 * Carbon (host font support and file type/creator).
 */

OSStatus
FSPathMakeFSSpec(const UInt8 *path, FSSpec *spec)
{
        OSStatus        result;
        FSRef           ref;

        /* convert the POSIX path to an FSRef */
        result = FSPathMakeRef(path, &ref, NULL);

        if (result != noErr)
                return result;

        /* and then convert the FSRef to an FSSpec */
        result = FSGetCatalogInfo(&ref, kFSCatInfoNone, NULL, NULL, spec, NULL);

        return result;
}

#endif /* PDF_TARGET_API_MAC_CARBON */

