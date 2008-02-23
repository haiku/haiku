/* Remote connection server.

   Copyright (C) 1994, 1995, 1996, 1997, 1999, 2000, 2001, 2003, 2004,
   2005, 2006, 2007 Free Software Foundation, Inc.

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

/* Copyright (C) 1983 Regents of the University of California.
   All rights reserved.

   Redistribution and use in source and binary forms are permitted provided
   that the above copyright notice and this paragraph are duplicated in all
   such forms and that any documentation, advertising materials, and other
   materials related to such distribution and use acknowledge that the
   software was developed by the University of California, Berkeley.  The
   name of the University may not be used to endorse or promote products
   derived from this software without specific prior written permission.
   THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  */

#include "system.h"
#include "system-ioctl.h"
#include <closeout.h>
#include <configmake.h>
#include <safe-read.h>
#include <full-write.h>
#include <version-etc.h>
#define obstack_chunk_alloc malloc
#define obstack_chunk_free free
#include <obstack.h>
#include <getopt.h>
#include <sys/socket.h>

#ifndef EXIT_FAILURE
# define EXIT_FAILURE 1
#endif
#ifndef EXIT_SUCCESS
# define EXIT_SUCCESS 0
#endif

/* Maximum size of a string from the requesting program.
   It must hold enough for any integer, possibly with a sign.  */
#define	STRING_SIZE (UINTMAX_STRSIZE_BOUND + 1)

const char *program_name;

/* File descriptor of the tape device, or negative if none open.  */
static int tape = -1;

/* Buffer containing transferred data, and its allocated size.  */
static char *record_buffer;
static size_t allocated_size;

/* Buffer for constructing the reply.  */
static char reply_buffer[BUFSIZ];

/* Obstack for arbitrary-sized strings */
struct obstack string_stk;

/* Debugging tools.  */

static FILE *debug_file;

#define	DEBUG(File) \
  if (debug_file) fprintf(debug_file, File)

#define	DEBUG1(File, Arg) \
  if (debug_file) fprintf(debug_file, File, Arg)

#define	DEBUG2(File, Arg1, Arg2) \
  if (debug_file) fprintf(debug_file, File, Arg1, Arg2)

static void
report_error_message (const char *string)
{
  DEBUG1 ("rmtd: E 0 (%s)\n", string);

  sprintf (reply_buffer, "E0\n%s\n", string);
  full_write (STDOUT_FILENO, reply_buffer, strlen (reply_buffer));
}

static void
report_numbered_error (int num)
{
  DEBUG2 ("rmtd: E %d (%s)\n", num, strerror (num));

  sprintf (reply_buffer, "E%d\n%s\n", num, strerror (num));
  full_write (STDOUT_FILENO, reply_buffer, strlen (reply_buffer));
}

static char *
get_string (void)
{
  for (;;)
    {
      char c;
      if (safe_read (STDIN_FILENO, &c, 1) != 1)
	exit (EXIT_SUCCESS);

      if (c == '\n')
	break;

      obstack_1grow (&string_stk, c);
    }
  obstack_1grow (&string_stk, 0);
  return obstack_finish (&string_stk);
}

static void
free_string (char *string)
{
  obstack_free (&string_stk, string);
}

static void
get_string_n (char *string)
{
  size_t counter;

  for (counter = 0; ; counter++)
    {
      if (safe_read (STDIN_FILENO, string + counter, 1) != 1)
	exit (EXIT_SUCCESS);

      if (string[counter] == '\n')
	break;

      if (counter == STRING_SIZE - 1)
	report_error_message (N_("Input string too long"));
    }
  string[counter] = '\0';
}

static long int
get_long (char const *string)
{
  char *p;
  long int n;
  errno = 0;
  n = strtol (string, &p, 10);
  if (errno == ERANGE)
    {
      report_numbered_error (errno);
      exit (EXIT_FAILURE);
    }
  if (!*string || *p)
    {
      report_error_message (N_("Number syntax error"));
      exit (EXIT_FAILURE);
    }
  return n;
}

