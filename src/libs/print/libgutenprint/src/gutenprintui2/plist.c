/*
 * "$Id: plist.c,v 1.18 2008/07/04 14:29:28 rlk Exp $"
 *
 *   Print plug-in for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
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

#include <gutenprint/gutenprint-intl-internal.h>
#include <gutenprint/gutenprint.h>
#include <gutenprintui2/gutenprintui.h>
#include "gutenprintui-internal.h"

#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

typedef enum
{
  PRINTERS_NONE,
  PRINTERS_LPC,
  PRINTERS_LPSTAT
} printer_system_t;

static int	compare_printers (stpui_plist_t *p1, stpui_plist_t *p2);

int		stpui_plist_current = 0,	/* Current system printer */
		stpui_plist_count = 0;	/* Number of system printers */
stpui_plist_t	*stpui_plist;			/* System printers */
int             stpui_show_all_paper_sizes = 0;
static char *printrc_name = NULL;
static char *image_type;
static gint image_raw_channels = 0;
static gint image_channel_depth = 8;
static stp_string_list_t *default_parameters = NULL;
stp_string_list_t *stpui_system_print_queues;
static const char *copy_count_name = "STPUICopyCount";

#define SAFE_FREE(x)			\
do						\
{						\
  if ((x))					\
    g_free((char *)(x));			\
  ((x)) = NULL;					\
} while (0)

typedef struct
{
  const char *printing_system_name;
  const char *printing_system_text;
  const char *print_command;
  const char *queue_select;
  const char *raw_flag;
  const char *key_file;
  const char *scan_command;
  const char *copy_count_command;
} print_system_t;

/*
 * Generic printing system, based on SysV lp
 *
 * CAUTION: Do not use lpstat -t or lpstat -p.
 * See bug 742187 (huge delays with lpstat -d -p) for an explanation.
 */
static const print_system_t default_printing_system =
  { "SysV", N_("System V lp"), "lp -s", "-d", "-oraw", "/usr/bin/lp",
    "/usr/bin/lpstat -v | awk '/^device for /i {sub(\":\", \"\", $3); print $3}'",
  "-n" };

static print_system_t known_printing_systems[] =
{
  { "CUPS", N_("CUPS"), "lp -s", "-d", "-oraw", "/usr/sbin/cupsd",
    "/usr/bin/lpstat -v | awk '/^device for /i {sub(\":\", \"\", $3); print $3}'",
    "-n" },
  { "SysV", N_("System V lp"), "lp -s", "-d", "-oraw", "/usr/bin/lp",
    "/usr/bin/lpstat -v | awk '/^device for /i {sub(\":\", \"\", $3); print $3}'",
    "-n" },
  { "lpd", N_("Berkeley lpd (/etc/lpc)"), "lpr", "-P", "-l", "/etc/lpc",
    "/etc/lpc status | awk '/^...*:/ {sub(\":.*\", \"\"); print}'",
    "-#" },
  { "lpd", N_("Berkeley lpd (/usr/bsd/lpc)"), "lpr", "-P", "-l", "/usr/bsd/lpc",
    "/usr/bsd/lpc status | awk '/^...*:/ {sub(\":.*\", \"\"); print}'",
    "-#" },
  { "lpd", N_("Berkeley lpd (/usr/etc/lpc"), "lpr", "-P", "-l", "/usr/etc/lpc",
    "/usr/etc/lpc status | awk '/^...*:/ {sub(\":.*\", \"\"); print}'",
    "-#" },
  { "lpd", N_("Berkeley lpd (/usr/libexec/lpc)"), "lpr", "-P", "-l", "/usr/libexec/lpc",
    "/usr/libexec/lpc status | awk '/^...*:/ {sub(\":.*\", \"\"); print}'",
    "-#" },
  { "lpd", N_("Berkeley lpd (/usr/sbin/lpc)"), "lpr", "-P", "-l", "/usr/sbin/lpc",
    "/usr/sbin/lpc status | awk '/^...*:/ {sub(\":.*\", \"\"); print}'",
    "-#" },
};

static unsigned print_system_count = sizeof(known_printing_systems) / sizeof(print_system_t);

static const print_system_t *global_printing_system = NULL;

static void
initialize_default_parameters(void)
{
  default_parameters = stp_string_list_create();
  stp_string_list_add_string(default_parameters, "PrintingSystem", "Autodetect");
  stp_string_list_add_string(default_parameters, "PrintCommand", "");
  stp_string_list_add_string(default_parameters, "QueueSelect", "");
  stp_string_list_add_string(default_parameters, "RawOutputFlag", "");
  stp_string_list_add_string(default_parameters, "ScanOnStartup", "False");
  stp_string_list_add_string(default_parameters, "ScanPrintersCommand", "");
}

void
stpui_set_global_parameter(const char *param, const char *value)
{
  stp_string_list_remove_string(default_parameters, param);
  stp_string_list_add_string(default_parameters, param, value);
}

const char *
stpui_get_global_parameter(const char *param)
{
  stp_param_string_t *ps = stp_string_list_find(default_parameters, param);
  if (ps)
    return ps->text;
  else
    return NULL;
}

static const print_system_t *
identify_print_system(void)
{
  int i;
  if (!global_printing_system)
    {
      for (i = 0; i < print_system_count; i++)
	{
	  if (!access(known_printing_systems[i].key_file, R_OK))
	    {
	      global_printing_system = &(known_printing_systems[i]);
	      break;
	    }
	}
      if (!global_printing_system)
	global_printing_system = &default_printing_system;
    }
  return global_printing_system;
}

char *
stpui_build_standard_print_command(const stpui_plist_t *plist,
				   const stp_printer_t *printer)
{
  const char *queue_name = stpui_plist_get_queue_name(plist);
  const char *extra_options = stpui_plist_get_extra_printer_options(plist);
  const char *family = stp_printer_get_family(printer);
  int copy_count = stpui_plist_get_copy_count(plist);
  int raw = 0;
  char *print_cmd;
  char *count_string = NULL;
  char *quoted_queue_name = NULL;
  if (!queue_name)
    queue_name = "";
  identify_print_system();
  if (strcmp(family, "ps") == 0)
    raw = 0;
  else
    raw = 1;
  
  if (copy_count > 1)
    stp_asprintf(&count_string, "%s %d ",
		 global_printing_system->copy_count_command, copy_count);

  if (queue_name[0])
    quoted_queue_name = g_shell_quote(queue_name);

  stp_asprintf(&print_cmd, "%s %s %s %s %s%s%s",
	       global_printing_system->print_command,
	       queue_name[0] ? global_printing_system->queue_select : "",
	       queue_name[0] ? quoted_queue_name : "",
	       count_string ? count_string : "",
	       raw ? global_printing_system->raw_flag : "",
	       extra_options ? " " : "",
	       extra_options ? extra_options : "");
  SAFE_FREE(count_string);
  SAFE_FREE(quoted_queue_name);
  return print_cmd;
}

static void
append_external_options(char **command, const stp_vars_t *v)
{
  stp_string_list_t *external_options;
  if (! command || ! *command)
    return;
  external_options = stp_get_external_options(v);
  if (external_options)
    {
      int count = stp_string_list_count(external_options);
      int i;
      for (i = 0; i < count; i++)
	{
	  stp_param_string_t *param = stp_string_list_param(external_options, i);
	  char *quoted_name=g_shell_quote(param->name);
	  char *quoted_value=g_shell_quote(param->text);
	  stp_catprintf(command, " -o%s=%s", quoted_name, quoted_value);
	  SAFE_FREE(quoted_name);
	  SAFE_FREE(quoted_value);
	}
      stp_string_list_destroy(external_options);
    }
}


void
stpui_set_printrc_file(const char *name)
{
  if (name && name == printrc_name)
    return;
  SAFE_FREE(printrc_name);
  if (name)
    printrc_name = g_strdup(name);
  else
    printrc_name = g_build_filename(g_get_home_dir(), ".gutenprintrc", NULL);
}

