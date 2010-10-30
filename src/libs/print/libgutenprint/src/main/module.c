/*
 * "$Id: module.c,v 1.26 2006/09/28 15:40:05 m0m Exp $"
 *
 *   Gutenprint module loader - load modules with libltdl/libdl.
 *
 *   Copyright 2002 Roger Leigh (rleigh@debian.org)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <unistd.h>


typedef struct stpi_internal_module_class
{
  stp_module_class_t class;
  const char *description;
} stpi_internal_module_class_t;


static void module_list_freefunc(void *item);
static int stp_module_register(stp_module_t *module);
#ifdef USE_DLOPEN
static void *stp_dlsym(void *handle, const char *symbol, const char *modulename);
#endif

static const stpi_internal_module_class_t module_classes[] =
  {
    {STP_MODULE_CLASS_MISC, N_("Miscellaneous (unclassified)")},
    {STP_MODULE_CLASS_FAMILY, N_("Family driver")},
    {STP_MODULE_CLASS_COLOR, N_("Color conversion module")},
    {STP_MODULE_CLASS_DITHER, N_("Dither algorithm")},
    {STP_MODULE_CLASS_INVALID, NULL} /* Must be last */
  };

#if !defined(USE_LTDL) && !defined(USE_DLOPEN)
extern stp_module_t print_canon_LTX_stp_module_data;
extern stp_module_t print_escp2_LTX_stp_module_data;
extern stp_module_t print_lexmark_LTX_stp_module_data;
extern stp_module_t print_pcl_LTX_stp_module_data;
extern stp_module_t print_ps_LTX_stp_module_data;
extern stp_module_t print_dyesub_LTX_stp_module_data;
extern stp_module_t print_raw_LTX_stp_module_data;
extern stp_module_t color_traditional_LTX_stp_module_data;

/*
 * A list of modules, for use when the modules are linked statically.
 */
static stp_module_t *static_modules[] =
  {
    &print_ps_LTX_stp_module_data,
    &print_canon_LTX_stp_module_data,
    &print_escp2_LTX_stp_module_data,
    &print_pcl_LTX_stp_module_data,
    &print_lexmark_LTX_stp_module_data,
    &print_dyesub_LTX_stp_module_data,
    &print_raw_LTX_stp_module_data,
    &color_traditional_LTX_stp_module_data,
    NULL
  };
#endif

static stp_list_t *module_list = NULL;


/*
 * Callback for removing a module from stp_module_list.
 */
static void
module_list_freefunc(void *item /* module to remove */)
{
  stp_module_t *module = (stp_module_t *) item;
  if (module && module->fini) /* Call the module exit function */
    module->fini();
#if defined(USE_LTDL) || defined(USE_DLOPEN)
  DLCLOSE(module->handle); /* Close the module if it's not static */
#endif
}


/*
 * Load all available modules.  Return nonzero on failure.
 */
int stp_module_load(void)
{
  /* initialise libltdl */
#ifdef USE_LTDL
  static int ltdl_is_initialised = 0;        /* Is libltdl initialised? */
#endif
  static int module_list_is_initialised = 0; /* Is the module list initialised? */
#if defined(USE_LTDL) || defined(USE_DLOPEN)
  stp_list_t *dir_list;                      /* List of directories to scan */
  stp_list_t *file_list;                     /* List of modules to open */
  stp_list_item_t *file;                     /* Pointer to current module */
#endif

#ifdef USE_LTDL
  if (!ltdl_is_initialised)
    {
      if (lt_dlinit())
	{
	  stp_erprintf("Error initialising libltdl: %s\n", DLERROR());
	  return 1;
	}
      ltdl_is_initialised = 1;
    }
  /* set default search paths */
  lt_dladdsearchdir(PKGMODULEDIR);
#endif

  /* initialise module_list */
  if (!module_list_is_initialised)
    {
      if (!(module_list = stp_list_create()))
	return 1;
      stp_list_set_freefunc(module_list, module_list_freefunc);
      module_list_is_initialised = 1;
    }

  /* search for available modules */
#if defined (USE_LTDL) || defined (USE_DLOPEN)
  if (!(dir_list = stp_list_create()))
    return 1;
  stp_list_set_freefunc(dir_list, stp_list_node_free_data);
  if (getenv("STP_MODULE_PATH"))
    {
      stp_path_split(dir_list, getenv("STP_MODULE_PATH"));
    }
  else
    {
#ifdef USE_LTDL
      stp_path_split(dir_list, getenv("LTDL_LIBRARY_PATH"));
      stp_path_split(dir_list, lt_dlgetsearchpath());
#else
      stp_path_split(dir_list, PKGMODULEDIR);
#endif
    }
#ifdef USE_LTDL
  file_list = stp_path_search(dir_list, ".la");
#else
  file_list = stp_path_search(dir_list, ".so");
#endif
  stp_list_destroy(dir_list);

  /* load modules */
  file = stp_list_get_start(file_list);
  while (file)
    {
      stp_module_open((const char *) stp_list_item_get_data(file));
      file = stp_list_item_next(file);
    }

  stp_list_destroy(file_list);
#else /* use a static module list */
  {
    int i=0;
    while (static_modules[i])
      {
	stp_module_register(static_modules[i]);
	i++;
      }
  }
#endif
  return 0;
  }


/*
 * Unload all modules and clean up.
 */
int
stp_module_exit(void)
{
  /* destroy the module list (modules unloaded by callback) */
  if (module_list)
    stp_list_destroy(module_list);
  /* shut down libltdl (forces close of any unclosed modules) */
#ifdef USE_LTDL
  return lt_dlexit();
#else
  return 0;
#endif
}


/*
 * Find all modules in a given class.
 */
