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

/* $Id: p_resource.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib resource routines
 *
 */

#include <errno.h>

#include "p_intern.h"

#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define RESOURCEFILE            "PDFLIBRESOURCE"        /* name of env var. */

#ifndef MVS
#define DEFAULTRESOURCEFILE     "pdflib.upr"
#else
#define DEFAULTRESOURCEFILE     "upr"
#endif

struct pdf_res_s {
    char                *name;
    char                *value;
    pdf_res             *prev;
    pdf_res             *next;
};

struct pdf_category_s {
    char                *category;
    pdf_res             *kids;
    pdf_category        *next;
};

struct pdf_virtfile_s {
    char                *name;
    const void          *data;
    size_t              size;
    pdc_bool            iscopy;
    int                 lockcount;
    pdf_virtfile        *next;
};

typedef enum {
    pdf_FontOutline,
    pdf_FontAFM,
    pdf_FontPFM,
    pdf_HostFont,
    pdf_Encoding,
    pdf_ICCProfile,
    pdf_StandardOutputIntent,
    pdf_SearchPath
} pdf_rescategory;

static const pdc_keyconn pdf_rescategories[] =
{
    {"FontOutline",          pdf_FontOutline},
    {"FontAFM",              pdf_FontAFM},
    {"FontPFM",              pdf_FontPFM},
    {"HostFont",             pdf_HostFont},
    {"Encoding",             pdf_Encoding},
    {"ICCProfile",           pdf_ICCProfile},
    {"StandardOutputIntent", pdf_StandardOutputIntent},
    {"SearchPath",           pdf_SearchPath},
    {NULL, 0}
};

static void
pdf_read_resourcefile(PDF *p, const char *filename)
{
    pdc_file   *fp = NULL;
    char      **linelist;
    char       *line;
    char       *category = NULL;
    char       *uprfilename = NULL;
#if defined(AS400) || defined(WIN32)
#define BUFSIZE 2048
    char        buffer[BUFSIZE];
#ifdef WIN32
    char        regkey[128];
    HKEY        hKey = NULL;
    DWORD       size, lType;
#endif
#endif
    int         il, nlines = 0, nextcat, begin;

#ifdef WIN32

/* don't add patchlevel's to registry searchpath */
#define stringiz1(x)	#x
#define stringiz(x)	stringiz1(x)

#define PDFLIBKEY  "Software\\PDFlib\\PDFlib\\"

    strcpy(regkey, PDFLIBKEY);
    strcat(regkey, PDFLIB_VERSIONSTRING);

    /* process registry entries */
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, regkey, 0L,
        (REGSAM) KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        size = BUFSIZE - 2;
        if (RegQueryValueExA(hKey, "SearchPath", (LPDWORD) NULL,
                             &lType, (LPBYTE) buffer, &size)
            == ERROR_SUCCESS && *buffer)
        {
            char **pathlist;
            int ip, np;

            np = pdc_split_stringlist(p->pdc, buffer,
                                      ";", &pathlist);
            for (ip = 0; ip < np; ip++)
                pdf_add_resource(p, "SearchPath", pathlist[ip]);
            pdc_cleanup_stringlist(p->pdc, pathlist);
        }

        size = BUFSIZE - 2;
        if (RegQueryValueExA(hKey, "prefix", (LPDWORD) NULL,
                             &lType, (LPBYTE) buffer, &size)
            == ERROR_SUCCESS && *buffer)
        {
            /* '/' because of downward compatibility */
            if (p->prefix)
            {
                pdc_free(p->pdc, p->prefix);
                p->prefix = NULL;
            }
            p->prefix = pdc_strdup(p->pdc,
                            &buffer[buffer[0] == '/' ? 1 : 0]);
        }

        RegCloseKey(hKey);
    }
#endif  /* WIN32 */

#ifdef AS400
    strcpy (buffer, "/pdflib/");
    strcat (buffer, PDFLIB_VERSIONSTRING);
    il = (int) strlen(buffer);
    strcat (buffer, "/fonts");
    pdf_add_resource(p, "SearchPath", buffer);
    strcpy(&buffer[il], "/bind/data");
    pdf_add_resource(p, "SearchPath", buffer);