const char *
stpui_get_printrc_file(void)
{
  if (!printrc_name)
    stpui_set_printrc_file(NULL);
  return printrc_name;
}

#define PLIST_ACCESSORS(name)						\
void									\
stpui_plist_set_##name(stpui_plist_t *p, const char *val)		\
{									\
  if (p->name == val)							\
    return;								\
  SAFE_FREE(p->name);							\
  p->name = g_strdup(val);						\
}									\
									\
void									\
stpui_plist_set_##name##_n(stpui_plist_t *p, const char *val, int n)	\
{									\
  if (p->name == val)							\
    return;								\
  SAFE_FREE(p->name);							\
  p->name = g_strndup(val, n);						\
}									\
									\
const char *								\
stpui_plist_get_##name(const stpui_plist_t *p)				\
{									\
  return p->name;							\
}

PLIST_ACCESSORS(output_filename)
PLIST_ACCESSORS(name)
PLIST_ACCESSORS(queue_name)
PLIST_ACCESSORS(extra_printer_options)
PLIST_ACCESSORS(custom_command)
PLIST_ACCESSORS(current_standard_command)

void
stpui_plist_set_command_type(stpui_plist_t *p, command_t val)
{
  switch (val)
    {
    case COMMAND_TYPE_DEFAULT:
    case COMMAND_TYPE_CUSTOM:
    case COMMAND_TYPE_FILE:
      p->command_type = val;
      break;
    default:
      p->command_type = COMMAND_TYPE_DEFAULT;
    }
}

command_t
stpui_plist_get_command_type(const stpui_plist_t *p)
{
  return p->command_type;
}

void
stpui_plist_set_copy_count(stpui_plist_t *p, gint copy_count)
{
  if (copy_count > 0)
    stp_set_int_parameter(p->v, copy_count_name, copy_count);
}

gint
stpui_plist_get_copy_count(const stpui_plist_t *p)
{
  if (stp_check_int_parameter(p->v, copy_count_name, STP_PARAMETER_ACTIVE))
    return stp_get_int_parameter(p->v, copy_count_name);
  else
    return 1;
}

void
stpui_set_image_type(const char *itype)
{
  image_type = g_strdup(itype);
}

void
stpui_set_image_raw_channels(gint channels)
{
  image_raw_channels = channels;
}

void
stpui_set_image_channel_depth(gint depth)
{
  image_channel_depth = depth;
}

static void
writefunc(void *file, const char *buf, size_t bytes)
{
  FILE *prn = (FILE *)file;
  fwrite(buf, 1, bytes, prn);
}

static void
stpui_errfunc(void *file, const char *buf, size_t bytes)
{
  g_message("%s",buf);
}

void
stpui_printer_initialize(stpui_plist_t *printer)
{
  char tmp[32];
  stpui_plist_set_name(printer, "");
  stpui_plist_set_output_filename(printer, "");
  stpui_plist_set_queue_name(printer, "");
  stpui_plist_set_extra_printer_options(printer, "");
  stpui_plist_set_custom_command(printer, "");
  stpui_plist_set_current_standard_command(printer, "");
  printer->command_type = COMMAND_TYPE_DEFAULT;
  printer->scaling = 100.0;
  printer->orientation = ORIENT_AUTO;
  printer->auto_size_roll_feed_paper = 0;
  printer->unit = 0;
  printer->v = stp_vars_create();
  stp_set_errfunc(printer->v, writefunc);
  stp_set_errdata(printer->v, stderr);
  stpui_plist_set_copy_count(printer, 1);
  stp_set_string_parameter(printer->v, "InputImageType", image_type);
  if (image_raw_channels)
    {
      (void) sprintf(tmp, "%d", image_raw_channels);
      stp_set_string_parameter(printer->v, "RawChannels", tmp);
    }
  if (image_channel_depth)
    {
      (void) sprintf(tmp, "%d", image_channel_depth);
      stp_set_string_parameter(printer->v, "ChannelBitDepth", tmp);
    }
  printer->invalid_mask = INVALID_TOP | INVALID_LEFT;
}

static void
stpui_plist_destroy(stpui_plist_t *printer)
{
  SAFE_FREE(printer->name);
  SAFE_FREE(printer->queue_name);
  SAFE_FREE(printer->extra_printer_options);
  SAFE_FREE(printer->custom_command);
  SAFE_FREE(printer->current_standard_command);
  SAFE_FREE(printer->output_filename);
  stp_vars_destroy(printer->v);
}

void
stpui_plist_copy(stpui_plist_t *vd, const stpui_plist_t *vs)
{
  if (vs == vd)
    return;
  stp_vars_copy(vd->v, vs->v);
  vd->scaling = vs->scaling;
  vd->orientation = vs->orientation;
  vd->auto_size_roll_feed_paper = vs->auto_size_roll_feed_paper;
  vd->unit = vs->unit;
  vd->invalid_mask = vs->invalid_mask;
  vd->command_type = vs->command_type;
  stpui_plist_set_name(vd, stpui_plist_get_name(vs));
  stpui_plist_set_queue_name(vd, stpui_plist_get_queue_name(vs));
  stpui_plist_set_extra_printer_options(vd, stpui_plist_get_extra_printer_options(vs));
  stpui_plist_set_custom_command(vd, stpui_plist_get_custom_command(vs));
  stpui_plist_set_current_standard_command(vd, stpui_plist_get_current_standard_command(vs));
  stpui_plist_set_output_filename(vd, stpui_plist_get_output_filename(vs));
  stpui_plist_set_copy_count(vd, stpui_plist_get_copy_count(vs));
}

static stpui_plist_t *
allocate_stpui_plist_copy(const stpui_plist_t *vs)
{
  stpui_plist_t *rep = g_malloc(sizeof(stpui_plist_t));
  memset(rep, 0, sizeof(stpui_plist_t));
  rep->v = stp_vars_create();
  stpui_plist_copy(rep, vs);
  return rep;
}

static void
check_plist(int count)
{
  static int current_plist_size = 0;
  int i;
  if (count <= current_plist_size)
    return;
  else if (current_plist_size == 0)
    {
      current_plist_size = count;
      stpui_plist = g_malloc(current_plist_size * sizeof(stpui_plist_t));
      for (i = 0; i < current_plist_size; i++)
	{
	  memset(&(stpui_plist[i]), 0, sizeof(stpui_plist_t));
	  stpui_printer_initialize(&(stpui_plist[i]));
	}
    }
  else
    {
      int old_plist_size = current_plist_size;
      current_plist_size *= 2;
      if (current_plist_size < count)
	current_plist_size = count;
      stpui_plist = g_realloc(stpui_plist, current_plist_size * sizeof(stpui_plist_t));
      for (i = old_plist_size; i < current_plist_size; i++)
	{
	  memset(&(stpui_plist[i]), 0, sizeof(stpui_plist_t));
	  stpui_printer_initialize(&(stpui_plist[i]));
	}
    }
}

#define GET_MANDATORY_INTERNAL_STRING_PARAM(param)		\
do {								\
  if ((commaptr = strchr(lineptr, ',')) == NULL)		\
    continue;							\
  stpui_plist_set_##param##_n(&key, lineptr, commaptr - line);	\
  lineptr = commaptr + 1;					\
} while (0)

#define GET_MANDATORY_STRING_PARAM(param)		\
do {							\
  if ((commaptr = strchr(lineptr, ',')) == NULL)	\
    continue;						\
  stp_set_##param##_n(key.v, lineptr, commaptr - line);	\
  lineptr = commaptr + 1;				\
} while (0)

static int
get_mandatory_string_param(stp_vars_t *v, const char *param, char **lineptr)
{
  char *commaptr = strchr(*lineptr, ',');
  if (commaptr == NULL)
    return 0;
  stp_set_string_parameter_n(v, param, *lineptr, commaptr - *lineptr);
  *lineptr = commaptr + 1;
  return 1;
}