static void
prepare_input_buffer (int fd, size_t size)
{
  if (size <= allocated_size)
    return;

  if (record_buffer)
    free (record_buffer);

  record_buffer = malloc (size);

  if (! record_buffer)
    {
      DEBUG (_("rmtd: Cannot allocate buffer space\n"));

      report_error_message (N_("Cannot allocate buffer space"));
      exit (EXIT_FAILURE);      /* exit status used to be 4 */
    }

  allocated_size = size;

#ifdef SO_RCVBUF
  if (0 <= fd)
    {
      int isize = size < INT_MAX ? size : INT_MAX;
      while (setsockopt (fd, SOL_SOCKET, SO_RCVBUF,
			 (char *) &isize, sizeof isize)
	     && 1024 < isize)
	isize >>= 1;
    }
#endif
}

/* Decode OFLAG_STRING, which represents the 2nd argument to `open'.
   OFLAG_STRING should contain an optional integer, followed by an optional
   symbolic representation of an open flag using only '|' to separate its
   components (e.g. "O_WRONLY|O_CREAT|O_TRUNC").  Prefer the symbolic
   representation if available, falling back on the numeric
   representation, or to zero if both formats are absent.

   This function should be the inverse of encode_oflag.  The numeric
   representation is not portable from one host to another, but it is
   for backward compatibility with old-fashioned clients that do not
   emit symbolic open flags.  */

static int
decode_oflag (char const *oflag_string)
{
  char *oflag_num_end;
  int numeric_oflag = strtol (oflag_string, &oflag_num_end, 10);
  int symbolic_oflag = 0;

  oflag_string = oflag_num_end;
  while (ISSPACE ((unsigned char) *oflag_string))
    oflag_string++;

  do
    {
      struct name_value_pair { char const *name; int value; };
      static struct name_value_pair const table[] =
      {
#ifdef O_APPEND
	{"APPEND", O_APPEND},
#endif
	{"CREAT", O_CREAT},
#ifdef O_DSYNC
	{"DSYNC", O_DSYNC},
#endif
	{"EXCL", O_EXCL},
#ifdef O_LARGEFILE
	{"LARGEFILE", O_LARGEFILE}, /* LFS extension for opening large files */
#endif
#ifdef O_NOCTTY
	{"NOCTTY", O_NOCTTY},
#endif
#if O_NONBLOCK
	{"NONBLOCK", O_NONBLOCK},
#endif
	{"RDONLY", O_RDONLY},
	{"RDWR", O_RDWR},
#ifdef O_RSYNC
	{"RSYNC", O_RSYNC},
#endif
#ifdef O_SYNC
	{"SYNC", O_SYNC},
#endif
	{"TRUNC", O_TRUNC},
	{"WRONLY", O_WRONLY}
      };
      struct name_value_pair const *t;
      size_t s;

      if (*oflag_string++ != 'O' || *oflag_string++ != '_')
	return numeric_oflag;

      for (t = table;
	   (strncmp (oflag_string, t->name, s = strlen (t->name)) != 0
	    || (oflag_string[s]
		&& strchr ("ABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789",
			   oflag_string[s])));
	   t++)
	if (t == table + sizeof table / sizeof *table - 1)
	  return numeric_oflag;

      symbolic_oflag |= t->value;
      oflag_string += s;
    }
  while (*oflag_string++ == '|');

  return symbolic_oflag;
}

static struct option const long_opts[] =
{
  {"help", no_argument, 0, 'h'},
  {"version", no_argument, 0, 'v'},
  {0, 0, 0, 0}
};

/* In-line localization is used only if --help or --version are
   locally used.  Otherwise, the localization burden lies with tar. */
static void
i18n_setup ()
{
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
}

static void usage (int) __attribute__ ((noreturn));

static void
usage (int status)
{
  i18n_setup ();

  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]\n\
Manipulate a tape drive, accepting commands from a remote process.\n\
\n\
  --version  Output version info.\n\
  --help     Output this help.\n"),
	      program_name);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
      close_stdout ();
    }

  exit (status);
}

