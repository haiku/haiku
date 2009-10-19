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

/* $Id: pdflibdl.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * C wrapper code for dynamically loading the PDFlib DLL at runtime.
 *
 * This module is not supported on all platforms.
 *
 */

#include <stdlib.h>

#include "pdflibdl.h"

/* enable this to avoid error messages */
//#define PDF_SILENT

/* ---------------------------------- WIN32 ----------------------------- */

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winbase.h>
#undef WIN32_LEAN_AND_MEAN

#define PDF_DLLNAME			"pdflib.dll"

static void *
pdf_dlopen(const char *filename)
{
    return (void *) LoadLibrary(filename);
}

static void *
pdf_dlsym(void *handle, const char *funcname)
{
    return (void *) GetProcAddress((HINSTANCE) handle, funcname);
}

static void
pdf_dlclose(void *handle)
{
    (void) FreeLibrary((HINSTANCE) handle);
}

/* ---------------------------------- MVS ----------------------------- */

#elif defined(__MVS__)

#include <dynit.h>
#include <dll.h>

#define PDF_DLLNAME			"PDFLIB"

static void *
pdf_dlopen(const char *filename)
{
    return dllload(filename);
}

static void *
pdf_dlsym(void *handle, const char *funcname)
{
    return dllqueryfn((dllhandle *) handle, funcname);
}

static void pdf_dlclose(void *handle)
{
    (void) dllfree((dllhandle *) handle);
}

/* ---------------------------------- Linux  ----------------------------- */

#elif defined(linux)

#include <dlfcn.h>

#define PDF_DLLNAME			"libpdf.so"

static void *
pdf_dlopen(const char *filename)
{
    return dlopen(filename, RTLD_LAZY);
}

static void *
pdf_dlsym(void *handle, const char *funcname)
{
    return dlsym(handle, funcname);
}

static void pdf_dlclose(void *handle)
{
    (void) dlclose(handle);
}

/* ---------------------------------- Mac OS X  ----------------------------- */

#elif defined(__ppc__) && defined(__APPLE__)

#define PDF_DLLNAME			"libpdf.dylib"

/*
 * The dl code for Mac OS X has been butchered from dlcompat,
 * see http://www.opendarwin.org/projects/dlcompat
 * It contained the copyright notice below.
 */

/*
Copyright (c) 2002 Peter O'Gorman <ogorman@users.sourceforge.net>

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <string.h>
#include <mach-o/dyld.h>

#if defined (__GNUC__) && __GNUC__ > 3
#define dl_restrict __restrict
#else
#define dl_restrict
#endif
/*
 * Structure filled in by dladdr().
 */

typedef struct dl_info {
        const char      *dli_fname;     /* Pathname of shared object */
        void            *dli_fbase;     /* Base address of shared object */
        const char      *dli_sname;     /* Name of nearest symbol */
        void            *dli_saddr;     /* Address of nearest symbol */
} Dl_info;

#define RTLD_LAZY	0x1
#define RTLD_NOW	0x2
#define RTLD_LOCAL	0x4
#define RTLD_GLOBAL	0x8
#define RTLD_NOLOAD	0x10
#define RTLD_NODELETE	0x80

/*
 * Special handle arguments for dlsym().
 */
#define	RTLD_NEXT		((void *) -1)	/* Search subsequent objects. */
#define	RTLD_DEFAULT	((void *) -2)	/* Use default search algorithm. */

static void *dlsymIntern(void *handle, const char *symbol);

