/* POSIX extended headers for tar.

   Copyright (C) 2003, 2004, 2005, 2006, 2007 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
   Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <system.h>

#include <fnmatch.h>
#include <hash.h>
#include <inttostr.h>
#include <quotearg.h>

#include "common.h"

#include <fnmatch.h>

static bool xheader_protected_pattern_p (char const *pattern);
static bool xheader_protected_keyword_p (char const *keyword);
static void xheader_set_single_keyword (char *) __attribute__ ((noreturn));

/* Used by xheader_finish() */
static void code_string (char const *string, char const *keyword,
			 struct xheader *xhdr);

/* Number of global headers written so far. */
static size_t global_header_count;
/* FIXME: Possibly it should be reset after changing the volume.
   POSIX %n specification says that it is expanded to the sequence
   number of current global header in *the* archive. However, for
   multi-volume archives this will yield duplicate header names
   in different volumes, which I'd like to avoid. The best way
   to solve this would be to use per-archive header count as required
   by POSIX *and* set globexthdr.name to, say,
   $TMPDIR/GlobalHead.%p.$NUMVOLUME.%n.

   However it should wait until buffer.c is finally rewritten */


/* Interface functions to obstacks */

static void
x_obstack_grow (struct xheader *xhdr, const char *ptr, size_t length)
{
  obstack_grow (xhdr->stk, ptr, length);
  xhdr->size += length;
}

static void
x_obstack_1grow (struct xheader *xhdr, char c)
{
  obstack_1grow (xhdr->stk, c);
  xhdr->size++;
}

static void
x_obstack_blank (struct xheader *xhdr, size_t length)
{
  obstack_blank (xhdr->stk, length);
  xhdr->size += length;
}


/* Keyword options */

struct keyword_list
{
  struct keyword_list *next;
  char *pattern;
  char *value;
};


/* List of keyword patterns set by delete= option */
static struct keyword_list *keyword_pattern_list;

/* List of keyword/value pairs set by `keyword=value' option */
static struct keyword_list *keyword_global_override_list;

/* List of keyword/value pairs set by `keyword:=value' option */
static struct keyword_list *keyword_override_list;

/* List of keyword/value pairs decoded from the last 'g' type header */
static struct keyword_list *global_header_override_list;

/* Template for the name field of an 'x' type header */
static char *exthdr_name;

/* Template for the name field of a 'g' type header */
static char *globexthdr_name;

bool
xheader_keyword_deleted_p (const char *kw)
{
  struct keyword_list *kp;

  for (kp = keyword_pattern_list; kp; kp = kp->next)
    if (fnmatch (kp->pattern, kw, 0) == 0)
      return true;
  return false;
}

static bool
xheader_keyword_override_p (const char *keyword)
{
  struct keyword_list *kp;

  for (kp = keyword_override_list; kp; kp = kp->next)
    if (strcmp (kp->pattern, keyword) == 0)
      return true;
  return false;
}

static void
xheader_list_append (struct keyword_list **root, char const *kw,
		     char const *value)
{
  struct keyword_list *kp = xmalloc (sizeof *kp);
  kp->pattern = xstrdup (kw);
  kp->value = value ? xstrdup (value) : NULL;
  kp->next = *root;
  *root = kp;
}

static void
xheader_list_destroy (struct keyword_list **root)
{
  if (root)
    {
      struct keyword_list *kw = *root;
      while (kw)
	{
	  struct keyword_list *next = kw->next;
	  free (kw->pattern);
	  free (kw->value);
	  free (kw);
	  kw = next;
	}
      *root = NULL;
    }
}

static void
xheader_set_single_keyword (char *kw)
{
  USAGE_ERROR ((0, 0, _("Keyword %s is unknown or not yet implemented"), kw));
}

static void
xheader_set_keyword_equal (char *kw, char *eq)
{
  bool global = true;
  char *p = eq;

  if (eq[-1] == ':')
    {
      p--;
      global = false;
    }

  while (p > kw && isspace (*p))
    p--;

  *p = 0;

  for (p = eq + 1; *p && isspace (*p); p++)
    ;

  if (strcmp (kw, "delete") == 0)
    {
      if (xheader_protected_pattern_p (p))
	USAGE_ERROR ((0, 0, _("Pattern %s cannot be used"), quote (p)));
      xheader_list_append (&keyword_pattern_list, p, NULL);
    }
  else if (strcmp (kw, "exthdr.name") == 0)
    assign_string (&exthdr_name, p);
  else if (strcmp (kw, "globexthdr.name") == 0)
    assign_string (&globexthdr_name, p);
  else
    {
      if (xheader_protected_keyword_p (kw))
	USAGE_ERROR ((0, 0, _("Keyword %s cannot be overridden"), kw));
      if (global)
	xheader_list_append (&keyword_global_override_list, kw, p);
      else
	xheader_list_append (&keyword_override_list, kw, p);
    }
}

void
xheader_set_option (char *string)
{
  char *token;
  for (token = strtok (string, ","); token; token = strtok (NULL, ","))
    {
      char *p = strchr (token, '=');
      if (!p)
	xheader_set_single_keyword (token);
      else
	xheader_set_keyword_equal (token, p);
    }
}

