/* File retrieval.
   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,
   2004, 2005, 2006, 2007, 2008 Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at
your option) any later version.

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
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "wget.h"
#include "utils.h"
#include "retr.h"
#include "progress.h"
#include "url.h"
#include "recur.h"
#include "ftp.h"
#include "http.h"
#include "host.h"
#include "connect.h"
#include "hash.h"
#include "convert.h"
#include "ptimer.h"

/* Total size of downloaded files.  Used to enforce quota.  */
SUM_SIZE_INT total_downloaded_bytes;

/* Total download time in seconds. */
double total_download_time;

/* If non-NULL, the stream to which output should be written.  This
   stream is initialized when `-O' is used.  */
FILE *output_stream;

/* Whether output_document is a regular file we can manipulate,
   i.e. not `-' or a device file. */
bool output_stream_regular;

static struct {
  wgint chunk_bytes;
  double chunk_start;
  double sleep_adjust;
} limit_data;

static void
limit_bandwidth_reset (void)
{
  xzero (limit_data);
}

/* Limit the bandwidth by pausing the download for an amount of time.
   BYTES is the number of bytes received from the network, and TIMER
   is the timer that started at the beginning of download.  */

static void
limit_bandwidth (wgint bytes, struct ptimer *timer)
{
  double delta_t = ptimer_read (timer) - limit_data.chunk_start;
  double expected;

  limit_data.chunk_bytes += bytes;

  /* Calculate the amount of time we expect downloading the chunk
     should take.  If in reality it took less time, sleep to
     compensate for the difference.  */
  expected = (double) limit_data.chunk_bytes / opt.limit_rate;

  if (expected > delta_t)
    {
      double slp = expected - delta_t + limit_data.sleep_adjust;
      double t0, t1;
      if (slp < 0.2)
        {
          DEBUGP (("deferring a %.2f ms sleep (%s/%.2f).\n",
                   slp * 1000, number_to_static_string (limit_data.chunk_bytes),
                   delta_t));
          return;
        }
      DEBUGP (("\nsleeping %.2f ms for %s bytes, adjust %.2f ms\n",
               slp * 1000, number_to_static_string (limit_data.chunk_bytes),
               limit_data.sleep_adjust));

      t0 = ptimer_read (timer);
      xsleep (slp);
      t1 = ptimer_measure (timer);

      /* Due to scheduling, we probably slept slightly longer (or
         shorter) than desired.  Calculate the difference between the
         desired and the actual sleep, and adjust the next sleep by
         that amount.  */
      limit_data.sleep_adjust = slp - (t1 - t0);
      /* If sleep_adjust is very large, it's likely due to suspension
         and not clock inaccuracy.  Don't enforce those.  */
      if (limit_data.sleep_adjust > 0.5)
        limit_data.sleep_adjust = 0.5;
      else if (limit_data.sleep_adjust < -0.5)
        limit_data.sleep_adjust = -0.5;
    }

  limit_data.chunk_bytes = 0;
  limit_data.chunk_start = ptimer_read (timer);
}

#ifndef MIN
# define MIN(i, j) ((i) <= (j) ? (i) : (j))
#endif

/* Write data in BUF to OUT.  However, if *SKIP is non-zero, skip that
   amount of data and decrease SKIP.  Increment *TOTAL by the amount
   of data written.  */

static int
write_data (FILE *out, const char *buf, int bufsize, wgint *skip,
            wgint *written)
{
  if (!out)
    return 1;
  if (*skip > bufsize)
    {
      *skip -= bufsize;
      return 1;
    }
  if (*skip)
    {
      buf += *skip;
      bufsize -= *skip;
      *skip = 0;
      if (bufsize == 0)
        return 1;
    }

  fwrite (buf, 1, bufsize, out);
  *written += bufsize;

  /* Immediately flush the downloaded data.  This should not hinder
     performance: fast downloads will arrive in large 16K chunks
     (which stdio would write out immediately anyway), and slow
     downloads wouldn't be limited by disk speed.  */
  fflush (out);
  return !ferror (out);
}

