/* File Transfer Protocol support.
   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,
   2004, 2005, 2006, 2007, 2008 Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget.  If not, see <http://www.gnu.org/licenses/>.

Additional permission under GNU GPL version 3 section 7

If you modify this program, or any covered work, by linking or
combining it with the OpenSSL project's OpenSSL library (or a
modified version of that library), containing parts covered by the
terms of the OpenSSL or SSLeay licenses, the Free Software Foundation
grants you additional permission to convey the resulting work.
Corresponding Source for a non-source form of such a combination
shall include the source code for the parts of OpenSSL used as well
as that of the covered work.  */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <assert.h>
#include <errno.h>
#include <time.h>

#include "wget.h"
#include "utils.h"
#include "url.h"
#include "retr.h"
#include "ftp.h"
#include "connect.h"
#include "host.h"
#include "netrc.h"
#include "convert.h"            /* for downloaded_file */
#include "recur.h"              /* for INFINITE_RECURSION */

/* File where the "ls -al" listing will be saved.  */
#ifdef MSDOS
#define LIST_FILENAME "_listing"
#else
#define LIST_FILENAME ".listing"
#endif

typedef struct
{
  int st;                       /* connection status */
  int cmd;                      /* command code */
  int csock;                    /* control connection socket */
  double dltime;                /* time of the download in msecs */
  enum stype rs;                /* remote system reported by ftp server */ 
  char *id;                     /* initial directory */
  char *target;                 /* target file name */
  struct url *proxy;            /* FTWK-style proxy */
} ccon;


/* Look for regexp "( *[0-9]+ *byte" (literal parenthesis) anywhere in
   the string S, and return the number converted to wgint, if found, 0
   otherwise.  */
static wgint
ftp_expected_bytes (const char *s)
{
  wgint res;

  while (1)
    {
      while (*s && *s != '(')
        ++s;
      if (!*s)
        return 0;
      ++s;                      /* skip the '(' */
      res = str_to_wgint (s, (char **) &s, 10);
      if (!*s)
        return 0;
      while (*s && ISSPACE (*s))
        ++s;
      if (!*s)
        return 0;
      if (TOLOWER (*s) != 'b')
        continue;
      if (strncasecmp (s, "byte", 4))
        continue;
      else
        break;
    }
  return res;
}

#ifdef ENABLE_IPV6
/* 
 * This function sets up a passive data connection with the FTP server.
 * It is merely a wrapper around ftp_epsv, ftp_lpsv and ftp_pasv.
 */
static uerr_t
ftp_do_pasv (int csock, ip_address *addr, int *port)
{
  uerr_t err;

  /* We need to determine the address family and need to call
     getpeername, so while we're at it, store the address to ADDR.
     ftp_pasv and ftp_lpsv can simply override it.  */
  if (!socket_ip_address (csock, addr, ENDPOINT_PEER))
    abort ();

  /* If our control connection is over IPv6, then we first try EPSV and then 
   * LPSV if the former is not supported. If the control connection is over 
   * IPv4, we simply issue the good old PASV request. */
  switch (addr->family)
    {
    case AF_INET:
      if (!opt.server_response)
        logputs (LOG_VERBOSE, "==> PASV ... ");
      err = ftp_pasv (csock, addr, port);
      break;
    case AF_INET6:
      if (!opt.server_response)
        logputs (LOG_VERBOSE, "==> EPSV ... ");
      err = ftp_epsv (csock, addr, port);

      /* If EPSV is not supported try LPSV */
      if (err == FTPNOPASV)
        {
          if (!opt.server_response)
            logputs (LOG_VERBOSE, "==> LPSV ... ");
          err = ftp_lpsv (csock, addr, port);
        }
      break;
    default:
      abort ();
    }

  return err;
}

/* 
 * This function sets up an active data connection with the FTP server.
 * It is merely a wrapper around ftp_eprt, ftp_lprt and ftp_port.
 */
static uerr_t
ftp_do_port (int csock, int *local_sock)
{
  uerr_t err;
  ip_address cip;

  if (!socket_ip_address (csock, &cip, ENDPOINT_PEER))
    abort ();

  /* If our control connection is over IPv6, then we first try EPRT and then 
   * LPRT if the former is not supported. If the control connection is over 
   * IPv4, we simply issue the good old PORT request. */
  switch (cip.family)
    {
    case AF_INET:
      if (!opt.server_response)
        logputs (LOG_VERBOSE, "==> PORT ... ");
      err = ftp_port (csock, local_sock);
      break;
    case AF_INET6:
      if (!opt.server_response)
        logputs (LOG_VERBOSE, "==> EPRT ... ");
      err = ftp_eprt (csock, local_sock);

      /* If EPRT is not supported try LPRT */
      if (err == FTPPORTERR)
        {
          if (!opt.server_response)
            logputs (LOG_VERBOSE, "==> LPRT ... ");
          err = ftp_lprt (csock, local_sock);
        }
      break;
    default:
      abort ();
    }
  return err;
}
#else

static uerr_t
ftp_do_pasv (int csock, ip_address *addr, int *port)
{
  if (!opt.server_response)
    logputs (LOG_VERBOSE, "==> PASV ... ");
  return ftp_pasv (csock, addr, port);
}

static uerr_t
ftp_do_port (int csock, int *local_sock)
{
  if (!opt.server_response)
    logputs (LOG_VERBOSE, "==> PORT ... ");
  return ftp_port (csock, local_sock);
}
#endif

static void
print_length (wgint size, wgint start, bool authoritative)
{
  logprintf (LOG_VERBOSE, _("Length: %s"), number_to_static_string (size));
  if (size >= 1024)
    logprintf (LOG_VERBOSE, " (%s)", human_readable (size));
  if (start > 0)
    {
      if (start >= 1024)
        logprintf (LOG_VERBOSE, _(", %s (%s) remaining"),
                   number_to_static_string (size - start),
                   human_readable (size - start));
      else
        logprintf (LOG_VERBOSE, _(", %s remaining"),
                   number_to_static_string (size - start));
    }
  logputs (LOG_VERBOSE, !authoritative ? _(" (unauthoritative)\n") : "\n");
}

/* Retrieves a file with denoted parameters through opening an FTP
   connection to the server.  It always closes the data connection,
   and closes the control connection in case of error.  */
