/*
 * "$Id: print-util.c,v 1.117 2010/06/26 20:02:02 rlk Exp $"
 *
 *   Print plug-in driver utility functions for the GIMP.
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

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include <math.h>
#include <limits.h>
#if defined(HAVE_VARARGS_H) && !defined(HAVE_STDARG_H)
#include <varargs.h>
#else
#include <stdarg.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "generic-options.h"

#define FMIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct
{
  stp_outfunc_t ofunc;
  void *odata;
  char *data;
  size_t bytes;
} debug_msgbuf_t;

/*
 * We cannot avoid use of the (non-ANSI) vsnprintf here; ANSI does
 * not provide a safe, length-limited sprintf function.
 */

#define STPI_VASPRINTF(result, bytes, format)				\
{									\
  int current_allocation = 64;						\
  result = stp_malloc(current_allocation);				\
  while (1)								\
    {									\
      va_list args;							\
      va_start(args, format);						\
      bytes = vsnprintf(result, current_allocation, format, args);	\
      va_end(args);							\
      if (bytes >= 0 && bytes < current_allocation)			\
	break;								\
      else								\
	{								\
	  stp_free (result);						\
	  if (bytes < 0)						\
	    current_allocation *= 2;					\
	  else								\
	    current_allocation = bytes + 1;				\
	  result = stp_malloc(current_allocation);			\
	}								\
    }									\
}

void
stp_zprintf(const stp_vars_t *v, const char *format, ...)
{
  char *result;
  int bytes;
  STPI_VASPRINTF(result, bytes, format);
  (stp_get_outfunc(v))((void *)(stp_get_outdata(v)), result, bytes);
  stp_free(result);
}

void
stp_asprintf(char **strp, const char *format, ...)
{
  char *result;
  int bytes;
  STPI_VASPRINTF(result, bytes, format);
  *strp = result;
}

void
stp_catprintf(char **strp, const char *format, ...)
{
  char *result1;
  char *result2;
  int bytes;
  STPI_VASPRINTF(result1, bytes, format);
  stp_asprintf(&result2, "%s%s", *strp, result1);
  stp_free(result1);
  *strp = result2;
}


void
stp_zfwrite(const char *buf, size_t bytes, size_t nitems, const stp_vars_t *v)
{
  (stp_get_outfunc(v))((void *)(stp_get_outdata(v)), buf, bytes * nitems);
}

void
stp_write_raw(const stp_raw_t *raw, const stp_vars_t *v)
{
  (stp_get_outfunc(v))((void *)(stp_get_outdata(v)), raw->data, raw->bytes);
}

void
stp_putc(int ch, const stp_vars_t *v)
{
  unsigned char a = (unsigned char) ch;
  (stp_get_outfunc(v))((void *)(stp_get_outdata(v)), (char *) &a, 1);
}

#define BYTE(expr, byteno) (((expr) >> (8 * byteno)) & 0xff)

void
stp_put16_le(unsigned short sh, const stp_vars_t *v)
{
  stp_putc(BYTE(sh, 0), v);
  stp_putc(BYTE(sh, 1), v);
}

void
stp_put16_be(unsigned short sh, const stp_vars_t *v)
{
  stp_putc(BYTE(sh, 1), v);
  stp_putc(BYTE(sh, 0), v);
}

void
stp_put32_le(unsigned int in, const stp_vars_t *v)
{
  stp_putc(BYTE(in, 0), v);
  stp_putc(BYTE(in, 1), v);
  stp_putc(BYTE(in, 2), v);
  stp_putc(BYTE(in, 3), v);
}

void
stp_put32_be(unsigned int in, const stp_vars_t *v)
{
  stp_putc(BYTE(in, 3), v);
  stp_putc(BYTE(in, 2), v);
  stp_putc(BYTE(in, 1), v);
  stp_putc(BYTE(in, 0), v);
}

void
stp_puts(const char *s, const stp_vars_t *v)
{
  (stp_get_outfunc(v))((void *)(stp_get_outdata(v)), s, strlen(s));
}

void
stp_putraw(const stp_raw_t *r, const stp_vars_t *v)
{
  (stp_get_outfunc(v))((void *)(stp_get_outdata(v)), r->data, r->bytes);
}