#endif  /* AS400 */

    /* searching for name of upr file */
    uprfilename = (char *)filename;
    if (!uprfilename || *uprfilename == '\0')
    {
        /* user-supplied upr file */
        uprfilename = pdc_getenv(RESOURCEFILE);
        if (!uprfilename || *uprfilename == '\0')
        {
            uprfilename = DEFAULTRESOURCEFILE;

            /* user-supplied upr file */
            fp = pdf_fopen(p, uprfilename, NULL, 0);
            if (fp == NULL)
            {
                uprfilename = NULL;
#ifdef WIN32
                /* process registry entries */
                if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, regkey, 0L,
                    (REGSAM) KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
                {
                    size = BUFSIZE - 2;
                    if (RegQueryValueExA(hKey, "resourcefile", (LPDWORD) NULL,
                                         &lType, (LPBYTE) buffer, &size)
                        == ERROR_SUCCESS && *buffer)
                    {
                        uprfilename = buffer;
                    }

                    RegCloseKey(hKey);
                }
#endif  /* WIN32 */
            }
        }

        if (!uprfilename || *uprfilename == '\0')
            return;

        if (p->resourcefilename)
        {
            pdc_free(p->pdc, p->resourcefilename);
            p->resourcefilename = NULL;
        }
        p->resourcefilename = pdc_strdup(p->pdc, uprfilename);
    }

    /* read upr file */
    if ((fp == NULL) && ((fp = pdf_fopen(p, uprfilename, "UPR ", 0)) == NULL))
	pdc_error(p->pdc, -1, 0, 0, 0, 0);

    nlines = pdc_read_textfile(p->pdc, fp, &linelist);
    pdc_fclose(fp);

    if (!nlines) return;

    /* Lines loop */
    begin = 1;
    nextcat = 0;
    for (il = 0; il < nlines; il++)
    {
        line = linelist[il];

        /* Next category */
        if (line[0] == '.' && strlen(line) == 1)
        {
            begin = 0;
            nextcat = 1;
            continue;
        }

        /* Skip category list */
        if (begin) continue;

        /* Prefiex or category expected */
        if (nextcat)
        {
            /* Directory prefix */
            if (line[0] == '/')
            {
                if (p->prefix)
                {
                    pdc_free(p->pdc, p->prefix);
                    p->prefix = NULL;
                }
                p->prefix = pdc_strdup(p->pdc, &line[1]);
                continue;
            }

            /* Ressource Category */
            category = line;
            nextcat = 0;
            continue;
        }

        /* Add resource */
        pdf_add_resource(p, category, line);
    }

    pdc_cleanup_stringlist(p->pdc, linelist);
}