/* Read the contents of file descriptor FD until it the connection
   terminates or a read error occurs.  The data is read in portions of
   up to 16K and written to OUT as it arrives.  If opt.verbose is set,
   the progress is shown.

   TOREAD is the amount of data expected to arrive, normally only used
   by the progress gauge.

   STARTPOS is the position from which the download starts, used by
   the progress gauge.  If QTYREAD is non-NULL, the value it points to
   is incremented by the amount of data read from the network.  If
   QTYWRITTEN is non-NULL, the value it points to is incremented by
   the amount of data written to disk.  The time it took to download
   the data is stored to ELAPSED.

   The function exits and returns the amount of data read.  In case of
   error while reading data, -1 is returned.  In case of error while
   writing data, -2 is returned.  */

int
fd_read_body (int fd, FILE *out, wgint toread, wgint startpos,
              wgint *qtyread, wgint *qtywritten, double *elapsed, int flags)
{
  int ret = 0;

  static char dlbuf[16384];
  int dlbufsize = sizeof (dlbuf);

  struct ptimer *timer = NULL;
  double last_successful_read_tm = 0;

  /* The progress gauge, set according to the user preferences. */
  void *progress = NULL;

  /* Non-zero if the progress gauge is interactive, i.e. if it can
     continually update the display.  When true, smaller timeout
     values are used so that the gauge can update the display when
     data arrives slowly. */
  bool progress_interactive = false;

  bool exact = !!(flags & rb_read_exactly);
  wgint skip = 0;

  /* How much data we've read/written.  */
  wgint sum_read = 0;
  wgint sum_written = 0;

  if (flags & rb_skip_startpos)
    skip = startpos;

  if (opt.verbose)
    {
      /* If we're skipping STARTPOS bytes, pass 0 as the INITIAL
         argument to progress_create because the indicator doesn't
         (yet) know about "skipping" data.  */
      progress = progress_create (skip ? 0 : startpos, startpos + toread);
      progress_interactive = progress_interactive_p (progress);
    }

  if (opt.limit_rate)
    limit_bandwidth_reset ();

  /* A timer is needed for tracking progress, for throttling, and for
     tracking elapsed time.  If either of these are requested, start
     the timer.  */
  if (progress || opt.limit_rate || elapsed)
    {
      timer = ptimer_new ();
      last_successful_read_tm = 0;
    }

  /* Use a smaller buffer for low requested bandwidths.  For example,
     with --limit-rate=2k, it doesn't make sense to slurp in 16K of
     data and then sleep for 8s.  With buffer size equal to the limit,
     we never have to sleep for more than one second.  */
  if (opt.limit_rate && opt.limit_rate < dlbufsize)
    dlbufsize = opt.limit_rate;

  /* Read from FD while there is data to read.  Normally toread==0
     means that it is unknown how much data is to arrive.  However, if
     EXACT is set, then toread==0 means what it says: that no data
     should be read.  */
  while (!exact || (sum_read < toread))
    {
      int rdsize = exact ? MIN (toread - sum_read, dlbufsize) : dlbufsize;
      double tmout = opt.read_timeout;
      if (progress_interactive)
        {
          /* For interactive progress gauges, always specify a ~1s
             timeout, so that the gauge can be updated regularly even
             when the data arrives very slowly or stalls.  */
          tmout = 0.95;
          if (opt.read_timeout)
            {
              double waittm;
              waittm = ptimer_read (timer) - last_successful_read_tm;
              if (waittm + tmout > opt.read_timeout)
                {
                  /* Don't let total idle time exceed read timeout. */
                  tmout = opt.read_timeout - waittm;
                  if (tmout < 0)
                    {
                      /* We've already exceeded the timeout. */
                      ret = -1, errno = ETIMEDOUT;
                      break;
                    }
                }
            }
        }
      ret = fd_read (fd, dlbuf, rdsize, tmout);

      if (progress_interactive && ret < 0 && errno == ETIMEDOUT)
        ret = 0;                /* interactive timeout, handled above */
      else if (ret <= 0)
        break;                  /* EOF or read error */

      if (progress || opt.limit_rate)
        {
          ptimer_measure (timer);
          if (ret > 0)
            last_successful_read_tm = ptimer_read (timer);
        }

      if (ret > 0)
        {
          sum_read += ret;
          if (!write_data (out, dlbuf, ret, &skip, &sum_written))
            {
              ret = -2;
              goto out;
            }
        }

      if (opt.limit_rate)
        limit_bandwidth (ret, timer);

      if (progress)
        progress_update (progress, ret, ptimer_read (timer));
#ifdef WINDOWS
      if (toread > 0 && !opt.quiet)
        ws_percenttitle (100.0 *
                         (startpos + sum_read) / (startpos + toread));
#endif
    }
  if (ret < -1)
    ret = -1;

 out:
  if (progress)
    progress_finish (progress, ptimer_read (timer));

  if (elapsed)
    *elapsed = ptimer_read (timer);
  if (timer)
    ptimer_destroy (timer);

  if (qtyread)
    *qtyread += sum_read;
  if (qtywritten)
    *qtywritten += sum_written;

  return ret;
}