/*
    string Includes:          Replaced By:
     %d                       The directory name of the file,
                              equivalent to the result of the
                              dirname utility on the translated
                              file name.
     %f                       The filename of the file, equivalent
                              to the result of the basename
                              utility on the translated file name.
     %p                       The process ID of the pax process.
     %n                       The value of the 3rd argument.
     %%                       A '%' character. */

char *
xheader_format_name (struct tar_stat_info *st, const char *fmt, size_t n)
{
  char *buf;
  size_t len = strlen (fmt);
  char *q;
  const char *p;
  char *dirp = NULL;
  char *dir = NULL;
  char *base = NULL;
  char pidbuf[UINTMAX_STRSIZE_BOUND];
  char const *pptr;
  char nbuf[UINTMAX_STRSIZE_BOUND];
  char const *nptr = NULL;

  for (p = fmt; *p && (p = strchr (p, '%')); )
    {
      switch (p[1])
	{
	case '%':
	  len--;
	  break;

	case 'd':
	  if (st)
	    {
	      if (!dirp)
		dirp = dir_name (st->orig_file_name);
	      dir = safer_name_suffix (dirp, false, absolute_names_option);
	      len += strlen (dir) - 2;
	    }
	  break;

	case 'f':
	  if (st)
	    {
	      base = last_component (st->orig_file_name);
	      len += strlen (base) - 2;
	    }
	  break;

	case 'p':
	  pptr = umaxtostr (getpid (), pidbuf);
	  len += pidbuf + sizeof pidbuf - 1 - pptr - 2;
	  break;

	case 'n':
	  nptr = umaxtostr (n, nbuf);
	  len += nbuf + sizeof nbuf - 1 - nptr - 2;
	  break;
	}
      p++;
    }

  buf = xmalloc (len + 1);
  for (q = buf, p = fmt; *p; )
    {
      if (*p == '%')
	{
	  switch (p[1])
	    {
	    case '%':
	      *q++ = *p++;
	      p++;
	      break;

	    case 'd':
	      if (dir)
		q = stpcpy (q, dir);
	      p += 2;
	      break;

	    case 'f':
	      if (base)
		q = stpcpy (q, base);
	      p += 2;
	      break;

	    case 'p':
	      q = stpcpy (q, pptr);
	      p += 2;
	      break;

	    case 'n':
	      if (nptr)
		{
		  q = stpcpy (q, nptr);
		  p += 2;
		  break;
		}
	      /* else fall through */

	    default:
	      *q++ = *p++;
	      if (*p)
		*q++ = *p++;
	    }
	}
      else
	*q++ = *p++;
    }

  free (dirp);

  /* Do not allow it to end in a slash */
  while (q > buf && ISSLASH (q[-1]))
    q--;
  *q = 0;
  return buf;
}

char *
xheader_xhdr_name (struct tar_stat_info *st)
{
  if (!exthdr_name)
    assign_string (&exthdr_name, "%d/PaxHeaders.%p/%f");
  return xheader_format_name (st, exthdr_name, 0);
}

#define GLOBAL_HEADER_TEMPLATE "/GlobalHead.%p.%n"

char *
xheader_ghdr_name (void)
{
  if (!globexthdr_name)
    {
      size_t len;
      const char *tmp = getenv ("TMPDIR");
      if (!tmp)
	tmp = "/tmp";
      len = strlen (tmp) + sizeof (GLOBAL_HEADER_TEMPLATE); /* Includes nul */
      globexthdr_name = xmalloc (len);
      strcpy(globexthdr_name, tmp);
      strcat(globexthdr_name, GLOBAL_HEADER_TEMPLATE);
    }

  return xheader_format_name (NULL, globexthdr_name, global_header_count + 1);
}

void
xheader_write (char type, char *name, struct xheader *xhdr)
{
  union block *header;
  size_t size;
  char *p;

  size = xhdr->size;
  header = start_private_header (name, size);
  header->header.typeflag = type;

  simple_finish_header (header);

  p = xhdr->buffer;

  do
    {
      size_t len;

      header = find_next_block ();
      len = BLOCKSIZE;
      if (len > size)
	len = size;
      memcpy (header->buffer, p, len);
      if (len < BLOCKSIZE)
	memset (header->buffer + len, 0, BLOCKSIZE - len);
      p += len;
      size -= len;
      set_next_block_after (header);
    }
  while (size > 0);
  xheader_destroy (xhdr);

  if (type == XGLTYPE)
    global_header_count++;
}

void
xheader_write_global (struct xheader *xhdr)
{
  char *name;
  struct keyword_list *kp;

  if (!keyword_global_override_list)
    return;

  xheader_init (xhdr);
  for (kp = keyword_global_override_list; kp; kp = kp->next)
    code_string (kp->value, kp->pattern, xhdr);
  xheader_finish (xhdr);
  xheader_write (XGLTYPE, name = xheader_ghdr_name (), xhdr);
  free (name);
}


/* General Interface */

struct xhdr_tab
{
  char const *keyword;
  void (*coder) (struct tar_stat_info const *, char const *,
		 struct xheader *, void const *data);
  void (*decoder) (struct tar_stat_info *, char const *, char const *, size_t);
  bool protect;
};

