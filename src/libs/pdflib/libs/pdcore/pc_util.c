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

/* $Id: pc_util.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Various utility routines
 *
 */

#include <errno.h>

#include "pc_util.h"

#ifdef AS400
#include <qp0z1170.h>   /* for getenv() emulation */
#endif

/* -------------------------- Time functions ------------------------------ */

#ifndef WINCE
#include <time.h>
#else
#include <winbase.h>
#endif

void
pdc_get_timestr(char *str)
{
#ifndef WINCE
    time_t      timer, gtimer;
    struct tm   ltime;
    double      diffminutes;
    int         utcoffset;
#else
    SYSTEMTIME  st;
#endif

#ifndef WINCE
    time(&timer);

    /* Calculate UTC offset with standard C functions. */
    /* gmtime(), localtime and mktime() all use the same internal */
    /* buffer. That is why it is done in multiple steps. */
    ltime = *gmtime(&timer);
    gtimer = mktime(&ltime);
    ltime = *localtime(&timer);
    ltime.tm_isdst = 0;
    diffminutes = difftime(mktime(&ltime), gtimer) / 60;
    if (diffminutes >= 0)
        utcoffset = (int)(diffminutes + 0.5);
    else
        utcoffset = (int)(diffminutes - 0.5);

    /* Get local time again, previous data is damaged by mktime(). */
    ltime = *localtime(&timer);

    if (utcoffset > 0)
        sprintf(str, "D:%04d%02d%02d%02d%02d%02d+%02d'%02d'",
            ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday,
            ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
            utcoffset / 60, utcoffset % 60);
    else if (utcoffset < 0)
        sprintf(str, "D:%04d%02d%02d%02d%02d%02d-%02d'%02d'",
            ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday,
            ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
            abs(utcoffset) / 60, abs(utcoffset) % 60);
    else
        sprintf(str, "D:%04d%02d%02d%02d%02d%02dZ",
            ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday,
            ltime.tm_hour, ltime.tm_min, ltime.tm_sec);

#else
    GetLocalTime (&st);
    sprintf(str, "D:%04d%02d%02d%02d%02d%02d",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
#endif  /* !WINCE */
}

/* -------------------------- Environment ------------------------------ */

char *
pdc_getenv(const char *name)
{
#ifdef HAVE_ENVVARS
    return getenv(name);
#else
    (void) name;

    return (char *) 0;
#endif
}


/* -------------------------- Bit arryas ------------------------------ */

void
pdc_setbit(char *bitarr, int bit)
{
    bitarr[bit/8] |= (char) (1<<(bit%8));
}

pdc_bool
pdc_getbit(char *bitarr, int bit)
{
    return (pdc_bool) (bitarr[bit/8] & (1<<(bit%8)));
}

void
pdc_setbit_text(char *bitarr, const pdc_byte *text, int len,
                int nbits, int size)
{
    int i, bit;
    pdc_ushort *ustext = (pdc_ushort *) text;

    for (i = 0; i < len; i += size)
    {
        if (size == sizeof(pdc_byte))
            bit = (int) text[i];
        else
            bit = ustext[i/size];
        if (bit < nbits) pdc_setbit(bitarr, bit);
    }
}


/* ---------- Get functions of integer binary data types --------------- */

pdc_short
pdc_get_le_short(pdc_byte *data)
{
    return (pdc_short) ((pdc_short) (data[1] << 8) | data[0]);
}

pdc_ushort
pdc_get_le_ushort(pdc_byte *data)
{
    return (pdc_ushort) ((data[1] << 8) | data[0]);
}

pdc_ulong
pdc_get_le_ulong3(pdc_byte *data)
{
    return (pdc_ulong) (((((data[2]) << 8) | data[1]) << 8) | data[0]);
}

pdc_long
pdc_get_le_long(pdc_byte *data)
{
    return ((pdc_long)
         (((((data[3] << 8) | data[2]) << 8) | data[1]) << 8) | data[0]);
}

pdc_ulong
pdc_get_le_ulong(pdc_byte *data)
{
    return (pdc_ulong)
         ((((((data[3] << 8) | data[2]) << 8) | data[1]) << 8) | data[0]);
}

pdc_short
pdc_get_be_short(pdc_byte *data)
{
    return (pdc_short) ((pdc_short) (data[0] << 8) | data[1]);
}

pdc_ushort
pdc_get_be_ushort(pdc_byte *data)
{
    return (pdc_ushort) ((data[0] << 8) | data[1]);
}

pdc_ulong
pdc_get_be_ulong3(pdc_byte *data)
{
    return (pdc_ulong) (((((data[0]) << 8) | data[1]) << 8) | data[2]);
}

pdc_long
pdc_get_be_long(pdc_byte *data)
{
    return ((pdc_long)
         (((((data[0] << 8) | data[1]) << 8) | data[2]) << 8) | data[3]);
}

pdc_ulong
pdc_get_be_ulong(pdc_byte *data)
{
    return (pdc_ulong)
        ((((((data[0] << 8) | data[1]) << 8) | data[2]) << 8) | data[3]);
}


/* ----------------- String handling for Unicode too ------------------- */

/* strlen() for unicode strings, which are terminated by two zero bytes.
 * wstrlen() returns the number of bytes in the Unicode string,
 * not including the two terminating null bytes.
 */
static size_t
wstrlen(const char *s)
{
    size_t len;

    for(len = 0;
        (pdc_byte) (s[len++]) != 0 ||
        (pdc_byte) (s[len++]) != 0; /* */ ) {
	/* */
    }

    return len-2;
}

/*
 * This function returns the length in bytes for C and Unicode strings.
 * Note that unlike strlen() it returns the length _including_ the
 * terminator, which may be one or two null bytes.
 */
size_t
pdc_strlen(const char *text)
{
    if (pdc_is_unicode(text))
	return wstrlen(text) + 2;
    else
	return strlen(text) + 1;
}


/* Allocate a local buffer and copy the string including
 * the terminating sentinel. If the string starts with the Unicode BOM
 * it is considered a Unicode string, and must be terminated by
 * two null bytes. Otherwise it is considered a plain C string and
 * must be terminated by a single null byte.
 * The caller is responsible for freeing the buffer.
 */
char *
pdc_strdup(pdc_core *pdc, const char *text)
{
    char *buf;
    size_t len;
    static const char fn[] = "pdc_strdup";

    if (text == NULL)
	pdc_error(pdc, PDC_E_INT_NULLARG, fn, 0, 0, 0);

    len = pdc_strlen(text);
    buf = (char *) pdc_malloc(pdc, len, fn);
    memcpy(buf, text, len);

    return buf;
}

/* Put out a 2 byte string for printing on the screen by replacing
 * characters < 0x20 by space. The string will be limited to 40 byte.
 */

#define PDC_STRPRINTLEN 80
const char *
pdc_strprint(pdc_core *pdc, const char *str, int len)
{
    pdc_byte c, tmpstr[PDC_STRPRINTLEN + 4];
    int i = 0, maxi;

    if (str)
    {
        if (!len)
            len = (int) strlen(str);
        maxi = (len > PDC_STRPRINTLEN) ? PDC_STRPRINTLEN : len;
        for (i = 0; i < maxi; i++)
        {
            c = (pdc_byte) str[i];
            tmpstr[i] = (c < 0x20) ? (pdc_byte) ' ' : c;
        }
    }
    tmpstr[i] = 0;
    if (len > PDC_STRPRINTLEN)
        strcat((char *) tmpstr, "...");

    return pdc_errprintf(pdc, "%s", tmpstr);
}

/*
 * Split a given text string into single strings which are separated by
 * arbitrary characters. This characters must be specified in a string.
 * If this string is NULL, " \f\n\r\t\v" (standard white spaces) is assumed.
 *
 * There is the convention that text inside braces {} will be taken verbatim.
 * Inside brace expressions braces must exist only in pairs. Braces are
 * masked by backslash.
 *
 * The caller is responsible for freeing the resultated string list
 * by calling the function pdc_cleanup_stringlist.
 *
 * Not for unicode strings.
 *
 * Return value: Number of strings.
 *               If braces aren't balanced the number is negative.
 *
 */

int
pdc_split_stringlist(pdc_core *pdc, const char *text, const char *i_separstr,
                     char ***stringlist)
{
    static const char fn[] = "pdc_split_stringlist";
    const char *separstr = " \f\n\r\t\v";
    const char *oldtext;
    char **strlist = NULL, *newtext;
    int i, it, len, jt = 0, jtb = 0, maxk = 0, count = 0, inside = 0;

    if (text == NULL)
	pdc_error(pdc, PDC_E_INT_NULLARG, fn, 0, 0, 0);

    if (stringlist)
        *stringlist = NULL;
    if (i_separstr)
        separstr = i_separstr;

    /* check for empty string */
    i = (int) strspn(text, separstr);
    oldtext = &text[i];
    len = (int) strlen(oldtext);
    if (!len) return 0;

    newtext = (char *) pdc_malloc(pdc, (size_t) (len + 1), fn);
    for (it = 0; it <= len; it++)
    {
        /* check for separators */
        if (it == len)
            i = 1;
        else if (inside <= 0)
            i = (int) strspn(&oldtext[it], separstr);
        else
            i = 0;

        /* close text part */
        if (i)
        {
            newtext[jt] = 0;
            if (count == maxk)
            {
                maxk += 16;
                strlist = (strlist == NULL) ?
                    (char **) pdc_malloc(pdc, maxk * sizeof(char *), fn):
                    (char **) pdc_realloc(pdc, strlist, maxk *
                                          sizeof(char *), fn);
            }
            strlist[count] = &newtext[jtb];
            count++;

            /* Exit */
            it += i;
            if (it >= len ) break;

            /* new text part */
            jt++;
            jtb = jt;
        }

        /* open and close brace */
        if (oldtext[it] == '{')
        {
            inside++;
            if (inside == 1)
                continue;
        }
        else if (oldtext[it] == '}')
        {
            inside--;
            if (inside == 0)
                continue;
        }

        /* masked braces */
        if (oldtext[it] == '\\' &&
            (oldtext[it+1] == '{' || oldtext[it+1] == '}'))
            it++;

        /* save character */
        newtext[jt] = oldtext[it];
        jt++;
    }

    if (stringlist)
        *stringlist = strlist;
    return inside ? -count : count;
}

void
pdc_cleanup_stringlist(pdc_core *pdc, char **stringlist)
{
    if(stringlist != NULL)
    {
	if(stringlist[0] != NULL)
	    pdc_free(pdc, stringlist[0]);

 	pdc_free(pdc, stringlist);
    }
}


/*
 * Compares its arguments and returns an integer less than,
 * equal to, or greater than zero, depending on whether s1
 * is lexicographically less than, equal to, or greater than s2.
 * Null pointer values for s1 and s2 are treated the same as pointers
 * to empty strings.
 *
 * Presupposition: basic character set
 *
 * Return value:  < 0  s1 <  s2;
 *		  = 0  s1 == s2;
 *		  > 0  s1 >  s2;
 *
 */
int
pdc_stricmp(const char *s1, const char *s2)
{
    char c1, c2;

    if (s1 == s2) return (0);
    if (s1 == NULL) return (-1);
    if (s2 == NULL) return (1);

    for (; *s1 != '\0' && *s2 != '\0';  s1++, s2++)
    {
	if ((c1 = *s1) == (c2 = *s2))
	    continue;

	if (islower((int)c1)) c1 = (char) toupper((int)c1);
	if (islower((int)c2)) c2 = (char) toupper((int)c2);
	if (c1 != c2)
	    break;
    }
    return (*s1 - *s2);
}


/*
 * Compares its arguments and returns an integer less than,
 * equal to, or greater than zero, depending on whether s1
 * is lexicographically less than, equal to, or greater than s2.
 * But only up to n characters compared (n less than or equal
 * to zero yields equality).Null pointer values for s1 and s2
 * are treated the same as pointers to empty strings.
 *
 * Presupposition: basic character set
 *
 * Return value:  < 0  s1 <  s2;
 *		  = 0  s1 == s2;
 *		  > 0  s1 >  s2;
 *
 */
int
pdc_strincmp(const char *s1, const char *s2, int n)
{
    char c1, c2;
    int  i;

    if (s1 == s2)   return (0);
    if (s1 == NULL) return (-1);
    if (s2 == NULL) return (1);

    for (i=0;  i < n && *s1 != '\0' && *s2 != '\0';  i++, s1++, s2++)
    {
	if ((c1 = *s1) == (c2 = *s2))
	    continue;

	if (islower((int)c1)) c1 = (char) toupper((int)c1);
	if (islower((int)c2)) c2 = (char) toupper((int)c2);
	if (c1 != c2)
	    break;
    }
    return ((i < n) ?  (int)(*s1 - *s2) : 0);
}

/*
 * pdc_strtrim removes trailing white space characters from an input string.
 * pdc_str2trim removes leading and trailing white space characters from an
 * input string..
 */
char *
pdc_strtrim(char *str)
{
    int i;

    for (i = (int) strlen(str) - 1; i >= 0; i--)
        if (!isspace((unsigned char) str[i])) break;
    str[i + 1] = '\0';

    return str;
}

char *
pdc_str2trim(char *str)
{
    int i;

    for (i = (int) strlen(str) - 1; i >= 0; i--)
        if (!isspace((unsigned char) str[i])) break;
    str[i + 1] = '\0';

    for (i = 0; ; i++)
        if (!isspace((unsigned char) str[i])) break;
    if (i > 0)
        memmove(str, &str[i], strlen(&str[i]) + 1);

    return str;
}

void
pdc_swap_bytes(char *instring, int inlen, char *outstring)
{
    char c;
    int i,j;

    if (instring == NULL)
        return;

    if (outstring == NULL)
        outstring = instring;

    inlen = 2 * inlen / 2;
    for (i = 0; i < inlen; i++)
    {
        j = i;
        i++;
        c = instring[j];
        outstring[j] = instring[i];
        outstring[i] = c;
    }
}

/*
 * Converts a null terminated and trimed string to a double number
 */
pdc_bool
pdc_str2double(const char *string, double *o_dz)
{
    const char *s = string;
    double dz = 0;
    int is = 1, isd = 0;

    *o_dz = 0;

    /* sign */
    if (*s == '-')
    {
        is = -1;
        s++;
    }
    else if (*s == '+')
        s++;

    if (!*s)
        return pdc_false;

    /* places before decimal point */
    isd = isdigit(*s);
    if (isd)
    {
        do
        {
            dz = 10 * dz + *s - '0';
            s++;
        }
        while (isdigit(*s));
    }

    /* decimal point */
    if (*s == '.' || *s == ',')
    {
        const char *sa;
        double adz = 0;

        s++;
        isd = isdigit(*s);
        if (!isd)
            return pdc_false;

        /* places after decimal point */
        sa = s;
        do
        {
            adz = 10 * adz + *s - '0';
            s++;
        }
        while (isdigit(*s));
        dz += adz / pow(10.0, (double)(s - sa));
    }

    /* power sign */
    if (*s == 'e' || *s == 'E')
    {
        s++;
        if (!isd)
            return pdc_false;

        /* sign */
        if (!*s)
        {
            dz *= 10;
        }
        else
        {
            int isp = 1;
            double pdz = 0, pdl = log10(dz);

            if (*s == '-')
            {
                isp = -1;
                s++;
            }
            else if (*s == '+')
                s++;

            if (!isdigit(*s))
                return pdc_false;
            do
            {
                pdz = 10 * pdz + *s - '0';
                s++;
            }
            while (isdigit(*s));


            if (*s || fabs(pdl + pdz) > 300.0)
                return pdc_false;

            dz *= pow(10.0, isp * pdz);
        }
    }
    else if(*s)
    {
        return pdc_false;
    }

    *o_dz = is * dz;
    return pdc_true;
}

pdc_bool
pdc_str2integer(const char *string, int *o_iz)
{
   double dz;

   if (pdc_str2double(string, &dz) == pdc_true && fabs(dz) <= INT_MAX)
   {
       *o_iz = (int) dz;
       if ((double) *o_iz == dz)
           return pdc_true;
   }

   *o_iz = 0;
   return pdc_false;
}

/* --------------------------- math  --------------------------- */

float
pdc_min(float a, float b)
{
    return (a < b ? a : b);
}

float
pdc_max(float a, float b)
{
    return (a > b ? a : b);
}