/* Read a hunk of data from FD, up until a terminator.  The hunk is
   limited by whatever the TERMINATOR callback chooses as its
   terminator.  For example, if terminator stops at newline, the hunk
   will consist of a line of data; if terminator stops at two
   newlines, it can be used to read the head of an HTTP response.
   Upon determining the boundary, the function returns the data (up to
   the terminator) in malloc-allocated storage.

   In case of read error, NULL is returned.  In case of EOF and no
   data read, NULL is returned and errno set to 0.  In case of having
   read some data, but encountering EOF before seeing the terminator,
   the data that has been read is returned, but it will (obviously)
   not contain the terminator.

   The TERMINATOR function is called with three arguments: the
   beginning of the data read so far, the beginning of the current
   block of peeked-at data, and the length of the current block.
   Depending on its needs, the function is free to choose whether to
   analyze all data or just the newly arrived data.  If TERMINATOR
   returns NULL, it means that the terminator has not been seen.
   Otherwise it should return a pointer to the charactre immediately
   following the terminator.

   The idea is to be able to read a line of input, or otherwise a hunk
   of text, such as the head of an HTTP request, without crossing the
   boundary, so that the next call to fd_read etc. reads the data
   after the hunk.  To achieve that, this function does the following:

   1. Peek at incoming data.

   2. Determine whether the peeked data, along with the previously
      read data, includes the terminator.

      2a. If yes, read the data until the end of the terminator, and
          exit.

      2b. If no, read the peeked data and goto 1.

   The function is careful to assume as little as possible about the
   implementation of peeking.  For example, every peek is followed by
   a read.  If the read returns a different amount of data, the
   process is retried until all data arrives safely.

   SIZEHINT is the buffer size sufficient to hold all the data in the
   typical case (it is used as the initial buffer size).  MAXSIZE is
   the maximum amount of memory this function is allowed to allocate,
   or 0 if no upper limit is to be enforced.

   This function should be used as a building block for other
   functions -- see fd_read_line as a simple example.  */

char *
fd_read_hunk (int fd, hunk_terminator_t terminator, long sizehint, long maxsize)
{
  long bufsize = sizehint;
  char *hunk = xmalloc (bufsize);
  int tail = 0;                 /* tail position in HUNK */

  assert (maxsize >= bufsize);

  while (1)
    {
      const char *end;
      int pklen, rdlen, remain;

      /* First, peek at the available data. */

      pklen = fd_peek (fd, hunk + tail, bufsize - 1 - tail, -1);
      if (pklen < 0)
        {
          xfree (hunk);
          return NULL;
        }
      end = terminator (hunk, hunk + tail, pklen);
      if (end)
        {
          /* The data contains the terminator: we'll drain the data up
             to the end of the terminator.  */
          remain = end - (hunk + tail);
          assert (remain >= 0);
          if (remain == 0)
            {
              /* No more data needs to be read. */
              hunk[tail] = '\0';
              return hunk;
            }
          if (bufsize - 1 < tail + remain)
            {
              bufsize = tail + remain + 1;
              hunk = xrealloc (hunk, bufsize);
            }
        }
      else
        /* No terminator: simply read the data we know is (or should
           be) available.  */
        remain = pklen;

      /* Now, read the data.  Note that we make no assumptions about
         how much data we'll get.  (Some TCP stacks are notorious for
         read returning less data than the previous MSG_PEEK.)  */

      rdlen = fd_read (fd, hunk + tail, remain, 0);
      if (rdlen < 0)
        {
          xfree_null (hunk);
          return NULL;
        }
      tail += rdlen;
      hunk[tail] = '\0';

      if (rdlen == 0)
        {
          if (tail == 0)
            {
              /* EOF without anything having been read */
              xfree (hunk);
              errno = 0;
              return NULL;
            }
          else
            /* EOF seen: return the data we've read. */
            return hunk;
        }
      if (end && rdlen == remain)
        /* The terminator was seen and the remaining data drained --
           we got what we came for.  */
        return hunk;

      /* Keep looping until all the data arrives. */

      if (tail == bufsize - 1)
        {
          /* Double the buffer size, but refuse to allocate more than
             MAXSIZE bytes.  */
          if (maxsize && bufsize >= maxsize)
            {
              xfree (hunk);
              errno = ENOMEM;
              return NULL;
            }
          bufsize <<= 1;
          if (maxsize && bufsize > maxsize)
            bufsize = maxsize;
          hunk = xrealloc (hunk, bufsize);
        }
    }
}

