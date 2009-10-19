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

/* $Id: pc_encoding.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Encoding data structures and routines
 *
 */

#ifndef PC_ENCODING_H
#define PC_ENCODING_H

/*
 * Symbolic names for predefined font encodings. 0 and above are used
 * as indices in the pdc_encodingvector array. The encodings above
 * ebcdic have no enumeration name, because they are loaded dynamically.
 *
 * The predefined encodings must not be changed or rearranged.
 * The order of encodings here must match that of pdc_core_encodings
 * and pdc_fixed_encoding_names in pc_encoding.c.
 */
typedef enum
{
    pdc_invalidenc = -5,
    pdc_glyphid = -4,
    pdc_unicode = -3,
    pdc_builtin = -2,
    pdc_cid = -1,
    pdc_winansi = 0,
    pdc_macroman = 1,
    pdc_ebcdic = 2,
    pdc_pdfdoc = 3,
    pdc_firstvarenc = 4
}
pdc_encoding;

typedef struct pdc_encodingvector_s pdc_encodingvector;

struct pdc_encodingvector_s
{
    char *apiname;             /* PDFlib's name of the encoding at the API */
    pdc_ushort codes[256];     /* unicode values */
    char *chars[256];          /* character names */
    char given[256];           /* flag whether character name is given */
    pdc_byte *sortedslots;     /* slots for sorted unicode values */
    int nslots;                /* number of sorted slots */
    unsigned long flags;       /* flags, see PDC_ENC_... */
};

#define PDC_ENC_INCORE       (1L<<0) /* encoding from in-core */
#define PDC_ENC_FILE         (1L<<1) /* encoding from file */
#define PDC_ENC_HOST         (1L<<2) /* encoding from host system */
#define PDC_ENC_USER         (1L<<3) /* encoding from user */
#define PDC_ENC_FONT         (1L<<4) /* encoding from font */
#define PDC_ENC_GENERATE     (1L<<5) /* encoding generated from Unicode page */
#define PDC_ENC_USED         (1L<<6) /* encoding already used */
#define PDC_ENC_SETNAMES     (1L<<7) /* character names are set */
#define PDC_ENC_ALLOCCHARS   (1L<<8) /* character names are allocated */
#define PDC_ENC_STDNAMES     (1L<<9) /* character names are all Adobe standard*/

#define PDC_ENC_MODSEPAR     "_"          /* separator of modified encoding */
#define PDC_ENC_MODWINANSI   "winansi_"   /* prefix of modified winansi enc */
#define PDC_ENC_MODMACROMAN  "macroman_"  /* prefix of modified macroman enc */

/* pc_encoding.c */
void pdc_init_encoding(pdc_core *pdc, pdc_encodingvector *ev,
                       const char *name);
pdc_encodingvector *pdc_new_encoding(pdc_core *pdc, const char *name);
void pdc_cleanup_encoding(pdc_core *pdc, pdc_encodingvector *ev);
int pdc_get_encoding_bytecode(pdc_core *pdc, pdc_encodingvector *ev,
                              pdc_ushort uv);
pdc_encodingvector *pdc_copy_core_encoding(pdc_core *pdc, const char *encoding);
const char *pdc_get_fixed_encoding_name(pdc_encoding enc);

#endif  /* PC_ENCODING_H */