static int
get_mandatory_file_param(stp_vars_t *v, const char *param, char **lineptr)
{
  char *commaptr = strchr(*lineptr, ',');
  if (commaptr == NULL)
    return 0;
  stp_set_file_parameter_n(v, param, *lineptr, commaptr - *lineptr);
  *lineptr = commaptr + 1;
  return 1;
}

#define GET_MANDATORY_INT_PARAM(param)			\
do {							\
  if ((commaptr = strchr(lineptr, ',')) == NULL)	\
    continue;						\
  stp_set_##param(key.v, atoi(lineptr));		\
  lineptr = commaptr + 1;				\
} while (0)

#define GET_MANDATORY_INTERNAL_INT_PARAM(param)		\
do {							\
  if ((commaptr = strchr(lineptr, ',')) == NULL)	\
    continue;						\
  key.param = atoi(lineptr);				\
  lineptr = commaptr + 1;				\
} while (0)

static void
get_optional_string_param(stp_vars_t *v, const char *param,
			  char **lineptr, int *keepgoing)
{
  if (*keepgoing)
    {
      char *commaptr = strchr(*lineptr, ',');
      if (commaptr == NULL)
	{
	  stp_set_string_parameter(v, param, *lineptr);
	  *keepgoing = 0;
	}
      else
	{
	  stp_set_string_parameter_n(v, param, *lineptr, commaptr - *lineptr);
	  *lineptr = commaptr + 1;
	}
    }
}

#define GET_OPTIONAL_INT_PARAM(param)					\
do {									\
  if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))	\
    {									\
      keepgoing = 0;							\
    }									\
  else									\
    {									\
      stp_set_##param(key.v, atoi(lineptr));				\
      lineptr = commaptr + 1;						\
    }									\
} while (0)

#define GET_OPTIONAL_INTERNAL_INT_PARAM(param)				\
do {									\
  if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))	\
    {									\
      keepgoing = 0;							\
    }									\
  else									\
    {									\
      key.param = atoi(lineptr);					\
      lineptr = commaptr + 1;						\
    }									\
} while (0)

#define IGNORE_OPTIONAL_PARAM(param)					\
do {									\
  if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))	\
    {									\
      keepgoing = 0;							\
    }									\
  else									\
    {									\
      lineptr = commaptr + 1;						\
    }									\
} while (0)

static void
get_optional_float_param(stp_vars_t *v, const char *param,
			 char **lineptr, int *keepgoing)
{
  if (*keepgoing)
    {
      char *commaptr = strchr(*lineptr, ',');
      if (commaptr == NULL)
	{
	  stp_set_float_parameter(v, param, atof(*lineptr));
	  *keepgoing = 0;
	}
      else
	{
	  stp_set_float_parameter(v, param, atof(*lineptr));
	  *lineptr = commaptr + 1;
	}
    }
}

#define GET_OPTIONAL_INTERNAL_FLOAT_PARAM(param)			\
do {									\
  if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))	\
    {									\
      keepgoing = 0;							\
    }									\
  else									\
    {									\
      key.param = atof(lineptr);					\
    }									\
} while (0)

static void *
psearch(const void *key, void *base, size_t nmemb, size_t size,
	int (*compar)(const void *, const void *))
{
  int i;
  char *cbase = (char *) base;
  for (i = 0; i < nmemb; i++)
    {
      if ((*compar)(key, (const void *) cbase) == 0)
	return (void *) cbase;
      cbase += size;
    }
  return NULL;
}

stpui_plist_t *
stpui_plist_create(const char *name, const char *driver)
{
  stpui_plist_t key;
  stpui_plist_t *answer = NULL;
  memset(&key, 0, sizeof(key));
  stpui_printer_initialize(&key);
  key.invalid_mask = 0;
  stpui_plist_set_name(&key, name);
  stp_set_driver(key.v, driver);
  if (stpui_plist_add(&key, 0))
    answer = psearch(&key, stpui_plist, stpui_plist_count,
		     sizeof(stpui_plist_t),
		     (int (*)(const void *, const void *)) compare_printers);
  SAFE_FREE(key.name);
  SAFE_FREE(key.queue_name);
  SAFE_FREE(key.extra_printer_options);
  SAFE_FREE(key.custom_command);
  SAFE_FREE(key.current_standard_command);
  SAFE_FREE(key.output_filename);
  stp_vars_destroy(key.v);
  return answer;
}

int
stpui_plist_add(const stpui_plist_t *key, int add_only)
{
  /*
   * The format of the list is the File printer followed by a qsort'ed list
   * of system printers. So, if we want to update the file printer, it is
   * always first in the list, else call psearch.
   */
  stpui_plist_t *p;
  if (!stp_get_printer(key->v))
    stp_set_driver(key->v, "ps2");
  if (stp_get_printer(key->v))
    {
      p = psearch(key, stpui_plist, stpui_plist_count,
		  sizeof(stpui_plist_t),
		  (int (*)(const void *, const void *)) compare_printers);
      if (p == NULL)
	{
#ifdef DEBUG
	  fprintf(stderr, "Adding new printer from printrc file: %s\n",
		  key->name);
#endif
	  check_plist(stpui_plist_count + 1);
	  p = stpui_plist + stpui_plist_count;
	  stpui_plist_count++;
	  stpui_plist_copy(p, key);
	  if (strlen(stpui_plist_get_queue_name(p)) == 0 &&
	      stp_string_list_is_present(stpui_system_print_queues,
					 stpui_plist_get_name(p)))
	    stpui_plist_set_queue_name(p, stpui_plist_get_name(p));
	}
      else
	{
	  if (add_only)
	    return 0;
#ifdef DEBUG
	  fprintf(stderr, "Updating printer %s.\n", key->name);
#endif
	  stpui_plist_copy(p, key);
	}
      return 1;
    }
  else
    {
      fprintf(stderr, "No printer found!\n");
      return 0;
    }
}

static void
stpui_printrc_load_v0(FILE *fp)
{
  char		line[1024];	/* Line in printrc file */
  char		*lineptr;	/* Pointer in line */
  char		*commaptr;	/* Pointer to next comma */
  stpui_plist_t	key;		/* Search key */
  int keepgoing = 1;
  (void) memset(line, 0, 1024);
  (void) memset(&key, 0, sizeof(stpui_plist_t));
  stpui_printer_initialize(&key);
  key.name = g_strdup(_("File"));
  while (fgets(line, sizeof(line), fp) != NULL)
    {
      /*
       * Read old format printrc lines...
       */

      stpui_printer_initialize(&key);
      key.invalid_mask = 0;
      lineptr = line;

      /*
       * Read the command-delimited printer definition data.  Note that
       * we can't use sscanf because %[^,] fails if the string is empty...
       */

      GET_MANDATORY_INTERNAL_STRING_PARAM(name);
      GET_MANDATORY_INTERNAL_STRING_PARAM(custom_command);
      GET_MANDATORY_STRING_PARAM(driver);

      if (! stp_get_printer(key.v))
	continue;

      if (!get_mandatory_file_param(key.v, "PPDFile", &lineptr))
	continue;
      if ((commaptr = strchr(lineptr, ',')) != NULL)
	{
	  switch (atoi(lineptr))
	    {
	    case 1:
	      stp_set_string_parameter(key.v, "PrintingMode", "Color");
	      break;
	    case 0:
	    default:
	      stp_set_string_parameter(key.v, "PrintingMode", "BW");
	      break;
	    }
	}
      else
	continue;
      
      if (!get_mandatory_string_param(key.v, "Resolution", &lineptr))
	continue;
      if (!get_mandatory_string_param(key.v, "PageSize", &lineptr))
	continue;
      if (!get_mandatory_string_param(key.v, "MediaType", &lineptr))
	continue;

      get_optional_string_param(key.v, "InputSlot", &lineptr, &keepgoing);
      get_optional_float_param(key.v, "Brightness", &lineptr, &keepgoing);

      GET_OPTIONAL_INTERNAL_FLOAT_PARAM(scaling);
      GET_OPTIONAL_INTERNAL_INT_PARAM(orientation);
      GET_OPTIONAL_INT_PARAM(left);
      GET_OPTIONAL_INT_PARAM(top);
      get_optional_float_param(key.v, "Gamma", &lineptr, &keepgoing);
      get_optional_float_param(key.v, "Contrast", &lineptr, &keepgoing);
      get_optional_float_param(key.v, "Cyan", &lineptr, &keepgoing);
      get_optional_float_param(key.v, "Magenta", &lineptr, &keepgoing);
      get_optional_float_param(key.v, "Yellow", &lineptr, &keepgoing);
      IGNORE_OPTIONAL_PARAM(linear);
      IGNORE_OPTIONAL_PARAM(image_type);
      get_optional_float_param(key.v, "Saturation", &lineptr, &keepgoing);
      get_optional_float_param(key.v, "Density", &lineptr, &keepgoing);
      get_optional_string_param(key.v, "InkType", &lineptr, &keepgoing);
      get_optional_string_param(key.v,"DitherAlgorithm",&lineptr,&keepgoing);
      GET_OPTIONAL_INTERNAL_INT_PARAM(unit);
      stpui_plist_add(&key, 0);
      g_free(key.name);
      stp_vars_destroy(key.v);
    }
  stpui_plist_current = 0;
}

