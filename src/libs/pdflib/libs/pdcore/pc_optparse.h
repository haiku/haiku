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

/* $Id: pc_optparse.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Definitions for option parser routines
 *
 */

#ifndef PC_OPTPARSE_H
#define PC_OPTPARSE_H

/*
 *  Optlist
 *  -------
 *  An optlist is a string containing pairs of the form
 *  "optionname optionvalue(s)". The separator characters
 *  are "\f\n\r\t\v =".
 *
 *  There are options of different types (see pdc_opttype):
 *
 *      Boolean   (pdc_booleanlist)
 *      Strings   (pdc_stringlist)
 *      Keywords  (pdc_keywordlist)
 *      Integers  (pdc_integerlist)
 *      Floats    (pdc_floatlist)
 *      Doubles   (pdc_doublelist)
 *      Handles   (pdc_colorhandle ...)
 *
 *  An option can have one or more values. Boolean options can be
 *  provided without any value. If an option has more than one value,
 *  then these values have to be set in braces. Examples:
 *
 *      dasharray {11 22 33}
 *
 *  Strings with white spaces have to be set in braces too.
 *  Examples:
 *
 *      fullname {Ludwig Wittgenstein}
 *      composers {{Gustav Mahler}}
 *      comment {}
 *
 *  The allowed option names and the limitations of their values
 *  must be defined in an array of enumeration type pdc_defopt
 *  (see below). Such an option definition specifies (in brackets
 *  the member name in the pdc_defopt struct)
 *
 *      - the name of the option (name)
 *      - the type of the option (type)
 *      - value restrictions by bit flags (flags)
 *      - the minimal and maximal permitted number of values
 *        (minnum, maxnum)
 *      - the minimal and maximal permitted value, or string
 *        length resp. (minval, maxval)
 *      - the permitted keywords in a keyword list (is required) or
 *        the permitted integer numbers in a integer list (is optional),
 *        resp. (keylist)
 *      - the number of default values and the default values
 *        themselves (defnum, defval)
 *
 *  Remarks:
 *
 *      - minnum = maxnum = 1: The program expects a single value,
 *        otherwise an array. If an array consists of only one value
 *        the braces can be omitted - but not in the case of strings
 *        with white spaces (see example above).
 *      - Boolean options have the values "true" or "false". A shorter
 *        equivalent notation is "+name" or "-name". for "name true"
 *        or "name false", resp.
 *      - White spaces in strings can be forbidden by the flag
 *        PDC_OPT_NOSPACES.
 *      - It's only possible to specify a single number interval (minval,
 *        maxval) which must contain the number. The flag PDC_OPT_NOZERO
 *        can forbid zero additionally.
 *      - Keywords will always be converted to integer numbers (keycodes)
 *        according to the specified pdc_keyconn array.
 *      - It is possible to specify keywords for integers, floats and
 *        doubles additionally by an optional keylist entry. For integers
 *        it is possible to specify the allowed integer values by an optional
 *        keylist and by the flag PDC_OPT_INTLIST.
 *      - If more than one keyword is permitted, then the flag
 *        PDC_OPT_BUILDOR decides, whether a bit pattern must be
 *        built by or-ing the single keycodes or not.
 *
 *  Program run:
 *
 *  An optlist will be parsed by the function pdc_parse_optionlist.
 *  After successfully parsing this function returns a pointer to the
 *  allocated pdc_resopt structures containing the option values.
 *  These structures must be freed by the function pdc_cleanup_optionlist.
 *
 *  Values must be fetched by the function pdc_get_optvalues. This can
 *  be achieved by specifying a variable pointer (lvalues) or by a pointer
 *  to a pointer (mvalues). In the first case the variable must be large
 *  enough to hold the values. In the second case the pointer is the pointer
 *  to the allocated array with the option values. If such a pointer is
 *  specified in a call of pdc_get_optvalues the pointer will not be freed
 *  in pdc_cleanup_optionlist, but the caller has the responsibility to
 *  free it after use.
 *
 *  pdc_stringlist:
 *  maxnum = 1: lvalues: char s[maxval+1]  (defined char array)
 *  maxnum > 1: lvalues: char *s[maxnum]   (defined char pointer rarray)
 *              mvalues: char **s          (pointer to a char pointer array)
 *
 */

typedef struct pdc_keyconn_s pdc_keyconn;
typedef struct pdc_clientdata_s pdc_clientdata;
typedef struct pdc_defopt_s pdc_defopt;
typedef struct pdc_resopt_s pdc_resopt;