/* This declaration must be extern, because ISO C99 section 6.9.2
   prohibits a tentative definition that has both internal linkage and
   incomplete type.  If we made it static, we'd have to declare its
   size which would be a maintenance pain; if we put its initializer
   here, we'd need a boatload of forward declarations, which would be
   even more of a pain.  */
extern struct xhdr_tab const xhdr_tab[];

static struct xhdr_tab const *
locate_handler (char const *keyword)
{
  struct xhdr_tab const *p;

  for (p = xhdr_tab; p->keyword; p++)
    if (strcmp (p->keyword, keyword) == 0)
      return p;
  return NULL;
}

static bool
xheader_protected_pattern_p (const char *pattern)
{
  struct xhdr_tab const *p;

  for (p = xhdr_tab; p->keyword; p++)
    if (p->protect && fnmatch (pattern, p->keyword, 0) == 0)
      return true;
  return false;
}

static bool
xheader_protected_keyword_p (const char *keyword)
{
  struct xhdr_tab const *p;

  for (p = xhdr_tab; p->keyword; p++)
    if (p->protect && strcmp (p->keyword, keyword) == 0)
      return true;
  return false;
}

/* Decode a single extended header record, advancing *PTR to the next record.
   Return true on success, false otherwise.  */
static bool
decode_record (struct xheader *xhdr,
	       char **ptr,
	       void (*handler) (void *, char const *, char const *, size_t),
	       void *data)
{
  char *start = *ptr;
  char *p = start;
  uintmax_t u;
  size_t len;
  char *len_lim;
  char const *keyword;
  char *nextp;
  size_t len_max = xhdr->buffer + xhdr->size - start;

  while (*p == ' ' || *p == '\t')
    p++;

  if (! ISDIGIT (*p))
    {
      if (*p)
	ERROR ((0, 0, _("Malformed extended header: missing length")));
      return false;
    }

  errno = 0;
  len = u = strtoumax (p, &len_lim, 10);
  if (len != u || errno == ERANGE)
    {
      ERROR ((0, 0, _("Extended header length is out of allowed range")));
      return false;
    }

  if (len_max < len)
    {
      int len_len = len_lim - p;
      ERROR ((0, 0, _("Extended header length %*s is out of range"),
	      len_len, p));
      return false;
    }

  nextp = start + len;

  for (p = len_lim; *p == ' ' || *p == '\t'; p++)
    continue;
  if (p == len_lim)
    {
      ERROR ((0, 0,
	      _("Malformed extended header: missing blank after length")));
      return false;
    }

  keyword = p;
  p = strchr (p, '=');
  if (! (p && p < nextp))
    {
      ERROR ((0, 0, _("Malformed extended header: missing equal sign")));
      return false;
    }

  if (nextp[-1] != '\n')
    {
      ERROR ((0, 0, _("Malformed extended header: missing newline")));
      return false;
    }

  *p = nextp[-1] = '\0';
  handler (data, keyword, p + 1, nextp - p - 2); /* '=' + trailing '\n' */
  *p = '=';
  nextp[-1] = '\n';
  *ptr = nextp;
  return true;
}

static void
run_override_list (struct keyword_list *kp, struct tar_stat_info *st)
{
  for (; kp; kp = kp->next)
    {
      struct xhdr_tab const *t = locate_handler (kp->pattern);
      if (t)
	t->decoder (st, t->keyword, kp->value, strlen (kp->value));
    }
}

static void
decx (void *data, char const *keyword, char const *value, size_t size)
{
  struct xhdr_tab const *t;
  struct tar_stat_info *st = data;

  if (xheader_keyword_deleted_p (keyword)
      || xheader_keyword_override_p (keyword))
    return;

  t = locate_handler (keyword);
  if (t)
    t->decoder (st, keyword, value, size);
  else
    WARN((0, 0, _("Ignoring unknown extended header keyword `%s'"),
	 keyword));
}

void
xheader_decode (struct tar_stat_info *st)
{
  run_override_list (keyword_global_override_list, st);
  run_override_list (global_header_override_list, st);

  if (st->xhdr.size)
    {
      char *p = st->xhdr.buffer + BLOCKSIZE;
      while (decode_record (&st->xhdr, &p, decx, st))
	continue;
    }
  run_override_list (keyword_override_list, st);
}

static void
decg (void *data, char const *keyword, char const *value,
      size_t size __attribute__((unused)))
{
  struct keyword_list **kwl = data;
  xheader_list_append (kwl, keyword, value);
}

void
xheader_decode_global (struct xheader *xhdr)
{
  if (xhdr->size)
    {
      char *p = xhdr->buffer + BLOCKSIZE;

      xheader_list_destroy (&global_header_override_list);
      while (decode_record (xhdr, &p, decg, &global_header_override_list))
	continue;
    }
}

void
xheader_init (struct xheader *xhdr)
{
  if (!xhdr->stk)
    {
      xhdr->stk = xmalloc (sizeof *xhdr->stk);
      obstack_init (xhdr->stk);
    }
}