stp_list_t *
stp_module_get_class(stp_module_class_t class /* Module class */)
{
  stp_list_t *list;                           /* List to return */
  stp_list_item_t *ln;                        /* Module to check*/

  list = stp_list_create(); /* No freefunc, so it can be destroyed
			       without unloading any modules! */
  if (!list)
    return NULL;

  ln = stp_list_get_start(module_list);
  while (ln)
    {
      /* Add modules of the same class to our list */
      if (((stp_module_t *) stp_list_item_get_data(ln))->class == class)
	stp_list_item_create(list, NULL, stp_list_item_get_data(ln));
      ln = stp_list_item_next(ln);
    }
  return list;
}


/*
 * Open a module.
 */
int
stp_module_open(const char *modulename /* Module filename */)
{
#if defined(USE_LTDL) || defined(USE_DLOPEN)
#ifdef USE_LTDL
  lt_dlhandle module;                  /* Handle for module */
#else
  void *module;                        /* Handle for module */
#endif
  stp_module_version_t *version;       /* Module version */
  stp_module_t *data;                  /* Module data */
  stp_list_item_t *reg_module;         /* Pointer to module list nodes */
  int error = 0;                       /* Error status */

  stp_deprintf(STP_DBG_MODULE, "stp-module: open: %s\n", modulename);
  while(1)
    {
      module = DLOPEN(modulename);
      if (!module)
	break;

      /* check version is valid */
      version = (stp_module_version_t *) DLSYM(module, "stp_module_version");
      if (!version)
	break;
      if (version->major != 1 && version->minor < 0)
	break;

      data = (void *) DLSYM(module, "stp_module_data");
      if (!data)
	break;
      data->handle = module; /* store module handle */

      /* check same module isn't already loaded */
      reg_module = stp_list_get_start(module_list);
      while (reg_module)
	{
	  if (!strcmp(data->name, ((stp_module_t *)
				   stp_list_item_get_data(reg_module))->name) &&
	      data->class == ((stp_module_t *)
			      stp_list_item_get_data(reg_module))->class)
	    {
	      stp_deprintf(STP_DBG_MODULE,
			    "stp-module: reject duplicate: %s\n",
			    data->name);
	      error = 1;
	      break;
	    }
	  reg_module = stp_list_item_next(reg_module);
	}
      if (error)
	break;

      /* Register the module */
      if (stp_module_register(data))
	break;

      return 0;
    }

  if (module)
    DLCLOSE(module);
#endif
  return 1;
}


/*
 * Register a loaded module.
 */
static int stp_module_register(stp_module_t *module /* Module to register */)
{
  /* Add to the module list */
  if (stp_list_item_create(module_list, NULL, module))
    return 1;

  stp_deprintf(STP_DBG_MODULE, "stp-module: register: %s\n", module->name);
  return 0;
}


/*
 * Initialise all loaded modules
 */
int stp_module_init(void)
{
  stp_list_item_t *module_item; /* Module list pointer */
  stp_module_t *module;         /* Module to initialise */

  module_item = stp_list_get_start(module_list);
  while (module_item)
    {
      module = (stp_module_t *) stp_list_item_get_data(module_item);
      if (module)
	{
	  stp_deprintf(STP_DBG_MODULE, "stp-module-init: %s\n", module->name);
	  /* Initialise module */
	  if (module->init && module->init())
	    {
	      stp_deprintf(STP_DBG_MODULE,
			   "stp-module-init: %s: Module init failed\n",
			   module->name);
	    }
	}
      module_item = stp_list_item_next(module_item);
    }
  return 0;
}



/*
 * Close a module.
 */
int
stp_module_close(stp_list_item_t *module /* Module to close */)
{
  return stp_list_item_destroy(module_list, module);
}


/*
 * If using dlopen, add modulename_LTX_ to symbol name
 */
#ifdef USE_DLOPEN
static void *stp_dlsym(void *handle,           /* Module */
		       const char *symbol,     /* Symbol name */
		       const char *modulename) /* Module name */
{
  int len;                                     /* Length of string */
  static char *full_symbol = NULL;             /* Symbol to return */
  char *module;                                /* Real module name */
  char *tmp = stp_strdup(modulename);          /* Temporary string */

  module = basename(tmp);

  if (full_symbol)
    {
      stp_free (full_symbol);
      full_symbol = NULL;
    }

  full_symbol = (char *) stp_malloc(sizeof(char) * (strlen(module) - 2));

  /* "_LTX_" + '\0' - ".so" */
  len = strlen(symbol) + strlen(module) + 3;
  full_symbol = (char *) stp_malloc(sizeof(char) * len);

  len = 0;
  strncpy (full_symbol, module, strlen(module) - 3);
  len = strlen(module) - 3;
  strcpy (full_symbol+len, "_LTX_");
  len += 5;
  strcpy (full_symbol+len, symbol);
  len += strlen(symbol);
  full_symbol[len] = '\0';

#if defined(__OpenBSD__)
/* OpenBSD needs a prepended underscore to match symbols */
 {
   char *prefix_symbol = stp_malloc(sizeof(char) * (strlen(full_symbol) + 2));
   prefix_symbol[0] = '_';
   strcpy(prefix_symbol+1, full_symbol);
   stp_free(full_symbol);
   full_symbol = prefix_symbol;
 }
#endif

 /* Change any hyphens to underscores */
 for (len = 0; full_symbol[len] != '\0'; len++)
   if (full_symbol[len] == '-')
     full_symbol[len] = '_';

 stp_deprintf(STP_DBG_MODULE, "SYMBOL: %s\n", full_symbol);

  return dlsym(handle, full_symbol);
}
#endif