void *pdf_dlopen(const char *path)
{
    int mode = RTLD_LAZY;
    void *module = 0;
    NSObjectFileImage ofi = 0;
    NSObjectFileImageReturnCode ofirc;
    static int (*make_private_module_public) (NSModule module) = 0;
    unsigned int flags =  
	NSLINKMODULE_OPTION_RETURN_ON_ERROR | NSLINKMODULE_OPTION_PRIVATE;

/* If we got no path, the app wants the global namespace, use -1 as the marker
       in this case */
    if (!path)
	    return (void *)-1;

    /* Create the object file image, works for things linked with the
	-bundle arg to ld */
    ofirc = NSCreateObjectFileImageFromFile(path, &ofi);
    switch (ofirc)
    {
	case NSObjectFileImageSuccess:
		/* It was okay, so use NSLinkModule to link in the image */
		if (!(mode & RTLD_LAZY)) flags += NSLINKMODULE_OPTION_BINDNOW;
		module = NSLinkModule(ofi, path,flags);
    /* Don't forget to destroy the object file image, unless you like leaks */
		NSDestroyObjectFileImage(ofi);
	    /* If the mode was global, then change the module, this avoids
	       multiply defined symbol errors to first load private then make
	       global. Silly, isn't it. */
		if ((mode & RTLD_GLOBAL))
		{
		  if (!make_private_module_public)
		  {
		    _dyld_func_lookup("__dyld_NSMakePrivateModulePublic", 
			(unsigned long *)&make_private_module_public);
		  }
		  make_private_module_public(module);
		}
		break;
	case NSObjectFileImageInappropriateFile:
/* It may have been a dynamic library rather than a bundle, try to load it */
		module = 
		    (void *)NSAddImage(path, NSADDIMAGE_OPTION_RETURN_ON_ERROR);
		break;
	case NSObjectFileImageFailure:
		/*
		error(0,"Object file setup failure :  \"%s\"", path);
		*/
		return 0;
	case NSObjectFileImageArch:
		/*
		error(0,"No object for this architecture :  \"%s\"", path);
		*/
		return 0;
	case NSObjectFileImageFormat:
		/*
		error(0,"Bad object file format :  \"%s\"", path);
		*/
		return 0;
	case NSObjectFileImageAccess:
		/*
		error(0,"Can't read object file :  \"%s\"", path);
		*/
		return 0;		
    }
    if (!module)
	    /*
	    error(0, "Can not open \"%s\"", path);
	    */
	    ;

    return module;
}