void
xheader_store (char const *keyword, struct tar_stat_info *st,
	       void const *data)
{
  struct xhdr_tab const *t;

  if (st->xhdr.buffer)
    return;
  t = locate_handler (keyword);
  if (!t || !t->coder)
    return;
  if (xheader_keyword_deleted_p (keyword)
      || xheader_keyword_override_p (keyword))
    return;
  xheader_init (&st->xhdr);
  t->coder (st, keyword, &st->xhdr, data);
}

void
xheader_read (struct xheader *xhdr, union block *p, size_t size)
{
  size_t j = 0;

  xheader_init (xhdr);
  size += BLOCKSIZE;
  xhdr->size = size;
  xhdr->buffer = xmalloc (size + 1);
  xhdr->buffer[size] = '\0';

  do
    {
      size_t len = size;

      if (len > BLOCKSIZE)
	len = BLOCKSIZE;

      memcpy (&xhdr->buffer[j], p->buffer, len);
      set_next_block_after (p);

      p = find_next_block ();

      j += len;
      size -= len;
    }
  while (size > 0);
}

static void
xheader_print_n (struct xheader *xhdr, char const *keyword,
		 char const *value, size_t vsize)
{
  size_t len = strlen (keyword) + vsize + 3; /* ' ' + '=' + '\n' */
  size_t p;
  size_t n = 0;
  char nbuf[UINTMAX_STRSIZE_BOUND];
  char const *np;

  do
    {
      p = n;
      np = umaxtostr (len + p, nbuf);
      n = nbuf + sizeof nbuf - 1 - np;
    }
  while (n != p);

  x_obstack_grow (xhdr, np, n);
  x_obstack_1grow (xhdr, ' ');
  x_obstack_grow (xhdr, keyword, strlen (keyword));
  x_obstack_1grow (xhdr, '=');
  x_obstack_grow (xhdr, value, vsize);
  x_obstack_1grow (xhdr, '\n');
}

static void
xheader_print (struct xheader *xhdr, char const *keyword, char const *value)
{
  xheader_print_n (xhdr, keyword, value, strlen (value));
}

void
xheader_finish (struct xheader *xhdr)
{
  struct keyword_list *kp;

  for (kp = keyword_override_list; kp; kp = kp->next)
    code_string (kp->value, kp->pattern, xhdr);

  xhdr->buffer = obstack_finish (xhdr->stk);
}

void
xheader_destroy (struct xheader *xhdr)
{
  if (xhdr->stk)
    {
      obstack_free (xhdr->stk, NULL);
      free (xhdr->stk);
      xhdr->stk = NULL;
    }
  else
    free (xhdr->buffer);
  xhdr->buffer = 0;
  xhdr->size = 0;
}


/* Buildable strings */

void
xheader_string_begin (struct xheader *xhdr)
{
  xhdr->string_length = 0;
}

void
xheader_string_add (struct xheader *xhdr, char const *s)
{
  if (xhdr->buffer)
    return;
  xheader_init (xhdr);
  xhdr->string_length += strlen (s);
  x_obstack_grow (xhdr, s, strlen (s));
}

bool
xheader_string_end (struct xheader *xhdr, char const *keyword)
{
  uintmax_t len;
  uintmax_t p;
  uintmax_t n = 0;
  size_t size;
  char nbuf[UINTMAX_STRSIZE_BOUND];
  char const *np;
  char *cp;

  if (xhdr->buffer)
    return false;
  xheader_init (xhdr);

  len = strlen (keyword) + xhdr->string_length + 3; /* ' ' + '=' + '\n' */

  do
    {
      p = n;
      np = umaxtostr (len + p, nbuf);
      n = nbuf + sizeof nbuf - 1 - np;
    }
  while (n != p);

  p = strlen (keyword) + n + 2;
  size = p;
  if (size != p)
    {
      ERROR ((0, 0,
        _("Generated keyword/value pair is too long (keyword=%s, length=%s)"),
	      keyword, nbuf));
      obstack_free (xhdr->stk, obstack_finish (xhdr->stk));
      return false;
    }
  x_obstack_blank (xhdr, p);
  x_obstack_1grow (xhdr, '\n');
  cp = obstack_next_free (xhdr->stk) - xhdr->string_length - p - 1;
  memmove (cp + p, cp, xhdr->string_length);
  cp = stpcpy (cp, np);
  *cp++ = ' ';
  cp = stpcpy (cp, keyword);
  *cp++ = '=';
  return true;
}


/* Implementations */

static void
out_of_range_header (char const *keyword, char const *value,
		     uintmax_t minus_minval, uintmax_t maxval)
{
  char minval_buf[UINTMAX_STRSIZE_BOUND + 1];
  char maxval_buf[UINTMAX_STRSIZE_BOUND];
  char *minval_string = umaxtostr (minus_minval, minval_buf + 1);
  char *maxval_string = umaxtostr (maxval, maxval_buf);
  if (minus_minval)
    *--minval_string = '-';

  /* TRANSLATORS: The first %s is the pax extended header keyword
     (atime, gid, etc.).  */
  ERROR ((0, 0, _("Extended header %s=%s is out of range %s..%s"),
	  keyword, value, minval_string, maxval_string));
}

