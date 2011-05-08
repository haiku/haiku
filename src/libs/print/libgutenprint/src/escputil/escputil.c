/*
 * "$Id: escputil.c,v 1.104 2010/12/11 22:04:07 rlk Exp $"
 *
 *   Printer maintenance utility for EPSON Stylus (R) printers
 *
 *   Copyright 2000-2003 Robert Krawitz (rlk@alum.mit.edu)
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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>
#if defined(HAVE_VARARGS_H) && !defined(HAVE_STDARG_H)
#include <varargs.h>
#else
#include <stdarg.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_POLL
#include <sys/poll.h>
#endif
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#endif
#ifdef HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#endif
#include <gutenprint/gutenprint-intl-internal.h>
#include "d4lib.h"

void do_align(void);
char *do_get_input (const char *prompt);
void do_head_clean(void);
void do_help(int code);
void do_identify(void);
void do_ink_level(void);
void do_extended_ink_info(int);
void do_nozzle_check(void);
void do_status(void);
int do_print_cmd(void);


const char *banner = N_("\
Escputil version " VERSION ", Copyright (C) 2000-2006 Robert Krawitz\n\
Escputil comes with ABSOLUTELY NO WARRANTY; for details type 'escputil -l'\n\
This is free software, and you are welcome to redistribute it\n\
under certain conditions; type 'escputil -l' for details.\n");

const char *license = N_("\
Copyright 2000-2006 Robert Krawitz (rlk@alum.mit.edu)\n\
\n\
This program is free software; you can redistribute it and/or modify it\n\
under the terms of the GNU General Public License as published by the Free\n\
Software Foundation; either version 2 of the License, or (at your option)\n\
any later version.\n\
\n\
This program is distributed in the hope that it will be useful, but\n\
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY\n\
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License\n\
for more details.\n\
\n\
You should have received a copy of the GNU General Public License\n\
along with this program; if not, write to the Free Software\n\
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n");


#if defined(HAVE_GETOPT_H) && defined(HAVE_GETOPT_LONG)

struct option optlist[] =
{
  { "printer-name",		1,	NULL,	(int) 'P' },
  { "raw-device",		1,	NULL,	(int) 'r' },
  { "ink-level",		0,	NULL,	(int) 'i' },
  { "extended-ink-info",	0,	NULL,	(int) 'e' },
  { "clean-head",		0,	NULL,	(int) 'c' },
  { "nozzle-check",		0,	NULL,	(int) 'n' },
  { "align-head",		0,	NULL,	(int) 'a' },
  { "status",           	0,      NULL,   (int) 's' },
  { "new",			0,	NULL,	(int) 'u' },
  { "help",			0,	NULL,	(int) 'h' },
  { "identify",			0,	NULL,	(int) 'd' },
  { "model",			1,	NULL,	(int) 'm' },
  { "quiet",			0,	NULL,	(int) 'q' },
  { "license",			0,	NULL,	(int) 'l' },
  { "list-models",		0,	NULL,	(int) 'M' },
  { "short-name",		0,	NULL,	(int) 'S' },
  { "choices",			0,	NULL,	(int) 'C' },
  { "patterns",			0,	NULL,	(int) 'p' },
  { NULL,			0,	NULL,	0 	  }
};

const char *help_msg = N_("\
Usage: escputil [-c | -n | -a | -i | -e | -s | -d | -l | -M]\n\
                [-P printer | -r device] [-u] [-q] [-m model] [ -S ]\n\
                [-C choices] [-p patterns]\n\
Perform maintenance on EPSON Stylus (R) printers.\n\
Examples: escputil --ink-level --raw-device /dev/usb/lp0\n\
          escputil --clean-head --new --printer-name MyQueue\n\
\n\
  Commands:\n\
    -c|--clean-head    Clean the print head.\n\
    -n|--nozzle-check  Print a nozzle test pattern.\n\
                       Dirty or clogged nozzles will show as gaps in the\n\
                       pattern.  If you see any gaps, you should clean\n\
                       the print head.\n\
    -a|--align-head    Align the print head.  CAUTION: Misuse of this\n\
                       utility may result in poor print quality and/or\n\
                       damage to the printer.\n\
    -s|--status        Retrieve printer status.\n\
    -i|--ink-level     Obtain the ink level from the printer.  This requires\n\
                       read/write access to the raw printer device.\n\
    -e|--extended-ink-info     Obtain the extended ink information from the\n\
                       printer.  This requires read/write access to the raw\n\
                       printer device.\n\
    -d|--identify      Query the printer for make and model information.\n\
                       This requires read/write access to the raw printer\n\
                       device.\n\
    -l|--license       Display the license/warranty terms of this program.\n\
    -M|--list-models   List the available printer models.\n\
    -h|--help          Print this help message.\n\
  Options:\n\
    -P|--printer-name  Specify the name of the printer queue to operate on.\n\
                       Default is the default system printer.\n\
    -r|--raw-device    Specify the name of the device to write to directly\n\
                       rather than going through a printer queue.\n\
    -m|--model         Specify the printer model.\n\
    -u|--new           The printer is a new printer (Stylus Color 740 or\n\
                       newer).  Only needed when not using a raw device or\n\
                       when the model is not specified.\n\
    -q|--quiet         Suppress the banner.\n\
    -S|--short-name    Print the short name of the printer with --identify.\n\
    -C|--choices       Specify the number of pattern choices for alignment\n\
    -p|--patterns      Specify the number of sets of patterns for alignment\n");
#else
const char *help_msg = N_("\
Usage: escputil [OPTIONS] [COMMAND]\n\
Usage: escputil [-c | -n | -a | -i | -e | -s | -d | -l | -M]\n\
                [-P printer | -r device] [-u] [-q] [-m model] [ -S ]\n\
                [-C choices] [-p patterns]\n\
Perform maintenance on EPSON Stylus (R) printers.\n\
Examples: escputil -i -r /dev/usb/lp0\n\
          escputil -c -u -P MyQueue\n\
\n\
  Commands:\n\
    -c Clean the print head.\n\
    -n Print a nozzle test pattern.\n\
          Dirty or clogged nozzles will show as gaps in the\n\
          pattern.  If you see any gaps, you should clean\n\
          the print head.\n\
    -a Align the print head.  CAUTION: Misuse of this\n\
          utility may result in poor print quality and/or\n\
          damage to the printer.\n\
    -s Retrieve printer status.\n\
    -i Obtain the ink level from the printer.  This requires\n\
          read/write access to the raw printer device.\n\
    -e Obtain the extended ink information from the printer.\n\
          Only for R800 printer and friends. This requires\n\
          read/write access to the raw printer device.\n\
    -d Query the printer for make and model information.  This\n\
          requires read/write access to the raw printer device.\n\
    -l Display the license/warranty terms of this program.\n\
    -M List the available printer models.\n\
    -h Print this help message.\n\
  Options:\n\
    -P Specify the name of the printer queue to operate on.\n\
          Default is the default system printer.\n\
    -r Specify the name of the device to write to directly\n\
          rather than going through a printer queue.\n\
    -u The printer is a new printer (Stylus Color 740 or newer).\n\
          Only needed when not using a raw device.\n\
    -q Suppress the banner.\n\
    -S Print the short name of the printer with -d.\n\
    -m Specify the precise printer model for head alignment.\n\
    -C Specify the number of pattern choices for alignment\n\
    -p Specify the number of sets of patterns for alignment\n");
#endif

char *the_printer = NULL;
char *raw_device = NULL;
char *printer_model = NULL;
char printer_cmd[1025];
int bufpos = 0;
int isnew = 0;
int interchangeable_inks = 0;
int found_unknown_old_printer = 0;
int print_short_name = 0;
const stp_printer_t *the_printer_t = NULL;
int printer_was_in_packet_mode = 0;
int alignment_passes = 0;
int alignment_choices = 0;

static int stp_debug = 0;
#define STP_DEBUG(x) do { if (stp_debug || getenv("STP_DEBUG")) x; } while (0)
static int send_size = 0x0200;
static int receive_size = 0x0200;
int socket_id = -1;

typedef enum
  {
    CMD_INK_LEVEL,
    CMD_STATUS
  } status_cmd_t;

static char *
c_strdup(const char *s)
{
  char *ret = stp_malloc(strlen(s) + 1);
  strcpy(ret, s);
  return ret;
}

static void
print_models(void)
{
  int printer_count = stp_printer_model_count();
  int i;
  for (i = 0; i < printer_count; i++)
    {
      const stp_printer_t *printer = stp_get_printer_by_index(i);
      if (strcmp(stp_printer_get_family(printer), "escp2") == 0)
	{
	  printf("%-15s %s\n", stp_printer_get_driver(printer),
		 gettext(stp_printer_get_long_name(printer)));
	}
    }
}

void
do_help(int code)
{
  printf("%s", gettext(help_msg));
  exit(code);
}

static void
exit_packet_mode_old(int do_init)
{
  static char hdr[] = "\000\000\000\033\001@EJL 1284.4\n@EJL     \n\033@";
  memcpy(printer_cmd + bufpos, hdr, sizeof(hdr) - 1); /* DON'T include null! */
  STP_DEBUG(printf("Exit packet mode (%d)\n", do_init));
  bufpos += sizeof(hdr) - 1;
  if (!do_init)
    bufpos -= 2;
}