static const char *
line_terminator (const char *start, const char *peeked, int peeklen)
{
  const char *p = memchr (peeked, '\n', peeklen);
  if (p)
    /* p+1 because the line must include '\n' */
    return p + 1;
  return NULL;
}

/* The maximum size of the single line we agree to accept.  This is
   not meant to impose an arbitrary limit, but to protect the user
   from Wget slurping up available memory upon encountering malicious
   or buggy server output.  Define it to 0 to remove the limit.  */
#define FD_READ_LINE_MAX 4096

/* Read one line from FD and return it.  The line is allocated using
   malloc, but is never larger than FD_READ_LINE_MAX.

   If an error occurs, or if no data can be read, NULL is returned.
   In the former case errno indicates the error condition, and in the
   latter case, errno is NULL.  */

char *
fd_read_line (int fd)
{
  return fd_read_hunk (fd, line_terminator, 128, FD_READ_LINE_MAX);
}

/* Return a printed representation of the download rate, along with
   the units appropriate for the download speed.  */

const char *
retr_rate (wgint bytes, double secs)
{
  static char res[20];
  static const char *rate_names[] = {"B/s", "KB/s", "MB/s", "GB/s" };
  int units;

  double dlrate = calc_rate (bytes, secs, &units);
  /* Use more digits for smaller numbers (regardless of unit used),
     e.g. "1022", "247", "12.5", "2.38".  */
  sprintf (res, "%.*f %s",
           dlrate >= 99.95 ? 0 : dlrate >= 9.995 ? 1 : 2,
           dlrate, rate_names[units]);

  return res;
}

/* Calculate the download rate and trim it as appropriate for the
   speed.  Appropriate means that if rate is greater than 1K/s,
   kilobytes are used, and if rate is greater than 1MB/s, megabytes
   are used.

   UNITS is zero for B/s, one for KB/s, two for MB/s, and three for
   GB/s.  */

double
calc_rate (wgint bytes, double secs, int *units)
{
  double dlrate;

  assert (secs >= 0);
  assert (bytes >= 0);

  if (secs == 0)
    /* If elapsed time is exactly zero, it means we're under the
       resolution of the timer.  This can easily happen on systems
       that use time() for the timer.  Since the interval lies between
       0 and the timer's resolution, assume half the resolution.  */
    secs = ptimer_resolution () / 2.0;

  dlrate = bytes / secs;
  if (dlrate < 1024.0)
    *units = 0;
  else if (dlrate < 1024.0 * 1024.0)
    *units = 1, dlrate /= 1024.0;
  else if (dlrate < 1024.0 * 1024.0 * 1024.0)
    *units = 2, dlrate /= (1024.0 * 1024.0);
  else
    /* Maybe someone will need this, one day. */
    *units = 3, dlrate /= (1024.0 * 1024.0 * 1024.0);

  return dlrate;
}


#define SUSPEND_POST_DATA do {                  \
  post_data_suspended = true;                   \
  saved_post_data = opt.post_data;              \
  saved_post_file_name = opt.post_file_name;    \
  opt.post_data = NULL;                         \
  opt.post_file_name = NULL;                    \
} while (0)

#define RESTORE_POST_DATA do {                          \
  if (post_data_suspended)                              \
    {                                                   \
      opt.post_data = saved_post_data;                  \
      opt.post_file_name = saved_post_file_name;        \
      post_data_suspended = false;                      \
    }                                                   \
} while (0)

static char *getproxy (struct url *);