static void
code_string (char const *string, char const *keyword, struct xheader *xhdr)
{
  char *outstr;
  if (!utf8_convert (true, string, &outstr))
    {
      /* FIXME: report error */
      outstr = xstrdup (string);
    }
  xheader_print (xhdr, keyword, outstr);
  free (outstr);
}

static void
decode_string (char **string, char const *arg)
{
  if (*string)
    {
      free (*string);
      *string = NULL;
    }
  if (!utf8_convert (false, arg, string))
    {
      /* FIXME: report error and act accordingly to --pax invalid=UTF-8 */
      assign_string (string, arg);
    }
}

static void
code_time (struct timespec t, char const *keyword, struct xheader *xhdr)
{
  char buf[TIMESPEC_STRSIZE_BOUND];
  xheader_print (xhdr, keyword, code_timespec (t, buf));
}

enum decode_time_status
  {
    decode_time_success,
    decode_time_range,
    decode_time_bad_header
  };

static enum decode_time_status
_decode_time (struct timespec *ts, char const *arg, char const *keyword)
{
  time_t s;
  unsigned long int ns = 0;
  char *p;
  char *arg_lim;
  bool negative = *arg == '-';

  errno = 0;

  if (ISDIGIT (arg[negative]))
    {
      if (negative)
	{
	  intmax_t i = strtoimax (arg, &arg_lim, 10);
	  if (TYPE_SIGNED (time_t) ? i < TYPE_MINIMUM (time_t) : i < 0)
	    return decode_time_range;
	  s = i;
	}
      else
	{
	  uintmax_t i = strtoumax (arg, &arg_lim, 10);
	  if (TYPE_MAXIMUM (time_t) < i)
	    return decode_time_range;
	  s = i;
	}

      p = arg_lim;

      if (errno == ERANGE)
	return decode_time_range;

      if (*p == '.')
	{
	  int digits = 0;
	  bool trailing_nonzero = false;

	  while (ISDIGIT (*++p))
	    if (digits < LOG10_BILLION)
	      {
		ns = 10 * ns + (*p - '0');
		digits++;
	      }
	    else
	      trailing_nonzero |= *p != '0';

	  while (digits++ < LOG10_BILLION)
	    ns *= 10;

	  if (negative)
	    {
	      /* Convert "-1.10000000000001" to s == -2, ns == 89999999.
		 I.e., truncate time stamps towards minus infinity while
		 converting them to internal form.  */
	      ns += trailing_nonzero;
	      if (ns != 0)
		{
		  if (s == TYPE_MINIMUM (time_t))
		    return decode_time_range;
		  s--;
		  ns = BILLION - ns;
		}
	    }
	}

      if (! *p)
	{
	  ts->tv_sec = s;
	  ts->tv_nsec = ns;
	  return decode_time_success;
	}
    }

  return decode_time_bad_header;
}

static bool
decode_time (struct timespec *ts, char const *arg, char const *keyword)
{
  switch (_decode_time (ts, arg, keyword))
    {
    case decode_time_success:
      return true;
    case decode_time_bad_header:
      ERROR ((0, 0, _("Malformed extended header: invalid %s=%s"),
	      keyword, arg));
      return false;
    case decode_time_range:
      out_of_range_header (keyword, arg, - (uintmax_t) TYPE_MINIMUM (time_t),
			   TYPE_MAXIMUM (time_t));
      return false;
    }
  return true;
}



static void
code_num (uintmax_t value, char const *keyword, struct xheader *xhdr)
{
  char sbuf[UINTMAX_STRSIZE_BOUND];
  xheader_print (xhdr, keyword, umaxtostr (value, sbuf));
}

static bool
decode_num (uintmax_t *num, char const *arg, uintmax_t maxval,
	    char const *keyword)
{
  uintmax_t u;
  char *arg_lim;

  if (! (ISDIGIT (*arg)
	 && (errno = 0, u = strtoumax (arg, &arg_lim, 10), !*arg_lim)))
    {
      ERROR ((0, 0, _("Malformed extended header: invalid %s=%s"),
	      keyword, arg));
      return false;
    }

  if (! (u <= maxval && errno != ERANGE))
    {
      out_of_range_header (keyword, arg, 0, maxval);
      return false;
    }

  *num = u;
  return true;
}

static void
dummy_coder (struct tar_stat_info const *st __attribute__ ((unused)),
	     char const *keyword __attribute__ ((unused)),
	     struct xheader *xhdr __attribute__ ((unused)),
	     void const *data __attribute__ ((unused)))
{
}

static void
dummy_decoder (struct tar_stat_info *st __attribute__ ((unused)),
	       char const *keyword __attribute__ ((unused)),
	       char const *arg __attribute__ ((unused)),
	       size_t size __attribute__((unused)))
{
}

static void
atime_coder (struct tar_stat_info const *st, char const *keyword,
	     struct xheader *xhdr, void const *data __attribute__ ((unused)))
{
  code_time (st->atime, keyword, xhdr);
}

static void
atime_decoder (struct tar_stat_info *st,
	       char const *keyword,
	       char const *arg,
	       size_t size __attribute__((unused)))
{
  struct timespec ts;
  if (decode_time (&ts, arg, keyword))
    st->atime = ts;
}