static void
initialize_print_cmd(int do_init)
{
  bufpos = 0;
  STP_DEBUG(printf("Initialize print command\n"));
  if (isnew)
    exit_packet_mode_old(do_init);
}

static void
initialize_print_cmd_new(int do_init)
{
  bufpos = 0;
  STP_DEBUG(printf("Initialize print command (force new)\n"));
  exit_packet_mode_old(do_init);
}

int
main(int argc, char **argv)
{
  int quiet = 0;
  int operation = 0;
  int c;

  /* Set up gettext */
#ifdef HAVE_LOCALE_H
  char *locale = stp_strdup(setlocale (LC_ALL, ""));
#endif
#ifdef ENABLE_NLS
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
#endif

  stp_init();
  if (getenv("STP_DEBUG"))
    stp_debug = 1;
  else
    stp_debug = 0;
  setDebug(stp_debug);
  while (1)
    {
#if defined(HAVE_GETOPT_H) && defined(HAVE_GETOPT_LONG)
      int option_index = 0;
      c = getopt_long(argc, argv, "P:r:iecnasduqm:hlMSC:p:", optlist, &option_index);
#else
      c = getopt(argc, argv, "P:r:iecnasduqm:hlMSC:p:");
#endif
      if (c == -1)
	break;
      switch (c)
	{
	case 'q':
	  quiet = 1;
	  break;
	case 'c':
	case 'i':
	case 'e':
	case 'n':
	case 'a':
	case 'd':
	case 's':
	case 'o':
	  if (operation)
	    do_help(1);
	  operation = c;
	  break;
	case 'P':
	  if (the_printer || raw_device)
	    {
	      printf(_("You may only specify one printer or raw device."));
	      do_help(1);
	    }
	  the_printer = c_strdup(optarg);
	  break;
	case 'r':
	  if (the_printer || raw_device)
	    {
	      printf(_("You may only specify one printer or raw device."));
	      do_help(1);
	    }
	  raw_device = c_strdup(optarg);
	  break;
	case 'm':
	  if (printer_model)
	    {
	      printf(_("You may only specify one printer model."));
	      do_help(1);
	    }
	  printer_model = c_strdup(optarg);
	  break;
	case 'u':
	  isnew = 1;
	  break;
	case 'h':
	  do_help(0);
	  break;
	case 'l':
	  printf("%s\n", gettext(license));
	  exit(0);
	case 'M':
	  print_models();
	  exit(0);
	case 'S':
	  print_short_name = 1;
	  break;
	case 'C':
	  alignment_choices = atoi(optarg);
	  if (alignment_choices < 1)
	    {
	      printf(_("Alignment choices must be at least 1."));
	      do_help(1);
	    }
	  break;
	case 'p':
	  alignment_passes = atoi(optarg);
	  if (alignment_passes < 1)
	    {
	      printf(_("Alignment passes must be at least 1."));
	      do_help(1);
	    }
	  break;
	default:
	  printf("%s\n", gettext(banner));
	  fprintf(stderr, _("Unknown option %c\n"), c);
	  do_help(1);
	}
    }
  if (!quiet)
    printf("%s\n", banner);
  if (operation == 0)
    {
      fprintf(stderr, _("Usage: %s [OPTIONS] command\n"), argv[0]);
#ifdef __GNU_LIBRARY__
      fprintf(stderr, _("Type `%s --help' for more information.\n"), argv[0]);
#else
      fprintf(stderr, _("Type `%s -h' for more information.\n"), argv[0]);
#endif
      exit(1);
    }
  initialize_print_cmd(1);
  switch(operation)
    {
    case 'c':
      do_head_clean();
      break;
    case 'n':
      do_nozzle_check();
      break;
    case 'i':
      do_ink_level();
      break;
    case 'e':
      do_extended_ink_info(1);
      break;
    case 'a':
      do_align();
      break;
    case 'd':
      do_identify();
      break;
    case 's':
      do_status();
      break;
    default:
      do_help(1);
    }
  exit(0);
}

static void
print_debug_data(const char *buf, size_t count)
{
  int i;
  for (i = 0; i < count; i++)
    {
      if (i % 16 == 0)
	printf("\n%4d: ", i);
      else if (i % 4 == 0)
	printf(" ");
      if (isgraph(buf[i]))
	printf("  %c", (unsigned) ((unsigned char) buf[i]));
      else
	printf(" %02x", (unsigned) ((unsigned char) buf[i]));
    }
  printf("\n");
}

int
do_print_cmd(void)
{
  FILE *pfile;
  int bytes = 0;
  int retries = 0;
  char command[1024];
  memcpy(printer_cmd + bufpos, "\f\033\000\033\000", 5);
  bufpos += 5;
  if (raw_device)
    {
      pfile = fopen(raw_device, "wb");
      if (!pfile)
	{
	  fprintf(stderr, _("Cannot open device %s: %s\n"), raw_device,
		  strerror(errno));
	  return 1;
	}
    }
  else
    {
      if (!access("/bin/lpr", X_OK) ||
          !access("/usr/bin/lpr", X_OK) ||
          !access("/usr/bsd/lpr", X_OK))
        {
        if (the_printer == NULL)
          strcpy(command, "lpr -l");
	else
          snprintf(command, 1023, "lpr -P%s -l", the_printer);
        }
      else if (the_printer == NULL)
	strcpy(command, "lp -s -oraw");
      else
	snprintf(command, 1023, "lp -s -oraw -d%s", the_printer);

      if ((pfile = popen(command, "w")) == NULL)
	{
	  fprintf(stderr, _("Cannot print to printer %s with %s\n"),
		  the_printer, command);
	  return 1;
	}
    }
  STP_DEBUG(printf("Sending print command to %s:",
		    raw_device ? raw_device : command));
  STP_DEBUG(print_debug_data(printer_cmd, bufpos));
  while (bytes < bufpos)
    {
      int status = fwrite(printer_cmd + bytes, 1, bufpos - bytes, pfile);
      if (status == 0)
	{
	  retries++;
	  if (retries > 2)
	    {
	      fprintf(stderr, _("Unable to send command to printer\n"));
	      if (raw_device)
		fclose(pfile);
	      else
		pclose(pfile);
	      return 1;
	    }
	}
      else if (status == -1)
	{
	  fprintf(stderr, _("Unable to send command to printer\n"));
	  if (raw_device)
	    fclose(pfile);
	  else
	    pclose(pfile);
	  return 1;
	}
      else
	{
	  bytes += status;
	  retries = 0;
	}
    }
  if (raw_device)
    fclose(pfile);
  else
    pclose(pfile);
  return 0;
}

static int
read_from_printer(int fd, char *buf, int bufsize, int quiet)
{
#ifdef HAVE_POLL
  struct pollfd ufds;
#endif
  int status;
  int retry = 100;
#ifdef HAVE_FCNTL_H
  fcntl(fd, F_SETFL,
	O_NONBLOCK | (fcntl(fd, F_GETFL) & ~(O_RDONLY|O_WRONLY|O_RDWR)));
#endif
  memset(buf, 0, bufsize);

  do
    {
#ifdef HAVE_POLL
      ufds.fd = fd;
      ufds.events = POLLIN;
      ufds.revents = 0;
      if ((status = poll(&ufds, 1, 1000)) < 0)
	break;
#endif
      status = read(fd, buf, bufsize - 1);
      if (status == 0 || (status < 0 && errno == EAGAIN))
	{
	  usleep(2000);
	  status = 0; /* not an error (read would have blocked) */
	}
    }
  while ((status == 0) && (--retry != 0));

  if (status > 0)
    {
      STP_DEBUG(printf("read_from_printer returns %d\n", status));
      STP_DEBUG(print_debug_data(buf, status));
    }
  else if (status == 0 && retry == 0)
    {
      if (!quiet)
	fprintf(stderr, _("Read from printer timed out\n"));
    }
  else if (status < 0)
    {
      if (!quiet)
	fprintf(stderr, _("Cannot read from %s: %s\n"), raw_device, strerror(errno));
    }

  return status;
}

static void
start_remote_sequence(void)
{
  static char remote_hdr[] = "\033@\033(R\010\000\000REMOTE1";
  memcpy(printer_cmd + bufpos, remote_hdr, sizeof(remote_hdr) - 1);
  bufpos += sizeof(remote_hdr) - 1;
  STP_DEBUG(printf("Start remote sequence\n"));
}

static void
end_remote_sequence(void)
{  
  static char remote_trailer[] = "\033\000\000\000\033\000";
  memcpy(printer_cmd + bufpos, remote_trailer, sizeof(remote_trailer) - 1);
  bufpos += sizeof(remote_trailer) - 1;
  STP_DEBUG(printf("End remote sequence\n"));
}