/* dlsymIntern is used by dlsym to find the symbol */
void *dlsymIntern(void *handle, const char *symbol)
{
    NSSymbol *nssym = 0;
    /* If the handle is -1, if is the app global context */
    if (handle == (void *)-1)
    {
	/* Global context, use NSLookupAndBindSymbol */
	if (NSIsSymbolNameDefined(symbol))
	{
		nssym = NSLookupAndBindSymbol(symbol);
	}

    }
    /* Now see if the handle is a struch mach_header* or not, use 
	NSLookupSymbol in image for libraries, and NSLookupSymbolInModule
	for bundles */
    else
    {
    /* Check for both possible magic numbers depending on x86/ppc byte order */
	if ((((struct mach_header *)handle)->magic == MH_MAGIC) ||
		(((struct mach_header *)handle)->magic == MH_CIGAM))
	{
	if (NSIsSymbolNameDefinedInImage((struct mach_header *)handle, symbol))
	    {
		nssym = NSLookupSymbolInImage((struct mach_header *)handle,
			symbol, NSLOOKUPSYMBOLINIMAGE_OPTION_BIND
			| NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
	    }

	}
	else
	{
		nssym = NSLookupSymbolInModule(handle, symbol);
	}
    }
    if (!nssym)
    {
	/*
	error(0, "Symbol \"%s\" Not found", symbol);
	*/
	return NULL;
    }
    return NSAddressOfSymbol(nssym);
}

int pdf_dlclose(void *handle)
{
    if ((((struct mach_header *)handle)->magic == MH_MAGIC) ||
	    (((struct mach_header *)handle)->magic == MH_CIGAM))
    {
	    /*
	    error(-1, "Can't remove dynamic libraries on darwin");
	    */
	    return 0;
    }
    if (!NSUnLinkModule(handle, 0))
    {
	    /*
	    error(0, "unable to unlink module %s", NSNameOfModule(handle));
	    */
	    return 1;
    }
    return 0;
}

/* dlsym, prepend the underscore and call dlsymIntern */
void *pdf_dlsym(void *handle, const char *symbol)
{
    static char undersym[257];	/* Saves calls to malloc(3) */
    int sym_len = strlen(symbol);
    void *value = NULL;
    char *malloc_sym = NULL;

    if (sym_len < 256)
    {
	    snprintf(undersym, 256, "_%s", symbol);
	    value = dlsymIntern(handle, undersym);
    }
    else
    {
	    malloc_sym = malloc(sym_len + 2);
	    if (malloc_sym)
	    {
		    sprintf(malloc_sym, "_%s", symbol);
		    value = dlsymIntern(handle, malloc_sym);
		    free(malloc_sym);
	    }
	    else
	    {
		    /*
		    error(-1, "Unable to allocate memory");
		    */
	    }
    }
    return value;
}

/* ---------------------------------- AS/400  ----------------------------- */

#elif defined __ILEC400__

#include <string.h>

#include <miptrnam.h>
#include <qleawi.h>
#include <qusec.h>
#include "mgosifc.h"

#define PDF_DLLNAME			"PDFLIB"

static void *
pdf_dlopen(const char *filename)
{
    char libName[11], objName[11], *s;
    HMODULE handle;
    _SYSPTR pSrvPgm;
    Qle_ABP_Info_t actInfo;
    int actInfoLen;
    Qus_EC_t errCode;

    memset(libName, '\0', sizeof(libName));
    if ((s = strchr(filename, '/')) == NULL) {
	s = filename;
    } else {
	memcpy(libName, filename,  s - filename);
	if (!strcmp(libName, "*LIBL"))
	    libName[0] = '\0';
	s += 1;
    }
    strcpy(objName, s);

    /* Get system pointer to service program */
    pSrvPgm = rslvsp(WLI_SRVPGM, objName, libName, _AUTH_NONE);

    /* Activate Bound Program */
    handle = malloc(sizeof(int));
    actInfoLen = sizeof(actInfo);
    errCode.Bytes_Provided = sizeof(errCode);
    QleActBndPgm(&pSrvPgm, handle, &actInfo, &actInfoLen, &errCode);

    if (errCode.Bytes_Available > 0)
	return NULL;

    return (void *) handle;
}

static void *
pdf_dlsym(void *handle, const char *funcname)
{
    int expID;
    int expNameLen;
    void *ret;
    int expType;
    Qus_EC_t errCode;

    expID = 0;
    expNameLen = strlen(funcname);
    errCode.Bytes_Provided = sizeof(errCode);
    QleGetExp((HMODULE) handle,
	&expID, &expNameLen, funcname, &ret, &expType, &errCode);

    if (errCode.Bytes_Available > 0)
	return NULL;

    return ret;
}

static void pdf_dlclose(void *handle)
{
    free((HMODULE) handle);
}

/* ---------------------------------- unknown  ----------------------------- */

#else

#error No DLL loading code for this platform available!

#endif

/* ---------------------------------- generic  ----------------------------- */

static void
pdf_dlerror(const char *msg)
{
#ifndef PDF_SILENT
    fprintf(stderr, msg);
#endif
}

/* Load the PDFlib DLL and fetch the API structure */

PDFLIB_API PDFlib_api * PDFLIB_CALL
PDF_new_dl(PDF **pp)
{
    PDFlib_api *PDFlib, *(PDFLIB_CALL *get_api)(void);
    char buf[256];
    void *handle;
    PDF *p;

    /* load the PDFLIB DLL... */
    handle = pdf_dlopen(PDF_DLLNAME);

    if (!handle)
    {
	pdf_dlerror("Error: couldn't load PDFlib DLL\n");
	return NULL;
    }

    /* ...and fetch function pointers */
    get_api = (PDFlib_api *(PDFLIB_CALL *)(void))
		pdf_dlsym(handle, "PDF_get_api");

    if (get_api == NULL)
    {
	pdf_dlerror(
	    "Error: couldn't find function PDF_get_api in PDFlib DLL\n");
	pdf_dlclose(handle);
	return NULL;
    }

    /* Fetch the API structure and boot the library. */
    PDFlib = (*get_api)();

    /*
     * Check the version number of the loaded DLL against that of
     * the included header file to avoid version mismatch.
     */

    if (PDFlib->sizeof_PDFlib_api != sizeof(PDFlib_api) ||
	PDFlib->major != PDFLIB_MAJORVERSION ||
	PDFlib->minor != PDFLIB_MINORVERSION) {
	sprintf(buf,
	"Error: loaded wrong version of PDFlib DLL\n"
	"Expected version %d.%d (API size %d), loaded %d.%d (API size %d)\n",
	PDFLIB_MAJORVERSION, PDFLIB_MINORVERSION, sizeof(PDFlib_api),
	PDFlib->major, PDFlib->minor, PDFlib->sizeof_PDFlib_api);
	pdf_dlerror(buf);
	pdf_dlclose(handle);
	return NULL;
    }

    /* Boot the library. */
    PDFlib->PDF_boot();

    /*
     * Create a new PDFlib object; use PDF_new2() so that we can store
     * the DLL handle within PDFlib and later retrieve it.
     */
    if ((p = PDFlib->PDF_new2(NULL, NULL, NULL, NULL, handle)) == (PDF *) NULL)
    {
        pdf_dlerror("Couldn't create PDFlib object (out of memory)!\n");
        return NULL;
    }

    /* Make the PDF * available to the client and return */
    *pp = p;
    return PDFlib;
}

/* delete the PDFlib object and unload the previously loaded PDFlib DLL */

PDFLIB_API void PDFLIB_CALL
PDF_delete_dl(PDFlib_api *PDFlib, PDF *p)
{
    void *handle;

    if (!PDFlib || !p)
	return;

    /* fetch the DLL handle (previously stored in PDFlib) */
    handle = PDFlib->PDF_get_opaque(p);

    if (!handle)
	return;

    PDFlib->PDF_delete(p);
    PDFlib->PDF_shutdown();

    pdf_dlclose(handle);
}