static void
gid_coder (struct tar_stat_info const *st, char const *keyword,
	   struct xheader *xhdr, void const *data __attribute__ ((unused)))
{
  code_num (st->stat.st_gid, keyword, xhdr);
}

static void
gid_decoder (struct tar_stat_info *st,
	     char const *keyword,
	     char const *arg,
	     size_t size __attribute__((unused)))
{
  uintmax_t u;
  if (decode_num (&u, arg, TYPE_MAXIMUM (gid_t), keyword))
    st->stat.st_gid = u;
}

static void
gname_coder (struct tar_stat_info const *st, char const *keyword,
	     struct xheader *xhdr, void const *data __attribute__ ((unused)))
{
  code_string (st->gname, keyword, xhdr);
}

static void
gname_decoder (struct tar_stat_info *st,
	       char const *keyword __attribute__((unused)),
	       char const *arg,
	       size_t size __attribute__((unused)))
{
  decode_string (&st->gname, arg);
}

static void
linkpath_coder (struct tar_stat_info const *st, char const *keyword,
		struct xheader *xhdr, void const *data __attribute__ ((unused)))
{
  code_string (st->link_name, keyword, xhdr);
}

static void
linkpath_decoder (struct tar_stat_info *st,
		  char const *keyword __attribute__((unused)),
		  char const *arg,
		  size_t size __attribute__((unused)))
{
  decode_string (&st->link_name, arg);
}

static void
ctime_coder (struct tar_stat_info const *st, char const *keyword,
	     struct xheader *xhdr, void const *data __attribute__ ((unused)))
{
  code_time (st->ctime, keyword, xhdr);
}

static void
ctime_decoder (struct tar_stat_info *st,
	       char const *keyword,
	       char const *arg,
	       size_t size __attribute__((unused)))
{
  struct timespec ts;
  if (decode_time (&ts, arg, keyword))
    st->ctime = ts;
}

static void
mtime_coder (struct tar_stat_info const *st, char const *keyword,
	     struct xheader *xhdr, void const *data)
{
  struct timespec const *mtime = data;
  code_time (mtime ? *mtime : st->mtime, keyword, xhdr);
}

static void
mtime_decoder (struct tar_stat_info *st,
	       char const *keyword,
	       char const *arg,
	       size_t size __attribute__((unused)))
{
  struct timespec ts;
  if (decode_time (&ts, arg, keyword))
    st->mtime = ts;
}

static void
path_coder (struct tar_stat_info const *st, char const *keyword,
	    struct xheader *xhdr, void const *data __attribute__ ((unused)))
{
  code_string (st->file_name, keyword, xhdr);
}

static void
path_decoder (struct tar_stat_info *st,
	      char const *keyword __attribute__((unused)),
	      char const *arg,
	      size_t size __attribute__((unused)))
{
  decode_string (&st->orig_file_name, arg);
  decode_string (&st->file_name, arg);
  st->had_trailing_slash = strip_trailing_slashes (st->file_name);
}

static void
size_coder (struct tar_stat_info const *st, char const *keyword,
	    struct xheader *xhdr, void const *data __attribute__ ((unused)))
{
  code_num (st->stat.st_size, keyword, xhdr);
}

static void
size_decoder (struct tar_stat_info *st,
	      char const *keyword,
	      char const *arg,
	      size_t size __attribute__((unused)))
{
  uintmax_t u;
  if (decode_num (&u, arg, TYPE_MAXIMUM (off_t), keyword))
    st->stat.st_size = u;
}

static void
uid_coder (struct tar_stat_info const *st, char const *keyword,
	   struct xheader *xhdr, void const *data __attribute__ ((unused)))
{
  code_num (st->stat.st_uid, keyword, xhdr);
}

static void
uid_decoder (struct tar_stat_info *st,
	     char const *keyword,
	     char const *arg,
	     size_t size __attribute__((unused)))
{
  uintmax_t u;
  if (decode_num (&u, arg, TYPE_MAXIMUM (uid_t), keyword))
    st->stat.st_uid = u;
}

static void
uname_coder (struct tar_stat_info const *st, char const *keyword,
	     struct xheader *xhdr, void const *data __attribute__ ((unused)))
{
  code_string (st->uname, keyword, xhdr);
}

static void
uname_decoder (struct tar_stat_info *st,
	       char const *keyword __attribute__((unused)),
	       char const *arg,
	       size_t size __attribute__((unused)))
{
  decode_string (&st->uname, arg);
}

static void
sparse_size_coder (struct tar_stat_info const *st, char const *keyword,
	     struct xheader *xhdr, void const *data)
{
  size_coder (st, keyword, xhdr, data);
}

static void
sparse_size_decoder (struct tar_stat_info *st,
		     char const *keyword,
		     char const *arg,
		     size_t size __attribute__((unused)))
{
  uintmax_t u;
  if (decode_num (&u, arg, TYPE_MAXIMUM (off_t), keyword))
    st->stat.st_size = u;
}