static void
respond (long int status)
{
  DEBUG1 ("rmtd: A %ld\n", status);

  sprintf (reply_buffer, "A%ld\n", status);
  full_write (STDOUT_FILENO, reply_buffer, strlen (reply_buffer));
}



static void
open_device (void)
{
  char *device_string = get_string ();
  char *oflag_string = get_string ();

  DEBUG2 ("rmtd: O %s %s\n", device_string, oflag_string);

  if (tape >= 0)
    close (tape);

  tape = open (device_string, decode_oflag (oflag_string), MODE_RW);
  if (tape < 0)
    report_numbered_error (errno);
  else
    respond (0);
  free_string (device_string);
  free_string (oflag_string);
}

static void
close_device (void)
{
  free_string (get_string ()); /* discard */
  DEBUG ("rmtd: C\n");

  if (close (tape) < 0)
    report_numbered_error (errno);
  else
    {
      tape = -1;
      respond (0);
    }
}

static void
lseek_device (void)
{
  char count_string[STRING_SIZE];
  char position_string[STRING_SIZE];
  off_t count = 0;
  int negative;
  int whence;
  char *p;

  get_string_n (count_string);
  get_string_n (position_string);
  DEBUG2 ("rmtd: L %s %s\n", count_string, position_string);

  /* Parse count_string, taking care to check for overflow.
     We can't use standard functions,
     since off_t might be longer than long.  */

  for (p = count_string;  *p == ' ' || *p == '\t';  p++)
    ;

  negative = *p == '-';
  p += negative || *p == '+';

  for (; *p; p++)
    {
      int digit = *p - '0';
      if (9 < (unsigned) digit)
	{
	  report_error_message (N_("Seek offset error"));
	  exit (EXIT_FAILURE);
	}
      else
	{
	  off_t c10 = 10 * count;
	  off_t nc = negative ? c10 - digit : c10 + digit;
	  if (c10 / 10 != count || (negative ? c10 < nc : nc < c10))
	    {
	      report_error_message (N_("Seek offset out of range"));
	      exit (EXIT_FAILURE);
	    }
	  count = nc;
	}
    }

  switch (get_long (position_string))
    {
    case 0:
      whence = SEEK_SET;
      break;

    case 1:
      whence = SEEK_CUR;
      break;

    case 2:
      whence = SEEK_END;
      break;

    default:
      report_error_message (N_("Seek direction out of range"));
      exit (EXIT_FAILURE);
    }

  count = lseek (tape, count, whence);
  if (count < 0)
    report_numbered_error (errno);
  else
    {
      /* Convert count back to string for reply.
	 We can't use sprintf, since off_t might be longer
	 than long.  */
      p = count_string + sizeof count_string;
      *--p = '\0';
      do
	*--p = '0' + (int) (count % 10);
      while ((count /= 10) != 0);

      DEBUG1 ("rmtd: A %s\n", p);

      sprintf (reply_buffer, "A%s\n", p);
      full_write (STDOUT_FILENO, reply_buffer, strlen (reply_buffer));
    }
}

static void
write_device (void)
{
  char count_string[STRING_SIZE];
  size_t size;
  size_t counter;
  size_t status = 0;

  get_string_n (count_string);
  size = get_long (count_string);
  DEBUG1 ("rmtd: W %s\n", count_string);

  prepare_input_buffer (STDIN_FILENO, size);
  for (counter = 0; counter < size; counter += status)
    {
      status = safe_read (STDIN_FILENO, &record_buffer[counter],
			  size - counter);
      if (status == SAFE_READ_ERROR || status == 0)
	{
	  DEBUG (_("rmtd: Premature eof\n"));

	  report_error_message (N_("Premature end of file"));
	  exit (EXIT_FAILURE); /* exit status used to be 2 */
	}
    }
  status = full_write (tape, record_buffer, size);
  if (status != size)
    report_numbered_error (errno);
  else
    respond (status);
}