static void
do_remote_cmd(const char *cmd, int nargs, ...)
{
  int i;
  va_list args;
  va_start(args, nargs);

  start_remote_sequence();
  memcpy(printer_cmd + bufpos, cmd, 2);
  STP_DEBUG(printf("Remote command: %s", cmd));
  bufpos += 2;
  printer_cmd[bufpos] = nargs % 256;
  printer_cmd[bufpos + 1] = (nargs >> 8) % 256;
  STP_DEBUG(printf(" %02x %02x",
		    (unsigned) printer_cmd[bufpos],
		    (unsigned) printer_cmd[bufpos + 1]));
  if (nargs > 0)
    for (i = 0; i < nargs; i++)
      {
	printer_cmd[bufpos + 2 + i] = va_arg(args, int);
	STP_DEBUG(printf(" %02x",
			  (unsigned) printer_cmd[bufpos + 2 + i]));
      }
  STP_DEBUG(printf("\n"));
  bufpos += 2 + nargs;
  end_remote_sequence();
}

static void
do_remote_cmd_only(const char *cmd, int nargs, ...)
{
  int i;
  va_list args;
  va_start(args, nargs);

  memcpy(printer_cmd + bufpos, cmd, 2);
  STP_DEBUG(printf("Remote command: %s", cmd));
  bufpos += 2;
  printer_cmd[bufpos] = nargs % 256;
  printer_cmd[bufpos + 1] = (nargs >> 8) % 256;
  STP_DEBUG(printf(" %02x %02x",
		    (unsigned) printer_cmd[bufpos],
		    (unsigned) printer_cmd[bufpos + 1]));
  if (nargs > 0)
    for (i = 0; i < nargs; i++)
      {
	printer_cmd[bufpos + 2 + i] = va_arg(args, int);
	STP_DEBUG(printf(" %02x",
			  (unsigned) printer_cmd[bufpos + 2 + i]));
      }
  STP_DEBUG(printf("\n"));
  bufpos += 2 + nargs;
}

static void
add_resets(int count)
{
  int i;
  STP_DEBUG(printf("Add %d resets\n", count));
  for (i = 0; i < count; i++)
    {
      printer_cmd[bufpos++] = '\033';
      printer_cmd[bufpos++] = '\000';
    }
}

static int
send_nulls(int fd)
{
  char buf[16384];
  (void) memset(buf, 0, sizeof(buf));
  (void) write(fd, buf, sizeof(buf));
}

static int
init_packet(int fd, int force)
{
  int status;
  int tries = 0;
  STP_DEBUG(printf("Init packet mode %d\n", force));

 Loop:
  if (!force)
    {
      STP_DEBUG(printf("Flushing early data...\n"));
      flushData(fd, (unsigned char) -1);
    }

  STP_DEBUG(printf("EnterIEEE...\n"));
  if (!EnterIEEE(fd))
    {
      STP_DEBUG(printf("EnterIEEE failed!\n"));
      if (tries++ < 5)
	{
	  STP_DEBUG(printf("Retrying\n"));
	  send_nulls(fd);
	  flushData(fd, (unsigned char) -1);
	  goto Loop;
	}
      return 1;
    }
  STP_DEBUG(printf("Init...\n"));
  if (!Init(fd))
    {
      STP_DEBUG(printf("Init failed!\n"));
      if (tries++ < 5)
	{
	  STP_DEBUG(printf("Retrying\n"));
	  send_nulls(fd);
	  flushData(fd, (unsigned char) -1);
	  goto Loop;
	}
      return 1;
    }

  STP_DEBUG(printf("GetSocket...\n"));
  socket_id = GetSocketID(fd, "EPSON-CTRL");
  if (!socket_id)
    {
      STP_DEBUG(printf("GetSocket failed!\n"));
      return 1;
    }
  STP_DEBUG(printf("OpenChannel...\n"));
  switch ( OpenChannel(fd, socket_id, &send_size, &receive_size) )
    {
    case -1:
      STP_DEBUG(printf("Fatal Error return 1\n"));
      return 1; /* unrecoverable error */
      break;
    case  0:
      STP_DEBUG(printf("Error\n")); /* recoverable error ? */
      return 1;
      break;
    }

  status = 1;
  STP_DEBUG(printf("Flushing data...\n"));
  flushData(fd, socket_id);
  return 0;
}

static volatile int alarm_interrupt;

static void
alarm_handler(int sig)
{
  alarm_interrupt = 1;
}

static int
open_raw_device(void)
{
  int fd;

  if (!raw_device)
   {
      fprintf(stderr, _("Please specify a raw device\n"));
      exit(1);
    }

  fd = open(raw_device, O_RDWR, 0666);
  if (fd == -1)
    {
      fprintf(stderr, _("Cannot open %s read/write: %s\n"), raw_device,
              strerror(errno));
      exit(1);
    }

  return fd;
}

static void
set_printer_model(void)
{
  int i = 0;
  int printer_count = stp_printer_model_count();
  for (i = 0; i < printer_count; i++)
    {
      the_printer_t = stp_get_printer_by_index(i);

      if (strcmp(stp_printer_get_family(the_printer_t), "escp2") == 0)
	{
	  const char *short_name = stp_printer_get_driver(the_printer_t);
	  const char *long_name = stp_printer_get_long_name(the_printer_t);
	  if (!strcasecmp(printer_model, short_name) ||
	      !strcasecmp(printer_model, long_name) ||
	      (!strncmp(short_name, "escp2-", strlen("escp2-")) &&
	       !strcasecmp(printer_model, short_name + strlen("escp2-"))) ||
	      (!strncasecmp(long_name, "Epson ", strlen("Epson ")) &&
	       !strcasecmp(printer_model, long_name + strlen("Epson "))))
	    {
	      const stp_vars_t *printvars;
	      stp_parameter_t desc;

	      printvars = stp_printer_get_defaults(the_printer_t);
	      stp_describe_parameter(printvars, "SupportsPacketMode",
				     &desc);
	      if (desc.p_type == STP_PARAMETER_TYPE_BOOLEAN)
		isnew = desc.deflt.boolean;
	      stp_parameter_description_destroy(&desc);
	      stp_describe_parameter(printvars, "InterchangeableInk",
				     &desc);
	      if (desc.p_type == STP_PARAMETER_TYPE_BOOLEAN)
		interchangeable_inks = desc.deflt.boolean;
	      stp_parameter_description_destroy(&desc);
	      STP_DEBUG(printf("Found it! %s\n", printer_model));
	      return;
	    }
	}
    }
  fprintf(stderr, _("Unknown printer %s!\n"), printer_model);
  the_printer_t = NULL;
}

static int
test_for_di(const unsigned char *buf)
{
  return !(strncmp("di", (const char *)buf, 2) &&
	   strncmp("@EJL ID", (const char *)buf, 7));
}

static int
test_for_st(const unsigned char *buf)
{
  return !(strncmp("st", (const char *)buf, 2) &&
	   strncmp("@BDC ST", (const char *)buf, 7));
}

static int
test_for_ii(const unsigned char *buf)
{
  return !(strncmp("ii", (const char *)buf, 2) &&
	   strncmp("@BDC PS", (const char *)buf, 7));
}