static void
sparse_numblocks_coder (struct tar_stat_info const *st, char const *keyword,
			struct xheader *xhdr,
			void const *data __attribute__ ((unused)))
{
  code_num (st->sparse_map_avail, keyword, xhdr);
}

static void
sparse_numblocks_decoder (struct tar_stat_info *st,
			  char const *keyword,
			  char const *arg,
			  size_t size __attribute__((unused)))
{
  uintmax_t u;
  if (decode_num (&u, arg, SIZE_MAX, keyword))
    {
      st->sparse_map_size = u;
      st->sparse_map = xcalloc (u, sizeof st->sparse_map[0]);
      st->sparse_map_avail = 0;
    }
}

static void
sparse_offset_coder (struct tar_stat_info const *st, char const *keyword,
		     struct xheader *xhdr, void const *data)
{
  size_t const *pi = data;
  code_num (st->sparse_map[*pi].offset, keyword, xhdr);
}

static void
sparse_offset_decoder (struct tar_stat_info *st,
		       char const *keyword,
		       char const *arg,
		       size_t size __attribute__((unused)))
{
  uintmax_t u;
  if (decode_num (&u, arg, TYPE_MAXIMUM (off_t), keyword))
    {
      if (st->sparse_map_avail < st->sparse_map_size)
	st->sparse_map[st->sparse_map_avail].offset = u;
      else
	ERROR ((0, 0, _("Malformed extended header: excess %s=%s"),
		"GNU.sparse.offset", arg));
    }
}

static void
sparse_numbytes_coder (struct tar_stat_info const *st, char const *keyword,
		       struct xheader *xhdr, void const *data)
{
  size_t const *pi = data;
  code_num (st->sparse_map[*pi].numbytes, keyword, xhdr);
}

static void
sparse_numbytes_decoder (struct tar_stat_info *st,
			 char const *keyword,
			 char const *arg,
			 size_t size __attribute__((unused)))
{
  uintmax_t u;
  if (decode_num (&u, arg, SIZE_MAX, keyword))
    {
      if (st->sparse_map_avail < st->sparse_map_size)
	st->sparse_map[st->sparse_map_avail++].numbytes = u;
      else
	ERROR ((0, 0, _("Malformed extended header: excess %s=%s"),
		keyword, arg));
    }
}

static void
sparse_map_decoder (struct tar_stat_info *st,
		    char const *keyword,
		    char const *arg,
		    size_t size __attribute__((unused)))
{
  int offset = 1;

  st->sparse_map_avail = 0;
  while (1)
    {
      uintmax_t u;
      char *delim;
      struct sp_array e;

      if (!ISDIGIT (*arg))
	{
	  ERROR ((0, 0, _("Malformed extended header: invalid %s=%s"),
		  keyword, arg));
	  return;
	}

      errno = 0;
      u = strtoumax (arg, &delim, 10);
      if (offset)
	{
	  e.offset = u;
	  if (!(u == e.offset && errno != ERANGE))
	    {
	      out_of_range_header (keyword, arg, 0, TYPE_MAXIMUM (off_t));
	      return;
	    }
	}
      else
	{
	  e.numbytes = u;
	  if (!(u == e.numbytes && errno != ERANGE))
	    {
	      out_of_range_header (keyword, arg, 0, TYPE_MAXIMUM (size_t));
	      return;
	    }
	  if (st->sparse_map_avail < st->sparse_map_size)
	    st->sparse_map[st->sparse_map_avail++] = e;
	  else
	    {
	      ERROR ((0, 0, _("Malformed extended header: excess %s=%s"),
		      keyword, arg));
	      return;
	    }
	}

      offset = !offset;

      if (*delim == 0)
	break;
      else if (*delim != ',')
	{
	  ERROR ((0, 0,
		  _("Malformed extended header: invalid %s: unexpected delimiter %c"),
		  keyword, *delim));
	  return;
	}

      arg = delim + 1;
    }

  if (!offset)
    ERROR ((0, 0,
	    _("Malformed extended header: invalid %s: odd number of values"),
	    keyword));
}

static void
dumpdir_coder (struct tar_stat_info const *st, char const *keyword,
	       struct xheader *xhdr, void const *data)
{
  xheader_print_n (xhdr, keyword, data, dumpdir_size (data));
}

static void
dumpdir_decoder (struct tar_stat_info *st,
		 char const *keyword __attribute__((unused)),
		 char const *arg,
		 size_t size)
{
  st->dumpdir = xmalloc (size);
  memcpy (st->dumpdir, arg, size);
}

static void
volume_label_coder (struct tar_stat_info const *st, char const *keyword,
		    struct xheader *xhdr, void const *data)
{
  code_string (data, keyword, xhdr);
}

static void
volume_label_decoder (struct tar_stat_info *st,
		      char const *keyword __attribute__((unused)),
		      char const *arg,
		      size_t size __attribute__((unused)))
{
  decode_string (&volume_label, arg);
}

static void
volume_size_coder (struct tar_stat_info const *st, char const *keyword,
		   struct xheader *xhdr, void const *data)
{
  off_t const *v = data;
  code_num (*v, keyword, xhdr);
}