void
pdf_add_resource(PDF *p, const char *category, const char *resource)
{
    static const char fn[] = "pdf_add_resource";
    pdf_rescategory rescat;
    pdf_category *cat, *lastcat = NULL;
    pdf_res *res, *lastres = NULL;
    char          *name;
    char          *value;
    char          *prefix = NULL;
    size_t        len;
    int           k, absolut;

    /* We no longer raise an error but silently ignore unknown categories */
    k = pdc_get_keycode(category, pdf_rescategories);
    if (k == PDC_KEY_NOTFOUND)
        return;
    rescat = (pdf_rescategory) k;

    /* Read resource configuration file if it is pending */
    if (p->resfilepending)
    {
        p->resfilepending = pdc_false;
        pdf_read_resourcefile(p, p->resourcefilename);
    }

    /* Find start of this category's resource list, if the category exists */
    for (cat = p->resources; cat != (pdf_category *) NULL; cat = cat->next)
    {
        lastcat = cat;
        if (!strcmp(cat->category, category))
            break;
    }
    if (cat == NULL)
    {
        cat = (pdf_category *) pdc_malloc(p->pdc, sizeof(pdf_category), fn);
        cat->category = pdc_strdup(p->pdc, category);
        cat->kids = NULL;
        cat->next = NULL;

        if (lastcat)
            lastcat->next = cat;
        else
            p->resources = cat;
    }

    /* Determine name and value of resource */
    absolut = 0;
    len = strlen(resource);
    value = strchr(resource, '=');
    if (value)
    {
        len = (size_t) (value - resource);
        value++;
        if (*value == '=')
        {
            absolut = 1;
            value++;
        }

        /* file name is assumed */
        if (value[0] != '\0' && value[0] == '.' && value[1] == '/')
        {
            value += 2;
        }
    }

    /* Copy resource name */
    name = (char *) pdc_malloc(p->pdc, len + 1, fn);
    strncpy(name, resource, len);
    name[len] = 0;
    pdc_strtrim(name);

    /* Find resource name in resource list */
    for (res = cat->kids; res != (pdf_res *) NULL; res = res->next)
    {
        if (!strcmp(res->name, name))
            break;
        lastres = res;
    }

    /* New resource */
    if (res)
    {
        pdc_free(p->pdc, name);
    }
    else
    {
        res = (pdf_res *) pdc_calloc(p->pdc, sizeof(pdf_res), fn);
        if (lastres)
            lastres->next = res;
        else
            cat->kids = res;
        res->prev = lastres;
        res->name = name;
    }

    /* New value */
    if (res->value)
        pdc_free(p->pdc, res->value);
    res->value = NULL;
    if (!value)
    {
        value = "";
    }
    else if (!absolut && p->prefix)
    {
        /* Directory prefix */
        prefix = p->prefix;
        if (prefix[0] != '\0' && prefix[0] == '.' && prefix[1] == '/')
            prefix += 2;
        if (prefix)
        {
            len = strlen(prefix) + strlen(value) + 6;
            res->value = (char *) pdc_malloc(p->pdc, len, fn);
            pdc_file_fullname(prefix, value, res->value);
        }
    }
    if (!res->value)
    {
        res->value = pdc_strdup(p->pdc, value);
        pdc_str2trim(res->value);
    }

#undef PDF_TEST_RESOURCE
#ifdef PDF_TEST_RESOURCE
    printf("%s.%s: '%s'\n", category, res->name, res->value);
#endif

    switch (rescat)
    {
        case pdf_FontOutline:
        case pdf_FontAFM:
        case pdf_FontPFM:
        case pdf_HostFont:
        case pdf_Encoding:
        case pdf_ICCProfile:
        if (!strlen(res->name) || !strlen(res->value))
            pdc_error(p->pdc, PDF_E_RES_BADRES, resource, category, 0, 0);
        break;

        default:
        break;
    }
}

char *
pdf_find_resource(PDF *p, const char *category, const char *name)
{
    pdf_category *cat;
    pdf_res *res;

    /* Read resource configuration file if it is pending */
    if (p->resfilepending)
    {
        p->resfilepending = pdc_false;
        pdf_read_resourcefile(p, p->resourcefilename);
    }

    for (cat = p->resources; cat != (pdf_category *) NULL; cat = cat->next)
    {
        if (!strcmp(cat->category, category))
        {
            for (res = cat->kids; res != (pdf_res *)NULL; res = res->next)
            {
                if (!strcmp(res->name, name))
                {
                    return res->value;
                }
            }
        }
    }

    return NULL;
}

void
pdf_cleanup_resources(PDF *p)
{
    pdf_category *cat, *lastcat;
    pdf_res *res, *lastres;

    for (cat = p->resources; cat != (pdf_category *) NULL; /* */) {
        for (res = cat->kids; res != (pdf_res *) NULL; /* */) {
            lastres = res;
            res = lastres->next;
            pdc_free(p->pdc, lastres->name);
            if (lastres->value)
                pdc_free(p->pdc, lastres->value);
            pdc_free(p->pdc, lastres);
        }
        lastcat = cat;
        cat = lastcat->next;
        pdc_free(p->pdc, lastcat->category);
        pdc_free(p->pdc, lastcat);
    }

    p->resources = NULL;
}

static pdf_virtfile *
pdf_find_pvf(PDF *p, const char *filename, pdf_virtfile **lastvfile)
{
    pdf_virtfile  *vfile;

    if (lastvfile != NULL)
        *lastvfile = NULL;
    for (vfile = p->filesystem; vfile != NULL; vfile = vfile->next)
    {
        if (!strcmp(vfile->name, filename))
            return vfile;
        if (lastvfile != NULL)
            *lastvfile = vfile;
    }
    return NULL;
}