static void
stpui_printrc_load_v1(FILE *fp)
{
  char		line[1024];	/* Line in printrc file */
  stpui_plist_t	key;		/* Search key */
  char *	current_printer = 0; /* printer to select */
  (void) memset(line, 0, 1024);
  (void) memset(&key, 0, sizeof(stpui_plist_t));
  stpui_printer_initialize(&key);
  key.name = g_strdup(_("File"));
  while (fgets(line, sizeof(line), fp) != NULL)
    {
      /*
       * Read new format printrc lines...
       */

      char *keyword, *end, *value;

      keyword = line;
      for (keyword = line; g_ascii_isspace(*keyword); keyword++)
	{
	  /* skip initial spaces... */
	}
      if (!g_ascii_isalpha(*keyword))
	continue;
      for (end = keyword; g_ascii_isalnum(*end) || *end == '-'; end++)
	{
	  /* find end of keyword... */
	}
      value = end;
      while (g_ascii_isspace(*value))
	{
	  /* skip over white space... */
	  value++;
	}
      if (*value != ':')
	continue;
      value++;
      *end = '\0';
      while (g_ascii_isspace(*value))
	{
	  /* skip over white space... */
	  value++;
	}
      for (end = value; *end && *end != '\n'; end++)
	{
	  /* find end of line... */
	}
      *end = '\0';
#ifdef DEBUG
      fprintf(stderr, "Keyword = `%s', value = `%s'\n", keyword, value);
#endif
      if (strcasecmp("current-printer", keyword) == 0)
	{
	  SAFE_FREE(current_printer);
	  current_printer = g_strdup(value);
	}
      else if (strcasecmp("printer", keyword) == 0)
	{
	  /* Switch to printer named VALUE */
	  stpui_plist_add(&key, 0);
#ifdef DEBUG
	  fprintf(stderr,
		  "output_to is now %s\n", stpui_plist_get_output_to(&key));
#endif

	  stp_vars_destroy(key.v);
	  stpui_printer_initialize(&key);
	  key.invalid_mask = 0;
	  stpui_plist_set_name(&key, value);
	}
      else if (strcasecmp("destination", keyword) == 0)
	stpui_plist_set_custom_command(&key, value);
      else if (strcasecmp("driver", keyword) == 0)
	stp_set_driver(key.v, value);
      else if (strcasecmp("ppd-file", keyword) == 0)
	stp_set_file_parameter(key.v, "PPDFile", value);
      else if (strcasecmp("output-type", keyword) == 0)
	{
	  switch (atoi(value))
	    {
	    case 1:
	      stp_set_string_parameter(key.v, "PrintingMode", "Color");
	      break;
	    case 0:
	    default:
	      stp_set_string_parameter(key.v, "PrintingMode", "BW");
	      break;
	    }
	}
      else if (strcasecmp("media-size", keyword) == 0)
	stp_set_string_parameter(key.v, "PageSize", value);
      else if (strcasecmp("media-type", keyword) == 0)
	stp_set_string_parameter(key.v, "MediaType", value);
      else if (strcasecmp("media-source", keyword) == 0)
	stp_set_float_parameter(key.v, "Brightness", atof(value));
      else if (strcasecmp("scaling", keyword) == 0)
	key.scaling = atof(value);
      else if (strcasecmp("orientation", keyword) == 0)
	key.orientation = atoi(value);
      else if (strcasecmp("left", keyword) == 0)
	stp_set_left(key.v, atoi(value));
      else if (strcasecmp("top", keyword) == 0)
	stp_set_top(key.v, atoi(value));
      else if (strcasecmp("linear", keyword) == 0)
	/* Ignore linear */
	;
      else if (strcasecmp("image-type", keyword) == 0)
	/* Ignore image type */
	;
      else if (strcasecmp("unit", keyword) == 0)
	key.unit = atoi(value);
      else if (strcasecmp("custom-page-width", keyword) == 0)
	stp_set_page_width(key.v, atoi(value));
      else if (strcasecmp("custom-page-height", keyword) == 0)
	stp_set_page_height(key.v, atoi(value));
      /* Special case Ink-Type and Dither-Algorithm */
      else if (strcasecmp("ink-type", keyword) == 0)
	stp_set_string_parameter(key.v, "InkType", value);
      else if (strcasecmp("dither-algorithm", keyword) == 0)
	stp_set_string_parameter(key.v, "DitherAlgorithm", value);
      else
	{
	  stp_parameter_t desc;
	  stp_curve_t *curve;
	  stp_describe_parameter(key.v, keyword, &desc);
	  switch (desc.p_type)
	    {
	    case STP_PARAMETER_TYPE_STRING_LIST:
	      stp_set_string_parameter(key.v, keyword, value);
	      break;
	    case STP_PARAMETER_TYPE_FILE:
	      stp_set_file_parameter(key.v, keyword, value);
	      break;
	    case STP_PARAMETER_TYPE_DOUBLE:
	      stp_set_float_parameter(key.v, keyword, atof(value));
	      break;
	    case STP_PARAMETER_TYPE_INT:
	      stp_set_int_parameter(key.v, keyword, atoi(value));
	      break;
	    case STP_PARAMETER_TYPE_BOOLEAN:
	      stp_set_boolean_parameter(key.v, keyword, atoi(value));
	      break;
	    case STP_PARAMETER_TYPE_CURVE:
	      curve = stp_curve_create_from_string(value);
	      if (curve)
		{
		  stp_set_curve_parameter(key.v, keyword, curve);
		  stp_curve_destroy(curve);
		}
	      break;
	    default:
	      if (strlen(value))
		{
		  char buf[1024];
		  snprintf(buf, sizeof(buf),
			   "Unrecognized keyword `%s' in printrc; value `%s' (%d)\n",
			   keyword, value, desc.p_type);
		}
	    }
	  stp_parameter_description_destroy(&desc);
	}
    }
  if (strlen(key.name) > 0)
    {
      stpui_plist_add(&key, 0);
      stp_vars_destroy(key.v);
      g_free(key.name);
    }
  if (current_printer)
    {
      int i;
      for (i = 0; i < stpui_plist_count; i ++)
	if (strcmp(current_printer, stpui_plist[i].name) == 0)
	  stpui_plist_current = i;
    }
}  

char *stpui_printrc_current_printer = NULL;
extern FILE *yyin;
extern int yyparse(void);