static const stp_printer_t *
initialize_printer(int quiet, int fail_if_not_found)
{
  int packet_initialized = 0;
  int fd;
  int tries = 0;
  int status;
  int forced_packet_mode = 0;
  char* pos;
  char* spos;
  unsigned char buf[1024];
  const char init_str[] = "\033\1@EJL ID\r\n";

  quiet = 0;

  fd = open_raw_device();

  if (!printer_model)
    {
      do
	{
	  alarm_interrupt = 0;
	  signal(SIGALRM, alarm_handler);
	  alarm(5);
	  status = SafeWrite(fd, init_str, sizeof(init_str) - 1);
	  alarm(0);
	  signal(SIGALRM, SIG_DFL);
	  STP_DEBUG(printf("status %d alarm %d\n", status, alarm_interrupt));
	  if (status != sizeof(init_str) - 1 && (status != -1 || !alarm_interrupt))
	    {
	      fprintf(stderr, _("Cannot write to %s: %s\n"), raw_device,
		      strerror(errno));
	      exit(1);
	    }
	  STP_DEBUG(printf("Try old command %d alarm %d\n",
			    tries, alarm_interrupt));
	  status = read_from_printer(fd, (char*)buf, 1024, 1);
	  if (status <= 0 && tries > 0)
	    {
	      forced_packet_mode = !init_packet(fd, 1);
	      status = 1;
	    }
	  if (!forced_packet_mode &&
	      status > 0 && !strstr((char *) buf, "@EJL ID") && tries < 3)
	    {
	      STP_DEBUG(printf("Found bad data: %s\n", buf));
	      /*
	       * We know the printer's not dead.  Try to turn off status
	       * and try again.
	       */
	      initialize_print_cmd(1);
	      do_remote_cmd("ST", 2, 0, 0);
	      add_resets(2);
	      (void) SafeWrite(fd, printer_cmd, bufpos);
	      status = 0;
	    }
	  tries++;
	} while (status <= 0);

      if (forced_packet_mode || ((buf[3] == status) && (buf[6] == 0x7f)))
	{
	  /*
	   * NX100 looks like it's in D4 mode, but it doesn't really
	   * respond correctly.  So we force it out of D4 mode and
	   * then back in to ensure that it's right.  Trying to force
	   * it into D4 mode alone isn't good enough.
	   */
	  initialize_print_cmd_new(0);
	  (void) SafeWrite(fd, printer_cmd, bufpos);
	  flushData(fd, (unsigned char) -1);
	  forced_packet_mode = !init_packet(fd, 1);
	  STP_DEBUG(printf("Printer in packet mode....\n"));
	  packet_initialized = 1;
	  isnew = 1;
	  /* request status command */
	  status =
	    writeAndReadData(fd, socket_id, (const unsigned char*)"di\1\0\1",
			     5, 1, (unsigned char *) buf, 1023,
			     &send_size, &receive_size, &test_for_di);
	  if (status <= 0)
	    {
	      fprintf(stderr, _("\nCannot write to %s: %s\n"),
		      raw_device, strerror(errno));
	      close(fd);
	      return NULL;
	    }
	}
      STP_DEBUG(printf("status: %i\n", status));
      STP_DEBUG(printf("Buf: %s\n", buf));
      if (status > 0)
	{
	  pos = strstr((char*)buf, "@EJL ID");
	  STP_DEBUG(printf("pos: %s\n", pos ? pos : "(null)"));
	  if (pos)
	    pos = strchr(pos, (int) ';');
	  STP_DEBUG(printf("pos: %s\n", pos ? pos : "(null)"));
	  if (pos)
	    pos = strchr(pos + 1, (int) ';');
	  STP_DEBUG(printf("pos: %s\n", pos ? pos : "(null)"));
	  if (pos)
	    pos = strchr(pos, (int) ':');
	  STP_DEBUG(printf("pos: %s\n", pos ? pos : "(null)"));
	  if (pos)
	    {
	      spos = strchr(pos, (int) ';');
	    }
	  if (!pos)
	    {
	      if (!quiet && fail_if_not_found)
		{
		  fprintf(stderr, _("\nCannot detect printer type.\n"
				    "Please use -m to specify your printer model.\n"));
		  do_help(1);
		}
	      /*
	       * Some printers seem to return status, but don't respond
	       * usefully to @EJL ID.  The Stylus Pro 7500 seems to be
	       * one of these.
	       */
	      if (status > 0 && !fail_if_not_found &&
		  strstr((char *) buf, "@BDC ST"))
		{
		  found_unknown_old_printer = 1;
		  /*
		   * Set the printer model to something rational so that
		   * attempts to describe parameters will succeed.
		   * However, make it clear that this is a dummy,
		   * so we don't actually try to print it out.
		   */
		  STP_DEBUG(printf("Can't find printer name, assuming Stylus Photo\n"));
		  printer_model = c_strdup("escp2-photo");
		}
	      else
		{
		  STP_DEBUG(printf("Can't get response to @EJL ID\n"));
		  close(fd);
		  return NULL;
		}
	    }
	  else
	    {
	      if (spos)
		*spos = '\000';
	      printer_model = pos + 1;
	      STP_DEBUG(printf("printer model: %s\n", printer_model));
	    }
	}
    }

  set_printer_model();

  if (isnew && !packet_initialized)
    {
      isnew = !init_packet(fd, 0);
    }

  close(fd);
  STP_DEBUG(printf("new? %s\n", isnew ? "yes" : "no"));
  return the_printer_t;
}

static const stp_printer_t *
get_printer(int quiet, int fail_if_not_found)
{
  if (the_printer_t)
    return the_printer_t;
  else
    {
      const stp_printer_t *printer =
	initialize_printer(quiet, fail_if_not_found);
      STP_DEBUG(printf("init done, printer found? %s...\n",
			printer ? "yes" : "no"));
      return printer;
    }
}

static const char *colors_new[] =
  {
    N_("Black"),		/* 00 */
    N_("Photo Black"),		/* 01 */
    N_("Unknown"),		/* 02 */
    N_("Cyan"),			/* 03 */
    N_("Magenta"),		/* 04 */
    N_("Yellow"),		/* 05 */
    N_("Light Cyan"),		/* 06 */
    N_("Light Magenta"),	/* 07 */
    N_("Unknown"),		/* 08 */
    N_("Unknown"),		/* 09 */
    N_("Light Black"),		/* 0a */
    N_("Matte Black"),		/* 0b */
    N_("Red"),			/* 0c */
    N_("Blue"),			/* 0d */
    N_("Gloss Optimizer"),	/* 0e */
    N_("Light Light Black"),	/* 0f */
    N_("Orange"),		/* 10 */
  };
static int color_count = sizeof(colors_new) / sizeof(const char *);

static const char *aux_colors[] =
  {
    N_("Black"),		/* 0 */
    N_("Cyan"),			/* 1 */
    N_("Magenta"),		/* 2 */
    N_("Yellow"),		/* 3 */
    N_("Light Cyan"),		/* 4 */
    N_("Light Magenta"),	/* 5 */
    NULL,			/* 6 */
    NULL,			/* 7 */
    NULL,			/* 8 */
    N_("Red"),			/* 9 */
    N_("Blue"),			/* a */
    NULL,			/* b */
    NULL,			/* c */
    N_("Orange"),		/* d */
    NULL,			/* e */
    NULL,			/* f */
  };
static int aux_color_count = sizeof(aux_colors) / sizeof(const char *);

static void
print_status(int param)
{
  switch (param)
    {
    case 0:
      printf(_("Status: Error\n"));
      break;
    case 1:
      printf(_("Status: Self-Printing\n"));
      break;
    case 2:
      printf(_("Status: Busy\n"));
      break;
    case 3:
      printf(_("Status: Waiting\n"));
      break;
    case 4:
      printf(_("Status: Idle\n"));
      break;
    case 5:
      printf(_("Status: Paused\n"));
      break;
    case 7:
      printf(_("Status: Cleaning\n"));
      break;
    case 8:
      printf(_("Status: Factory shipment\n"));
      break;
    case 0xa:
      printf(_("Status: Shutting down\n"));
      break;
    default:
      printf(_("Status: Unknown (%d)\n"), param);
      break;
    }
}

static void
print_error(int param)
{
  switch (param)
    {
    case 0:
      printf(_("Error: Fatal Error\n"));
      break;
    case 1:
      printf(_("Error: Other interface is selected\n"));
      break;
    case 2:
      printf(_("Error: Cover Open\n"));
      break;
    case 4:
      printf(_("Error: Paper jam\n"));
      break;
    case 5:
      printf(_("Error: Ink out\n"));
      break;
    case 6:
      printf(_("Error: Paper out\n"));
      break;
    case 0xc:
      printf(_("Error: Miscellaneous paper error\n"));
      break;
    case 0x10:
      printf(_("Error: Maintenance cartridge overflow\n"));
      break;
    case 0x11:
      printf(_("Error: Wait return from the tear-off position\n"));
      break;
    case 0x12:
      printf(_("Error: Double feed error\n"));
      break;
    case 0x1a:
      printf(_("Error: Ink cartridge lever released\n\n"));
      break;
    case 0x1c:
      printf(_("Error: Unrecoverable cutter error\n"));
      break;
    case 0x1d:
      printf(_("Error: Recoverable cutter jam\n"));
      break;
    case 0x22:
      printf(_("Error: No maintenance cartridge present\n"));
      break;
    case 0x25:
      printf(_("Error: Rear cover open\n"));
      break;
    case 0x29:
      printf(_("Error: CD Tray Out\n"));
      break;
    case 0x2a:
      printf(_("Error: Card loading error\n"));
      break;
    case 0x2b:
      printf(_("Error: Tray cover open\n"));
      break;
    case 0x36:
      printf(_("Error: Maintenance cartridge cover open\n"));
      break;
    case 0x37:
      printf(_("Error: Front cover open\n"));
      break;
    case 0x41:
      printf(_("Error: Maintenance request\n"));
      break;
    default:
      printf(_("Error: Unknown (%d)\n"), param);
      break;
    }
}

static void
print_warning(int param, const stp_string_list_t *color_list)
{
  if (param >= 0x10 && param < 0x20)
    {
      param &= 0xf;
      if (color_list && param < stp_string_list_count(color_list))
	printf(_("Warning: %s Ink Low\n"),
	       gettext(stp_string_list_param(color_list, param)->text));
      else
	printf(_("Warning: Channel %d Ink Low\n"), param);
    }
  else if (param >= 0x50 && param < 0x60)
    {
      param &= 0xf;
      if (color_list && param < stp_string_list_count(color_list))
	printf(_("Warning: %s Cleaning Disabled\n"),
	       gettext(stp_string_list_param(color_list, param)->text));
      else
	printf(_("Warning: Channel %d Cleaning \n"), param);
    }
  else
    {
      switch (param)
	{
	case 0x20:
	  printf(_("Warning: Maintenance cartridge near full\n"));
	  break;
	case 0x21:
	  printf(_("Warning: Maintenance request pending\n"));
	  break;
	default:
	  printf(_("Warning: Unknown (%d)\n"), param);
	  break;
	}
    }
}