/* Retrieve the given URL.  Decides which loop to call -- HTTP, FTP,
   FTP, proxy, etc.  */

/* #### This function should be rewritten so it doesn't return from
   multiple points. */

uerr_t
retrieve_url (const char *origurl, char **file, char **newloc,
              const char *refurl, int *dt, bool recursive)
{
  uerr_t result;
  char *url;
  bool location_changed;
  int dummy;
  char *mynewloc, *proxy;
  struct url *u, *proxy_url;
  int up_error_code;            /* url parse error code */
  char *local_file;
  int redirection_count = 0;

  bool post_data_suspended = false;
  char *saved_post_data = NULL;
  char *saved_post_file_name = NULL;

  /* If dt is NULL, use local storage.  */
  if (!dt)
    {
      dt = &dummy;
      dummy = 0;
    }
  url = xstrdup (origurl);
  if (newloc)
    *newloc = NULL;
  if (file)
    *file = NULL;

  u = url_parse (url, &up_error_code);
  if (!u)
    {
      logprintf (LOG_NOTQUIET, "%s: %s.\n", url, url_error (up_error_code));
      xfree (url);
      return URLERROR;
    }

  if (!refurl)
    refurl = opt.referer;

 redirected:

  result = NOCONERROR;
  mynewloc = NULL;
  local_file = NULL;
  proxy_url = NULL;

  proxy = getproxy (u);
  if (proxy)
    {
      /* Parse the proxy URL.  */
      proxy_url = url_parse (proxy, &up_error_code);
      if (!proxy_url)
        {
          logprintf (LOG_NOTQUIET, _("Error parsing proxy URL %s: %s.\n"),
                     proxy, url_error (up_error_code));
          xfree (url);
          RESTORE_POST_DATA;
          return PROXERR;
        }
      if (proxy_url->scheme != SCHEME_HTTP && proxy_url->scheme != u->scheme)
        {
          logprintf (LOG_NOTQUIET, _("Error in proxy URL %s: Must be HTTP.\n"), proxy);
          url_free (proxy_url);
          xfree (url);
          RESTORE_POST_DATA;
          return PROXERR;
        }
    }

  if (u->scheme == SCHEME_HTTP
#ifdef HAVE_SSL
      || u->scheme == SCHEME_HTTPS
#endif
      || (proxy_url && proxy_url->scheme == SCHEME_HTTP))
    {
      result = http_loop (u, &mynewloc, &local_file, refurl, dt, proxy_url);
    }
  else if (u->scheme == SCHEME_FTP)
    {
      /* If this is a redirection, temporarily turn off opt.ftp_glob
         and opt.recursive, both being undesirable when following
         redirects.  */
      bool oldrec = recursive, glob = opt.ftp_glob;
      if (redirection_count)
        oldrec = glob = false;

      result = ftp_loop (u, dt, proxy_url, recursive, glob);
      recursive = oldrec;

      /* There is a possibility of having HTTP being redirected to
         FTP.  In these cases we must decide whether the text is HTML
         according to the suffix.  The HTML suffixes are `.html',
         `.htm' and a few others, case-insensitive.  */
      if (redirection_count && local_file && u->scheme == SCHEME_FTP)
        {
          if (has_html_suffix_p (local_file))
            *dt |= TEXTHTML;
        }
    }

  if (proxy_url)
    {
      url_free (proxy_url);
      proxy_url = NULL;
    }

  location_changed = (result == NEWLOCATION);
  if (location_changed)
    {
      char *construced_newloc;
      struct url *newloc_parsed;

      assert (mynewloc != NULL);

      if (local_file)
        xfree (local_file);

      /* The HTTP specs only allow absolute URLs to appear in
         redirects, but a ton of boneheaded webservers and CGIs out
         there break the rules and use relative URLs, and popular
         browsers are lenient about this, so wget should be too. */
      construced_newloc = uri_merge (url, mynewloc);
      xfree (mynewloc);
      mynewloc = construced_newloc;

      /* Now, see if this new location makes sense. */
      newloc_parsed = url_parse (mynewloc, &up_error_code);
      if (!newloc_parsed)
        {
          logprintf (LOG_NOTQUIET, "%s: %s.\n", escnonprint_uri (mynewloc),
                     url_error (up_error_code));
          url_free (u);
          xfree (url);
          xfree (mynewloc);
          RESTORE_POST_DATA;
          return result;
        }

      /* Now mynewloc will become newloc_parsed->url, because if the
         Location contained relative paths like .././something, we
         don't want that propagating as url.  */
      xfree (mynewloc);
      mynewloc = xstrdup (newloc_parsed->url);

      /* Check for max. number of redirections.  */
      if (++redirection_count > opt.max_redirect)
        {
          logprintf (LOG_NOTQUIET, _("%d redirections exceeded.\n"),
                     opt.max_redirect);
          url_free (newloc_parsed);
          url_free (u);
          xfree (url);
          xfree (mynewloc);
          RESTORE_POST_DATA;
          return WRONGCODE;
        }

      xfree (url);
      url = mynewloc;
      url_free (u);
      u = newloc_parsed;

      /* If we're being redirected from POST, we don't want to POST
         again.  Many requests answer POST with a redirection to an
         index page; that redirection is clearly a GET.  We "suspend"
         POST data for the duration of the redirections, and restore
         it when we're done. */
      if (!post_data_suspended)
        SUSPEND_POST_DATA;

      goto redirected;
    }

  if (local_file)
    {
      if (*dt & RETROKF)
        {
          register_download (u->url, local_file);
          if (redirection_count && 0 != strcmp (origurl, u->url))
            register_redirection (origurl, u->url);
          if (*dt & TEXTHTML)
            register_html (u->url, local_file);
        }
    }

  if (file)
    *file = local_file ? local_file : NULL;
  else
    xfree_null (local_file);

  url_free (u);

  if (redirection_count)
    {
      if (newloc)
        *newloc = url;
      else
        xfree (url);
    }
  else
    {
      if (newloc)
        *newloc = NULL;
      xfree (url);
    }

  RESTORE_POST_DATA;

  return result;
}