static void
stpui_printrc_load_v2(FILE *fp)
{
  int retval;
  char *locale;
  yyin = fp;

  stpui_printrc_current_printer = NULL;
#ifdef HAVE_LOCALE_H
  locale = g_strdup(setlocale(LC_NUMERIC, NULL));
  setlocale(LC_NUMERIC, "C");
#endif
  retval = yyparse();
#ifdef HAVE_LOCALE_H
  setlocale(LC_NUMERIC, locale);
  SAFE_FREE(locale);
#endif
  if (stpui_printrc_current_printer)
    {
      int i;
      for (i = 0; i < stpui_plist_count; i ++)
	{
	  if (strcmp(stpui_printrc_current_printer, stpui_plist[i].name) == 0)
	    stpui_plist_current = i;
	  if (!stp_check_boolean_parameter(stpui_plist[i].v,
					   "PageSizeExtended",
					   STP_PARAMETER_ACTIVE))
	    stp_set_boolean_parameter(stpui_plist[i].v, "PageSizeExtended", 0);
	}
      SAFE_FREE(stpui_printrc_current_printer);
    }
}

/*
 * 'stpui_printrc_load()' - Load the printer resource configuration file.
 */
void
stpui_printrc_load(void)
{
  FILE		*fp;		/* Printrc file */
  char		line[1024];	/* Line in printrc file */
  int		format = 0;	/* rc file format version */
  const char *filename = stpui_get_printrc_file();

  initialize_default_parameters();
  check_plist(1);

 /*
  * Get the printer list...
  */

  stpui_get_system_printers();

  if ((fp = fopen(filename, "r")) != NULL)
    {
      (void) memset(line, 0, 1024);
      if (fgets(line, sizeof(line), fp) != NULL)
	{
#ifdef HAVE_LOCALE_H
	  char *locale = g_strdup(setlocale(LC_NUMERIC, NULL));
	  setlocale(LC_NUMERIC, "C");
#endif
	  if (strncmp("#PRINTRCv", line, 9) == 0)
	    {
	      /* Force locale to "C", so that numbers scan correctly */
#ifdef DEBUG
	      fprintf(stderr, "Found printrc version tag: `%s'\n", line);
	      fprintf(stderr, "Version number: `%s'\n", &(line[9]));
#endif
	      (void) sscanf(&(line[9]), "%d", &format);
	    }
#ifdef HAVE_LOCALE_H
	  setlocale(LC_NUMERIC, locale);
	  SAFE_FREE(locale);
#endif
	}
      rewind(fp);
      switch (format)
	{
	case 0:
	  stpui_printrc_load_v0(fp);
	  break;
	case 1:
	  stpui_printrc_load_v1(fp);
	  break;
	case 2:
	case 3:
	case 4:
	  stpui_printrc_load_v2(fp);
	  break;
	}
      (void) fclose(fp);
    }
  if (stpui_plist_count == 0)
    stpui_plist_create(_("Printer"), "ps2");
}

/*
 * 'stpui_printrc_save()' - Save the current printer resource configuration.
 */
void
stpui_printrc_save(void)
{
  FILE		*fp;		/* Printrc file */
  int		i;		/* Looping var */
  size_t global_settings_count = stp_string_list_count(default_parameters);
  stpui_plist_t	*p;		/* Current printer */
  const char *filename = stpui_get_printrc_file();


  if ((fp = fopen(filename, "w")) != NULL)
    {
      /*
       * Write the contents of the printer list...
       */

      /* Force locale to "C", so that numbers print correctly */
#ifdef HAVE_LOCALE_H
      char *locale = g_strdup(setlocale(LC_NUMERIC, NULL));
      setlocale(LC_NUMERIC, "C");
#endif
#ifdef DEBUG
      fprintf(stderr, "Number of printers: %d\n", stpui_plist_count);
#endif

      fputs("#PRINTRCv4 written by Gutenprint " PLUG_IN_VERSION "\n\n", fp);

      fprintf(fp, "Global-Settings:\n");
      fprintf(fp, "  Current-Printer: \"%s\"\n",
	      stpui_plist[stpui_plist_current].name);
      fprintf(fp, "  Show-All-Paper-Sizes: %s\n",
	      stpui_show_all_paper_sizes ? "True" : "False");
      for (i = 0; i < global_settings_count; i++)
	{
	  stp_param_string_t *ps = stp_string_list_param(default_parameters, i);
	  fprintf(fp, "  %s \"%s\"\n", ps->name, ps->text);
	}
      fprintf(fp, "End-Global-Settings:\n");

      for (i = 0, p = stpui_plist; i < stpui_plist_count; i ++, p ++)
	{
	  int count;
	  int j;
	  stp_parameter_list_t *params = stp_get_parameter_list(p->v);
	  count = stp_parameter_list_count(params);
	  fprintf(fp, "\nPrinter: \"%s\" \"%s\"\n",
		  p->name, stp_get_driver(p->v));
	  fprintf(fp, "  Command-Type: %d\n", p->command_type);
	  fprintf(fp, "  Queue-Name: \"%s\"\n", p->queue_name);
	  fprintf(fp, "  Output-Filename: \"%s\"\n", p->output_filename);
	  fprintf(fp, "  Extra-Printer-Options: \"%s\"\n", p->extra_printer_options);
	  fprintf(fp, "  Custom-Command: \"%s\"\n", p->custom_command);
	  fprintf(fp, "  Scaling: %.3f\n", p->scaling);
	  fprintf(fp, "  Orientation: %d\n", p->orientation);
	  fprintf(fp, "  Autosize-Roll-Paper: %d\n", p->auto_size_roll_feed_paper);
	  fprintf(fp, "  Unit: %d\n", p->unit);

	  fprintf(fp, "  Left: %d\n", stp_get_left(p->v));
	  fprintf(fp, "  Top: %d\n", stp_get_top(p->v));
	  fprintf(fp, "  Custom_Page_Width: %d\n", stp_get_page_width(p->v));
	  fprintf(fp, "  Custom_Page_Height: %d\n", stp_get_page_height(p->v));
	  fprintf(fp, "  Parameter %s Int True %d\n", copy_count_name,
		  stpui_plist_get_copy_count(p));

	  for (j = 0; j < count; j++)
	    {
	      const stp_parameter_t *param = stp_parameter_list_param(params, j);
	      if (strcmp(param->name, "AppGamma") == 0)
		continue;
	      switch (param->p_type)
		{
		case STP_PARAMETER_TYPE_STRING_LIST:
		  if (stp_check_string_parameter(p->v, param->name,
						 STP_PARAMETER_INACTIVE))
		    fprintf(fp, "  Parameter %s String %s \"%s\"\n",
			    param->name,
			    ((stp_get_string_parameter_active
			      (p->v, param->name) == STP_PARAMETER_ACTIVE) ?
			     "True" : "False"),
			    stp_get_string_parameter(p->v, param->name));
		  break;
		case STP_PARAMETER_TYPE_FILE:
		  if (stp_check_file_parameter(p->v, param->name,
						 STP_PARAMETER_INACTIVE))
		    fprintf(fp, "  Parameter %s File %s \"%s\"\n", param->name,
			    ((stp_get_file_parameter_active
			      (p->v, param->name) == STP_PARAMETER_ACTIVE) ?
			     "True" : "False"),
			    stp_get_file_parameter(p->v, param->name));
		  break;
		case STP_PARAMETER_TYPE_DOUBLE:
		  if (stp_check_float_parameter(p->v, param->name,
						 STP_PARAMETER_INACTIVE))
		    fprintf(fp, "  Parameter %s Double %s %f\n", param->name,
			    ((stp_get_float_parameter_active
			      (p->v, param->name) == STP_PARAMETER_ACTIVE) ?
			     "True" : "False"),
			    stp_get_float_parameter(p->v, param->name));
		  break;
		case STP_PARAMETER_TYPE_DIMENSION:
		  if (stp_check_dimension_parameter(p->v, param->name,
						    STP_PARAMETER_INACTIVE))
		    fprintf(fp, "  Parameter %s Dimension %s %d\n", param->name,
			    ((stp_get_dimension_parameter_active
			      (p->v, param->name) == STP_PARAMETER_ACTIVE) ?
			     "True" : "False"),
			    stp_get_dimension_parameter(p->v, param->name));
		  break;
		case STP_PARAMETER_TYPE_INT:
		  if (stp_check_int_parameter(p->v, param->name,
						 STP_PARAMETER_INACTIVE))
		    fprintf(fp, "  Parameter %s Int %s %d\n", param->name,
			    ((stp_get_int_parameter_active
			      (p->v, param->name) == STP_PARAMETER_ACTIVE) ?
			     "True" : "False"),
			    stp_get_int_parameter(p->v, param->name));
		  break;
		case STP_PARAMETER_TYPE_BOOLEAN:
		  if (stp_check_boolean_parameter(p->v, param->name,
						 STP_PARAMETER_INACTIVE))
		    fprintf(fp, "  Parameter %s Boolean %s %s\n", param->name,
			    ((stp_get_boolean_parameter_active
			      (p->v, param->name) == STP_PARAMETER_ACTIVE) ?
			     "True" : "False"),
			    (stp_get_boolean_parameter(p->v, param->name) ?
			     "True" : "False"));
		  break;
		case STP_PARAMETER_TYPE_CURVE:
		  if (stp_check_curve_parameter(p->v, param->name,
						 STP_PARAMETER_INACTIVE))
		    {
		      const stp_curve_t *curve =
			stp_get_curve_parameter(p->v, param->name);
		      if (curve)
			{
			  fprintf(fp, "  Parameter %s Curve %s '",
				  param->name,
				  ((stp_get_curve_parameter_active
				    (p->v, param->name) ==
				    STP_PARAMETER_ACTIVE) ?
				   "True" : "False"));
			  stp_curve_write(fp, curve);
			  fprintf(fp, "'\n");
			}
		    }
		  break;
		default:
		  break;
		}
	    }
	  stp_parameter_list_destroy(params);
#ifdef DEBUG
	  fprintf(stderr, "Wrote printer %d: %s\n", i, p->name);
#endif
	}
#ifdef HAVE_LOCALE_H
      setlocale(LC_NUMERIC, locale);
      SAFE_FREE(locale);
#endif
      fclose(fp);
    }
  else
    fprintf(stderr, "could not open printrc file \"%s\"\n",filename);
}

