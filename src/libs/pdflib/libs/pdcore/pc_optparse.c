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

/* $Id: pc_optparse.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Parser options routines
 *
 */

#include "pc_util.h"
#include "pc_optparse.h"

#define PDC_OPT_LISTSEPS    "\f\n\r\t\v ="

/* result of an option */
struct pdc_resopt_s
{
    int               numdef;  /* number of definitions */
    const pdc_defopt *defopt;  /* pointer to option definition */
    int               num;     /* number of parsed values */
    void             *val;     /* list of parsed values */
};

/* sizes of option types. must be parallel to pdc_opttype */
static const size_t pdc_typesizes[] =
{
    sizeof (pdc_bool),
    sizeof (char *),
    sizeof (int),
    sizeof (int),
    sizeof (float),
    sizeof (double),
    sizeof (int),
    sizeof (int),
    sizeof (int),
    sizeof (int),
    sizeof (int),
    sizeof (int),
    sizeof (int),
    sizeof (int),
    sizeof (int),
};

static const pdc_keyconn pdc_handletypes[] =
{
    {"color",      pdc_colorhandle},
    {"document",   pdc_documenthandle},
    {"font",       pdc_fonthandle},
    {"gstate",     pdc_gstatehandle},
    {"iccprofile", pdc_iccprofilehandle},
    {"image",      pdc_imagehandle},
    {"page",       pdc_pagehandle},
    {"pattern",    pdc_patternhandle},
    {"shading",    pdc_shadinghandle},
};

int
pdc_get_keycode(const char *keyword, const pdc_keyconn *keyconn)
{
    int i;
    for (i = 0; keyconn[i].word != 0; i++)
    {
        if (!strcmp(keyword, keyconn[i].word))
            return keyconn[i].code;
    }
    return PDC_KEY_NOTFOUND;
}

int
pdc_get_keycode_ci(const char *keyword, const pdc_keyconn *keyconn)
{
    int i;
    for (i = 0; keyconn[i].word != 0; i++)
    {
        if (!pdc_stricmp(keyword, keyconn[i].word))
            return keyconn[i].code;
    }
    return PDC_KEY_NOTFOUND;
}

const char *
pdc_get_keyword(int keycode, const pdc_keyconn *keyconn)
{
    int i;
    for (i = 0; keyconn[i].word != 0; i++)
    {
        if (keycode == keyconn[i].code)
            return keyconn[i].word;
    }
    return NULL;
}

const char *
pdc_get_int_keyword(char *keyword, const pdc_keyconn *keyconn)
{
    int i;
    for (i = 0; keyconn[i].word != 0; i++)
    {
        if (!pdc_stricmp(keyword, keyconn[i].word))
            return keyconn[i].word;
    }
    return NULL;
}

void
pdc_cleanup_optstringlist(pdc_core *pdc, char **stringlist, int ns)
{
    int j;

    for (j = 0; j < ns; j++)
    {
        if (stringlist[j])
            pdc_free(pdc, stringlist[j]);
    }
    pdc_free(pdc, stringlist);
}

static void
pdc_delete_optvalue(pdc_core *pdc, pdc_resopt *resopt)
{
    if (resopt->val)
    {
        if (resopt->defopt->type == pdc_stringlist)
        {
            int j;
            char **s = (char **) resopt->val;
            for (j = 0; j < resopt->num; j++)
                if (s[j])
                    pdc_free(pdc, s[j]);
        }
        pdc_free(pdc, resopt->val);
        resopt->val = NULL;
    }
    resopt->num = 0;
}

static int
pdc_optname_compare(const void *a, const void *b)
{
    return (strcmp(((pdc_resopt *)a)->defopt->name,
                   ((pdc_resopt *)b)->defopt->name));
}