static void
print_self_printing_state(int param)
{
  switch (param)
    {
    case 0:
      printf(_("Printing nozzle self-test\n"));
      break;
    default:
      break;
    }
}

static const char *
looking_at_command(const char *buf, const char *cmd)
{
  if (!strncmp(buf, cmd, strlen(cmd)))
    {
      const char *answer = buf + strlen(cmd);
      if (answer[0] == ':')
	return (answer + 1);
    }
  return NULL;
}

static const char *
find_group(const char *buf)
{
  while (buf[0] == ';')
    buf++;
  buf = strchr(buf, ';');
  if (buf && buf[1])
    return buf + 1;
  else
    return NULL;
}

static int
get_digit(char digit)
{
  if (digit >= '0' && digit <= '9')
    return digit - '0';
  else if (digit >= 'A' && digit <= 'F')
    return digit - 'A' + 10;
  else if (digit >= 'a' && digit <= 'f')
    return digit - 'a' + 10;
  else
    return 0;
}

static void
print_old_ink_levels(const char *ind, stp_string_list_t *color_list)
{
  int i;
  for (i = 0; i < stp_string_list_count(color_list); i++)
    {
      int val;
      if (!ind[0] || ind[0] == ';')
	return;
      val = (get_digit(ind[0]) << 4) + get_digit(ind[1]);
      printf("%20s    %20d\n",
	     gettext(stp_string_list_param(color_list, i)->text), val);
      ind += 2;
    }
}

static void
do_old_status(status_cmd_t cmd, const char *buf, const stp_printer_t *printer)
{
  if (cmd == CMD_STATUS)
    printf(_("Printer Name: %s\n"),
	   printer ? stp_printer_get_long_name(printer) : _("Unknown"));
  do
    {
      const char *ind;
      if (cmd == CMD_STATUS && (ind = looking_at_command(buf, "ST")) != 0)
	print_status(atoi(ind));
      else if (cmd == CMD_STATUS &&
	       (ind = looking_at_command(buf, "ER")) != 0)
	print_error(atoi(ind));
      else if ((ind = looking_at_command(buf, "IQ")) != 0)
	{
	  stp_string_list_t *color_list = NULL;
	  if (printer)
	    {
	      stp_parameter_t desc;
	      const stp_vars_t *printvars = stp_printer_get_defaults(printer);
	      stp_describe_parameter(printvars, "ChannelNames", &desc);
	      if (desc.p_type == STP_PARAMETER_TYPE_STRING_LIST)
		{
		  color_list = stp_string_list_create_copy(desc.bounds.str);
		  STP_DEBUG(printf("Using color list from driver (%ld %ld)\n",
				   (long)stp_string_list_count(desc.bounds.str),
				   (long)stp_string_list_count(color_list)));
		  stp_parameter_description_destroy(&desc);
		}
	    }
	  if (!color_list)/* use defaults */
	    {
	      stp_string_list_add_string(color_list, "Black", _("Black"));
	      stp_string_list_add_string(color_list, "Cyan", _("Cyan"));
	      stp_string_list_add_string(color_list, "Magenta", _("Magenta"));
	      stp_string_list_add_string(color_list, "Yellow", _("Yellow"));
	      stp_string_list_add_string(color_list, "LightCyan", _("Light Cyan"));
	      stp_string_list_add_string(color_list, "LightMagenta", _("Light Magenta"));
	    }	  
	  
	  if (cmd == CMD_STATUS)
	    printf(_("Ink Levels:\n"));
	  printf("%20s    %20s\n", _("Ink color"), _("Percent remaining"));
	  print_old_ink_levels(ind, color_list);
	  stp_string_list_destroy(color_list);
	  if (cmd == CMD_STATUS)
	    printf("\n");
	}
      STP_DEBUG(printf("looking at %s\n", buf));
    } while ((buf = find_group(buf)) != NULL);
}  

static void
do_new_status(status_cmd_t cmd, char *buf, int bytes,
	      const stp_printer_t *printer)
{
  int i = 0;
  int j;
  const char *ind;
  const stp_string_list_t *color_list = NULL;
  stp_parameter_t desc;
  const stp_vars_t *printvars = stp_printer_get_defaults(printer);
  stp_describe_parameter(printvars, "ChannelNames", &desc);
  if (desc.p_type == STP_PARAMETER_TYPE_STRING_LIST)
    color_list = desc.bounds.str;
  STP_DEBUG(printf("New format bytes: %d bytes\n", bytes));
  if (cmd == CMD_STATUS)
    printf(_("Printer Name: %s\n"),
	    printer ? stp_printer_get_long_name(printer) : _("Unknown"));
  while (i < bytes)
    {
      unsigned hdr = buf[i];
      unsigned total_param_count = buf[i + 1];
      unsigned param = buf[i + 2];
      STP_DEBUG(printf("Header: %x param count: %d\n", hdr, total_param_count));
      if (hdr == 0x0f)	/* Always report ink */
	{
	  size_t count = (total_param_count - 1) / param;
	  ind = buf + i + 3;
	  if (cmd == CMD_STATUS)
	    printf(_("Ink Levels:\n"));
	  printf("%20s    %20s\n", _("Ink color"), _("Percent remaining"));
	  for (j = 0; j < count; j++)
	    {
	      STP_DEBUG(printf("    Ink %d: ind[0] %d ind[1] %d ind[2] %d interchangeable %d param %d count %d aux %d\n",
				j, ind[0], ind[1], ind[2], interchangeable_inks,
				param, color_count, aux_color_count));
	      if (ind[0] < color_count && param == 3 /* &&
		  (interchangeable_inks || ind[1] >= aux_color_count ||
		   ! aux_colors[(int) ind[1]]) */)
		{
		  STP_DEBUG(printf("Case 0\n"));
		  printf("%20s    %20d\n",
			 gettext(colors_new[(int) ind[0]]), ind[2]);
		}
	      else if (ind[1] < aux_color_count && aux_colors[(int) ind[1]])
		{
		  STP_DEBUG(printf("Case 1\n"));
		  printf("%20s    %20d\n",
			 gettext(aux_colors[(int) ind[1]]), ind[2]);
		}
	      else
		{
		  STP_DEBUG(printf("Case 2\n"));
		  printf("%8s 0x%02x 0x%02x    %20d\n",
			 _("Unknown"), (unsigned char) ind[0],
			 (unsigned char) ind[1], ind[2]);
		}
	      ind += param;
	    }
	  if (cmd == CMD_STATUS)
	    printf("\n");
	}
      else if (cmd == CMD_STATUS)
	{
	  switch (hdr)
	    {
	    case 0x1:	/* Status */
	      print_status(param);
	      break;
	    case 0x2:	/* Error */
	      print_error(param);
	      break;
	    case 0x3:	/* Self-printing */
	      print_self_printing_state(param);
	      break; 
	    case 0x4:	/* Warning */
	      for (j = 0; j < total_param_count; j++)
		{
		  param = (unsigned) buf[i + j + 2];
		  print_warning(param, color_list);
		}
	      break;
	    case 0x19:	/* Job name */
	      if (total_param_count > 5)
		{
		  printf(_("Job Name: "));
		  for (j = 5; j < total_param_count; j++)
		    putchar(buf[i + j + 2]);
		  putchar('\n');
		}
	      break;
	    default:
	      /* Ignore other commands */
	      break;
	    }
	}
      i += total_param_count + 2;
    }
  stp_parameter_description_destroy(&desc);
  exit(0);
}

static void
do_status_command_internal(status_cmd_t cmd)
{
  int fd;
  int status;
  int retry = 4;
  char buf[1024];
  const stp_printer_t *printer;
  const char *cmd_name = NULL;
  switch (cmd)
    {
    case CMD_INK_LEVEL:
      cmd_name = _("ink levels");
      break;
    case CMD_STATUS:
      cmd_name = _("status");
      break;
    }
  if (!raw_device)
    {
      fprintf(stderr,_("Obtaining %s requires using a raw device.\n"),
	      gettext(cmd_name));
      exit(1);
    }

  STP_DEBUG(printf("%s...\n", cmd_name));
  printer = get_printer(1, 0);
  if (!found_unknown_old_printer)
    STP_DEBUG(printf("%s found %s%s\n", gettext(cmd_name),
		      printer ? stp_printer_get_long_name(printer) :
		      printer_model ? printer_model : "(null)",
		      printer ? "" : "(Unknown model)"));

  fd = open_raw_device();

  if (isnew)
    {
      /* request status command */
      status =
	writeAndReadData(fd, socket_id, (const unsigned char*)"st\1\0\1",
			 5, 1, (unsigned char *) buf, 1023,
			 &send_size, &receive_size, &test_for_st);
      if (status <= 0)
	{
	  fprintf(stderr, _("\nCannot write to %s: %s\n"), raw_device, strerror(errno));
	  CloseChannel(fd, socket_id);
	  exit(1);
	}
      buf[status] = '\0';
      if (buf[7] == '2')
	do_new_status(cmd, buf + 12, status - 12, printer);
      else
	do_old_status(cmd, buf + 9, printer);
      CloseChannel(fd, socket_id);
    }
  else
    {
      do
        {
          add_resets(2);
          initialize_print_cmd(1);
          do_remote_cmd("ST", 2, 0, 1);
          add_resets(2);
          if (SafeWrite(fd, printer_cmd, bufpos) < bufpos)
            {
              fprintf(stderr, _("Cannot write to %s: %s\n"), raw_device,
                      strerror(errno));
              exit(1);
            }
          status = read_from_printer(fd, buf, 1024, 1);
          if (status < 0)
            {
              exit(1);
            }
	} while (--retry != 0 && !status);
      buf[status] = '\0';
      if (status > 9)
	do_old_status(cmd, buf + 9, printer);
    }
  (void) close(fd);
  exit(0);
}