static uerr_t
getftp (struct url *u, wgint *len, wgint restval, ccon *con)
{
  int csock, dtsock, local_sock, res;
  uerr_t err = RETROK;          /* appease the compiler */
  FILE *fp;
  char *user, *passwd, *respline;
  char *tms;
  const char *tmrate;
  int cmd = con->cmd;
  bool pasv_mode_open = false;
  wgint expected_bytes = 0;
  bool rest_failed = false;
  int flags;
  wgint rd_size;

  assert (con != NULL);
  assert (con->target != NULL);

  /* Debug-check of the sanity of the request by making sure that LIST
     and RETR are never both requested (since we can handle only one
     at a time.  */
  assert (!((cmd & DO_LIST) && (cmd & DO_RETR)));
  /* Make sure that at least *something* is requested.  */
  assert ((cmd & (DO_LIST | DO_CWD | DO_RETR | DO_LOGIN)) != 0);

  user = u->user;
  passwd = u->passwd;
  search_netrc (u->host, (const char **)&user, (const char **)&passwd, 1);
  user = user ? user : (opt.ftp_user ? opt.ftp_user : opt.user);
  if (!user) user = "anonymous";
  passwd = passwd ? passwd : (opt.ftp_passwd ? opt.ftp_passwd : opt.passwd);
  if (!passwd) passwd = "-wget@";

  dtsock = -1;
  local_sock = -1;
  con->dltime = 0;

  if (!(cmd & DO_LOGIN))
    csock = con->csock;
  else                          /* cmd & DO_LOGIN */
    {
      char type_char;
      char    *host = con->proxy ? con->proxy->host : u->host;
      int      port = con->proxy ? con->proxy->port : u->port;
      char *logname = user;

      if (con->proxy)
        {
          /* If proxy is in use, log in as username@target-site. */
          logname = concat_strings (user, "@", u->host, (char *) 0);
        }

      /* Login to the server: */

      /* First: Establish the control connection.  */

      csock = connect_to_host (host, port);
      if (csock == E_HOST)
        return HOSTERR;
      else if (csock < 0)
        return (retryable_socket_connect_error (errno)
                ? CONERROR : CONIMPOSSIBLE);

      if (cmd & LEAVE_PENDING)
        con->csock = csock;
      else
        con->csock = -1;

      /* Second: Login with proper USER/PASS sequence.  */
      logprintf (LOG_VERBOSE, _("Logging in as %s ... "), escnonprint (user));
      if (opt.server_response)
        logputs (LOG_ALWAYS, "\n");
      err = ftp_login (csock, logname, passwd);

      if (con->proxy)
        xfree (logname);

      /* FTPRERR, FTPSRVERR, WRITEFAILED, FTPLOGREFUSED, FTPLOGINC */
      switch (err)
        {
        case FTPRERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          return err;
        case FTPSRVERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("Error in server greeting.\n"));
          fd_close (csock);
          con->csock = -1;
          return err;
        case WRITEFAILED:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET,
                   _("Write failed, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          return err;
        case FTPLOGREFUSED:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("The server refuses login.\n"));
          fd_close (csock);
          con->csock = -1;
          return FTPLOGREFUSED;
        case FTPLOGINC:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("Login incorrect.\n"));
          fd_close (csock);
          con->csock = -1;
          return FTPLOGINC;
        case FTPOK:
          if (!opt.server_response)
            logputs (LOG_VERBOSE, _("Logged in!\n"));
          break;
        default:
          abort ();
        }
      /* Third: Get the system type */
      if (!opt.server_response)
        logprintf (LOG_VERBOSE, "==> SYST ... ");
      err = ftp_syst (csock, &con->rs);
      /* FTPRERR */
      switch (err)
        {
        case FTPRERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          return err;
        case FTPSRVERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET,
                   _("Server error, can't determine system type.\n"));
          break;
        case FTPOK:
          /* Everything is OK.  */
          break;
        default:
          abort ();
        }
      if (!opt.server_response && err != FTPSRVERR)
        logputs (LOG_VERBOSE, _("done.    "));

      /* Fourth: Find the initial ftp directory */

      if (!opt.server_response)
        logprintf (LOG_VERBOSE, "==> PWD ... ");
      err = ftp_pwd (csock, &con->id);
      /* FTPRERR */
      switch (err)
        {
        case FTPRERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          return err;
        case FTPSRVERR :
          /* PWD unsupported -- assume "/". */
          xfree_null (con->id);
          con->id = xstrdup ("/");
          break;
        case FTPOK:
          /* Everything is OK.  */
          break;
        default:
          abort ();
        }
      /* VMS will report something like "PUB$DEVICE:[INITIAL.FOLDER]".
         Convert it to "/INITIAL/FOLDER" */ 
      if (con->rs == ST_VMS)
        {
          char *path = strchr (con->id, '[');
          char *pathend = path ? strchr (path + 1, ']') : NULL;
          if (!path || !pathend)
            DEBUGP (("Initial VMS directory not in the form [...]!\n"));
          else
            {
              char *idir = con->id;
              DEBUGP (("Preprocessing the initial VMS directory\n"));
              DEBUGP (("  old = '%s'\n", con->id));
              /* We do the conversion in-place by copying the stuff
                 between [ and ] to the beginning, and changing dots
                 to slashes at the same time.  */
              *idir++ = '/';
              for (++path; path < pathend; path++, idir++)
                *idir = *path == '.' ? '/' : *path;
              *idir = '\0';
              DEBUGP (("  new = '%s'\n\n", con->id));
            }
        }
      if (!opt.server_response)
        logputs (LOG_VERBOSE, _("done.\n"));

      /* Fifth: Set the FTP type.  */
      type_char = ftp_process_type (u->params);
      if (!opt.server_response)
        logprintf (LOG_VERBOSE, "==> TYPE %c ... ", type_char);
      err = ftp_type (csock, type_char);
      /* FTPRERR, WRITEFAILED, FTPUNKNOWNTYPE */
      switch (err)
        {
        case FTPRERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          return err;
        case WRITEFAILED:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET,
                   _("Write failed, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          return err;
        case FTPUNKNOWNTYPE:
          logputs (LOG_VERBOSE, "\n");
          logprintf (LOG_NOTQUIET,
                     _("Unknown type `%c', closing control connection.\n"),
                     type_char);
          fd_close (csock);
          con->csock = -1;
          return err;
        case FTPOK:
          /* Everything is OK.  */
          break;
        default:
          abort ();
        }
      if (!opt.server_response)
        logputs (LOG_VERBOSE, _("done.  "));
    } /* do login */

  if (cmd & DO_CWD)
    {
      if (!*u->dir)
        logputs (LOG_VERBOSE, _("==> CWD not needed.\n"));
      else
        {
          char *target = u->dir;

          DEBUGP (("changing working directory\n"));

          /* Change working directory.  To change to a non-absolute
             Unix directory, we need to prepend initial directory
             (con->id) to it.  Absolute directories "just work".

             A relative directory is one that does not begin with '/'
             and, on non-Unix OS'es, one that doesn't begin with
             "[a-z]:".

             This is not done for OS400, which doesn't use
             "/"-delimited directories, nor does it support directory
             hierarchies.  "CWD foo" followed by "CWD bar" leaves us
             in "bar", not in "foo/bar", as would be customary
             elsewhere.  */

          if (target[0] != '/'
              && !(con->rs != ST_UNIX
                   && ISALPHA (target[0])
                   && target[1] == ':')
              && con->rs != ST_OS400)
            {
              int idlen = strlen (con->id);
              char *ntarget, *p;

              /* Strip trailing slash(es) from con->id. */
              while (idlen > 0 && con->id[idlen - 1] == '/')
                --idlen;
              p = ntarget = (char *)alloca (idlen + 1 + strlen (u->dir) + 1);
              memcpy (p, con->id, idlen);
              p += idlen;
              *p++ = '/';
              strcpy (p, target);

              DEBUGP (("Prepended initial PWD to relative path:\n"));
              DEBUGP (("   pwd: '%s'\n   old: '%s'\n  new: '%s'\n",
                       con->id, target, ntarget));
              target = ntarget;
            }

          /* If the FTP host runs VMS, we will have to convert the absolute
             directory path in UNIX notation to absolute directory path in
             VMS notation as VMS FTP servers do not like UNIX notation of
             absolute paths.  "VMS notation" is [dir.subdir.subsubdir]. */

          if (con->rs == ST_VMS)
            {
              char *tmpp;
              char *ntarget = (char *)alloca (strlen (target) + 2);
              /* We use a converted initial dir, so directories in
                 TARGET will be separated with slashes, something like
                 "/INITIAL/FOLDER/DIR/SUBDIR".  Convert that to
                 "[INITIAL.FOLDER.DIR.SUBDIR]".  */
              strcpy (ntarget, target);
              assert (*ntarget == '/');
              *ntarget = '[';
              for (tmpp = ntarget + 1; *tmpp; tmpp++)
                if (*tmpp == '/')
                  *tmpp = '.';
              *tmpp++ = ']';
              *tmpp = '\0';
              DEBUGP (("Changed file name to VMS syntax:\n"));
              DEBUGP (("  Unix: '%s'\n  VMS: '%s'\n", target, ntarget));
              target = ntarget;
            }

          if (!opt.server_response)
            logprintf (LOG_VERBOSE, "==> CWD %s ... ", escnonprint (target));
          err = ftp_cwd (csock, target);
          /* FTPRERR, WRITEFAILED, FTPNSFOD */
          switch (err)
            {
            case FTPRERR:
              logputs (LOG_VERBOSE, "\n");
              logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
              fd_close (csock);
              con->csock = -1;
              return err;
            case WRITEFAILED:
              logputs (LOG_VERBOSE, "\n");
              logputs (LOG_NOTQUIET,
                       _("Write failed, closing control connection.\n"));
              fd_close (csock);
              con->csock = -1;
              return err;
            case FTPNSFOD:
              logputs (LOG_VERBOSE, "\n");
              logprintf (LOG_NOTQUIET, _("No such directory `%s'.\n\n"),
                         escnonprint (u->dir));
              fd_close (csock);
              con->csock = -1;
              return err;
            case FTPOK:
              break;
            default:
              abort ();
            }
          if (!opt.server_response)
            logputs (LOG_VERBOSE, _("done.\n"));
        }
    }
  else /* do not CWD */
    logputs (LOG_VERBOSE, _("==> CWD not required.\n"));

  if ((cmd & DO_RETR) && *len == 0)
    {
      if (opt.verbose)
        {
          if (!opt.server_response)
            logprintf (LOG_VERBOSE, "==> SIZE %s ... ", escnonprint (u->file));
        }

      err = ftp_size (csock, u->file, len);
      /* FTPRERR */
      switch (err)
        {
        case FTPRERR:
        case FTPSRVERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          return err;
        case FTPOK:
          /* Everything is OK.  */
          break;
        default:
          abort ();
        }
        if (!opt.server_response)
          logprintf (LOG_VERBOSE, *len ? "%s\n" : _("done.\n"),
                     number_to_static_string (*len));
    }

  /* If anything is to be retrieved, PORT (or PASV) must be sent.  */
  if (cmd & (DO_LIST | DO_RETR))
    {
      if (opt.ftp_pasv)
        {
          ip_address passive_addr;
          int        passive_port;
          err = ftp_do_pasv (csock, &passive_addr, &passive_port);
          /* FTPRERR, WRITEFAILED, FTPNOPASV, FTPINVPASV */
          switch (err)
            {
            case FTPRERR:
              logputs (LOG_VERBOSE, "\n");
              logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
              fd_close (csock);
              con->csock = -1;
              return err;
            case WRITEFAILED:
              logputs (LOG_VERBOSE, "\n");
              logputs (LOG_NOTQUIET,
                       _("Write failed, closing control connection.\n"));
              fd_close (csock);
              con->csock = -1;
              return err;
            case FTPNOPASV:
              logputs (LOG_VERBOSE, "\n");
              logputs (LOG_NOTQUIET, _("Cannot initiate PASV transfer.\n"));
              break;
            case FTPINVPASV:
              logputs (LOG_VERBOSE, "\n");
              logputs (LOG_NOTQUIET, _("Cannot parse PASV response.\n"));
              break;
            case FTPOK:
              break;
            default:
              abort ();
            }   /* switch (err) */
          if (err==FTPOK)
            {
              DEBUGP (("trying to connect to %s port %d\n", 
                      print_address (&passive_addr), passive_port));
              dtsock = connect_to_ip (&passive_addr, passive_port, NULL);
              if (dtsock < 0)
                {
                  int save_errno = errno;
                  fd_close (csock);
                  con->csock = -1;
                  logprintf (LOG_VERBOSE, _("couldn't connect to %s port %d: %s\n"),
                             print_address (&passive_addr), passive_port,
                             strerror (save_errno));
                  return (retryable_socket_connect_error (save_errno)
                          ? CONERROR : CONIMPOSSIBLE);
                }

              pasv_mode_open = true;  /* Flag to avoid accept port */
              if (!opt.server_response)
                logputs (LOG_VERBOSE, _("done.    "));
            } /* err==FTP_OK */
        }

      if (!pasv_mode_open)   /* Try to use a port command if PASV failed */
        {
          err = ftp_do_port (csock, &local_sock);
          /* FTPRERR, WRITEFAILED, bindport (FTPSYSERR), HOSTERR,
             FTPPORTERR */
          switch (err)
            {
            case FTPRERR:
              logputs (LOG_VERBOSE, "\n");
              logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
              fd_close (csock);
              con->csock = -1;
              fd_close (dtsock);
              fd_close (local_sock);
              return err;
            case WRITEFAILED:
              logputs (LOG_VERBOSE, "\n");
              logputs (LOG_NOTQUIET,
                       _("Write failed, closing control connection.\n"));
              fd_close (csock);
              con->csock = -1;
              fd_close (dtsock);
              fd_close (local_sock);
              return err;
            case CONSOCKERR:
              logputs (LOG_VERBOSE, "\n");
              logprintf (LOG_NOTQUIET, "socket: %s\n", strerror (errno));
              fd_close (csock);
              con->csock = -1;
              fd_close (dtsock);
              fd_close (local_sock);
              return err;
            case FTPSYSERR:
              logputs (LOG_VERBOSE, "\n");
              logprintf (LOG_NOTQUIET, _("Bind error (%s).\n"),
                         strerror (errno));
              fd_close (dtsock);
              return err;
            case FTPPORTERR:
              logputs (LOG_VERBOSE, "\n");
              logputs (LOG_NOTQUIET, _("Invalid PORT.\n"));
              fd_close (csock);
              con->csock = -1;
              fd_close (dtsock);
              fd_close (local_sock);
              return err;
            case FTPOK:
              break;
            default:
              abort ();
            } /* port switch */
          if (!opt.server_response)
            logputs (LOG_VERBOSE, _("done.    "));
        } /* dtsock == -1 */
    } /* cmd & (DO_LIST | DO_RETR) */

  /* Restart if needed.  */
  if (restval && (cmd & DO_RETR))
    {
      if (!opt.server_response)
        logprintf (LOG_VERBOSE, "==> REST %s ... ",
                   number_to_static_string (restval));
      err = ftp_rest (csock, restval);

      /* FTPRERR, WRITEFAILED, FTPRESTFAIL */
      switch (err)
        {
        case FTPRERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          fd_close (dtsock);
          fd_close (local_sock);
          return err;
        case WRITEFAILED:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET,
                   _("Write failed, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          fd_close (dtsock);
          fd_close (local_sock);
          return err;
        case FTPRESTFAIL:
          logputs (LOG_VERBOSE, _("\nREST failed, starting from scratch.\n"));
          rest_failed = true;
          break;
        case FTPOK:
          break;
        default:
          abort ();
        }
      if (err != FTPRESTFAIL && !opt.server_response)
        logputs (LOG_VERBOSE, _("done.    "));
    } /* restval && cmd & DO_RETR */

  if (cmd & DO_RETR)
    {
      /* If we're in spider mode, don't really retrieve anything.  The
         fact that we got to this point should be proof enough that
         the file exists, vaguely akin to HTTP's concept of a "HEAD"
         request.  */
      if (opt.spider)
        {
          fd_close (csock);
          con->csock = -1;
          fd_close (dtsock);
          fd_close (local_sock);
          return RETRFINISHED;
        }

      if (opt.verbose)
        {
          if (!opt.server_response)
            {
              if (restval)
                logputs (LOG_VERBOSE, "\n");
              logprintf (LOG_VERBOSE, "==> RETR %s ... ", escnonprint (u->file));
            }
        }

      err = ftp_retr (csock, u->file);
      /* FTPRERR, WRITEFAILED, FTPNSFOD */
      switch (err)
        {
        case FTPRERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          fd_close (dtsock);
          fd_close (local_sock);
          return err;
        case WRITEFAILED:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET,
                   _("Write failed, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          fd_close (dtsock);
          fd_close (local_sock);
          return err;
        case FTPNSFOD:
          logputs (LOG_VERBOSE, "\n");
          logprintf (LOG_NOTQUIET, _("No such file `%s'.\n\n"),
                     escnonprint (u->file));
          fd_close (dtsock);
          fd_close (local_sock);
          return err;
        case FTPOK:
          break;
        default:
          abort ();
        }

      if (!opt.server_response)
        logputs (LOG_VERBOSE, _("done.\n"));
      expected_bytes = ftp_expected_bytes (ftp_last_respline);
    } /* do retrieve */

  if (cmd & DO_LIST)
    {
      if (!opt.server_response)
        logputs (LOG_VERBOSE, "==> LIST ... ");
      /* As Maciej W. Rozycki (macro@ds2.pg.gda.pl) says, `LIST'
         without arguments is better than `LIST .'; confirmed by
         RFC959.  */
      err = ftp_list (csock, NULL);
      /* FTPRERR, WRITEFAILED */
      switch (err)
        {
        case FTPRERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          fd_close (dtsock);
          fd_close (local_sock);
          return err;
        case WRITEFAILED:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET,
                   _("Write failed, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          fd_close (dtsock);
          fd_close (local_sock);
          return err;
        case FTPNSFOD:
          logputs (LOG_VERBOSE, "\n");
          logprintf (LOG_NOTQUIET, _("No such file or directory `%s'.\n\n"),
                     ".");
          fd_close (dtsock);
          fd_close (local_sock);
          return err;
        case FTPOK:
          break;
        default:
          abort ();
        }
      if (!opt.server_response)
        logputs (LOG_VERBOSE, _("done.\n"));
      expected_bytes = ftp_expected_bytes (ftp_last_respline);
    } /* cmd & DO_LIST */

  if (!(cmd & (DO_LIST | DO_RETR)) || (opt.spider && !(cmd & DO_LIST)))
    return RETRFINISHED;

  /* Some FTP servers return the total length of file after REST
     command, others just return the remaining size. */
  if (*len && restval && expected_bytes
      && (expected_bytes == *len - restval))
    {
      DEBUGP (("Lying FTP server found, adjusting.\n"));
      expected_bytes = *len;
    }

  /* If no transmission was required, then everything is OK.  */
  if (!pasv_mode_open)  /* we are not using pasive mode so we need
                              to accept */
    {
      /* Wait for the server to connect to the address we're waiting
         at.  */
      dtsock = accept_connection (local_sock);
      if (dtsock < 0)
        {
          logprintf (LOG_NOTQUIET, "accept: %s\n", strerror (errno));
          return err;
        }
    }

  /* Open the file -- if output_stream is set, use it instead.  */
  if (!output_stream || con->cmd & DO_LIST)
    {
      mkalldirs (con->target);
      if (opt.backups)
        rotate_backups (con->target);

      if (restval && !(con->cmd & DO_LIST))
        fp = fopen (con->target, "ab");
      else if (opt.noclobber || opt.always_rest || opt.timestamping || opt.dirstruct
               || opt.output_document)
        fp = fopen (con->target, "wb");
      else
        {
          fp = fopen_excl (con->target, true);
          if (!fp && errno == EEXIST)
            {
              /* We cannot just invent a new name and use it (which is
                 what functions like unique_create typically do)
                 because we told the user we'd use this name.
                 Instead, return and retry the download.  */
              logprintf (LOG_NOTQUIET, _("%s has sprung into existence.\n"),
                         con->target);
              fd_close (csock);
              con->csock = -1;
              fd_close (dtsock);
              fd_close (local_sock);
              return FOPEN_EXCL_ERR;
            }
        }
      if (!fp)
        {
          logprintf (LOG_NOTQUIET, "%s: %s\n", con->target, strerror (errno));
          fd_close (csock);
          con->csock = -1;
          fd_close (dtsock);
          fd_close (local_sock);
          return FOPENERR;
        }
    }
  else
    fp = output_stream;

  if (*len)
    {
      print_length (*len, restval, true);
      expected_bytes = *len;    /* for fd_read_body's progress bar */
    }
  else if (expected_bytes)
    print_length (expected_bytes, restval, false);

  /* Get the contents of the document.  */
  flags = 0;
  if (restval && rest_failed)
    flags |= rb_skip_startpos;
  *len = restval;
  rd_size = 0;
  res = fd_read_body (dtsock, fp,
                      expected_bytes ? expected_bytes - restval : 0,
                      restval, &rd_size, len, &con->dltime, flags);

  tms = datetime_str (time (NULL));
  tmrate = retr_rate (rd_size, con->dltime);
  total_download_time += con->dltime;

  fd_close (local_sock);
  /* Close the local file.  */
  if (!output_stream || con->cmd & DO_LIST)
    fclose (fp);

  /* If fd_read_body couldn't write to fp, bail out.  */
  if (res == -2)
    {
      logprintf (LOG_NOTQUIET, _("%s: %s, closing control connection.\n"),
                 con->target, strerror (errno));
      fd_close (csock);
      con->csock = -1;
      fd_close (dtsock);
      return FWRITEERR;
    }
  else if (res == -1)
    {
      logprintf (LOG_NOTQUIET, _("%s (%s) - Data connection: %s; "),
                 tms, tmrate, fd_errstr (dtsock));
      if (opt.server_response)
        logputs (LOG_ALWAYS, "\n");
    }
  fd_close (dtsock);

  /* Get the server to tell us if everything is retrieved.  */
  err = ftp_response (csock, &respline);
  if (err != FTPOK)
    {
      /* The control connection is decidedly closed.  Print the time
         only if it hasn't already been printed.  */
      if (res != -1)
        logprintf (LOG_NOTQUIET, "%s (%s) - ", tms, tmrate);
      logputs (LOG_NOTQUIET, _("Control connection closed.\n"));
      /* If there is an error on the control connection, close it, but
         return FTPRETRINT, since there is a possibility that the
         whole file was retrieved nevertheless (but that is for
         ftp_loop_internal to decide).  */
      fd_close (csock);
      con->csock = -1;
      return FTPRETRINT;
    } /* err != FTPOK */
  /* If retrieval failed for any reason, return FTPRETRINT, but do not
     close socket, since the control connection is still alive.  If
     there is something wrong with the control connection, it will
     become apparent later.  */
  if (*respline != '2')
    {
      xfree (respline);
      if (res != -1)
        logprintf (LOG_NOTQUIET, "%s (%s) - ", tms, tmrate);
      logputs (LOG_NOTQUIET, _("Data transfer aborted.\n"));
      return FTPRETRINT;
    }
  xfree (respline);

  if (res == -1)
    {
      /* What now?  The data connection was erroneous, whereas the
         response says everything is OK.  We shall play it safe.  */
      return FTPRETRINT;
    }

  if (!(cmd & LEAVE_PENDING))
    {
      /* Closing the socket is faster than sending 'QUIT' and the
         effect is the same.  */
      fd_close (csock);
      con->csock = -1;
    }
  /* If it was a listing, and opt.server_response is true,
     print it out.  */
  if (opt.server_response && (con->cmd & DO_LIST))
    {
      mkalldirs (con->target);
      fp = fopen (con->target, "r");
      if (!fp)
        logprintf (LOG_ALWAYS, "%s: %s\n", con->target, strerror (errno));
      else
        {
          char *line;
          /* The lines are being read with read_whole_line because of
             no-buffering on opt.lfile.  */
          while ((line = read_whole_line (fp)) != NULL)
            {
              char *p = strchr (line, '\0');
              while (p > line && (p[-1] == '\n' || p[-1] == '\r'))
                *--p = '\0';
              logprintf (LOG_ALWAYS, "%s\n", escnonprint (line));
              xfree (line);
            }
          fclose (fp);
        }
    } /* con->cmd & DO_LIST && server_response */

  return RETRFINISHED;
}

/* A one-file FTP loop.  This is the part where FTP retrieval is
   retried, and retried, and retried, and...

   This loop either gets commands from con, or (if ON_YOUR_OWN is
   set), makes them up to retrieve the file given by the URL.  */
static uerr_t
ftp_loop_internal (struct url *u, struct fileinfo *f, ccon *con)
{
  int count, orig_lp;
  wgint restval, len = 0;
  char *tms, *locf;
  const char *tmrate = NULL;
  uerr_t err;
  struct_stat st;

  if (!con->target)
    con->target = url_file_name (u);

  /* If the output_document was given, then this check was already done and
     the file didn't exist. Hence the !opt.output_document */
  if (opt.noclobber && !opt.output_document && file_exists_p (con->target))
    {
      logprintf (LOG_VERBOSE,
                 _("File `%s' already there; not retrieving.\n"), con->target);
      /* If the file is there, we suppose it's retrieved OK.  */
      return RETROK;
    }

  /* Remove it if it's a link.  */
  remove_link (con->target);
  if (!opt.output_document)
    locf = con->target;
  else
    locf = opt.output_document;

  count = 0;

  if (con->st & ON_YOUR_OWN)
    con->st = ON_YOUR_OWN;

  orig_lp = con->cmd & LEAVE_PENDING ? 1 : 0;

  /* THE loop.  */
  do
    {
      /* Increment the pass counter.  */
      ++count;
      sleep_between_retrievals (count);
      if (con->st & ON_YOUR_OWN)
        {
          con->cmd = 0;
          con->cmd |= (DO_RETR | LEAVE_PENDING);
          if (con->csock != -1)
            con->cmd &= ~ (DO_LOGIN | DO_CWD);
          else
            con->cmd |= (DO_LOGIN | DO_CWD);
        }
      else /* not on your own */
        {
          if (con->csock != -1)
            con->cmd &= ~DO_LOGIN;
          else
            con->cmd |= DO_LOGIN;
          if (con->st & DONE_CWD)
            con->cmd &= ~DO_CWD;
          else
            con->cmd |= DO_CWD;
        }

      /* Decide whether or not to restart.  */
      if (con->cmd & DO_LIST)
        restval = 0;
      else if (opt.always_rest
          && stat (locf, &st) == 0
          && S_ISREG (st.st_mode))
        /* When -c is used, continue from on-disk size.  (Can't use
           hstat.len even if count>1 because we don't want a failed
           first attempt to clobber existing data.)  */
        restval = st.st_size;
      else if (count > 1)
        restval = len;          /* start where the previous run left off */
      else
        restval = 0;

      /* Get the current time string.  */
      tms = datetime_str (time (NULL));
      /* Print fetch message, if opt.verbose.  */
      if (opt.verbose)
        {
          char *hurl = url_string (u, URL_AUTH_HIDE_PASSWD);
          char tmp[256];
          strcpy (tmp, "        ");
          if (count > 1)
            sprintf (tmp, _("(try:%2d)"), count);
          logprintf (LOG_VERBOSE, "--%s--  %s\n  %s => `%s'\n",
                     tms, hurl, tmp, locf);
#ifdef WINDOWS
          ws_changetitle (hurl);
#endif
          xfree (hurl);
        }
      /* Send getftp the proper length, if fileinfo was provided.  */
      if (f)
        len = f->size;
      else
        len = 0;
      err = getftp (u, &len, restval, con);

      if (con->csock == -1)
        con->st &= ~DONE_CWD;
      else
        con->st |= DONE_CWD;

      switch (err)
        {
        case HOSTERR: case CONIMPOSSIBLE: case FWRITEERR: case FOPENERR:
        case FTPNSFOD: case FTPLOGINC: case FTPNOPASV: case CONTNOTSUPPORTED:
          /* Fatal errors, give up.  */
          return err;
        case CONSOCKERR: case CONERROR: case FTPSRVERR: case FTPRERR:
        case WRITEFAILED: case FTPUNKNOWNTYPE: case FTPSYSERR:
        case FTPPORTERR: case FTPLOGREFUSED: case FTPINVPASV:
        case FOPEN_EXCL_ERR:
          printwhat (count, opt.ntry);
          /* non-fatal errors */
          if (err == FOPEN_EXCL_ERR)
            {
              /* Re-determine the file name. */
              xfree_null (con->target);
              con->target = url_file_name (u);
              locf = con->target;
            }
          continue;
        case FTPRETRINT:
          /* If the control connection was closed, the retrieval
             will be considered OK if f->size == len.  */
          if (!f || len != f->size)
            {
              printwhat (count, opt.ntry);
              continue;
            }
          break;
        case RETRFINISHED:
          /* Great!  */
          break;
        default:
          /* Not as great.  */
          abort ();
        }
      tms = datetime_str (time (NULL));
      if (!opt.spider)
        tmrate = retr_rate (len - restval, con->dltime);

      /* If we get out of the switch above without continue'ing, we've
         successfully downloaded a file.  Remember this fact. */
      downloaded_file (FILE_DOWNLOADED_NORMALLY, locf);

      if (con->st & ON_YOUR_OWN)
        {
          fd_close (con->csock);
          con->csock = -1;
        }
      if (!opt.spider)
        logprintf (LOG_VERBOSE, _("%s (%s) - `%s' saved [%s]\n\n"),
                   tms, tmrate, locf, number_to_static_string (len));
      if (!opt.verbose && !opt.quiet)
        {
          /* Need to hide the password from the URL.  The `if' is here
             so that we don't do the needless allocation every
             time. */
          char *hurl = url_string (u, URL_AUTH_HIDE_PASSWD);
          logprintf (LOG_NONVERBOSE, "%s URL: %s [%s] -> \"%s\" [%d]\n",
                     tms, hurl, number_to_static_string (len), locf, count);
          xfree (hurl);
        }

      if ((con->cmd & DO_LIST))
        /* This is a directory listing file. */
        {
          if (!opt.remove_listing)
            /* --dont-remove-listing was specified, so do count this towards the
               number of bytes and files downloaded. */
            {
              total_downloaded_bytes += len;
              opt.numurls++;
            }

          /* Deletion of listing files is not controlled by --delete-after, but
             by the more specific option --dont-remove-listing, and the code
             to do this deletion is in another function. */
        }
      else if (!opt.spider)
        /* This is not a directory listing file. */
        {
          /* Unlike directory listing files, don't pretend normal files weren't
             downloaded if they're going to be deleted.  People seeding proxies,
             for instance, may want to know how many bytes and files they've
             downloaded through it. */
          total_downloaded_bytes += len;
          opt.numurls++;

          if (opt.delete_after)
            {
              DEBUGP (("\
Removing file due to --delete-after in ftp_loop_internal():\n"));
              logprintf (LOG_VERBOSE, _("Removing %s.\n"), locf);
              if (unlink (locf))
                logprintf (LOG_NOTQUIET, "unlink: %s\n", strerror (errno));
            }
        }

      /* Restore the original leave-pendingness.  */
      if (orig_lp)
        con->cmd |= LEAVE_PENDING;
      else
        con->cmd &= ~LEAVE_PENDING;
      return RETROK;
    } while (!opt.ntry || (count < opt.ntry));

  if (con->csock != -1 && (con->st & ON_YOUR_OWN))
    {
      fd_close (con->csock);
      con->csock = -1;
    }
  return TRYLIMEXC;
}

/* Return the directory listing in a reusable format.  The directory
   is specifed in u->dir.  */
static uerr_t
ftp_get_listing (struct url *u, ccon *con, struct fileinfo **f)
{
  uerr_t err;
  char *uf;                     /* url file name */
  char *lf;                     /* list file name */
  char *old_target = con->target;

  con->st &= ~ON_YOUR_OWN;
  con->cmd |= (DO_LIST | LEAVE_PENDING);
  con->cmd &= ~DO_RETR;

  /* Find the listing file name.  We do it by taking the file name of
     the URL and replacing the last component with the listing file
     name.  */
  uf = url_file_name (u);
  lf = file_merge (uf, LIST_FILENAME);
  xfree (uf);
  DEBUGP ((_("Using `%s' as listing tmp file.\n"), lf));

  con->target = lf;
  err = ftp_loop_internal (u, NULL, con);
  con->target = old_target;

  if (err == RETROK)
    *f = ftp_parse_ls (lf, con->rs);
  else
    *f = NULL;
  if (opt.remove_listing)
    {
      if (unlink (lf))
        logprintf (LOG_NOTQUIET, "unlink: %s\n", strerror (errno));
      else
        logprintf (LOG_VERBOSE, _("Removed `%s'.\n"), lf);
    }
  xfree (lf);
  con->cmd &= ~DO_LIST;
  return err;
}

static uerr_t ftp_retrieve_dirs (struct url *, struct fileinfo *, ccon *);
static uerr_t ftp_retrieve_glob (struct url *, ccon *, int);
static struct fileinfo *delelement (struct fileinfo *, struct fileinfo **);
static void freefileinfo (struct fileinfo *f);

/* Retrieve a list of files given in struct fileinfo linked list.  If
   a file is a symbolic link, do not retrieve it, but rather try to
   set up a similar link on the local disk, if the symlinks are
   supported.

   If opt.recursive is set, after all files have been retrieved,
   ftp_retrieve_dirs will be called to retrieve the directories.  */
static uerr_t
ftp_retrieve_list (struct url *u, struct fileinfo *f, ccon *con)
{
  static int depth = 0;
  uerr_t err;
  struct fileinfo *orig;
  wgint local_size;
  time_t tml;
  bool dlthis;

  /* Increase the depth.  */
  ++depth;
  if (opt.reclevel != INFINITE_RECURSION && depth > opt.reclevel)
    {
      DEBUGP ((_("Recursion depth %d exceeded max. depth %d.\n"),
               depth, opt.reclevel));
      --depth;
      return RECLEVELEXC;
    }

  assert (f != NULL);
  orig = f;

  con->st &= ~ON_YOUR_OWN;
  if (!(con->st & DONE_CWD))
    con->cmd |= DO_CWD;
  else
    con->cmd &= ~DO_CWD;
  con->cmd |= (DO_RETR | LEAVE_PENDING);

  if (con->csock < 0)
    con->cmd |= DO_LOGIN;
  else
    con->cmd &= ~DO_LOGIN;

  err = RETROK;                 /* in case it's not used */

  while (f)
    {
      char *old_target, *ofile;

      if (opt.quota && total_downloaded_bytes > opt.quota)
        {
          --depth;
          return QUOTEXC;
        }
      old_target = con->target;

      ofile = xstrdup (u->file);
      url_set_file (u, f->name);

      con->target = url_file_name (u);
      err = RETROK;

      dlthis = true;
      if (opt.timestamping && f->type == FT_PLAINFILE)
        {
          struct_stat st;
          /* If conversion of HTML files retrieved via FTP is ever implemented,
             we'll need to stat() <file>.orig here when -K has been specified.
             I'm not implementing it now since files on an FTP server are much
             more likely than files on an HTTP server to legitimately have a
             .orig suffix. */
          if (!stat (con->target, &st))
            {
              bool eq_size;
              bool cor_val;
              /* Else, get it from the file.  */
              local_size = st.st_size;
              tml = st.st_mtime;
#ifdef WINDOWS
              /* Modification time granularity is 2 seconds for Windows, so
                 increase local time by 1 second for later comparison. */
              tml++;
#endif
              /* Compare file sizes only for servers that tell us correct
                 values. Assume sizes being equal for servers that lie
                 about file size.  */
              cor_val = (con->rs == ST_UNIX || con->rs == ST_WINNT);
              eq_size = cor_val ? (local_size == f->size) : true;
              if (f->tstamp <= tml && eq_size)
                {
                  /* Remote file is older, file sizes can be compared and
                     are both equal. */
                  logprintf (LOG_VERBOSE, _("\
Remote file no newer than local file `%s' -- not retrieving.\n"), con->target);
                  dlthis = false;
                }
              else if (eq_size)
                {
                  /* Remote file is newer or sizes cannot be matched */
                  logprintf (LOG_VERBOSE, _("\
Remote file is newer than local file `%s' -- retrieving.\n\n"),
                             con->target);
                }
              else
                {
                  /* Sizes do not match */
                  logprintf (LOG_VERBOSE, _("\
The sizes do not match (local %s) -- retrieving.\n\n"),
                             number_to_static_string (local_size));
                }
            }
        }       /* opt.timestamping && f->type == FT_PLAINFILE */
      switch (f->type)
        {
        case FT_SYMLINK:
          /* If opt.retr_symlinks is defined, we treat symlinks as
             if they were normal files.  There is currently no way
             to distinguish whether they might be directories, and
             follow them.  */
          if (!opt.retr_symlinks)
            {
#ifdef HAVE_SYMLINK
              if (!f->linkto)
                logputs (LOG_NOTQUIET,
                         _("Invalid name of the symlink, skipping.\n"));
              else
                {
                  struct_stat st;
                  /* Check whether we already have the correct
                     symbolic link.  */
                  int rc = lstat (con->target, &st);
                  if (rc == 0)
                    {
                      size_t len = strlen (f->linkto) + 1;
                      if (S_ISLNK (st.st_mode))
                        {
                          char *link_target = (char *)alloca (len);
                          size_t n = readlink (con->target, link_target, len);
                          if ((n == len - 1)
                              && (memcmp (link_target, f->linkto, n) == 0))
                            {
                              logprintf (LOG_VERBOSE, _("\
Already have correct symlink %s -> %s\n\n"),
                                         con->target, escnonprint (f->linkto));
                              dlthis = false;
                              break;
                            }
                        }
                    }
                  logprintf (LOG_VERBOSE, _("Creating symlink %s -> %s\n"),
                             con->target, escnonprint (f->linkto));
                  /* Unlink before creating symlink!  */
                  unlink (con->target);
                  if (symlink (f->linkto, con->target) == -1)
                    logprintf (LOG_NOTQUIET, "symlink: %s\n", strerror (errno));
                  logputs (LOG_VERBOSE, "\n");
                } /* have f->linkto */
#else  /* not HAVE_SYMLINK */
              logprintf (LOG_NOTQUIET,
                         _("Symlinks not supported, skipping symlink `%s'.\n"),
                         con->target);
#endif /* not HAVE_SYMLINK */
            }
          else                /* opt.retr_symlinks */
            {
              if (dlthis)
                err = ftp_loop_internal (u, f, con);
            } /* opt.retr_symlinks */
          break;
        case FT_DIRECTORY:
          if (!opt.recursive)
            logprintf (LOG_NOTQUIET, _("Skipping directory `%s'.\n"),
                       escnonprint (f->name));
          break;
        case FT_PLAINFILE:
          /* Call the retrieve loop.  */
          if (dlthis)
            err = ftp_loop_internal (u, f, con);
          break;
        case FT_UNKNOWN:
          logprintf (LOG_NOTQUIET, _("%s: unknown/unsupported file type.\n"),
                     escnonprint (f->name));
          break;
        }       /* switch */

      /* Set the time-stamp information to the local file.  Symlinks
         are not to be stamped because it sets the stamp on the
         original.  :( */
      if (!(f->type == FT_SYMLINK && !opt.retr_symlinks)
          && f->tstamp != -1
          && dlthis
          && file_exists_p (con->target))
        {
          /* #### This code repeats in http.c and ftp.c.  Move it to a
             function!  */
          const char *fl = NULL;
          if (opt.output_document)
            {
              if (output_stream_regular)
                fl = opt.output_document;
            }
          else
            fl = con->target;
          if (fl)
            touch (fl, f->tstamp);
        }
      else if (f->tstamp == -1)
        logprintf (LOG_NOTQUIET, _("%s: corrupt time-stamp.\n"), con->target);

      if (f->perms && f->type == FT_PLAINFILE && dlthis)
        {
          if (opt.preserve_perm)
            chmod (con->target, f->perms);
        }
      else
        DEBUGP (("Unrecognized permissions for %s.\n", con->target));

      xfree (con->target);
      con->target = old_target;

      url_set_file (u, ofile);
      xfree (ofile);

      /* Break on fatals.  */
      if (err == QUOTEXC || err == HOSTERR || err == FWRITEERR)
        break;
      con->cmd &= ~ (DO_CWD | DO_LOGIN);
      f = f->next;
    }

  /* We do not want to call ftp_retrieve_dirs here */
  if (opt.recursive &&
      !(opt.reclevel != INFINITE_RECURSION && depth >= opt.reclevel))
    err = ftp_retrieve_dirs (u, orig, con);
  else if (opt.recursive)
    DEBUGP ((_("Will not retrieve dirs since depth is %d (max %d).\n"),
             depth, opt.reclevel));
  --depth;
  return err;
}

/* Retrieve the directories given in a file list.  This function works
   by simply going through the linked list and calling
   ftp_retrieve_glob on each directory entry.  The function knows
   about excluded directories.  */
static uerr_t
ftp_retrieve_dirs (struct url *u, struct fileinfo *f, ccon *con)
{
  char *container = NULL;
  int container_size = 0;

  for (; f; f = f->next)
    {
      int size;
      char *odir, *newdir;

      if (opt.quota && total_downloaded_bytes > opt.quota)
        break;
      if (f->type != FT_DIRECTORY)
        continue;

      /* Allocate u->dir off stack, but reallocate only if a larger
         string is needed.  It's a pity there's no "realloca" for an
         item on the bottom of the stack.  */
      size = strlen (u->dir) + 1 + strlen (f->name) + 1;
      if (size > container_size)
        container = (char *)alloca (size);
      newdir = container;

      odir = u->dir;
      if (*odir == '\0'
          || (*odir == '/' && *(odir + 1) == '\0'))
        /* If ODIR is empty or just "/", simply append f->name to
           ODIR.  (In the former case, to preserve u->dir being
           relative; in the latter case, to avoid double slash.)  */
        sprintf (newdir, "%s%s", odir, f->name);
      else
        /* Else, use a separator. */
        sprintf (newdir, "%s/%s", odir, f->name);

      DEBUGP (("Composing new CWD relative to the initial directory.\n"));
      DEBUGP (("  odir = '%s'\n  f->name = '%s'\n  newdir = '%s'\n\n",
               odir, f->name, newdir));
      if (!accdir (newdir))
        {
          logprintf (LOG_VERBOSE, _("\
Not descending to `%s' as it is excluded/not-included.\n"),
                     escnonprint (newdir));
          continue;
        }

      con->st &= ~DONE_CWD;

      odir = xstrdup (u->dir);  /* because url_set_dir will free
                                   u->dir. */
      url_set_dir (u, newdir);
      ftp_retrieve_glob (u, con, GLOB_GETALL);
      url_set_dir (u, odir);
      xfree (odir);

      /* Set the time-stamp?  */
    }

  if (opt.quota && total_downloaded_bytes > opt.quota)
    return QUOTEXC;
  else
    return RETROK;
}

/* Return true if S has a leading '/'  or contains '../' */
static bool
has_insecure_name_p (const char *s)
{
  if (*s == '/')
    return true;

  if (strstr (s, "../") != 0)
    return true;

  return false;
}

/* A near-top-level function to retrieve the files in a directory.
   The function calls ftp_get_listing, to get a linked list of files.
   Then it weeds out the file names that do not match the pattern.
   ftp_retrieve_list is called with this updated list as an argument.

   If the argument ACTION is GLOB_GETONE, just download the file (but
   first get the listing, so that the time-stamp is heeded); if it's
   GLOB_GLOBALL, use globbing; if it's GLOB_GETALL, download the whole
   directory.  */
static uerr_t
ftp_retrieve_glob (struct url *u, ccon *con, int action)
{
  struct fileinfo *f, *start;
  uerr_t res;

  con->cmd |= LEAVE_PENDING;

  res = ftp_get_listing (u, con, &start);
  if (res != RETROK)
    return res;
  /* First: weed out that do not conform the global rules given in
     opt.accepts and opt.rejects.  */
  if (opt.accepts || opt.rejects)
    {
      f = start;
      while (f)
        {
          if (f->type != FT_DIRECTORY && !acceptable (f->name))
            {
              logprintf (LOG_VERBOSE, _("Rejecting `%s'.\n"),
                         escnonprint (f->name));
              f = delelement (f, &start);
            }
          else
            f = f->next;
        }
    }
  /* Remove all files with possible harmful names */
  f = start;
  while (f)
    {
      if (has_insecure_name_p (f->name))
        {
          logprintf (LOG_VERBOSE, _("Rejecting `%s'.\n"),
                     escnonprint (f->name));
          f = delelement (f, &start);
        }
      else
        f = f->next;
    }
  /* Now weed out the files that do not match our globbing pattern.
     If we are dealing with a globbing pattern, that is.  */
  if (*u->file)
    {
      if (action == GLOB_GLOBALL)
        {
          int (*matcher) (const char *, const char *, int)
            = opt.ignore_case ? fnmatch_nocase : fnmatch;
          int matchres = 0;

          f = start;
          while (f)
            {
              matchres = matcher (u->file, f->name, 0);
              if (matchres == -1)
                {
                  logprintf (LOG_NOTQUIET, _("Error matching %s against %s: %s\n"),
                             u->file, escnonprint (f->name), strerror (errno));
                  break;
                }
              if (matchres == FNM_NOMATCH)
                f = delelement (f, &start); /* delete the element from the list */
              else
                f = f->next;        /* leave the element in the list */
            }
          if (matchres == -1)
            {
              freefileinfo (start);
              return RETRBADPATTERN;
            }
        }
      else if (action == GLOB_GETONE)
        {
          int (*cmp) (const char *, const char *)
            = opt.ignore_case ? strcasecmp : strcmp;
          f = start;
          while (f)
            {
              if (0 != cmp(u->file, f->name))
                f = delelement (f, &start);
              else
                f = f->next;
            }
        }
    }
  if (start)
    {
      /* Just get everything.  */
      ftp_retrieve_list (u, start, con);
    }
  else
    {
      if (action == GLOB_GLOBALL)
        {
          /* No luck.  */
          /* #### This message SUCKS.  We should see what was the
             reason that nothing was retrieved.  */
          logprintf (LOG_VERBOSE, _("No matches on pattern `%s'.\n"),
                     escnonprint (u->file));
        }
      else if (*u->file) /* GLOB_GETONE or GLOB_GETALL */
        {
          /* Let's try retrieving it anyway.  */
          con->st |= ON_YOUR_OWN;
          res = ftp_loop_internal (u, NULL, con);
          return res;
        }
    }
  freefileinfo (start);
  if (opt.quota && total_downloaded_bytes > opt.quota)
    return QUOTEXC;
  else
    /* #### Should we return `res' here?  */
    return RETROK;
}

/* The wrapper that calls an appropriate routine according to contents
   of URL.  Inherently, its capabilities are limited on what can be
   encoded into a URL.  */
uerr_t
ftp_loop (struct url *u, int *dt, struct url *proxy, bool recursive, bool glob)
{
  ccon con;                     /* FTP connection */
  uerr_t res;

  *dt = 0;

  xzero (con);

  con.csock = -1;
  con.st = ON_YOUR_OWN;
  con.rs = ST_UNIX;
  con.id = NULL;
  con.proxy = proxy;

  /* If the file name is empty, the user probably wants a directory
     index.  We'll provide one, properly HTML-ized.  Unless
     opt.htmlify is 0, of course.  :-) */
  if (!*u->file && !recursive)
    {
      struct fileinfo *f;
      res = ftp_get_listing (u, &con, &f);

      if (res == RETROK)
        {
          if (opt.htmlify && !opt.spider)
            {
              char *filename = (opt.output_document
                                ? xstrdup (opt.output_document)
                                : (con.target ? xstrdup (con.target)
                                   : url_file_name (u)));
              res = ftp_index (filename, u, f);
              if (res == FTPOK && opt.verbose)
                {
                  if (!opt.output_document)
                    {
                      struct_stat st;
                      wgint sz;
                      if (stat (filename, &st) == 0)
                        sz = st.st_size;
                      else
                        sz = -1;
                      logprintf (LOG_NOTQUIET,
                                 _("Wrote HTML-ized index to `%s' [%s].\n"),
                                 filename, number_to_static_string (sz));
                    }
                  else
                    logprintf (LOG_NOTQUIET,
                               _("Wrote HTML-ized index to `%s'.\n"),
                               filename);
                }
              xfree (filename);
            }
          freefileinfo (f);
        }
    }
  else
    {
      bool ispattern = false;
      if (glob)
        {
          /* Treat the URL as a pattern if the file name part of the
             URL path contains wildcards.  (Don't check for u->file
             because it is unescaped and therefore doesn't leave users
             the option to escape literal '*' as %2A.)  */
          char *file_part = strrchr (u->path, '/');
          if (!file_part)
            file_part = u->path;
          ispattern = has_wildcards_p (file_part);
        }
      if (ispattern || recursive || opt.timestamping)
        {
          /* ftp_retrieve_glob is a catch-all function that gets called
             if we need globbing, time-stamping or recursion.  Its
             third argument is just what we really need.  */
          res = ftp_retrieve_glob (u, &con,
                                   ispattern ? GLOB_GLOBALL : GLOB_GETONE);
        }
      else
        res = ftp_loop_internal (u, NULL, &con);
    }
  if (res == FTPOK)
    res = RETROK;
  if (res == RETROK)
    *dt |= RETROKF;
  /* If a connection was left, quench it.  */
  if (con.csock != -1)
    fd_close (con.csock);
  xfree_null (con.id);
  con.id = NULL;
  xfree_null (con.target);
  con.target = NULL;
  return res;
}

/* Delete an element from the fileinfo linked list.  Returns the
   address of the next element, or NULL if the list is exhausted.  It
   can modify the start of the list.  */
static struct fileinfo *
delelement (struct fileinfo *f, struct fileinfo **start)
{
  struct fileinfo *prev = f->prev;
  struct fileinfo *next = f->next;

  xfree (f->name);
  xfree_null (f->linkto);
  xfree (f);

  if (next)
    next->prev = prev;
  if (prev)
    prev->next = next;
  else
    *start = next;
  return next;
}

/* Free the fileinfo linked list of files.  */
static void
freefileinfo (struct fileinfo *f)
{
  while (f)
    {
      struct fileinfo *next = f->next;
      xfree (f->name);
      if (f->linkto)
        xfree (f->linkto);
      xfree (f);
      f = next;
    }
}
