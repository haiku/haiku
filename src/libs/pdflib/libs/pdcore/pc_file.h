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

/* $Id: pc_file.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Definitions for file routines
 *
 */

#ifndef PC_FILE_H
#define PC_FILE_H

#if (defined(MAC) || defined(MACOSX))

#include <Files.h>

#ifdef PDF_TARGET_API_MAC_CARBON

OSStatus FSPathMakeFSSpec(const UInt8 *path, FSSpec *spec);

#else

#include <Aliases.h>

OSErr FSpLocationFromFullPath(short fullPathLength,
		const void *fullPath, FSSpec *spec);

#endif /* !PDF_TARGET_API_MAC_CARBON */
#endif /* (defined(MAC) || defined(MACOSX)) */

#define PDC_FILENAMELEN  1024    /* maximum file name length */

#define PDC_FILE_BINARY (1L<<0)  /* open as binary file,
                                    otherwise as text file */

#define PDC_OK_FREAD(file, buffer, len) \
    (pdc_fread(buffer, 1, len, file) == len)

typedef struct pdc_file_s pdc_file;

int     pdc_get_fopen_errnum(pdc_core *pdc, int errnum);
void    *pdc_read_file(pdc_core *pdc, FILE *fp, size_t *o_filelen, int incore);
int     pdc_read_textfile(pdc_core *pdc, pdc_file *sfp, char ***linelist);
void    pdc_tempname(char *outbuf, int outlen, const char *inbuf, int inlen);

pdc_file *pdc_fopen(pdc_core *pdc, const char *filename, const char *qualifier,
                    const pdc_byte *data, size_t size, int flags);
pdc_core *pdc_file_getpdc(pdc_file *sfp);
char   *pdc_file_name(pdc_file *sfp);
size_t  pdc_file_size(pdc_file *sfp);
pdc_bool pdc_file_isvirtual(pdc_file *sfp);
char   *pdc_fgets_comp(char *s, int size, pdc_file *sfp);
long    pdc_ftell(pdc_file *sfp);
int     pdc_fseek(pdc_file *sfp, long offset, int whence);
size_t  pdc_fread(void *ptr, size_t size, size_t nmemb, pdc_file *sfp);
const void *pdc_freadall(pdc_file *sfp, size_t *filelen, pdc_bool *ismem);
int     pdc_ungetc(int c, pdc_file *sfp);
int     pdc_fgetc(pdc_file *sfp);
char   *pdc_fgets(char *s, int size, pdc_file *sfp);
int     pdc_feof(pdc_file *sfp);
void    pdc_fclose(pdc_file *sfp);
void    pdc_file_fullname(const char *dirname, const char *basename,
                          char *fullname);



#endif  /* PC_FILE_H */