pdc_resopt *
pdc_parse_optionlist(pdc_core *pdc, const char *optlist,
                     const pdc_defopt *defopt,
                     const pdc_clientdata *clientdata, pdc_bool verbose)
{
    static const char *fn = "pdc_parse_optionlist";
    const char *stemp1 = NULL, *stemp2 = NULL, *stemp3 = NULL;
    char **items = NULL, *keyword = NULL;
    char **values = NULL, *value = NULL, **strings = NULL;
    int i, nd, is, iss, it, iv, numdef, nitems = 0, nvalues, errcode = 0;
    void *resval;
    double dz, maxval;
    int iz;
    size_t len;
    const pdc_defopt *dopt = NULL;
    pdc_resopt *resopt = NULL;
    pdc_bool ignore = pdc_false;
    pdc_bool boolval = pdc_false;
    pdc_bool tocheck = pdc_false;
    pdc_bool issorted = pdc_true;
    pdc_bool ishandle = pdc_true;
    pdc_bool hastobepos;

    /* decrement handles */
    hastobepos = clientdata && clientdata->hastobepos ? pdc_true : pdc_false;

    /* split option list */
    if (optlist)
        nitems = pdc_split_stringlist(pdc, optlist, PDC_OPT_LISTSEPS, &items);
    if (nitems < 0)
    {
        keyword = (char *) optlist;
        errcode = PDC_E_OPT_NOTBAL;
        goto PDC_OPT_SYNTAXERROR;
    }

    /* initialize result list */
    for (numdef = 0; defopt[numdef].name != NULL; numdef++) {
	/* */ ;
    }
    resopt = (pdc_resopt *) pdc_calloc(pdc, numdef * sizeof(pdc_resopt), fn);
    for (i = 0; i < numdef; i++)
    {
        resopt[i].numdef = numdef;
        resopt[i].defopt = &defopt[i];

        if (defopt[i].flags & PDC_OPT_IGNOREIF1 ||
            defopt[i].flags & PDC_OPT_IGNOREIF2 ||
            defopt[i].flags & PDC_OPT_REQUIRIF1 ||
            defopt[i].flags & PDC_OPT_REQUIRIF2 ||
            defopt[i].flags & PDC_OPT_REQUIRED)
            tocheck = pdc_true;

        if (i && issorted)
            issorted = (strcmp(defopt[i-1].name, defopt[i].name) <= 0) ?
                       pdc_true : pdc_false;
    }

    /* loop over all option list elements */
    for (is = 0; is < nitems; is++)
    {
        /* search keyword */
        boolval = pdc_undef;
        keyword = items[is];
        for (it = 0; it < numdef; it++)
        {
            /* special handling for booleans */
            if (defopt[it].type == pdc_booleanlist)
            {
                if (!strcmp(keyword, defopt[it].name) ||
                    (keyword[1] != 0 && !strcmp(&keyword[2], defopt[it].name)))
                {
                    iss = is + 1;
                    if (iss == nitems ||
                        (strcmp(items[iss], "true") &&
                         strcmp(items[iss], "false")))
                    {
                        if (!strncmp(keyword, "no", 2))
                        {
                            boolval = pdc_false;
                            break;
                        }
                        else
                        {
                            boolval = pdc_true;
                            break;
                        }
                    }
                }
            }

            if (!strcmp(keyword, defopt[it].name)) break;
        }
        if (it == numdef)
        {
            errcode = PDC_E_OPT_UNKNOWNKEY;
            goto PDC_OPT_SYNTAXERROR;
        }

        /* initialize */
        dopt = &defopt[it];
        ignore = pdc_false;
        nvalues = 1;
        values = NULL;
        ishandle = pdc_true;

        /* compatibility */
        if (clientdata && clientdata->compatibility)
        {
            int compatibility = clientdata->compatibility;

            for (iv = PDC_1_3; iv < PDC_X_X_LAST; iv++)
            {
                if ((dopt->flags & (1L<<iv)) && compatibility < iv)
                {
                    errcode = PDC_E_OPT_VERSION;
                    stemp2 = pdc_errprintf(pdc, "%d.%d",
                                 compatibility / 10, compatibility % 10);
                    goto PDC_OPT_SYNTAXERROR;
                }
            }
        }

        /* not supported */
        if (dopt->flags & PDC_OPT_UNSUPP)
        {
            ignore = pdc_true;
            if (verbose)
                pdc_warning(pdc, PDC_E_OPT_UNSUPP, dopt->name, 0, 0, 0);
        }

        /* parse values */
        if (boolval == pdc_undef)
        {
            is++;
            if (is == nitems)
            {
                errcode = PDC_E_OPT_NOVALUES;
                goto PDC_OPT_SYNTAXERROR;
            }
            if (!ignore &&
                (dopt->type != pdc_stringlist || dopt->maxnum > 1))
                nvalues = pdc_split_stringlist(pdc, items[is], NULL, &values);
        }

        /* ignore */
        if (ignore) continue;

        /* number of values check */
        if (nvalues < dopt->minnum)
        {
            stemp2 = pdc_errprintf(pdc, "%d", dopt->minnum);
            errcode = PDC_E_OPT_TOOFEWVALUES;
            goto PDC_OPT_SYNTAXERROR;
        }
        else if (nvalues > dopt->maxnum)
        {
            stemp2 = pdc_errprintf(pdc, "%d", dopt->maxnum);
            errcode = PDC_E_OPT_TOOMANYVALUES;
            goto PDC_OPT_SYNTAXERROR;
        }

        /* option already exists */
        if (resopt[it].num)
        {
            pdc_delete_optvalue(pdc, &resopt[it]);
        }

        /* no values */
        if (!nvalues ) continue;

        /* maximal value */
        switch (dopt->type)
        {
            case pdc_colorhandle:
            maxval = clientdata->maxcolor;
            break;

            case pdc_documenthandle:
            maxval = clientdata->maxdocument;
            break;

            case pdc_fonthandle:
            maxval = clientdata->maxfont;
            break;

            case pdc_iccprofilehandle:
            maxval = clientdata->maxiccprofile;
            break;

            case pdc_imagehandle:
            maxval = clientdata->maximage;
            break;

            case pdc_pagehandle:
            maxval = clientdata->maxpage;
            break;

            case pdc_patternhandle:
            maxval = clientdata->maxpattern;
            break;

            case pdc_shadinghandle:
            maxval = clientdata->maxshading;
            break;

            case pdc_gstatehandle:
            maxval = clientdata->maxgstate;
            break;

            default:
            maxval = dopt->maxval;
            ishandle = pdc_false;
            break;
        }

        /* allocate value array */
        resopt[it].val = pdc_calloc(pdc,
                            (size_t) (nvalues * pdc_typesizes[dopt->type]), fn);
        resopt[it].num = nvalues;

        /* analyze type */
        resval = resopt[it].val;
        for (iv = 0; iv < nvalues; iv++)
        {
            errcode = 0;
            if (dopt->maxnum > 1 && nvalues)
                value = values[iv];
            else
                value = items[is];
            switch (dopt->type)
            {
                /* boolean list */
                case pdc_booleanlist:
                if (boolval == pdc_true || !strcmp(value, "true"))
                {
                    *(pdc_bool *) resval = pdc_true;
                }
                else if (boolval == pdc_false || !strcmp(value, "false"))
                {
                    *(pdc_bool *) resval = pdc_false;
                }
                else
                {
                    errcode = PDC_E_OPT_ILLBOOLEAN;
                }
                break;

                /* string list */
                case pdc_stringlist:
                if (dopt->flags & PDC_OPT_NOSPACES)
                {
                    if (pdc_split_stringlist(pdc, value, NULL, &strings) > 1)
                        errcode = PDC_E_OPT_ILLSPACES;
                    pdc_cleanup_stringlist(pdc, strings);
                }
                if (!errcode)
                {
                    len = strlen(value);
                    dz = (double) len;
                    if (dz < dopt->minval)
                    {
                        stemp3 = pdc_errprintf(pdc, "%d", (int) dopt->minval);
                        errcode = PDC_E_OPT_TOOSHORTSTR;
                    }
                    else if (dz > maxval)
                    {
                        stemp3 = pdc_errprintf(pdc, "%d", (int) maxval);
                        errcode = PDC_E_OPT_TOOLONGSTR;
                    }
                    *((char **) resval) = pdc_strdup(pdc, value);
                }
                break;

                /* keyword list */
                case pdc_keywordlist:
                iz = pdc_get_keycode(value, dopt->keylist);
                if (iz == PDC_KEY_NOTFOUND)
                {
                    errcode = PDC_E_OPT_ILLKEYWORD;
                }
                else
                {
                    *(int *) resval = iz;
                }
                break;

                /* number list */
                case pdc_integerlist:
                case pdc_floatlist:
                case pdc_doublelist:
                if (dopt->keylist)
                {
                    /* optional keyword and/or allowed integer list */
                    iz = pdc_get_keycode(value, dopt->keylist);
                    if (iz == PDC_KEY_NOTFOUND)
                    {
                        if (dopt->flags & PDC_OPT_INTLIST)
                        {
                            errcode = PDC_E_OPT_ILLINTEGER;
                            break;
                        }
                    }
                    else
                    {
                        switch (dopt->type)
                        {
                            default:
                            case pdc_integerlist:
                            *(int *) resval = iz;
                            break;

                            case pdc_floatlist:
                            *(float *) resval = (float) iz;
                            break;

                            case pdc_doublelist:
                            *(double *) resval = (double) iz;
                            break;
                        }
                        break;
                    }
                }
                case pdc_colorhandle:
                case pdc_documenthandle:
                case pdc_fonthandle:
                case pdc_iccprofilehandle:
                case pdc_imagehandle:
                case pdc_pagehandle:
                case pdc_patternhandle:
                case pdc_shadinghandle:
                case pdc_gstatehandle:
                if (pdc_str2double(value, &dz) == pdc_false)
                {
                    errcode = PDC_E_OPT_ILLNUMBER;
                }
                else
                {
                    if (ishandle && hastobepos) dz -= 1;
                    if (dz < dopt->minval)
                    {
                        if (ishandle)
                        {
                            stemp3 = pdc_get_keyword(dopt->type,
                                                     pdc_handletypes);
                            errcode = PDC_E_OPT_ILLHANDLE;
                        }
                        else
                        {
                            stemp3 = pdc_errprintf(pdc, "%g", dopt->minval);
                            errcode = PDC_E_OPT_TOOSMALLVAL;
                        }
                    }
                    else if (dz > maxval)
                    {
                        if (ishandle)
                        {
                            stemp3 = pdc_get_keyword(dopt->type,
                                                     pdc_handletypes);
                            errcode = PDC_E_OPT_ILLHANDLE;
                        }
                        else
                        {
                            stemp3 = pdc_errprintf(pdc, "%g", maxval);
                            errcode = PDC_E_OPT_TOOBIGVAL;
                        }
                    }
                    else if (dopt->flags & PDC_OPT_NOZERO &&
                             fabs(dz) < PDC_FLOAT_PREC)
                    {
                        errcode = PDC_E_OPT_ZEROVAL;
                    }
                    else if (dopt->type == pdc_doublelist)
                    {
                        *(double *) resval = dz;
                    }
                    else if (dopt->type == pdc_floatlist)
                    {
                        *(float *) resval = (float) dz;
                    }
                    else
                    {
                        iz = (int) dz;
                        if ((double) iz != dz)
                        {
                            errcode = PDC_E_OPT_ILLINTEGER;
                        }
                        else
                        {
                            *(int *) resval = iz;
                        }
                    }
                }
                break;
            }

            if (errcode)
            {
                stemp2 = pdc_errprintf(pdc, "%s", value);
                goto PDC_OPT_SYNTAXERROR;
            }

            /* increment value pointer */
            resval = (void *) ((char *)(resval) + pdc_typesizes[dopt->type]);
        }
        pdc_cleanup_stringlist(pdc, values);
        values = NULL;

        /* build OR bit pattern */
        if (dopt->flags & PDC_OPT_BUILDOR && nvalues > 1)
        {
            int *bcode = (int *) resopt[it].val;
            for (iv = 1; iv < nvalues; iv++)
            {
                bcode[0] |= bcode[iv];
            }
            resopt[it].num = 1;
        }
    }
    pdc_cleanup_stringlist(pdc, items);
    items = NULL;

    /* required and to be ignored options */
    for (is = 0; tocheck && is < numdef; is++)
    {
        /* to be ignored option */
        if (resopt[is].num)
        {
            nd = 0;
            if (defopt[is].flags & PDC_OPT_IGNOREIF1) nd = 1;
            if (defopt[is].flags & PDC_OPT_IGNOREIF2) nd = 2;
            for (it = is - 1; it >= is - nd && it >= 0; it--)
            {
                if (resopt[it].num)
                {
                    pdc_delete_optvalue(pdc, &resopt[is]);
                    if (verbose)
                        pdc_warning(pdc, PDC_E_OPT_IGNORE, defopt[is].name,
                                    defopt[it].name, 0, 0);
                }
            }
        }

        /* required option */
        if (!resopt[is].num &&
            ((defopt[is].flags & PDC_OPT_REQUIRED) ||
             (defopt[is].flags & PDC_OPT_REQUIRIF1 && resopt[is-1].num) ||
             (defopt[is].flags & PDC_OPT_REQUIRIF2 &&
              (resopt[is-1].num || resopt[is-2].num))))
        {
            keyword = (char *) defopt[is].name;
            errcode = PDC_E_OPT_NOTFOUND;
            goto PDC_OPT_SYNTAXERROR;
        }
    }

    /* is no sorted */
    if (!issorted)
    {
        qsort((void *)resopt, (size_t) numdef, sizeof(pdc_resopt),
              pdc_optname_compare);
    }

#undef PDC_OPTPARSE
#ifdef PDC_OPTPARSE
    printf("\n");
    for (is = 0; is < numdef; is++)
    {
        printf("[%02d] %s (number = %d, pointer = %p):\n", is,
               resopt[is].defopt->name, resopt[is].num, resopt[is].val);
        for (iv = 0; iv < resopt[is].num; iv++)
        {
            switch (resopt[is].defopt->type)
            {
                case pdc_booleanlist:
                case pdc_keywordlist:
                case pdc_integerlist:
                case pdc_colorhandle:
                case pdc_documenthandle:
                case pdc_fonthandle:
                case pdc_gstatehandle:
                case pdc_iccprofilehandle:
                case pdc_imagehandle:
                case pdc_pagehandle:
                case pdc_patternhandle:
                case pdc_shadinghandle:
                printf("     [%d]: %d\n",iv, *((int *) resopt[is].val + iv));
                break;

                case pdc_stringlist:
                printf("     [%d]: %s\n",iv, *((char **) resopt[is].val + iv));
                break;

                case pdc_floatlist:
                printf("     [%d]: %f\n",iv, *((float *) resopt[is].val + iv));
                break;

                case pdc_doublelist:
                printf("     [%d]: %f\n",iv, *((double *) resopt[is].val + iv));
                break;
            }
        }
    }
    printf("\n");
#endif

    return resopt;

    PDC_OPT_SYNTAXERROR:
    stemp1 = pdc_errprintf(pdc, "%s", keyword);
    pdc_cleanup_stringlist(pdc, items);
    pdc_cleanup_stringlist(pdc, values);
    pdc_cleanup_optionlist(pdc, resopt);

    switch (errcode)
    {
        case PDC_E_OPT_UNKNOWNKEY:
        case PDC_E_OPT_NOVALUES:
        case PDC_E_OPT_NOTFOUND:
        case PDC_E_OPT_NOTBAL:
        pdc_set_errmsg(pdc, errcode, stemp1, 0, 0, 0);
        break;

        case PDC_E_OPT_TOOFEWVALUES:
        case PDC_E_OPT_TOOMANYVALUES:
        case PDC_E_OPT_ILLBOOLEAN:
        case PDC_E_OPT_ILLKEYWORD:
        case PDC_E_OPT_ILLINTEGER:
        case PDC_E_OPT_ILLNUMBER:
        case PDC_E_OPT_ZEROVAL:
        case PDC_E_OPT_ILLSPACES:
        case PDC_E_OPT_VERSION:
        pdc_set_errmsg(pdc, errcode, stemp1, stemp2, 0, 0);
        break;

        case PDC_E_OPT_TOOSHORTSTR:
        case PDC_E_OPT_TOOLONGSTR:
        case PDC_E_OPT_TOOSMALLVAL:
        case PDC_E_OPT_TOOBIGVAL:
        case PDC_E_OPT_ILLHANDLE:
        pdc_set_errmsg(pdc, errcode, stemp1, stemp2, stemp3, 0);
        break;
    }

    if (verbose)
        pdc_error(pdc, -1, 0, 0, 0, 0);

    return NULL;
}