/* Find the URLs in the file and call retrieve_url() for each of them.
   If HTML is true, treat the file as HTML, and construct the URLs
   accordingly.

   If opt.recursive is set, call retrieve_tree() for each file.  */

uerr_t
retrieve_from_file (const char *file, bool html, int *count)
{
  uerr_t status;
  struct urlpos *url_list, *cur_url;

  url_list = (html ? get_urls_html (file, NULL, NULL)
              : get_urls_file (file));
  status = RETROK;             /* Suppose everything is OK.  */
  *count = 0;                  /* Reset the URL count.  */

  for (cur_url = url_list; cur_url; cur_url = cur_url->next, ++*count)
    {
      char *filename = NULL, *new_file = NULL;
      int dt;

      if (cur_url->ignore_when_downloading)
        continue;

      if (opt.quota && total_downloaded_bytes > opt.quota)
        {
          status = QUOTEXC;
          break;
        }
      if ((opt.recursive || opt.page_requisites)
          && (cur_url->url->scheme != SCHEME_FTP || getproxy (cur_url->url)))
        {
          int old_follow_ftp = opt.follow_ftp;

          /* Turn opt.follow_ftp on in case of recursive FTP retrieval */
          if (cur_url->url->scheme == SCHEME_FTP) 
            opt.follow_ftp = 1;
          
          status = retrieve_tree (cur_url->url->url);

          opt.follow_ftp = old_follow_ftp;
        }
      else
        status = retrieve_url (cur_url->url->url, &filename, &new_file, NULL, &dt, opt.recursive);

      if (filename && opt.delete_after && file_exists_p (filename))
        {
          DEBUGP (("\
Removing file due to --delete-after in retrieve_from_file():\n"));
          logprintf (LOG_VERBOSE, _("Removing %s.\n"), filename);
          if (unlink (filename))
            logprintf (LOG_NOTQUIET, "unlink: %s\n", strerror (errno));
          dt &= ~RETROKF;
        }

      xfree_null (new_file);
      xfree_null (filename);
    }

  /* Free the linked list of URL-s.  */
  free_urlpos (url_list);

  return status;
}

/* Print `giving up', or `retrying', depending on the impending
   action.  N1 and N2 are the attempt number and the attempt limit.  */
void
printwhat (int n1, int n2)
{
  logputs (LOG_VERBOSE, (n1 == n2) ? _("Giving up.\n\n") : _("Retrying.\n\n"));
}