void
stp_send_command(const stp_vars_t *v, const char *command,
		 const char *format, ...)
{
  int i = 0;
  char fchar;
  const char *out_str;
  const stp_raw_t *out_raw;
  unsigned short byte_count = 0;
  va_list args;

  if (strlen(format) > 0)
    {
      va_start(args, format);
      for (i = 0; i < strlen(format); i++)
	{
	  switch (format[i])
	    {
	    case 'a':
	    case 'b':
	    case 'B':
	    case 'd':
	    case 'D':
	      break;
	    case 'c':
	      (void) va_arg(args, unsigned int);
	      byte_count += 1;
	      break;
	    case 'h':
	    case 'H':
	      (void) va_arg(args, unsigned int);
	      byte_count += 2;
	      break;
	    case 'l':
	    case 'L':
	      (void) va_arg(args, unsigned int);
	      byte_count += 4;
	      break;
	    case 'r':
	      out_raw = va_arg(args, const stp_raw_t *);
	      byte_count += out_raw->bytes;
	      break;
	    case 's':
	      out_str = va_arg(args, const char *);
	      byte_count += strlen(out_str);
	      break;
	    }
	}
      va_end(args);
    }

  stp_puts(command, v);

  va_start(args, format);
  while ((fchar = format[0]) != '\0')
    {
      switch (fchar)
	{
	case 'a':
	  stp_putc(byte_count, v);
	  break;
	case 'b':
	  stp_put16_le(byte_count, v);
	  break;
	case 'B':
	  stp_put16_be(byte_count, v);
	  break;
	case 'd':
	  stp_put32_le(byte_count, v);
	  break;
	case 'D':
	  stp_put32_be(byte_count, v);
	  break;
	case 'c':
	  stp_putc(va_arg(args, unsigned int), v);
	  break;
	case 'h':
	  stp_put16_le(va_arg(args, unsigned int), v);
	  break;
	case 'H':
	  stp_put16_be(va_arg(args, unsigned int), v);
	  break;
	case 'l':
	  stp_put32_le(va_arg(args, unsigned int), v);
	  break;
	case 'L':
	  stp_put32_be(va_arg(args, unsigned int), v);
	  break;
	case 's':
	  stp_puts(va_arg(args, const char *), v);
	  break;
	case 'r':
	  stp_putraw(va_arg(args, const stp_raw_t *), v);
	  break;
	}
      format++;
    }
  va_end(args);
}

void
stp_eprintf(const stp_vars_t *v, const char *format, ...)
{
  int bytes;
  if (stp_get_errfunc(v))
    {
      char *result;
      STPI_VASPRINTF(result, bytes, format);
      (stp_get_errfunc(v))((void *)(stp_get_errdata(v)), result, bytes);
      stp_free(result);
    }
  else
    {
      va_list args;
      va_start(args, format);
      vfprintf(stderr, format, args);
      va_end(args);
    }
}

void
stp_erputc(int ch)
{
  putc(ch, stderr);
}

void
stp_erprintf(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}

static unsigned long stpi_debug_level = 0;

static void
stpi_init_debug(void)
{
  static int debug_initialized = 0;
  if (!debug_initialized)
    {
      const char *dval = getenv("STP_DEBUG");
      debug_initialized = 1;
      if (dval)
	{
	  stpi_debug_level = strtoul(dval, 0, 0);
	  stp_erprintf("Gutenprint %s %s\n", VERSION, RELEASE_DATE);
	}
    }
}

unsigned long
stp_get_debug_level(void)
{
  stpi_init_debug();
  return stpi_debug_level;
}

void
stp_dprintf(unsigned long level, const stp_vars_t *v, const char *format, ...)
{
  int bytes;
  stpi_init_debug();
  if (level & stpi_debug_level)
    {
      if (stp_get_errfunc(v))
	{
	  char *result;
	  STPI_VASPRINTF(result, bytes, format);
	  (stp_get_errfunc(v))((void *)(stp_get_errdata(v)), result, bytes);
	  stp_free(result);
	} else {
	  va_list args;
	  va_start(args, format);
	  vfprintf(stderr, format, args);
	  va_end(args);
	}
    }
}

void
stp_deprintf(unsigned long level, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  stpi_init_debug();
  if (level & stpi_debug_level)
    vfprintf(stderr, format, args);
  va_end(args);
}