/* types of option values */
typedef enum
{
    pdc_booleanlist = 0,
    pdc_stringlist,
    pdc_keywordlist,
    pdc_integerlist,
    pdc_floatlist,
    pdc_doublelist,

    /* correspondig member of pdc_clientdata_s must be specified */
    pdc_colorhandle,
    pdc_documenthandle,
    pdc_fonthandle,
    pdc_gstatehandle,
    pdc_iccprofilehandle,
    pdc_imagehandle,
    pdc_pagehandle,
    pdc_patternhandle,
    pdc_shadinghandle
}
pdc_opttype;

/* keyword - keycode */
struct pdc_keyconn_s
{
    char *word;
    int  code;
};

/* client data */
struct pdc_clientdata_s
{
    int compatibility;
    int maxcolor;
    int maxdocument;
    int maxfont;
    int maxgstate;
    int maxiccprofile;
    int maximage;
    int maxpage;
    int maxpattern;
    int maxshading;
    pdc_bool hastobepos;
};

/* definition of an option */
struct pdc_defopt_s
{
    const char        *name;    /* name of option keyword */
    pdc_opttype        type;    /* type of option */
    int                flags;   /* flags (see below) */
    int                minnum;  /* permitted minimal number of values */
    int                maxnum;  /* permitted maximal number of values */
    double             minval;  /* minimal permitted value / length of string */
    double             maxval;  /* maximal permitted value / length of string */
    const pdc_keyconn *keylist; /* list of permitted keywords - keycodes */
};

#define PDC_OPT_TERMINATE \
    {NULL, pdc_booleanlist, 0L, 0, 0, 0.0, 0.0, NULL}

#define PDC_OPT_NONE       (0)      /* no flag specified */
#define PDC_OPT_NOZERO     (1L<<0)  /* zero value not allowed */
#define PDC_OPT_NOSPACES   (1L<<1)  /* white spaces in strings not allowed */
#define PDC_OPT_REQUIRED   (1L<<2)  /* option is required */
#define PDC_OPT_BUILDOR    (1L<<3)  /* build an OR bit pattern by keycodes */
#define PDC_OPT_INTLIST    (1L<<4)  /* keylist is list of allowed integers */
#define PDC_OPT_IGNOREIF1  (1L<<5)  /* option is ignored if previous option is
                                     * specified */
#define PDC_OPT_IGNOREIF2  (1L<<6)  /* option is ignored if either of
                                     * previous two options is specified */
#define PDC_OPT_UNSUPP     (1L<<8)  /* option is not supported in this
                                     * configuration */
#define PDC_OPT_REQUIRIF1  (1L<<9)  /* option is require if previous option is
                                     * specified */
#define PDC_OPT_REQUIRIF2  (1L<<10) /* option is require if either of
                                     * previous two options is specified */

/* member "compatibility" of pdc_clientdata_s must be specified (1L<<12) ... */
#define PDC_OPT_PDC_1_3   (1L<<PDC_1_3) /* compatibility PDC_1_3 */
#define PDC_OPT_PDC_1_4   (1L<<PDC_1_4) /* compatibility PDC_1_4 */
#define PDC_OPT_PDC_1_5   (1L<<PDC_1_5) /* compatibility PDC_1_5 */

/* key word not found */
#define PDC_KEY_NOTFOUND  -1234567890

/* pc_optparse.c */
int pdc_get_keycode(const char *keyword, const pdc_keyconn *keyconn);
int pdc_get_keycode_ci(const char *keyword, const pdc_keyconn *keyconn);
const char *pdc_get_keyword(int keycode, const pdc_keyconn *keyconn);
const char *pdc_get_int_keyword(char *keyword, const pdc_keyconn *keyconn);
pdc_resopt *pdc_parse_optionlist(pdc_core *pdc, const char *optlist,
                                 const pdc_defopt *defopt,
                                 const pdc_clientdata *clientdata,
                                 pdc_bool verbose);
int pdc_get_optvalues(pdc_core *pdc, const char *keyword,
                      pdc_resopt *resopt, void *lvalues, void **mvalues);
void pdc_cleanup_optionlist(pdc_core *pdc, pdc_resopt *resopt);
void pdc_cleanup_optstringlist(pdc_core *pdc, char **stringlist, int ns);
const char *pdc_get_handletype(pdc_opttype type);

#endif  /* PC_OPTPARSE_H */