/* definitions of pvf options */
static const pdc_defopt pdf_create_pvf_options[] =
{
    {"copy", pdc_booleanlist, 0, 1, 1, 0.0, 0.0, NULL},

    PDC_OPT_TERMINATE
};

void
pdf__create_pvf(PDF *p, const char *filename, int reserved,
                const void *data, size_t size, const char *optlist)
{
    static const char fn[] = "pdf__create_pvf";
    pdc_bool iscopy = pdc_false;
    pdf_virtfile  *vfile, *lastvfile = NULL;
    pdc_resopt *results;

    (void) reserved;

    /* Parse optlist */
    results = pdc_parse_optionlist(p->pdc, optlist, pdf_create_pvf_options,
                                   NULL, pdc_true);
    pdc_get_optvalues(p->pdc, "copy", results, &iscopy, NULL);
    pdc_cleanup_optionlist(p->pdc, results);

    /* Find virtual file in file system */
    vfile = pdf_find_pvf(p, filename, &lastvfile);

    /* Name already exists */
    if (vfile != NULL)
        pdc_error(p->pdc, PDC_E_PVF_NAMEEXISTS, filename, 0, 0, 0);

    /* New virtual file */
    vfile = (pdf_virtfile *) pdc_calloc(p->pdc, sizeof(pdf_virtfile), fn);
    if (lastvfile)
        lastvfile->next = vfile;
    else
        p->filesystem = vfile;

    /* Fill up file struct */
    vfile->name = pdc_strdup(p->pdc, filename);
    if (iscopy == pdc_true)
    {
        vfile->data = (const void *) pdc_malloc(p->pdc, size, fn);
        memcpy((void *) vfile->data, data, size);
    }
    else
    {
        vfile->data = data;
    }
    vfile->size = size;
    vfile->iscopy = iscopy;
    vfile->lockcount = 0;
    vfile->next = NULL;
}

int
pdf__delete_pvf(PDF *p, const char *filename, int reserved)
{
    pdf_virtfile  *vfile, *lastvfile = NULL;

    (void) reserved;

    /* Find virtual file in file system */
    vfile = pdf_find_pvf(p, filename, &lastvfile);
    if (vfile)
    {
        /* File exists but locked */
        if (vfile->lockcount > 0)
        {
            return pdc_undef;
        }

        /* Delete */
        if (vfile->iscopy == pdc_true)
        {
            pdc_free(p->pdc, (void *) vfile->data);
            vfile->data = NULL;
        }
        pdc_free(p->pdc, vfile->name);
        if (lastvfile)
            lastvfile->next = vfile->next;
        else
            p->filesystem = vfile->next;
        pdc_free(p->pdc, vfile);
    }

    return pdc_true;
}

PDFLIB_API void PDFLIB_CALL
PDF_create_pvf(PDF *p, const char *filename, int reserved,
               const void *data, size_t size, const char *optlist)
{
    static const char fn[] = "PDF_create_pvf";

    if (!pdf_enter_api(p, fn, pdf_state_all,
        "(p[%p], \"%s\", %d, \"%p\", %d, \"%s\")\n",
        (void *) p, filename, reserved, data, size, optlist))
    {
        return;
    }

    if (filename == NULL || *filename == '\0')
        pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "filename", 0, 0, 0);

    if (!data)
        pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "data", 0, 0, 0);

    if (!size)
        pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "size", 0, 0, 0);

    pdf__create_pvf(p, filename, reserved, data, size, optlist);
}

PDFLIB_API int PDFLIB_CALL
PDF_delete_pvf(PDF *p, const char *filename, int reserved)
{
    static const char fn[] = "PDF_delete_pvf";
    pdc_bool retval = pdc_true;

    if (!pdf_enter_api(p, fn, pdf_state_all,
        "(p[%p], \"%s\", %d)",
        (void *) p, filename, reserved))
    {
        PDF_RETURN_BOOLEAN(p, retval);
    }

    if (filename == NULL || *filename == '\0')
        pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "filename", 0, 0, 0);

    retval = pdf__delete_pvf(p, filename, reserved);

    PDF_RETURN_BOOLEAN(p, retval);
}