void
do_ink_level(void)
{
  do_status_command_internal(CMD_INK_LEVEL);
}

void
do_extended_ink_info(int extended_output)
{
  int fd;
  int status;
  char buf[1024];
  unsigned val, id, id2, year, year2, month, month2;
  unsigned iv[6];

  char *ind;
  int i;
  const stp_printer_t *printer;
  const stp_vars_t *printvars;
  stp_parameter_t desc;

  if (!raw_device)
    {
      fprintf(stderr,_("Obtaining extended ink information requires using a raw device.\n"));
      exit(1);
    }

  printer = get_printer(1, 0);
  if (printer)
    {
      printvars = stp_printer_get_defaults(printer);
      stp_describe_parameter(printvars, "ChannelNames", &desc);
    }
  else
    fprintf(stderr,
	    "Warning! Printer %s is not known; information may be incomplete or incorrect\n",
	    printer_model ? printer_model : "(unknown printer)");

  fd = open_raw_device();

  if (isnew)
    {
      stp_string_list_t *color_list = stp_string_list_create();

      if (printer && desc.p_type == STP_PARAMETER_TYPE_STRING_LIST)
	{
	  STP_DEBUG(printf("Using color list from driver (%ld %ld)\n",
			    (long)stp_string_list_count(desc.bounds.str),
			    (long)stp_string_list_count(color_list)));
	  color_list = stp_string_list_create_copy(desc.bounds.str);
	  stp_parameter_description_destroy(&desc);
	}
      else
	{
	  /*
	   * If we're using the "new" ink status format and we don't know
	   * about the printer, take the colors from the ink status
	   * message rather than from the ink list.  This gives us a
	   * last chance to determine the inks
	   */
	  /* request status command */
	  status =
	    writeAndReadData(fd, socket_id, (const unsigned char*)"st\1\0\1",
			     5, 1, (unsigned char *) buf, 1023,
			     &send_size, &receive_size, &test_for_st);
	  if (status <= 0)
	    {
	      stp_parameter_description_destroy(&desc);
	      fprintf(stderr, _("\nCannot write to %s: %s\n"),
		      raw_device, strerror(errno));
	      exit(1);
	    }
	  buf[status] = '\0';
	  if ( buf[7] == '2' )
	    {
	      STP_DEBUG(printf("New format ink!\n"));
	      /* new binary format ! */
	      i = 10;
	      while (buf[i] != 0x0f && i < status)
		i += buf[i + 1] + 2;
	      ind = buf + i;
	      i = 3;
	      while (i < ind[1])
		{
		  if (ind[i] < color_count)
		    {
		      STP_DEBUG(printf("   Case 0: Ink %d %d (%s)\n",
				       i, ind[i], colors_new[(int) ind[i]]));
		      stp_string_list_add_string(color_list,
						 colors_new[(int) ind[i]],
						 colors_new[(int) ind[i]]);
		    }
		  else if (ind[i] == 0x40 && ind[i + 1] < aux_color_count)
		    {
		      STP_DEBUG(printf("   Case 1: Ink %d %d (%s)\n",
				       i, ind[i+1], aux_colors[(int) ind[i+1]]));
		      stp_string_list_add_string(color_list,
						 aux_colors[(int) ind[i + 1]],
						 aux_colors[(int) ind[i + 1]]);
		    }
		  else
		    {
		      STP_DEBUG(printf("   Case 2: Unknown\n"));
		      stp_string_list_add_string(color_list, "Unknown",
						 "Unknown");
		    }
		  i+=3;
		}
	    }
	  STP_DEBUG(printf("Using color list from status message\n"));
	}

      for (i = 0; i < stp_string_list_count(color_list); i++)
        {
	  char req[] = "ii\2\0\1\1";
	  req[5] = i + 1;
	  status =
	    writeAndReadData(fd, socket_id, (const unsigned char*)req,
			     6, 1, (unsigned char *) buf, 1023,
			     &send_size, &receive_size, &test_for_ii);
	  if (status <= 0)
	    {
	      CloseChannel(fd, socket_id);
	      exit(1);
	    }
	  ind = strchr(buf, 'I');
	  if (!ind)
	    {
	      STP_DEBUG(printf("Case 0: failure %i (%s)\n", i, buf));
	      printf("Cannot identify cartridge in slot %d\n", i);
	    }
	  else if (sscanf(ind,
			  "II:01;IQT:%x;TSH:%*4s;PDY:%x;PDM:%x;IC1:%x;IC2:%*x;IK1:%*x;IK2:%*x;TOV:%*x;TVU:%*x;LOG:EPSON;IQT:%x,%x,%x,%x,%x;TSH:%*4s;PDY:%x;PDM:%x;IC1:%x;IC2:%*xIK1:%*x;IK2;%*x;TOV:%*x;TVU:%*x;LOG:EPSON;",
			  &iv[0], &year, &month, &id,
			  &iv[1], &iv[2], &iv[3], &iv[4], &iv[5],
			  &year2, &month2, &id2) == 12 ||
		   sscanf(ind,
			  "II:01;IQT:%x;TSH:%*4s;PDY:%x;PDM:%x;IC1:%x;IC2:%*x;IK1:%*x;IK2:%*x;TOV:%*x;TVU:%*x;LOG:INKbyEPSON;IQT:%x,%x,%x,%x,%x;TSH:%*4s;PDY:%x;PDM:%x;IC1:%x;IC2:%*xIK1:%*x;IK2;%*x;TOV:%*x;TVU:%*x;LOG:INKbyEPSON;",
			  &iv[0], &year, &month, &id,
			  &iv[1], &iv[2], &iv[3], &iv[4], &iv[5],
			  &year2, &month2, &id2) == 12)
	    {
	      int j;
	      STP_DEBUG(printf("Case 1: i %i iv %ud %ud %ud %ud %ud %ud year %ud %ud mo %ud %ud id %ud %ud\n",
				i, iv[0], iv[1], iv[2], iv[3], iv[4], iv[5],
				year, year2, month, month2, id, id2));
	      printf("%20s    %20s   %12s   %7s\n",
		     _("Ink cartridge"), _("Percent remaining"), _("Part number"),
		     _("Date"));
	      printf("%20s    %20d    T0%03d            %2d%02d-%02d\n",
		     gettext(stp_string_list_param(color_list, 0)->text),
		     iv[0], id, (year > 80 ? 19 : 20), year, month);
	      for (j = 1; j < 6; j++)
		printf("%20s    %20d    T0%03d            %2d%02d-%02d\n",
		       gettext(stp_string_list_param(color_list, j)->text),
		       iv[j], id2, (year2 > 80 ? 19 : 20), year2, month2);
	      break;
	    }
	  else if (sscanf(ind,
			  "II:01;IQT:%x;TSH:%*4s;PDY:%x;PDM:%x;IC1:%x;IC2:%*x;IK1:%*x;IK2:%*x;TOV:%*x;TVU:%*x;LOG:EPSON;IQT:%x,%x,%x;TSH:%*4s;PDY:%x;PDM:%x;IC1:%x;IC2:%*xIK1:%*x;IK2;%*x;TOV:%*x;TVU:%*x;LOG:EPSON;",
			  &iv[0], &year, &month, &id,
			  &iv[1], &iv[2], &iv[3],
			  &year2, &month2, &id2) == 10 ||
		   sscanf(ind,
			  "II:01;IQT:%x;TSH:%*4s;PDY:%x;PDM:%x;IC1:%x;IC2:%*x;IK1:%*x;IK2:%*x;TOV:%*x;TVU:%*x;LOG:INKbyEPSON;IQT:%x,%x,%x;TSH:%*4s;PDY:%x;PDM:%x;IC1:%x;IC2:%*xIK1:%*x;IK2;%*x;TOV:%*x;TVU:%*x;LOG:INKbyEPSON;",
			  &iv[0], &year, &month, &id,
			  &iv[1], &iv[2], &iv[3],
			  &year2, &month2, &id2) == 10)
	    {
	      int j;
	      STP_DEBUG(printf("Case 2: i %i iv %ud %ud %ud %ud year %ud %ud mo %ud %ud id %ud %ud\n",
				i, iv[0], iv[1], iv[2], iv[3],
				year, year2, month, month2, id, id2));
	      printf("%20s    %20s   %12s   %7s\n",
		     _("Ink cartridge"), _("Percent remaining"), _("Part number"),
		     _("Date"));
	      printf("%20s    %20d    T0%03d            %2d%02d-%02d\n",
		     gettext(stp_string_list_param(color_list, 0)->text),
		     iv[0], id, (year > 80 ? 19 : 20), year, month);
	      for (j = 1; j < 4; j++)
		printf("%20s    %20d    T0%03d            %2d%02d-%02d\n",
		       gettext(stp_string_list_param(color_list, j)->text),
		       iv[j], id2, (year2 > 80 ? 19 : 20), year2, month2);
	      break;
	    }
	  else if (sscanf(ind,
			  "II:01;IQT:%x;TSH:%*4s;PDY:%x;PDM:%x;IC1:%x;IC2:%*x;IK1:%*x;IK2:%*x;TOV:%*x;TVU:%*x;LOG:EPSON;",
			  &val, &year, &month, &id ) == 4 ||
		   sscanf(ind,
			  "II:01;IQT:%x;TSH:%*4s;PDY:%x;PDM:%x;IC1:%x;IC2:%*x;IK1:%*x;IK2:%*x;TOV:%*x;TVU:%*x;LOG:INKbyEPSON;",
			  &val, &year, &month, &id ) == 4)
	    {
	      STP_DEBUG(printf("Case 3: i %i val %ud year %ud mo %ud id %ud\n",
				i, val, year, month, id));
	      if (i == 0)
		printf("%20s    %20s   %12s   %7s\n",
		       _("Ink cartridge"), _("Percent remaining"), _("Part number"),
		       _("Date"));
	      printf("%20s    %20d    T0%03d            %2d%02d-%02d\n",
		     gettext(stp_string_list_param(color_list, i)->text),
		     val, id, (year > 80 ? 19 : 20), year, month);
	    }
	  else if (sscanf(ind,
			  "IQT:%x;TSH:%*4s;PDY:%x;PDM:%x;IC1:%x;IC2:%*x;IK1:%*x;IK2:%*x;TOV:%*x;TVU:%*x;LOG:EPSON;",
			  &val, &year, &month, &id ) == 4 ||
		   sscanf(ind,
			  "IQT:%x;TSH:%*4s;PDY:%x;PDM:%x;IC1:%x;IC2:%*x;IK1:%*x;IK2:%*x;TOV:%*x;TVU:%*x;LOG:INKbyEPSON;",
			  &val, &year, &month, &id ) == 4)
	    {
	      STP_DEBUG(printf("Case 4: i %i val %ud year %ud mo %ud id %ud\n",
				i, val, year, month, id));
	      if (i == 0)
		printf("%20s    %20s   %12s   %7s\n",
		       _("Ink cartridge"), _("Percent remaining"), _("Part number"),
		       _("Date"));
	      printf("%20s    %20d    T0%03d            %2d%02d-%02d\n",
		     gettext(stp_string_list_param(color_list, i)->text),
		     val, id, (year > 80 ? 19 : 20), year, month);
	    }
	  else
	    {
	      STP_DEBUG(printf("Case 5: failure %i (%s)\n", i, ind));
	      printf("Cannot identify cartridge in slot %d\n", i);
	    }
	}
      stp_string_list_destroy(color_list);
      CloseChannel(fd, socket_id);
    }
  else
    {
      (void) close(fd);
      do_ink_level();
    }
  exit(0);
}