static void
read_device (void)
{
  char count_string[STRING_SIZE];
  size_t size;
  size_t status;

  get_string_n (count_string);
  DEBUG1 ("rmtd: R %s\n", count_string);

  size = get_long (count_string);
  prepare_input_buffer (-1, size);
  status = safe_read (tape, record_buffer, size);
  if (status == SAFE_READ_ERROR)
    report_numbered_error (errno);
  else
    {
      sprintf (reply_buffer, "A%lu\n", (unsigned long int) status);
      full_write (STDOUT_FILENO, reply_buffer, strlen (reply_buffer));
      full_write (STDOUT_FILENO, record_buffer, status);
    }
}

static void
mtioctop (void)
{
  char operation_string[STRING_SIZE];
  char count_string[STRING_SIZE];

  get_string_n (operation_string);
  get_string_n (count_string);
  DEBUG2 ("rmtd: I %s %s\n", operation_string, count_string);

#ifdef MTIOCTOP
  {
    struct mtop mtop;
    const char *p;
    off_t count = 0;
    int negative;

    /* Parse count_string, taking care to check for overflow.
       We can't use standard functions,
       since off_t might be longer than long.  */

    for (p = count_string;  *p == ' ' || *p == '\t';  p++)
      ;

    negative = *p == '-';
    p += negative || *p == '+';

    for (;;)
      {
	int digit = *p++ - '0';
	if (9 < (unsigned) digit)
	  break;
	else
	  {
	    off_t c10 = 10 * count;
	    off_t nc = negative ? c10 - digit : c10 + digit;
	    if (c10 / 10 != count
		|| (negative ? c10 < nc : nc < c10))
	      {
		report_error_message (N_("Seek offset out of range"));
		exit (EXIT_FAILURE);
	      }
	    count = nc;
	  }
      }

    mtop.mt_count = count;
    if (mtop.mt_count != count)
      {
	report_error_message (N_("Seek offset out of range"));
	exit (EXIT_FAILURE);
      }
    mtop.mt_op = get_long (operation_string);

    if (ioctl (tape, MTIOCTOP, (char *) &mtop) < 0)
      {
	report_numbered_error (errno);
	return;
      }
  }
#endif
  respond (0);
}

static void
status_device (void)
{
  DEBUG ("rmtd: S\n");

#ifdef MTIOCGET
  {
    struct mtget operation;

    if (ioctl (tape, MTIOCGET, (char *) &operation) < 0)
      report_numbered_error (errno);
    else
      {
	respond (sizeof operation);
	full_write (STDOUT_FILENO, (char *) &operation, sizeof operation);
      }
  }
#endif
}

int
main (int argc, char **argv)
{
  char command;

  program_name = argv[0];

  obstack_init (&string_stk);

  switch (getopt_long (argc, argv, "", long_opts, NULL))
    {
    default:
      usage (EXIT_FAILURE);

    case 'h':
      usage (EXIT_SUCCESS);

    case 'v':
      i18n_setup ();
      version_etc (stdout, "rmt", PACKAGE_NAME, PACKAGE_VERSION,
		   "John Gilmore", "Jay Fenlason", (char *) NULL);
      close_stdout ();
      return EXIT_SUCCESS;

    case -1:
      break;
    }

  if (optind < argc)
    {
      if (optind != argc - 1)
	usage (EXIT_FAILURE);
      debug_file = fopen (argv[optind], "w");
      if (debug_file == 0)
	{
	  report_numbered_error (errno);
	  return EXIT_FAILURE;
	}
      setbuf (debug_file, 0);
    }

  while (1)
    {
      errno = 0;

      if (safe_read (STDIN_FILENO, &command, 1) != 1)
	return EXIT_SUCCESS;

      switch (command)
	{
	case 'O':
	  open_device ();
	  break;

	case 'C':
	  close_device ();
	  break;

	case 'L':
	  lseek_device ();
	  break;

	case 'W':
	  write_device ();
	  break;

	case 'R':
	  read_device ();
	  break;

	case 'I':
	  mtioctop ();
	  break;

	case 'S':
	  status_device ();
	  break;

	default:
	  DEBUG1 ("rmtd: Garbage command %c\n", command);
	  report_error_message (N_("Garbage command"));
	  return EXIT_FAILURE;	/* exit status used to be 3 */
	}
    }
}