static void
fill_buffer_writefunc(void *priv, const char *buffer, size_t bytes)
{
  debug_msgbuf_t *msgbuf = (debug_msgbuf_t *) priv;
  if (msgbuf->bytes == 0)
    msgbuf->data = stp_malloc(bytes + 1);
  else
    msgbuf->data = stp_realloc(msgbuf->data, msgbuf->bytes + bytes + 1);
  memcpy(msgbuf->data + msgbuf->bytes, buffer, bytes);
  msgbuf->bytes += bytes;
  msgbuf->data[msgbuf->bytes] = '\0';
}

void
stp_init_debug_messages(stp_vars_t *v)
{
  int verified_flag = stp_get_verified(v);
  debug_msgbuf_t *msgbuf = stp_malloc(sizeof(debug_msgbuf_t));
  msgbuf->ofunc = stp_get_errfunc(v);
  msgbuf->odata = stp_get_errdata(v);
  msgbuf->data = NULL;
  msgbuf->bytes = 0;
  stp_set_errfunc((stp_vars_t *) v, fill_buffer_writefunc);
  stp_set_errdata((stp_vars_t *) v, msgbuf);
  stp_set_verified((stp_vars_t *) v, verified_flag);
}

void
stp_flush_debug_messages(stp_vars_t *v)
{
  int verified_flag = stp_get_verified(v);
  debug_msgbuf_t *msgbuf = (debug_msgbuf_t *)stp_get_errdata(v);
  stp_set_errfunc((stp_vars_t *) v, msgbuf->ofunc);
  stp_set_errdata((stp_vars_t *) v, msgbuf->odata);
  stp_set_verified((stp_vars_t *) v, verified_flag);
  if (msgbuf->bytes > 0)
    {
      stp_eprintf(v, "%s", msgbuf->data);
      stp_free(msgbuf->data);
    }
  stp_free(msgbuf);
}

/* pointers to the allocation functions to use, which may be set by
   client applications */
void *(*stp_malloc_func)(size_t size) = malloc;
void *(*stpi_realloc_func)(void *ptr, size_t size) = realloc;
void (*stpi_free_func)(void *ptr) = free;

void *
stp_malloc (size_t size)
{
  register void *memptr = NULL;

  if ((memptr = stp_malloc_func (size)) == NULL)
    {
      fputs("Virtual memory exhausted.\n", stderr);
      stp_abort();
    }
  return (memptr);
}

void *
stp_zalloc (size_t size)
{
  register void *memptr = stp_malloc(size);
  (void) memset(memptr, 0, size);
  return (memptr);
}

void *
stp_realloc (void *ptr, size_t size)
{
  register void *memptr = NULL;

  if (size > 0 && ((memptr = stpi_realloc_func (ptr, size)) == NULL))
    {
      fputs("Virtual memory exhausted.\n", stderr);
      stp_abort();
    }
  return (memptr);
}

void
stp_free(void *ptr)
{
  stpi_free_func(ptr);
}

int
stp_init(void)
{
  static int stpi_is_initialised = 0;
  if (!stpi_is_initialised)
    {
      /* Things that are only initialised once */
      /* Set up gettext */
#ifdef HAVE_LOCALE_H
      char *locale = stp_strdup(setlocale (LC_ALL, ""));
#endif
#ifdef ENABLE_NLS
      bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
#endif
#ifdef HAVE_LOCALE_H
      setlocale(LC_ALL, locale);
      stp_free(locale);
#endif
      stpi_init_debug();
      stp_xml_preinit();
      stpi_init_printer();
      stpi_init_paper();
      stpi_init_dither();
      /* Load modules */
      if (stp_module_load())
	return 1;
      /* Load XML data */
      if (stp_xml_init_defaults())
	return 1;
      /* Initialise modules */
      if (stp_module_init())
	return 1;
      /* Set up defaults for core parameters */
      stp_initialize_printer_defaults();
    }

  stpi_is_initialised = 1;
  return 0;
}

size_t
stp_strlen(const char *s)
{
  return strlen(s);
}

char *
stp_strndup(const char *s, int n)
{
  char *ret;
  if (!s || n < 0)
    {
      ret = stp_malloc(1);
      ret[0] = 0;
      return ret;
    }
  else
    {
      ret = stp_malloc(n + 1);
      memcpy(ret, s, n);
      ret[n] = 0;
      return ret;
    }
}