static void
volume_size_decoder (struct tar_stat_info *st,
		     char const *keyword,
		     char const *arg, size_t size)
{
  uintmax_t u;
  if (decode_num (&u, arg, TYPE_MAXIMUM (uintmax_t), keyword))
    continued_file_size = u;
}

/* FIXME: Merge with volume_size_coder */
static void
volume_offset_coder (struct tar_stat_info const *st, char const *keyword,
		     struct xheader *xhdr, void const *data)
{
  off_t const *v = data;
  code_num (*v, keyword, xhdr);
}

static void
volume_offset_decoder (struct tar_stat_info *st,
		       char const *keyword,
		       char const *arg, size_t size)
{
  uintmax_t u;
  if (decode_num (&u, arg, TYPE_MAXIMUM (uintmax_t), keyword))
    continued_file_offset = u;
}

static void
volume_filename_decoder (struct tar_stat_info *st,
			 char const *keyword __attribute__((unused)),
			 char const *arg,
			 size_t size __attribute__((unused)))
{
  decode_string (&continued_file_name, arg);
}

static void
sparse_major_coder (struct tar_stat_info const *st, char const *keyword,
		    struct xheader *xhdr, void const *data)
{
  code_num (st->sparse_major, keyword, xhdr);
}

static void
sparse_major_decoder (struct tar_stat_info *st,
		      char const *keyword,
		      char const *arg,
		      size_t size)
{
  uintmax_t u;
  if (decode_num (&u, arg, TYPE_MAXIMUM (unsigned), keyword))
    st->sparse_major = u;
}

static void
sparse_minor_coder (struct tar_stat_info const *st, char const *keyword,
		      struct xheader *xhdr, void const *data)
{
  code_num (st->sparse_minor, keyword, xhdr);
}

static void
sparse_minor_decoder (struct tar_stat_info *st,
		      char const *keyword,
		      char const *arg,
		      size_t size)
{
  uintmax_t u;
  if (decode_num (&u, arg, TYPE_MAXIMUM (unsigned), keyword))
    st->sparse_minor = u;
}

struct xhdr_tab const xhdr_tab[] = {
  { "atime",	atime_coder,	atime_decoder,	  false },
  { "comment",	dummy_coder,	dummy_decoder,	  false },
  { "charset",	dummy_coder,	dummy_decoder,	  false },
  { "ctime",	ctime_coder,	ctime_decoder,	  false },
  { "gid",	gid_coder,	gid_decoder,	  false },
  { "gname",	gname_coder,	gname_decoder,	  false },
  { "linkpath", linkpath_coder, linkpath_decoder, false },
  { "mtime",	mtime_coder,	mtime_decoder,	  false },
  { "path",	path_coder,	path_decoder,	  false },
  { "size",	size_coder,	size_decoder,	  false },
  { "uid",	uid_coder,	uid_decoder,	  false },
  { "uname",	uname_coder,	uname_decoder,	  false },

  /* Sparse file handling */
  { "GNU.sparse.name",       path_coder, path_decoder,
    true },
  { "GNU.sparse.major",      sparse_major_coder, sparse_major_decoder,
    true },
  { "GNU.sparse.minor",      sparse_minor_coder, sparse_minor_decoder,
    true },
  { "GNU.sparse.realsize",   sparse_size_coder, sparse_size_decoder,
    true },
  { "GNU.sparse.numblocks",  sparse_numblocks_coder, sparse_numblocks_decoder,
    true },

  /* tar 1.14 - 1.15.90 keywords. */
  { "GNU.sparse.size",       sparse_size_coder, sparse_size_decoder, true },
  /* tar 1.14 - 1.15.1 keywords. Multiple instances of these appeared in 'x'
     headers, and each of them was meaningful. It confilcted with POSIX specs,
     which requires that "when extended header records conflict, the last one
     given in the header shall take precedence." */
  { "GNU.sparse.offset",     sparse_offset_coder, sparse_offset_decoder,
    true },
  { "GNU.sparse.numbytes",   sparse_numbytes_coder, sparse_numbytes_decoder,
    true },
  /* tar 1.15.90 keyword, introduced to remove the above-mentioned conflict. */
  { "GNU.sparse.map",        NULL /* Unused, see pax_dump_header() */,
    sparse_map_decoder, false },

  { "GNU.dumpdir",           dumpdir_coder, dumpdir_decoder,
    true },

  /* Keeps the tape/volume label. May be present only in the global headers.
     Equivalent to GNUTYPE_VOLHDR.  */
  { "GNU.volume.label", volume_label_coder, volume_label_decoder, true },

  /* These may be present in a first global header of the archive.
     They provide the same functionality as GNUTYPE_MULTIVOL header.
     The GNU.volume.size keeps the real_s_sizeleft value, which is
     otherwise kept in the size field of a multivolume header.  The
     GNU.volume.offset keeps the offset of the start of this volume,
     otherwise kept in oldgnu_header.offset.  */
  { "GNU.volume.filename", volume_label_coder, volume_filename_decoder,
    true },
  { "GNU.volume.size", volume_size_coder, volume_size_decoder, true },
  { "GNU.volume.offset", volume_offset_coder, volume_offset_decoder, true },

  { NULL, NULL, NULL, false }
};