/*
 * 'compare_printers()' - Compare system printer names for qsort().
 */

static int
compare_printers(stpui_plist_t *p1, stpui_plist_t *p2)
{
  return (strcmp(p1->name, p2->name));
}

/*
 * 'stpui_get_system_printers()' - Get a complete list of printers from the spooler.
 */

void
stpui_get_system_printers(void)
{
  FILE *pfile;			/* Pipe to status command */
  char  line[1025];		/* Line from status command */

  stpui_system_print_queues = stp_string_list_create();
  stp_string_list_add_string(stpui_system_print_queues, "",
			     _("(Default Printer)"));

 /*
  * Run the command, if any, to get the available printers...
  */

  identify_print_system();
  if (global_printing_system)
  {
    const char *old_locale = getenv("LC_ALL");
    const char *old_lc_messages = getenv("LC_MESSAGES");
    const char *old_lang = getenv("LANG");
    (void) setenv("LC_ALL", "C", 1);
    (void) setenv("LC_MESSAGES", "C", 1);
    (void) setenv("LANG", "C", 1);
    if ((pfile = popen(global_printing_system->scan_command, "r")) != NULL)
    {
     /*
      * Read input as needed...
      */

      while (fgets(line, sizeof(line), pfile) != NULL)
	{
	  char *tmp_ptr;
	  if ((tmp_ptr = strchr(line, '\n')))
	    tmp_ptr[0] = '\0';
	  if ((tmp_ptr = strchr(line, '\r')))
	    tmp_ptr[0] = '\0';
	  if (strlen(line) > 0)
	    {
	      if (!stp_string_list_is_present(stpui_system_print_queues, line))
		stp_string_list_add_string(stpui_system_print_queues,
					   line, line);
	    }
	}
      pclose(pfile);
      if (old_locale)
	setenv("LC_ALL", old_locale, 1);
      else
	unsetenv("LC_ALL");
      if (old_lc_messages)
	setenv("LC_MESSAGES", old_lc_messages, 1);
      else
	unsetenv("LC_MESSAGES");
      if (old_lang)
	setenv("LANG", old_lang, 1);
      else
	unsetenv("LANG");
    }
  }
}

const stpui_plist_t *
stpui_get_current_printer(void)
{
  return &(stpui_plist[stpui_plist_current]);
}

/*
 * 'usr1_handler()' - Make a note when we receive SIGUSR1.
 */

static volatile int usr1_interrupt;

static void
usr1_handler (int sig)
{
  usr1_interrupt = 1;
}