int
pdc_get_optvalues(pdc_core *pdc, const char *keyword, pdc_resopt *resopt,
                  void *lvalues, void **mvalues)
{
    pdc_resopt *ropt = NULL;
    void *values = NULL;
    int nvalues = 0;
    size_t nbytes;
    if (mvalues) *mvalues = NULL;

    (void) pdc;

    if (resopt)
    {
        int i, cmp;
        int lo = 0;
        int hi = resopt[0].numdef;

        while (lo < hi)
        {
            i = (lo + hi) / 2;
            cmp = strcmp(keyword, resopt[i].defopt->name);

            /* keyword found */
            if (cmp == 0)
            {
                ropt = &resopt[i];
                nvalues = ropt->num;
                values = ropt->val;
                break;
            }

            if (cmp < 0)
                hi = i;
            else
                lo = i + 1;
        }
    }

    if (nvalues)
    {
        /* copy values */
        if (lvalues)
        {
            if (ropt->defopt->type == pdc_stringlist &&
                ropt->defopt->maxnum == 1)
            {
                strcpy((char *)lvalues, *((char **) values));
            }
            else
            {
                nbytes = (size_t) (nvalues * pdc_typesizes[ropt->defopt->type]);
                memcpy(lvalues, values, nbytes);
            }
        }

        /* copy pointer */
        if (mvalues)
        {
            *mvalues = values;
            ropt->val = NULL;
        }
    }

    return nvalues;
}

void
pdc_cleanup_optionlist(pdc_core *pdc, pdc_resopt *resopt)
{
    if (resopt)
    {
        int i;

        for (i = 0; i < resopt[0].numdef; i++)
            pdc_delete_optvalue(pdc, &resopt[i]);

        pdc_free(pdc, resopt);
    }
}

const char *
pdc_get_handletype(pdc_opttype type)
{
    return pdc_get_keyword(type, pdc_handletypes);
}