/* If opt.wait or opt.waitretry are specified, and if certain
   conditions are met, sleep the appropriate number of seconds.  See
   the documentation of --wait and --waitretry for more information.

   COUNT is the count of current retrieval, beginning with 1. */

void
sleep_between_retrievals (int count)
{
  static bool first_retrieval = true;

  if (first_retrieval)
    {
      /* Don't sleep before the very first retrieval. */
      first_retrieval = false;
      return;
    }

  if (opt.waitretry && count > 1)
    {
      /* If opt.waitretry is specified and this is a retry, wait for
         COUNT-1 number of seconds, or for opt.waitretry seconds.  */
      if (count <= opt.waitretry)
        xsleep (count - 1);
      else
        xsleep (opt.waitretry);
    }
  else if (opt.wait)
    {
      if (!opt.random_wait || count > 1)
        /* If random-wait is not specified, or if we are sleeping
           between retries of the same download, sleep the fixed
           interval.  */
        xsleep (opt.wait);
      else
        {
          /* Sleep a random amount of time averaging in opt.wait
             seconds.  The sleeping amount ranges from 0.5*opt.wait to
             1.5*opt.wait.  */
          double waitsecs = (0.5 + random_float ()) * opt.wait;
          DEBUGP (("sleep_between_retrievals: avg=%f,sleep=%f\n",
                   opt.wait, waitsecs));
          xsleep (waitsecs);
        }
    }
}

/* Free the linked list of urlpos.  */
void
free_urlpos (struct urlpos *l)
{
  while (l)
    {
      struct urlpos *next = l->next;
      if (l->url)
        url_free (l->url);
      xfree_null (l->local_name);
      xfree (l);
      l = next;
    }
}

/* Rotate FNAME opt.backups times */
void
rotate_backups(const char *fname)
{
  int maxlen = strlen (fname) + 1 + numdigit (opt.backups) + 1;
  char *from = (char *)alloca (maxlen);
  char *to = (char *)alloca (maxlen);
  struct_stat sb;
  int i;

  if (stat (fname, &sb) == 0)
    if (S_ISREG (sb.st_mode) == 0)
      return;

  for (i = opt.backups; i > 1; i--)
    {
      sprintf (from, "%s.%d", fname, i - 1);
      sprintf (to, "%s.%d", fname, i);
      rename (from, to);
    }

  sprintf (to, "%s.%d", fname, 1);
  rename(fname, to);
}

static bool no_proxy_match (const char *, const char **);

/* Return the URL of the proxy appropriate for url U.  */

static char *
getproxy (struct url *u)
{
  char *proxy = NULL;
  char *rewritten_url;
  static char rewritten_storage[1024];

  if (!opt.use_proxy)
    return NULL;
  if (no_proxy_match (u->host, (const char **)opt.no_proxy))
    return NULL;

  switch (u->scheme)
    {
    case SCHEME_HTTP:
      proxy = opt.http_proxy ? opt.http_proxy : getenv ("http_proxy");
      break;
#ifdef HAVE_SSL
    case SCHEME_HTTPS:
      proxy = opt.https_proxy ? opt.https_proxy : getenv ("https_proxy");
      break;
#endif
    case SCHEME_FTP:
      proxy = opt.ftp_proxy ? opt.ftp_proxy : getenv ("ftp_proxy");
      break;
    case SCHEME_INVALID:
      break;
    }
  if (!proxy || !*proxy)
    return NULL;

  /* Handle shorthands.  `rewritten_storage' is a kludge to allow
     getproxy() to return static storage. */
  rewritten_url = rewrite_shorthand_url (proxy);
  if (rewritten_url)
    {
      strncpy (rewritten_storage, rewritten_url, sizeof (rewritten_storage));
      rewritten_storage[sizeof (rewritten_storage) - 1] = '\0';
      proxy = rewritten_storage;
    }

  return proxy;
}

/* Returns true if URL would be downloaded through a proxy. */

bool
url_uses_proxy (const char *url)
{
  bool ret;
  struct url *u = url_parse (url, NULL);
  if (!u)
    return false;
  ret = getproxy (u) != NULL;
  url_free (u);
  return ret;
}

/* Should a host be accessed through proxy, concerning no_proxy?  */
static bool
no_proxy_match (const char *host, const char **no_proxy)
{
  if (!no_proxy)
    return false;
  else
    return sufmatch (no_proxy, host);
}