/*
 *
 * Process control for actually printing.  Documented 20040821
 * by Robert Krawitz.
 *
 * In addition to the print command itself and the output generator,
 * we spawn two additional processes to monitor the print job and clean
 * up.  We do this because the GIMP is very unfriendly about how it
 * terminates plugins when the user cancels an operation: it sends a
 * SIGKILL, which prevents the plugin from cleaning up.  Since the
 * plugin is sending data to an lpr process, this SIGKILL closes off
 * the input to lpr.  lpr doesn't know that its parent has died
 * inappropriately, and happily proceeds to print the partial job.
 *
 * (The child may not actually be lpr, of course, but we'll just use
 * that nomenclature for convenience.)
 *
 * The first such process is the "lpr monitor".  Its job is to
 * watch the parent (the actual data generator).  If its parent dies,
 * it kills the print command.  Notice that it must keep the file
 * descriptor used to write to the lpr process open, since as soon as
 * the last writer to this pipe is closed, the lpr process sees its
 * input close.  Therefore, it first kills the child with SIGTERM and
 * then closes the pipe.  Perhaps a more robust method would be to
 * send a SIGTERM followed by a SIGKILL, but we can worry about that
 * later.  The lpr monitor process is killed with SIGUSR1 by the
 * master upon successful completion, at which point it exits.  The lpr
 * monitor itself detects that the master has finished by periodically
 * sending it a kill 0 (a null signal).  When the parent exits, this
 * attempt to signal will return failure.  This has a potential race
 * condition if another process is created with the same PID between
 * checks.  A more robust (but more complicated) solution would involve
 * IPC of some kind.
 *
 * The second such process (the "error monitor") monitors the stderr
 * (and stdout) of the lpr process, to send any error messages back
 * to the user.  Since the GIMP is normally not launched from a
 * terminal window, any errors would get lost.  The error monitor
 * captures this output and reports it back.  It stays around until
 * its input is closed (normally by the lpr process exiting), at
 * which point it reports to the master that it has finished, and the
 * master can clean up and return.
 *
 * The actual master process spawns the lpr monitor, which spawns
 * the process that will later run the lpr command, which itself
 * spawns the error monitor.
 *
 * This architecture is perhaps unnecessarily complex; the lpr monitor
 * and error monitor could perhaps be combined into a single process
 * that watches both for the parent to go away and for the error messages.
 *
 * The following diagrams illustrate the control flow during the normal
 * case and also when the print job is cancelled.  The notation for file
 * descriptors is a number prefixed with < for an input file descriptor
 * and suffixed > for an output file descriptor.  For example, <0 means
 * input file descriptor 0 and 1> means output file descriptor 1.  The
 * key to the file descriptors is given below.  A file descriptor named
 * x,y refers to file descriptor y duplicated onto file descriptor x.
 * So "<0,3" means input file descriptor 3 (pipefd[0]) dup2'ed onto
 * file descriptor 0.
 * 
 * fd0 = fd 0
 * fd1 = fd 1
 * fd2 = fd 2
 * fd3 = pipefd[0]
 * fd4 = pipefd[1]
 * fd5 = syncfd[0]
 * fd6 = syncfd[1]
 * fd7 = errfd[0]
 * fd8 = errfd[1]
 *
 * 
 *                            NORMAL CASE
 * 
 * PARENT             CHILD 1              CHILD 2          CHILD 3
 * (print generator)  (lpr monitor)        (print command)  (error monitor)
 * |
 * stpui_print
 * |
 * | <0 1> 2>
 * |
 * | pipe(syncfd)
 * |
 * | <0 1> 2> <5 6>
 * |
 * | pipe(pipefd)
 * |
 * | <0 1> 2> <3 4>
 * |    <5 6>
 * |
 * | fork =============|
 * |                   |
 * | close(63)         | close(syncfd[0])
 * |                   | <0 1> 2> <3 4>
 * | <0 1> 2> 4> <5    | 6>
 * |                   |
 * |                   | fork =============|
 * |                   |                   |
 * |                   | close(01263)      | dup2(pipefd[0], 0)
 * |                   |                   | close(pipefd[0]
 * |                   | 4>                | close(pipefd[1]
 * |                   |                   |
 * |                   |                   | 1> 2> <0,3 6>
 * |                   |                   |
 * |                   |                   | pipe(errfd)
 * |                   |                   | 1> 2> <0,3 6>
 * |                   |                   | <7 8>
 * |                   |                   |
 * |                   |                   | fork =============|
 * |                   |                   |                   | close(012348)
 * |                   |                   | close(12)         | 6> <7
 * |                   |                   |                   |
 * |                   |                   | <0,3 6> <7 8>     |
 * |                   |                   |                   |
 * |                   |                   | dup2(errfd[1],1)  |
 * |                   |                   | dup2(errfd[1],2)  |
 * |                   |                   | close(errfd[1])   |
 * |                   |                   | close(pipefd[0])  |
 * |                   |                   | close(pipefd[1])  |
 * |                   |                   | close(syncfd[1])  |
 * |<<<<<<<<<<<<<<<<<<<|kill(0,0)          * EXEC lpr          |
 * |>>>>>>>>>>>>>>>>>>>|OK                 | <0,3 1,8> 2,8>    |
 * |                   |                   |                   |
 * | write>>>>>>>>>>>>>+>>>>>>>>>>>>>>>>>>>|                   |
 * |                   |                   | write(2,8)??>>>>>>|read(<7)->warn
 * |                   |                   |                   |
 * | close(4)>>>>>>>>>>+>>>>>>>>>>>>>>>>>>>|                   |
 * | <0 1> 2> <5       |                   |                   |
 * | kill>>>>>>>>>>>>>>|                   |                   |
 * |                   | close(4)>>>>>>>>>>| eof(<0,3)         |
 * |                   |                   |                   |
 * |                   | *no open fd*      | 1,8> 2,8>         |
 * |                   | exit              |                   |
 * |                   |                   | exit>>>>>>>>>>>>>>| eof(<7)
 * | wait<<<<<<<<<<<<<<X                   X                   | 6>
 * |                                                           |
 * | read(<5)<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<| write(6>)
 * |                                                           |
 * | close(5)                                                  | exit
 * |                                                           X
 * | 0< 1> 2>
 * |
 * | return
 * X
 *
 * 
 *                            ERROR CASE (job cancelled)
 * 
 * PARENT             CHILD 1              CHILD 2          CHILD 3
 * (print generator)  (lpr monitor)        (print command)  (error monitor)
 * |
 * stpui_print
 * |
 * | <0 1> 2>
 * |
 * | pipe(syncfd)
 * |
 * | <0 1> 2> <5 6>
 * |
 * | pipe(pipefd)
 * |
 * | <0 1> 2> <3 4>
 * |    <5 6>
 * |
 * | fork =============|
 * |                   |
 * | close(63)         | close(syncfd[0]
 * |                   | <0 1> 2> <3 4>
 * | <0 1> 2> 4> <5    | 6>
 * |                   |
 * |                   | fork =============|
 * |                   |                   |
 * |                   | close(01263)      | dup2(pipefd[0], 0)
 * |                   |                   | close(pipefd[0]
 * |                   | 4>                | close(pipefd[1]
 * |                   |                   |
 * |                   |                   | 1> 2> <3,0 6>
 * |                   |                   |
 * |                   |                   | pipe(errfd)
 * |                   |                   | 1> 2> <3,0 6>
 * |                   |                   | <7 8>
 * |                   |                   |
 * |                   |                   | fork =============|
 * |                   |                   |                   | close(012348)
 * |                   |                   | close(12)         | 6> <7
 * |                   |                   |                   |
 * |                   |                   | <3,0 6> <7 8>     |
 * |                   |                   |                   |
 * |                   |                   | dup2(errfd[1],1)  |
 * |                   |                   | dup2(errfd[1],2)  |
 * |                   |                   | close(3468)       |
 * |<<<<<<<<<<<<<<<<<<<|kill(0,0)          * EXEC lpr          |
 * |>>>>>>>>>>>>>>>>>>>|OK                 | <0,3 1,8> 2,8>    |
 * |                   |                   |                   |
 * | write>>>>>>>>>>>>>+>>>>>>>>>>>>>>>>>>>|                   |
 * |                   |                   | write(2,8)??>>>>>>|read(<7)->warn
 * | KILLED            |                   |                   |
 * | close(01245)>>>>>>+>>>>>>>>>>>>>>>>>>>|                   |
 * X                   |                   |                   |
 *  <<<<<<<<<<<<<<<<<<<|kill(0,0)          |                   |
 *  >>>>>>>>>>>>>>>>>>>|DEAD!              |                   |
 *                     |kill(2)>>>>>>>>>>>>|Terminated         |
 *                     |close(4>)>>>>>>>>>>|eof(0,3)           |
 *                     |                   | 1,8> 2,8>         |
 *                     | *no open fd*      |exit>>>>>>>>>>>>>>>|eof(7)
 *                     | exit              X                   |
 *                     X                                       | 6>
 *                                                            <| write(6)
 *                                                             | exit/SIGPIPE
 *                                                             X
 */