void
do_identify(void)
{
  const stp_printer_t *printer;
  if (!raw_device)
    {
      fprintf(stderr,
	      _("Printer identification requires using a raw device.\n"));
      exit(1);
    }
  if (printer_model)
    printer_model = NULL;
  printer = get_printer(1, 1);
  if (printer && !found_unknown_old_printer)
    {
      if (print_short_name)
	printf("%s\n", stp_printer_get_driver(printer));
      else
	printf("%s\n", gettext(stp_printer_get_long_name(printer)));
      exit(0);
    }
  else if (printer_model)
    printf("EPSON %s\n", printer_model);
  else
    {
      fprintf(stderr, _("Cannot identify printer model.\n"));
      exit(1);
    }
}

void
do_status(void)
{
  do_status_command_internal(CMD_STATUS);
  exit(1);
}


void
do_head_clean(void)
{
  if (printer_model)
    set_printer_model();
  else if (raw_device)
    (void) get_printer(1, 0);
  initialize_print_cmd(1);
  do_remote_cmd("CH", 2, 0, 0);
  printf(_("Cleaning heads...\n"));
  exit(do_print_cmd());
}

void
do_nozzle_check(void)
{
  if (printer_model)
    set_printer_model();
  else if (raw_device)
    (void) get_printer(1, 0);
  initialize_print_cmd(1);
  start_remote_sequence();
  do_remote_cmd_only("VI", 2, 0, 0);
  do_remote_cmd_only("NC", 2, 0, 0x10);
  do_remote_cmd_only("NC", 2, 0, 0);
  end_remote_sequence();
  printf(_("Running nozzle check, please ensure paper is in the printer.\n"));
  exit(do_print_cmd());
}

const char *new_align_help = N_("\
Please read these instructions very carefully before proceeding.\n\
\n\
This utility lets you align the print head of your Epson Stylus inkjet\n\
printer.  Misuse of this utility may cause your print quality to degrade\n\
and possibly damage your printer.  This utility has not been reviewed by\n\
Seiko Epson for correctness, and is offered with no warranty at all.  The\n\
entire risk of using this utility lies with you.\n\
\n\
This utility prints %d test patterns.  Each pattern looks very similar.\n\
The patterns consist of a series of pairs of vertical lines that overlap.\n\
Below each pair of lines is a number between %d and %d.\n\
\n\
When you inspect the pairs of lines, you should find the pair of lines that\n\
is best in alignment, that is, that best forms a single vertical line.\n\
Inspect the pairs very carefully to find the best match.  Using a loupe\n\
or magnifying glass is recommended for the most critical inspection.\n\
It is also suggested that you use a good quality paper for the test,\n\
so that the lines are well-formed and do not spread through the paper.\n\
After picking the number matching the best pair, place the paper back in\n\
the paper input tray before typing it in.\n\
\n\
Each pattern is similar, but later patterns use finer dots for more\n\
critical alignment.  You must run all of the passes to correctly align your\n\
printer.  After running all the alignment passes, the alignment\n\
patterns will be printed once more.  You should find that the middle-most\n\
pair (#%d out of the %d) is the best for all patterns.\n\
\n\
After the passes are printed once more, you will be offered the\n\
choices of (s)aving the result in the printer, (r)epeating the process,\n\
or (q)uitting without saving.  Quitting will not restore the previous\n\
settings, but powering the printer off and back on will.  If you quit,\n\
you must repeat the entire process if you wish to later save the results.\n\
It is essential that you not turn your printer off during this procedure.\n\n");

const char *old_align_help = N_("\
Please read these instructions very carefully before proceeding.\n\
\n\
This utility lets you align the print head of your Epson Stylus inkjet\n\
printer.  Misuse of this utility may cause your print quality to degrade\n\
and possibly damage your printer.  This utility has not been reviewed by\n\
Seiko Epson for correctness, and is offered with no warranty at all.  The\n\
entire risk of using this utility lies with you.\n\
\n\
This utility prints a test pattern that consist of a series of pairs of\n\
vertical lines that overlap.  Below each pair of lines is a number between\n\
%d and %d.\n\
\n\
When you inspect the pairs of lines, you should find the pair of lines that\n\
is best in alignment, that is, that best forms a single vertical align.\n\
Inspect the pairs very carefully to find the best match.  Using a loupe\n\
or magnifying glass is recommended for the most critical inspection.\n\
It is also suggested that you use a good quality paper for the test,\n\
so that the lines are well-formed and do not spread through the paper.\n\
After picking the number matching the best pair, place the paper back in\n\
the paper input tray before typing it in.\n\
\n\
After running the alignment pattern, it will be printed once more.  You\n\
should find that the middle-most pair (#%d out of the %d) is the best.\n\
You will then be offered the choices of (s)aving the result in the printer,\n\
(r)epeating the process, or (q)uitting without saving.  Quitting will not\n\
restore the previous settings, but powering the printer off and back on will.\n\
If you quit, you must repeat the entire process if you wish to later save\n\
the results.  It is essential that you not turn off your printer during\n\
this procedure.\n\n");

static void
do_align_help(int passes, int choices)
{
  if (passes > 1)
    printf(gettext(new_align_help), passes, 1, choices, (choices + 1) / 2, choices);
  else
    printf(gettext(old_align_help), 1, choices, (choices + 1) / 2, choices);
  fflush(stdout);
}

static void
printer_error(void)
{
  printf(_("Unable to send command to the printer, exiting.\n"));
  exit(1);
}