void
pdf_lock_pvf(PDF *p, const char *filename)
{
    pdf_virtfile *vfile = pdf_find_pvf(p, filename, NULL);
    if (vfile)
        (vfile->lockcount)++;
}

void
pdf_unlock_pvf(PDF *p, const char *filename)
{
    pdf_virtfile *vfile = pdf_find_pvf(p, filename, NULL);
    if (vfile)
        (vfile->lockcount)--;
}

void
pdf_cleanup_filesystem(PDF *p)
{
    pdf_virtfile *vfile, *nextvfile;

    for (vfile = p->filesystem; vfile != NULL; /* */)
    {
        nextvfile = vfile->next;
        if (vfile->iscopy == pdc_true && vfile->data)
            pdc_free(p->pdc, (void *) vfile->data);
        if (vfile->name)
            pdc_free(p->pdc, vfile->name);
        pdc_free(p->pdc, vfile);
        vfile = nextvfile;
    }
    p->filesystem = NULL;
}
#if defined(_MSC_VER) && defined(_MANAGED)
#pragma unmanaged
#endif
pdc_file *
pdf_fopen(PDF *p, const char *filename, const char *qualifier, int flags)
{
    const pdc_byte *data = NULL;
    size_t size = 0;
    pdf_virtfile *vfile;

    vfile = pdf_find_pvf(p, filename, NULL);
    if (vfile)
    {
        size = vfile->size;
        data = (const pdc_byte *) vfile->data;
        return pdc_fopen(p->pdc, filename, qualifier, data, size, flags);
    }
    else
    {
        pdf_category *cat;

        /* Bad filename */
        if (!*filename || !strcmp(filename, ".") || !strcmp(filename, ".."))
        {
            pdc_set_errmsg(p->pdc, PDC_E_IO_ILLFILENAME, filename, 0, 0, 0);
            return NULL;
        }

        /* Read resource configuration file if it is pending */
        if (p->resfilepending)
        {
            p->resfilepending = pdc_false;
            pdf_read_resourcefile(p, p->resourcefilename);
        }

#ifdef PDF_TEST_RESOURCE
    printf("Search for file '%s'\n", filename);
#endif

        /* Searching resource category */
        for (cat = p->resources; cat != (pdf_category *) NULL; cat = cat->next)
            if (!strcmp(cat->category, "SearchPath")) break;

        if (!cat)
        {
            /* No resource category */
            return pdc_fopen(p->pdc, filename, qualifier, NULL, 0, flags);
        }
        else
        {
            pdf_res *res = cat->kids;
            pdf_res *lastres = cat->kids;
            char *pathname = NULL;
            char fullname[PDC_FILENAMELEN];
            FILE *fp = NULL;
            int errnum = PDC_E_IO_RDOPEN_NF;

            /* Find last SearchPath entry */
            while (res != (pdf_res *) NULL)
            {
                lastres = res;
                res = res->next;
            }

            /* First local search and then search with concatenated
             * filename with search paths one after another backwards
             */
            while (1)
            {
                /* Test opening */
                pdc_file_fullname(pathname, filename, fullname);

#ifdef PDF_TEST_RESOURCE
    printf("    pathname '%s' -> fullname: '%s'\n", pathname, fullname);
#endif
                fp = fopen(fullname, READBMODE);
                if (fp)
                {
                    /* File found */
                    fclose(fp);
                    return
			pdc_fopen(p->pdc, fullname, qualifier, NULL, 0, flags);
                }
                errnum = pdc_get_fopen_errnum(p->pdc, PDC_E_IO_RDOPEN);
                if (errno != 0 && errnum != PDC_E_IO_RDOPEN_NF) break;

                if (lastres == (pdf_res *) NULL)
                    break;

                pathname = lastres->name;
                lastres = lastres->prev;
            }

	    pdc_set_errmsg(p->pdc, errnum, qualifier, filename, 0, 0);
	    return NULL;
        }
    }
}
#if defined(_MSC_VER) && defined(_MANAGED)
#pragma managed
#endif