int
stpui_print(const stpui_plist_t *printer, stpui_image_t *image)
{
  int		ppid = getpid (), /* PID of plugin */
		opid,		/* PID of output process */
		cpid = 0,	/* PID of control/monitor process */
		pipefd[2],	/* Fds of the pipe connecting all the above */
		errfd[2],	/* Message logger from lp command */
		syncfd[2];	/* Sync the logger */
  FILE		*prn = NULL;	/* Print file/command */
  int		do_sync = 0;
  int		print_status = 0;
  int		dummy;

  /*
   * Open the file/execute the print command...
   */

  if (stpui_plist_get_command_type(printer) == COMMAND_TYPE_DEFAULT ||
      stpui_plist_get_command_type(printer) == COMMAND_TYPE_CUSTOM)
    {
      /*
       * The following IPC code is only necessary because the GIMP kills
       * plugins with SIGKILL if its "Cancel" button is pressed; this
       * gives the plugin no chance whatsoever to clean up after itself.
       */
      do_sync = 1;
      usr1_interrupt = 0;
      signal (SIGUSR1, usr1_handler);
      if (pipe (syncfd) != 0)
	{
	  do_sync = 0;
	}
      if (pipe (pipefd) != 0)
	{
	  prn = NULL;
	}
      else
	{
	  cpid = fork ();
	  if (cpid < 0)		/* Error */
	    {
	      do_sync = 0;
	      prn = NULL;
	    }
	  else if (cpid == 0)	/* Child 1 (lpr monitor and printer command) */
	    {
	      /* LPR monitor process.  Printer output is piped to us. */
	      close(syncfd[0]);
	      opid = fork ();
	      if (opid < 0)
		{
		  /* Errors will cause the plugin to get a SIGPIPE.  */
		  exit (1);
		}
	      else if (opid == 0) /* Child 2 (printer command) */
		{
		  dup2 (pipefd[0], 0);
		  close (pipefd[0]);
		  close (pipefd[1]);
		  if (pipe(errfd) == 0)
		    {
		      opid = fork();
		      if (opid < 0)
			_exit(1);
		      else if (opid == 0) /* Child 3 (monitors stderr) */
			{ 
			  stp_outfunc_t errfunc = stpui_get_errfunc();
			  void *errdata = stpui_get_errdata();
			  /* calls g_message on anything it sees */
			  char buf[4096];

			  close (pipefd[0]);
			  close (pipefd[1]);
			  close (0);
			  close (1);
			  close (2);
			  close (errfd[1]);
			  while (1)
			    {
			      ssize_t bytes = read(errfd[0], buf, 4095);
			      if (bytes > 0)
				{
				  buf[bytes] = '\0';
				  (*errfunc)(errdata, buf, bytes);
				}
			      else
				{
				  if (bytes < 0)
				    {
				      snprintf(buf, 4095,
					       "Read messages failed: %s\n",
					       strerror(errno));
				      (*errfunc)(errdata, buf, strlen(buf));
				    }
				  write(syncfd[1], "Done", 5);
				  _exit(0);
				}
			    }
			  write(syncfd[1], "Done", 5);
			  _exit(0);
			}
		      else	/* Child 2 (printer command) */
			{
			  char *command;
			  char *locale;
			  if (stpui_plist_get_command_type(printer) ==
			      COMMAND_TYPE_DEFAULT)
			    {
			      command =
				stpui_build_standard_print_command
				(printer, stp_get_printer(printer->v));
			      append_external_options(&command, printer->v);
			    }
			  else
			    command =
			      (char *) stpui_plist_get_custom_command(printer);
			  (void) close(2);
			  (void) close(1);
			  dup2 (errfd[1], 2);
			  dup2 (errfd[1], 1);
			  close(errfd[1]);
			  close (pipefd[0]);
			  close (pipefd[1]);
			  close(syncfd[1]);
#ifdef HAVE_LOCALE_H
			  locale = g_strdup(setlocale(LC_NUMERIC, NULL));
			  setlocale(LC_NUMERIC, "C");
#endif
			  execl("/bin/sh", "/bin/sh", "-c", command, NULL);
			  /* NOTREACHED */
			  _exit (1);
			}
		      /* NOTREACHED */
		      _exit(1);
		    }
		  else		/* pipe() failed! */
		    {
		      _exit(1);
		    }
		}
	      else		/* Child 1 (lpr monitor) */
		{
		  /*
		   * If the print plugin gets SIGKILLed by gimp, we kill lpr
		   * in turn.  If the plugin signals us with SIGUSR1 that it's
		   * finished printing normally, we close our end of the pipe,
		   * and go away.
		   *
		   * Note that we keep pipefd[1] -- which is the pipe from
		   * the print plugin to the lpr process -- open during this.
		   * If we don't, and the parent gets killed, lpr will notice
		   * its stdin getting closed off and start printing.
		   * This way its stdin stays open until we kill it.
		   */
		  close (0);
		  close (1);
		  close (2);
		  close (syncfd[1]);
		  close (pipefd[0]);
		  while (usr1_interrupt == 0)
		    {
		      /*
		       * Note potential race condition, if some other process
		       * happens to get the same pid!
		       */
		      if (kill (ppid, 0) < 0)
			{
			  /*
			   * The print plugin has been killed!
			   * Note that there is no possibility of the print
			   * job sending us a SIGUSR1 and then exiting;
			   * the parent (print plugin) stays around after
			   * sending us the SIGUSR1, and then waits
			   * for us to die.
			   */
			  kill (opid, SIGTERM);
			  waitpid (opid, &dummy, 0);
			  close (pipefd[1]);
			  /*
			   * We do not want to allow cleanup before exiting.
			   * The exiting parent has already closed the
			   * connection  to the X server; if we try to clean
			   * up, we'll notice that fact and complain.
			   */
			  _exit (0);
			}
		      sleep (5);
		    }
		  /* We got SIGUSR1.  */
		  close (pipefd[1]);
		  /*
		   * We do not want to allow cleanup before exiting.
		   * The exiting parent has already closed the connection
		   * to the X server; if we try to clean up, we'll notice
		   * that fact and complain.
		   */
		  _exit (0);
		}
	    }
	  else			/* Parent (actually generates the output) */
	    {
	      close (syncfd[1]);
	      close (pipefd[0]);
	      /* Parent process.  We generate the printer output. */
	      prn = fdopen (pipefd[1], "w");
	      /* and fall through... */
	    }
	}
    }
  else
    prn = fopen (stpui_plist_get_output_filename(printer), "wb");

  if (prn != NULL)
    {
      char tmp[32];
      stpui_plist_t *np = allocate_stpui_plist_copy(printer);
      const stp_vars_t *current_vars =
	stp_printer_get_defaults(stp_get_printer(np->v));
      int orientation;
      stp_merge_printvars(np->v, current_vars);
      stp_set_string_parameter(np->v, "InputImageType", image_type);
      if (image_raw_channels)
	{
	  sprintf(tmp, "%d", image_raw_channels);
	  stp_set_string_parameter(np->v, "RawChannels", tmp);
	}
      sprintf(tmp, "%d", image_channel_depth);
      stp_set_string_parameter(np->v, "ChannelBitDepth", tmp);

      /*
       * Set up the orientation
       */
      orientation = np->orientation;
      if (orientation == ORIENT_AUTO)
	orientation = stpui_compute_orientation();
      switch (orientation)
	{
	case ORIENT_PORTRAIT:
	  break;
	case ORIENT_LANDSCAPE:
	  if (image->rotate_cw)
	    (image->rotate_cw)(image);
	  break;
	case ORIENT_UPSIDEDOWN:
	  if (image->rotate_180)
	    (image->rotate_180)(image);
	  break;
	case ORIENT_SEASCAPE:
	  if (image->rotate_ccw)
	    (image->rotate_ccw)(image);
	  break;
	}

      /*
       * Finally, call the print driver to send the image to the printer
       * and close the output file/command...
       */

      stp_set_outfunc(np->v, writefunc);
      stp_set_errfunc(np->v, stpui_get_errfunc());
      stp_set_outdata(np->v, prn);
      stp_set_errdata(np->v, stpui_get_errdata());
      print_status = stp_print(np->v, &(image->im));

      /*
       * Note that we do not use popen() to create the output, therefore
       * we do not use pclose() to close it.  See bug 1013565.
       */
      (void) fclose(prn);
      if (stpui_plist_get_command_type(printer) == COMMAND_TYPE_DEFAULT ||
	  stpui_plist_get_command_type(printer) == COMMAND_TYPE_CUSTOM)
	{
	  /*
	   * It is important for us to first close off the lpr process,
	   * then kill the lpr monitor (child 1), and then wait for it
	   * to die before exiting.
	   */
	  kill (cpid, SIGUSR1);
	  waitpid (cpid, &dummy, 0);
	}

      /*
       * Make sure that any errors have been reported back to the user
       * prior to completion.  In addition, explicitly close off the
       * synchronization file descriptor since we're merely returning,
       * not exiting, and don't want to leave any pollution around.
       */
      if (do_sync)
	{
	  char buf[8];
	  (void) read(syncfd[0], buf, 8);
	  (void) close(syncfd[0]);
	}
      stpui_plist_destroy(np);
      g_free(np);
      return 1;
    }
  return 0;
}

/*
 * End of "$Id: plist.c,v 1.18 2008/07/04 14:29:28 rlk Exp $".
 */