char *
stp_strdup(const char *s)
{
  char *ret;
  if (!s)
    {
      ret = stp_malloc(1);
      ret[0] = '\0';
      return ret;
    }
  else
    return stp_strndup(s, stp_strlen(s));
}

const char *
stp_set_output_codeset(const char *codeset)
{
#if defined(ENABLE_NLS) && !defined(__APPLE__)
  return (const char *)(bind_textdomain_codeset(PACKAGE, codeset));
#else
  return "US-ASCII";
#endif
}

stp_curve_t *
stp_read_and_compose_curves(const char *s1, const char *s2,
			    stp_curve_compose_t comp,
			    size_t piecewise_point_count)
{
  stp_curve_t *ret = NULL;
  stp_curve_t *t1 = NULL;
  stp_curve_t *t2 = NULL;
  if (s1)
    t1 = stp_curve_create_from_string(s1);
  if (s2)
    t2 = stp_curve_create_from_string(s2);
  if (t1 && t2)
    {
      if (stp_curve_is_piecewise(t1) && stp_curve_is_piecewise(t2))
	{
	  stp_curve_resample(t1, piecewise_point_count);
	  stp_curve_resample(t2, piecewise_point_count);
	}
      stp_curve_compose(&ret, t1, t2, comp, -1);
    }
  if (ret)
    {
      stp_curve_destroy(t1);
      stp_curve_destroy(t2);
      return ret;
    }
  else if (t1)
    {
      if(t2)
        stp_curve_destroy(t2);
      return t1;
    }
  else
    return t2;
}

void
stp_merge_printvars(stp_vars_t *user, const stp_vars_t *print)
{
  int i;
  stp_parameter_list_t params = stp_get_parameter_list(print);
  int count = stp_parameter_list_count(params);
  stp_deprintf(STP_DBG_VARS, "Merging printvars from %s\n",
	       stp_get_driver(print));
  for (i = 0; i < count; i++)
    {
      const stp_parameter_t *p = stp_parameter_list_param(params, i);
      if (p->p_type == STP_PARAMETER_TYPE_DOUBLE &&
	  p->p_class == STP_PARAMETER_CLASS_OUTPUT &&
	  stp_check_float_parameter(print, p->name, STP_PARAMETER_DEFAULTED))
	{
	  stp_parameter_t desc;
	  double prnval = stp_get_float_parameter(print, p->name);
	  double usrval;
	  stp_describe_parameter(print, p->name, &desc);
	  if (stp_check_float_parameter(user, p->name, STP_PARAMETER_ACTIVE))
	    usrval = stp_get_float_parameter(user, p->name);
	  else
	    usrval = desc.deflt.dbl;
	  if (strcmp(p->name, "Gamma") == 0)
	    usrval /= prnval;
	  else
	    usrval *= prnval;
	  if (usrval < desc.bounds.dbl.lower)
	    usrval = desc.bounds.dbl.lower;
	  else if (usrval > desc.bounds.dbl.upper)
	    usrval = desc.bounds.dbl.upper;
	  if (!stp_check_float_parameter(user, p->name, STP_PARAMETER_ACTIVE))
	    {
	      stp_clear_float_parameter(user, p->name);
	      stp_set_default_float_parameter(user, p->name, usrval);
	    }
	  else
	    stp_set_float_parameter(user, p->name, usrval);
	  stp_parameter_description_destroy(&desc);
	}
    }
  stp_deprintf(STP_DBG_VARS, "Exiting merge printvars\n");
  stp_parameter_list_destroy(params);
}

stp_parameter_list_t
stp_get_parameter_list(const stp_vars_t *v)
{
  stp_parameter_list_t ret = stp_parameter_list_create();
  stp_parameter_list_t tmp_list;

  tmp_list = stp_printer_list_parameters(v);
  stp_parameter_list_append(ret, tmp_list);
  stp_parameter_list_destroy(tmp_list);

  tmp_list = stp_color_list_parameters(v);
  stp_parameter_list_append(ret, tmp_list);
  stp_parameter_list_destroy(tmp_list);

  tmp_list = stp_dither_list_parameters(v);
  stp_parameter_list_append(ret, tmp_list);
  stp_parameter_list_destroy(tmp_list);

  tmp_list = stp_list_generic_parameters(v);
  stp_parameter_list_append(ret, tmp_list);
  stp_parameter_list_destroy(tmp_list);

  return ret;
}

void
stp_abort(void)
{
  abort();
}