static int
do_final_alignment(void)
{
  int retry_count = 0;
  while (1)
    {
      char *inbuf;
      retry_count++;
      if (retry_count > 10)
	{
	  printf(_("Exiting\n"));
	  exit(1);
	}
      printf(_("Please inspect the final output very carefully to ensure that your\n"
	       "printer is in proper alignment. You may now:\n"
	       "  (s)ave the results in the printer,\n"
	       "  (q)uit without saving the results, or\n"
	       "  (r)epeat the entire process from the beginning.\n"
	       "You will then be asked to confirm your choice.\n"
	       "What do you want to do (s, q, r)?\n"));
      fflush(stdout);
      inbuf = do_get_input(_("> "));
      if (!inbuf)
	continue;
      switch (inbuf[0])
	{
	case 'q':
	case 'Q':
	  printf(_("Please confirm by typing 'q' again that you wish to quit without saving:\n"));
	  fflush(stdout);
	  inbuf = do_get_input (_("> "));
	  if (!inbuf)
	    continue;
	  if (inbuf[0] == 'q' || inbuf[0] == 'Q')
	    {
	      printf(_("OK, your printer is aligned, but the alignment has not been saved.\n"
		       "If you wish to save the alignment, you must repeat this process.\n"));
	      return 1;
	    }
	  break;
	case 'r':
	case 'R':
	  printf(_("Please confirm by typing 'r' again that you wish to repeat the\n"
		   "alignment process:\n"));
	  fflush(stdout);
	  inbuf = do_get_input(_("> "));
	  if (!inbuf)
	    continue;
	  if (inbuf[0] == 'r' || inbuf[0] == 'R')
	    {
	      printf(_("Repeating the alignment process.\n"));
	      return 0;
	    }
	  break;
	case 's':
	case 'S':
	  printf(_("This will permanently alter the configuration of your printer.\n"
		   "WARNING: this procedure has not been approved by Seiko Epson, and\n"
		   "it may damage your printer. Proceed?\n"
		   "Please confirm by typing 's' again that you wish to save the settings\n"
		   "to your printer:\n"));

	  fflush(stdout);
	  inbuf = do_get_input(_("> "));
	  if (!inbuf)
	    continue;
	  if (inbuf[0] == 's' || inbuf[0] == 'S')
	    {
	      printf(_("About to save settings..."));
	      fflush(stdout);
	      initialize_print_cmd(1);
	      do_remote_cmd("SV", 0);
	      if (do_print_cmd())
		{
		  printf(_("failed!\n"));
		  printf(_("Your settings were not saved successfully.  You must repeat the\n"
			   "alignment procedure.\n"));
		  exit(1);
		}
	      printf(_("succeeded!\n"));
	      printf(_("Your alignment settings have been saved to the printer.\n"));
	      return 1;
	    }
	  break;
	default:
	  printf(_("Unrecognized command.\n"));
	  continue;
	}
      printf(_("Final command was not confirmed.\n"));
    }
}

const char *printer_msg =
N_("This procedure assumes that your printer is an %s.\n"
   "If this is not your printer model, please type control-C now and\n"
   "choose your actual printer model.\n\n"
   "Please place a sheet of paper in your printer to begin the head\n"
   "alignment procedure.\n");

/*
 * This is the thorny one.
 */
void
do_align(void)
{
  char *inbuf;
  long answer;
  char *endptr;
  int curpass;
  stp_parameter_t desc;
  const char *printer_name;
  stp_vars_t *v = stp_vars_create();

  if (printer_model)
    set_printer_model();
  else if (raw_device)
    (void) get_printer(0, 0);

  if (!the_printer_t || found_unknown_old_printer)
    return;

  printer_name = stp_printer_get_long_name(the_printer_t);
  stp_set_driver(v, stp_printer_get_driver(the_printer_t));

  if (alignment_passes == 0)
    {
      stp_describe_parameter(v, "AlignmentPasses", &desc);
      if (desc.p_type != STP_PARAMETER_TYPE_INT)
	{
	  fprintf(stderr,
		  "Unable to retrieve number of alignment passes for printer %s\n",
		  printer_name);
	  return;
	}
      alignment_passes = desc.deflt.integer;
      stp_parameter_description_destroy(&desc);
    }

  if (alignment_choices == 0)
    {
      stp_describe_parameter(v, "AlignmentChoices", &desc);
      if (desc.p_type != STP_PARAMETER_TYPE_INT)
	{
	  fprintf(stderr,
		  "Unable to retrieve number of alignment choices for printer %s\n",
		  printer_name);
	  return;
	}
      alignment_choices = desc.deflt.integer;
      stp_parameter_description_destroy(&desc);
    }

  if (alignment_passes <= 0 || alignment_choices <= 0)
    {
      printf("No alignment required for printer %s\n", printer_name);
      return;
    }

  do
    {
      do_align_help(alignment_passes, alignment_choices);
      printf(gettext(printer_msg), gettext(printer_name));
      inbuf = do_get_input(_("Press enter to continue > "));
    top:
      initialize_print_cmd(1);
      for (curpass = 0; curpass < alignment_passes; curpass++)
	do_remote_cmd("DT", 3, 0, curpass, 0);
      if (do_print_cmd())
	printer_error();
      printf(_("Please inspect the print, and choose the best pair of lines in each pattern.\n"
	       "Type a pair number, '?' for help, or 'r' to repeat the procedure.\n"));
      initialize_print_cmd(1);
      for (curpass = 1; curpass <= alignment_passes; curpass ++)
	{
	  int retry_count = 0;
	reread:
	  retry_count++;
	  if (retry_count > 10)
	    {
	      printf(_("Exiting\n"));
	      return;
	    }
	  printf(_("Pass #%d"), curpass);
	  inbuf = do_get_input(_("> "));
	  if (!inbuf)
	    goto reread;
	  switch (inbuf[0])
	    {
	    case 'r':
	    case 'R':
	      printf(_("Please insert a fresh sheet of paper.\n"));
	      fflush(stdout);
	      initialize_print_cmd(1);
	      (void) do_get_input(_("Press enter to continue > "));
	      /* Ick. Surely there's a cleaner way? */
	      goto top;
	    case 'h':
	    case '?':
	      do_align_help(alignment_passes, alignment_choices);
	      fflush(stdout);
	    case '\n':
	    case '\000':
	      goto reread;
	    default:
	      break;
	    }
	  answer = strtol(inbuf, &endptr, 10);
	  if (errno == ERANGE)
	    {
	      printf(_("Number out of range!\n"));
	      goto reread;
	    }
	  if (endptr == inbuf)
	    {
	      printf(_("I cannot understand what you typed!\n"));
	      fflush(stdout);
	      goto reread;
	    }
	  if (answer < 1 || answer > alignment_choices)
	    {
	      printf(_("The best pair of lines should be numbered between 1 and %d.\n"),
		     alignment_choices);
	      fflush(stdout);
	      goto reread;
	    }
	  do_remote_cmd("DA", 4, 0, curpass - 1, 0, answer);
	}
      printf(_("Attempting to set alignment..."));
      if (do_print_cmd())
	printer_error();
      printf(_("succeeded.\n"));
      printf(_("Please verify that the alignment is correct.  After the alignment pattern\n"
	       "is printed again, please ensure that the best pattern for each line is\n"
	       "pattern %d.  If it is not, you should repeat the process to get the best\n"
	       "quality printing.\n"), (alignment_choices + 1) / 2);
      printf(_("Please insert a fresh sheet of paper.\n"));
      (void) do_get_input(_("Press enter to continue > "));
      initialize_print_cmd(1);
      for (curpass = 0; curpass < alignment_passes; curpass++)
	do_remote_cmd("DT", 3, 0, curpass, 0);
      if (do_print_cmd())
	printer_error();
    } while (!do_final_alignment());
  exit(0);
}

char *
do_get_input (const char *prompt)
{
	static char *input = NULL;
#if (HAVE_LIBREADLINE == 0 || !defined HAVE_READLINE_READLINE_H)
	char *fgets_status;
#endif
	/* free only if previously allocated */
	if (input)
	{
		stp_free (input);
		input = NULL;
	}
#if (HAVE_LIBREADLINE > 0 && defined HAVE_READLINE_READLINE_H)
	/* get input with libreadline, if present */
	input = readline (prompt);
	/* if input, add to history list */
#ifdef HAVE_READLINE_HISTORY_H
	if (input && *input)
	{
		add_history (input);
	}
#endif
#else
	/* no libreadline; use fgets instead */
	input = stp_malloc (sizeof (char) * BUFSIZ);
	memset(input, 0, BUFSIZ);
	printf ("%s", prompt);
	fgets_status = fgets (input, BUFSIZ, stdin);
	if (fgets_status == NULL)
	{
		fprintf (stdout, _("Error in input\n"));
		return (NULL);
	}
	else if (strlen (input) == 1 && input[0] == '\n')
	{
		/* user just hit enter: empty input buffer */
		/* remove line feed */
		input[0] = '\0';
	}
	else
	{
		/* remove line feed */
		input[strlen (input) - 1] = '\0';
	}
#endif
	return (input);
}
