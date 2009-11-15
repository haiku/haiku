/* HTTP support.
   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,
   2004, 2005, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.

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

#include "wget.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <locale.h>

#include "hash.h"
#include "http.h"
#include "utils.h"
#include "url.h"
#include "host.h"
#include "retr.h"
#include "connect.h"
#include "netrc.h"
#ifdef HAVE_SSL
# include "ssl.h"
#endif
#ifdef ENABLE_NTLM
# include "http-ntlm.h"
#endif
#include "cookies.h"
#ifdef ENABLE_DIGEST
# include "gen-md5.h"
#endif
#include "convert.h"
#include "spider.h"

#ifdef TESTING
#include "test.h"
#endif

#ifdef __VMS
# include "vms.h"
#endif /* def __VMS */

extern char *version_string;

/* Forward decls. */
struct http_stat;
static char *create_authorization_line (const char *, const char *,
                                        const char *, const char *,
                                        const char *, bool *);
static char *basic_authentication_encode (const char *, const char *);
static bool known_authentication_scheme_p (const char *, const char *);
static void ensure_extension (struct http_stat *, const char *, int *);
static void load_cookies (void);

#ifndef MIN
# define MIN(x, y) ((x) > (y) ? (y) : (x))
#endif


static bool cookies_loaded_p;
static struct cookie_jar *wget_cookie_jar;

#define TEXTHTML_S "text/html"
#define TEXTXHTML_S "application/xhtml+xml"
#define TEXTCSS_S "text/css"

/* Some status code validation macros: */
#define H_20X(x)        (((x) >= 200) && ((x) < 300))
#define H_PARTIAL(x)    ((x) == HTTP_STATUS_PARTIAL_CONTENTS)
#define H_REDIRECTED(x) ((x) == HTTP_STATUS_MOVED_PERMANENTLY          \
                         || (x) == HTTP_STATUS_MOVED_TEMPORARILY       \
                         || (x) == HTTP_STATUS_SEE_OTHER               \
                         || (x) == HTTP_STATUS_TEMPORARY_REDIRECT)

/* HTTP/1.0 status codes from RFC1945, provided for reference.  */
/* Successful 2xx.  */
#define HTTP_STATUS_OK                    200
#define HTTP_STATUS_CREATED               201
#define HTTP_STATUS_ACCEPTED              202
#define HTTP_STATUS_NO_CONTENT            204
#define HTTP_STATUS_PARTIAL_CONTENTS      206

/* Redirection 3xx.  */
#define HTTP_STATUS_MULTIPLE_CHOICES      300
#define HTTP_STATUS_MOVED_PERMANENTLY     301
#define HTTP_STATUS_MOVED_TEMPORARILY     302
#define HTTP_STATUS_SEE_OTHER             303 /* from HTTP/1.1 */
#define HTTP_STATUS_NOT_MODIFIED          304
#define HTTP_STATUS_TEMPORARY_REDIRECT    307 /* from HTTP/1.1 */

/* Client error 4xx.  */
#define HTTP_STATUS_BAD_REQUEST           400
#define HTTP_STATUS_UNAUTHORIZED          401
#define HTTP_STATUS_FORBIDDEN             403
#define HTTP_STATUS_NOT_FOUND             404
#define HTTP_STATUS_RANGE_NOT_SATISFIABLE 416

/* Server errors 5xx.  */
#define HTTP_STATUS_INTERNAL              500
#define HTTP_STATUS_NOT_IMPLEMENTED       501
#define HTTP_STATUS_BAD_GATEWAY           502
#define HTTP_STATUS_UNAVAILABLE           503

enum rp {
  rel_none, rel_name, rel_value, rel_both
};

struct request {
  const char *method;
  char *arg;

  struct request_header {
    char *name, *value;
    enum rp release_policy;
  } *headers;
  int hcount, hcapacity;
};

extern int numurls;

/* Create a new, empty request.  At least request_set_method must be
   called before the request can be used.  */

static struct request *
request_new (void)
{
  struct request *req = xnew0 (struct request);
  req->hcapacity = 8;
  req->headers = xnew_array (struct request_header, req->hcapacity);
  return req;
}

/* Set the request's method and its arguments.  METH should be a
   literal string (or it should outlive the request) because it will
   not be freed.  ARG will be freed by request_free.  */

static void
request_set_method (struct request *req, const char *meth, char *arg)
{
  req->method = meth;
  req->arg = arg;
}

/* Return the method string passed with the last call to
   request_set_method.  */

static const char *
request_method (const struct request *req)
{
  return req->method;
}

/* Free one header according to the release policy specified with
   request_set_header.  */

static void
release_header (struct request_header *hdr)
{
  switch (hdr->release_policy)
    {
    case rel_none:
      break;
    case rel_name:
      xfree (hdr->name);
      break;
    case rel_value:
      xfree (hdr->value);
      break;
    case rel_both:
      xfree (hdr->name);
      xfree (hdr->value);
      break;
    }
}

/* Set the request named NAME to VALUE.  Specifically, this means that
   a "NAME: VALUE\r\n" header line will be used in the request.  If a
   header with the same name previously existed in the request, its
   value will be replaced by this one.  A NULL value means do nothing.

   RELEASE_POLICY determines whether NAME and VALUE should be released
   (freed) with request_free.  Allowed values are:

    - rel_none     - don't free NAME or VALUE
    - rel_name     - free NAME when done
    - rel_value    - free VALUE when done
    - rel_both     - free both NAME and VALUE when done

   Setting release policy is useful when arguments come from different
   sources.  For example:

     // Don't free literal strings!
     request_set_header (req, "Pragma", "no-cache", rel_none);

     // Don't free a global variable, we'll need it later.
     request_set_header (req, "Referer", opt.referer, rel_none);

     // Value freshly allocated, free it when done.
     request_set_header (req, "Range",
                         aprintf ("bytes=%s-", number_to_static_string (hs->restval)),
                         rel_value);
   */

static void
request_set_header (struct request *req, char *name, char *value,
                    enum rp release_policy)
{
  struct request_header *hdr;
  int i;

  if (!value)
    {
      /* A NULL value is a no-op; if freeing the name is requested,
         free it now to avoid leaks.  */
      if (release_policy == rel_name || release_policy == rel_both)
        xfree (name);
      return;
    }

  for (i = 0; i < req->hcount; i++)
    {
      hdr = &req->headers[i];
      if (0 == strcasecmp (name, hdr->name))
        {
          /* Replace existing header. */
          release_header (hdr);
          hdr->name = name;
          hdr->value = value;
          hdr->release_policy = release_policy;
          return;
        }
    }

  /* Install new header. */

  if (req->hcount >= req->hcapacity)
    {
      req->hcapacity <<= 1;
      req->headers = xrealloc (req->headers, req->hcapacity * sizeof (*hdr));
    }
  hdr = &req->headers[req->hcount++];
  hdr->name = name;
  hdr->value = value;
  hdr->release_policy = release_policy;
}

/* Like request_set_header, but sets the whole header line, as
   provided by the user using the `--header' option.  For example,
   request_set_user_header (req, "Foo: bar") works just like
   request_set_header (req, "Foo", "bar").  */

static void
request_set_user_header (struct request *req, const char *header)
{
  char *name;
  const char *p = strchr (header, ':');
  if (!p)
    return;
  BOUNDED_TO_ALLOCA (header, p, name);
  ++p;
  while (c_isspace (*p))
    ++p;
  request_set_header (req, xstrdup (name), (char *) p, rel_name);
}

/* Remove the header with specified name from REQ.  Returns true if
   the header was actually removed, false otherwise.  */

static bool
request_remove_header (struct request *req, char *name)
{
  int i;
  for (i = 0; i < req->hcount; i++)
    {
      struct request_header *hdr = &req->headers[i];
      if (0 == strcasecmp (name, hdr->name))
        {
          release_header (hdr);
          /* Move the remaining headers by one. */
          if (i < req->hcount - 1)
            memmove (hdr, hdr + 1, (req->hcount - i - 1) * sizeof (*hdr));
          --req->hcount;
          return true;
        }
    }
  return false;
}

#define APPEND(p, str) do {                     \
  int A_len = strlen (str);                     \
  memcpy (p, str, A_len);                       \
  p += A_len;                                   \
} while (0)

/* Construct the request and write it to FD using fd_write.  */

static int
request_send (const struct request *req, int fd)
{
  char *request_string, *p;
  int i, size, write_error;

  /* Count the request size. */
  size = 0;

  /* METHOD " " ARG " " "HTTP/1.0" "\r\n" */
  size += strlen (req->method) + 1 + strlen (req->arg) + 1 + 8 + 2;

  for (i = 0; i < req->hcount; i++)
    {
      struct request_header *hdr = &req->headers[i];
      /* NAME ": " VALUE "\r\n" */
      size += strlen (hdr->name) + 2 + strlen (hdr->value) + 2;
    }

  /* "\r\n\0" */
  size += 3;

  p = request_string = alloca_array (char, size);

  /* Generate the request. */

  APPEND (p, req->method); *p++ = ' ';
  APPEND (p, req->arg);    *p++ = ' ';
  memcpy (p, "HTTP/1.0\r\n", 10); p += 10;

  for (i = 0; i < req->hcount; i++)
    {
      struct request_header *hdr = &req->headers[i];
      APPEND (p, hdr->name);
      *p++ = ':', *p++ = ' ';
      APPEND (p, hdr->value);
      *p++ = '\r', *p++ = '\n';
    }

  *p++ = '\r', *p++ = '\n', *p++ = '\0';
  assert (p - request_string == size);

#undef APPEND

  DEBUGP (("\n---request begin---\n%s---request end---\n", request_string));

  /* Send the request to the server. */

  write_error = fd_write (fd, request_string, size - 1, -1);
  if (write_error < 0)
    logprintf (LOG_VERBOSE, _("Failed writing HTTP request: %s.\n"),
               fd_errstr (fd));
  return write_error;
}

/* Release the resources used by REQ. */

static void
request_free (struct request *req)
{
  int i;
  xfree_null (req->arg);
  for (i = 0; i < req->hcount; i++)
    release_header (&req->headers[i]);
  xfree_null (req->headers);
  xfree (req);
}

static struct hash_table *basic_authed_hosts;

/* Find out if this host has issued a Basic challenge yet; if so, give
 * it the username, password. A temporary measure until we can get
 * proper authentication in place. */

static bool
maybe_send_basic_creds (const char *hostname, const char *user,
                        const char *passwd, struct request *req)
{
  bool do_challenge = false;

  if (opt.auth_without_challenge)
    {
      DEBUGP(("Auth-without-challenge set, sending Basic credentials.\n"));
      do_challenge = true;
    }
  else if (basic_authed_hosts
      && hash_table_contains(basic_authed_hosts, hostname))
    {
      DEBUGP(("Found %s in basic_authed_hosts.\n", quote (hostname)));
      do_challenge = true;
    }
  else
    {
      DEBUGP(("Host %s has not issued a general basic challenge.\n",
              quote (hostname)));
    }
  if (do_challenge)
    {
      request_set_header (req, "Authorization",
                          basic_authentication_encode (user, passwd),
                          rel_value);
    }
  return do_challenge;
}

static void
register_basic_auth_host (const char *hostname)
{
  if (!basic_authed_hosts)
    {
      basic_authed_hosts = make_nocase_string_hash_table (1);
    }
  if (!hash_table_contains(basic_authed_hosts, hostname))
    {
      hash_table_put (basic_authed_hosts, xstrdup(hostname), NULL);
      DEBUGP(("Inserted %s into basic_authed_hosts\n", quote (hostname)));
    }
}


/* Send the contents of FILE_NAME to SOCK.  Make sure that exactly
   PROMISED_SIZE bytes are sent over the wire -- if the file is
   longer, read only that much; if the file is shorter, report an error.  */

static int
post_file (int sock, const char *file_name, wgint promised_size)
{
  static char chunk[8192];
  wgint written = 0;
  int write_error;
  FILE *fp;

  DEBUGP (("[writing POST file %s ... ", file_name));

  fp = fopen (file_name, "rb");
  if (!fp)
    return -1;
  while (!feof (fp) && written < promised_size)
    {
      int towrite;
      int length = fread (chunk, 1, sizeof (chunk), fp);
      if (length == 0)
        break;
      towrite = MIN (promised_size - written, length);
      write_error = fd_write (sock, chunk, towrite, -1);
      if (write_error < 0)
        {
          fclose (fp);
          return -1;
        }
      written += towrite;
    }
  fclose (fp);

  /* If we've written less than was promised, report a (probably
     nonsensical) error rather than break the promise.  */
  if (written < promised_size)
    {
      errno = EINVAL;
      return -1;
    }

  assert (written == promised_size);
  DEBUGP (("done]\n"));
  return 0;
}

/* Determine whether [START, PEEKED + PEEKLEN) contains an empty line.
   If so, return the pointer to the position after the line, otherwise
   return NULL.  This is used as callback to fd_read_hunk.  The data
   between START and PEEKED has been read and cannot be "unread"; the
   data after PEEKED has only been peeked.  */

static const char *
response_head_terminator (const char *start, const char *peeked, int peeklen)
{
  const char *p, *end;

  /* If at first peek, verify whether HUNK starts with "HTTP".  If
     not, this is a HTTP/0.9 request and we must bail out without
     reading anything.  */
  if (start == peeked && 0 != memcmp (start, "HTTP", MIN (peeklen, 4)))
    return start;

  /* Look for "\n[\r]\n", and return the following position if found.
     Start two chars before the current to cover the possibility that
     part of the terminator (e.g. "\n\r") arrived in the previous
     batch.  */
  p = peeked - start < 2 ? start : peeked - 2;
  end = peeked + peeklen;

  /* Check for \n\r\n or \n\n anywhere in [p, end-2). */
  for (; p < end - 2; p++)
    if (*p == '\n')
      {
        if (p[1] == '\r' && p[2] == '\n')
          return p + 3;
        else if (p[1] == '\n')
          return p + 2;
      }
  /* p==end-2: check for \n\n directly preceding END. */
  if (p[0] == '\n' && p[1] == '\n')
    return p + 2;

  return NULL;
}

/* The maximum size of a single HTTP response we care to read.  Rather
   than being a limit of the reader implementation, this limit
   prevents Wget from slurping all available memory upon encountering
   malicious or buggy server output, thus protecting the user.  Define
   it to 0 to remove the limit.  */

#define HTTP_RESPONSE_MAX_SIZE 65536

/* Read the HTTP request head from FD and return it.  The error
   conditions are the same as with fd_read_hunk.

   To support HTTP/0.9 responses, this function tries to make sure
   that the data begins with "HTTP".  If this is not the case, no data
   is read and an empty request is returned, so that the remaining
   data can be treated as body.  */

static char *
read_http_response_head (int fd)
{
  return fd_read_hunk (fd, response_head_terminator, 512,
                       HTTP_RESPONSE_MAX_SIZE);
}

struct response {
  /* The response data. */
  const char *data;

  /* The array of pointers that indicate where each header starts.
     For example, given this HTTP response:

       HTTP/1.0 200 Ok
       Description: some
        text
       Etag: x

     The headers are located like this:

     "HTTP/1.0 200 Ok\r\nDescription: some\r\n text\r\nEtag: x\r\n\r\n"
     ^                   ^                             ^          ^
     headers[0]          headers[1]                    headers[2] headers[3]

     I.e. headers[0] points to the beginning of the request,
     headers[1] points to the end of the first header and the
     beginning of the second one, etc.  */

  const char **headers;
};

/* Create a new response object from the text of the HTTP response,
   available in HEAD.  That text is automatically split into
   constituent header lines for fast retrieval using
   resp_header_*.  */

static struct response *
resp_new (const char *head)
{
  const char *hdr;
  int count, size;

  struct response *resp = xnew0 (struct response);
  resp->data = head;

  if (*head == '\0')
    {
      /* Empty head means that we're dealing with a headerless
         (HTTP/0.9) response.  In that case, don't set HEADERS at
         all.  */
      return resp;
    }

  /* Split HEAD into header lines, so that resp_header_* functions
     don't need to do this over and over again.  */

  size = count = 0;
  hdr = head;
  while (1)
    {
      DO_REALLOC (resp->headers, size, count + 1, const char *);
      resp->headers[count++] = hdr;

      /* Break upon encountering an empty line. */
      if (!hdr[0] || (hdr[0] == '\r' && hdr[1] == '\n') || hdr[0] == '\n')
        break;

      /* Find the end of HDR, including continuations. */
      do
        {
          const char *end = strchr (hdr, '\n');
          if (end)
            hdr = end + 1;
          else
            hdr += strlen (hdr);
        }
      while (*hdr == ' ' || *hdr == '\t');
    }
  DO_REALLOC (resp->headers, size, count + 1, const char *);
  resp->headers[count] = NULL;

  return resp;
}

/* Locate the header named NAME in the request data, starting with
   position START.  This allows the code to loop through the request
   data, filtering for all requests of a given name.  Returns the
   found position, or -1 for failure.  The code that uses this
   function typically looks like this:

     for (pos = 0; (pos = resp_header_locate (...)) != -1; pos++)
       ... do something with header ...

   If you only care about one header, use resp_header_get instead of
   this function.  */

static int
resp_header_locate (const struct response *resp, const char *name, int start,
                    const char **begptr, const char **endptr)
{
  int i;
  const char **headers = resp->headers;
  int name_len;

  if (!headers || !headers[1])
    return -1;

  name_len = strlen (name);
  if (start > 0)
    i = start;
  else
    i = 1;

  for (; headers[i + 1]; i++)
    {
      const char *b = headers[i];
      const char *e = headers[i + 1];
      if (e - b > name_len
          && b[name_len] == ':'
          && 0 == strncasecmp (b, name, name_len))
        {
          b += name_len + 1;
          while (b < e && c_isspace (*b))
            ++b;
          while (b < e && c_isspace (e[-1]))
            --e;
          *begptr = b;
          *endptr = e;
          return i;
        }
    }
  return -1;
}

/* Find and retrieve the header named NAME in the request data.  If
   found, set *BEGPTR to its starting, and *ENDPTR to its ending
   position, and return true.  Otherwise return false.

   This function is used as a building block for resp_header_copy
   and resp_header_strdup.  */

static bool
resp_header_get (const struct response *resp, const char *name,
                 const char **begptr, const char **endptr)
{
  int pos = resp_header_locate (resp, name, 0, begptr, endptr);
  return pos != -1;
}

/* Copy the response header named NAME to buffer BUF, no longer than
   BUFSIZE (BUFSIZE includes the terminating 0).  If the header
   exists, true is returned, false otherwise.  If there should be no
   limit on the size of the header, use resp_header_strdup instead.

   If BUFSIZE is 0, no data is copied, but the boolean indication of
   whether the header is present is still returned.  */

static bool
resp_header_copy (const struct response *resp, const char *name,
                  char *buf, int bufsize)
{
  const char *b, *e;
  if (!resp_header_get (resp, name, &b, &e))
    return false;
  if (bufsize)
    {
      int len = MIN (e - b, bufsize - 1);
      memcpy (buf, b, len);
      buf[len] = '\0';
    }
  return true;
}

/* Return the value of header named NAME in RESP, allocated with
   malloc.  If such a header does not exist in RESP, return NULL.  */

static char *
resp_header_strdup (const struct response *resp, const char *name)
{
  const char *b, *e;
  if (!resp_header_get (resp, name, &b, &e))
    return NULL;
  return strdupdelim (b, e);
}

/* Parse the HTTP status line, which is of format:

   HTTP-Version SP Status-Code SP Reason-Phrase

   The function returns the status-code, or -1 if the status line
   appears malformed.  The pointer to "reason-phrase" message is
   returned in *MESSAGE.  */

static int
resp_status (const struct response *resp, char **message)
{
  int status;
  const char *p, *end;

  if (!resp->headers)
    {
      /* For a HTTP/0.9 response, assume status 200. */
      if (message)
        *message = xstrdup (_("No headers, assuming HTTP/0.9"));
      return 200;
    }

  p = resp->headers[0];
  end = resp->headers[1];

  if (!end)
    return -1;

  /* "HTTP" */
  if (end - p < 4 || 0 != strncmp (p, "HTTP", 4))
    return -1;
  p += 4;

  /* Match the HTTP version.  This is optional because Gnutella
     servers have been reported to not specify HTTP version.  */
  if (p < end && *p == '/')
    {
      ++p;
      while (p < end && c_isdigit (*p))
        ++p;
      if (p < end && *p == '.')
        ++p;
      while (p < end && c_isdigit (*p))
        ++p;
    }

  while (p < end && c_isspace (*p))
    ++p;
  if (end - p < 3 || !c_isdigit (p[0]) || !c_isdigit (p[1]) || !c_isdigit (p[2]))
    return -1;

  status = 100 * (p[0] - '0') + 10 * (p[1] - '0') + (p[2] - '0');
  p += 3;

  if (message)
    {
      while (p < end && c_isspace (*p))
        ++p;
      while (p < end && c_isspace (end[-1]))
        --end;
      *message = strdupdelim (p, end);
    }

  return status;
}

/* Release the resources used by RESP.  */

static void
resp_free (struct response *resp)
{
  xfree_null (resp->headers);
  xfree (resp);
}

/* Print a single line of response, the characters [b, e).  We tried
   getting away with
      logprintf (LOG_VERBOSE, "%s%.*s\n", prefix, (int) (e - b), b);
   but that failed to escape the non-printable characters and, in fact,
   caused crashes in UTF-8 locales.  */

static void
print_response_line(const char *prefix, const char *b, const char *e)
{
  char *copy;
  BOUNDED_TO_ALLOCA(b, e, copy);
  logprintf (LOG_ALWAYS, "%s%s\n", prefix,
             quotearg_style (escape_quoting_style, copy));
}

/* Print the server response, line by line, omitting the trailing CRLF
   from individual header lines, and prefixed with PREFIX.  */

static void
print_server_response (const struct response *resp, const char *prefix)
{
  int i;
  if (!resp->headers)
    return;
  for (i = 0; resp->headers[i + 1]; i++)
    {
      const char *b = resp->headers[i];
      const char *e = resp->headers[i + 1];
      /* Skip CRLF */
      if (b < e && e[-1] == '\n')
        --e;
      if (b < e && e[-1] == '\r')
        --e;
      print_response_line(prefix, b, e);
    }
}

/* Parse the `Content-Range' header and extract the information it
   contains.  Returns true if successful, false otherwise.  */
static bool
parse_content_range (const char *hdr, wgint *first_byte_ptr,
                     wgint *last_byte_ptr, wgint *entity_length_ptr)
{
  wgint num;

  /* Ancient versions of Netscape proxy server, presumably predating
     rfc2068, sent out `Content-Range' without the "bytes"
     specifier.  */
  if (0 == strncasecmp (hdr, "bytes", 5))
    {
      hdr += 5;
      /* "JavaWebServer/1.1.1" sends "bytes: x-y/z", contrary to the
         HTTP spec. */
      if (*hdr == ':')
        ++hdr;
      while (c_isspace (*hdr))
        ++hdr;
      if (!*hdr)
        return false;
    }
  if (!c_isdigit (*hdr))
    return false;
  for (num = 0; c_isdigit (*hdr); hdr++)
    num = 10 * num + (*hdr - '0');
  if (*hdr != '-' || !c_isdigit (*(hdr + 1)))
    return false;
  *first_byte_ptr = num;
  ++hdr;
  for (num = 0; c_isdigit (*hdr); hdr++)
    num = 10 * num + (*hdr - '0');
  if (*hdr != '/' || !c_isdigit (*(hdr + 1)))
    return false;
  *last_byte_ptr = num;
  ++hdr;
  if (*hdr == '*')
    num = -1;
  else
    for (num = 0; c_isdigit (*hdr); hdr++)
      num = 10 * num + (*hdr - '0');
  *entity_length_ptr = num;
  return true;
}

/* Read the body of the request, but don't store it anywhere and don't
   display a progress gauge.  This is useful for reading the bodies of
   administrative responses to which we will soon issue another
   request.  The response is not useful to the user, but reading it
   allows us to continue using the same connection to the server.

   If reading fails, false is returned, true otherwise.  In debug
   mode, the body is displayed for debugging purposes.  */

static bool
skip_short_body (int fd, wgint contlen)
{
  enum {
    SKIP_SIZE = 512,                /* size of the download buffer */
    SKIP_THRESHOLD = 4096        /* the largest size we read */
  };
  char dlbuf[SKIP_SIZE + 1];
  dlbuf[SKIP_SIZE] = '\0';        /* so DEBUGP can safely print it */

  /* We shouldn't get here with unknown contlen.  (This will change
     with HTTP/1.1, which supports "chunked" transfer.)  */
  assert (contlen != -1);

  /* If the body is too large, it makes more sense to simply close the
     connection than to try to read the body.  */
  if (contlen > SKIP_THRESHOLD)
    return false;

  DEBUGP (("Skipping %s bytes of body: [", number_to_static_string (contlen)));

  while (contlen > 0)
    {
      int ret = fd_read (fd, dlbuf, MIN (contlen, SKIP_SIZE), -1);
      if (ret <= 0)
        {
          /* Don't normally report the error since this is an
             optimization that should be invisible to the user.  */
          DEBUGP (("] aborting (%s).\n",
                   ret < 0 ? fd_errstr (fd) : "EOF received"));
          return false;
        }
      contlen -= ret;
      /* Safe even if %.*s bogusly expects terminating \0 because
         we've zero-terminated dlbuf above.  */
      DEBUGP (("%.*s", ret, dlbuf));
    }

  DEBUGP (("] done.\n"));
  return true;
}

/* Extract a parameter from the string (typically an HTTP header) at
   **SOURCE and advance SOURCE to the next parameter.  Return false
   when there are no more parameters to extract.  The name of the
   parameter is returned in NAME, and the value in VALUE.  If the
   parameter has no value, the token's value is zeroed out.

   For example, if *SOURCE points to the string "attachment;
   filename=\"foo bar\"", the first call to this function will return
   the token named "attachment" and no value, and the second call will
   return the token named "filename" and value "foo bar".  The third
   call will return false, indicating no more valid tokens.  */

bool
extract_param (const char **source, param_token *name, param_token *value,
               char separator)
{
  const char *p = *source;

  while (c_isspace (*p)) ++p;
  if (!*p)
    {
      *source = p;
      return false;             /* no error; nothing more to extract */
    }

  /* Extract name. */
  name->b = p;
  while (*p && !c_isspace (*p) && *p != '=' && *p != separator) ++p;
  name->e = p;
  if (name->b == name->e)
    return false;               /* empty name: error */
  while (c_isspace (*p)) ++p;
  if (*p == separator || !*p)           /* no value */
    {
      xzero (*value);
      if (*p == separator) ++p;
      *source = p;
      return true;
    }
  if (*p != '=')
    return false;               /* error */

  /* *p is '=', extract value */
  ++p;
  while (c_isspace (*p)) ++p;
  if (*p == '"')                /* quoted */
    {
      value->b = ++p;
      while (*p && *p != '"') ++p;
      if (!*p)
        return false;
      value->e = p++;
      /* Currently at closing quote; find the end of param. */
      while (c_isspace (*p)) ++p;
      while (*p && *p != separator) ++p;
      if (*p == separator)
        ++p;
      else if (*p)
        /* garbage after closed quote, e.g. foo="bar"baz */
        return false;
    }
  else                          /* unquoted */
    {
      value->b = p;
      while (*p && *p != separator) ++p;
      value->e = p;
      while (value->e != value->b && c_isspace (value->e[-1]))
        --value->e;
      if (*p == separator) ++p;
    }
  *source = p;
  return true;
}

#undef MAX
#define MAX(p, q) ((p) > (q) ? (p) : (q))

/* Parse the contents of the `Content-Disposition' header, extracting
   the information useful to Wget.  Content-Disposition is a header
   borrowed from MIME; when used in HTTP, it typically serves for
   specifying the desired file name of the resource.  For example:

       Content-Disposition: attachment; filename="flora.jpg"

   Wget will skip the tokens it doesn't care about, such as
   "attachment" in the previous example; it will also skip other
   unrecognized params.  If the header is syntactically correct and
   contains a file name, a copy of the file name is stored in
   *filename and true is returned.  Otherwise, the function returns
   false.

   The file name is stripped of directory components and must not be
   empty.  */

static bool
parse_content_disposition (const char *hdr, char **filename)
{
  param_token name, value;
  while (extract_param (&hdr, &name, &value, ';'))
    if (BOUNDED_EQUAL_NO_CASE (name.b, name.e, "filename") && value.b != NULL)
      {
        /* Make the file name begin at the last slash or backslash. */
        const char *last_slash = memrchr (value.b, '/', value.e - value.b);
        const char *last_bs = memrchr (value.b, '\\', value.e - value.b);
        if (last_slash && last_bs)
          value.b = 1 + MAX (last_slash, last_bs);
        else if (last_slash || last_bs)
          value.b = 1 + (last_slash ? last_slash : last_bs);
        if (value.b == value.e)
          continue;
        /* Start with the directory prefix, if specified. */
        if (opt.dir_prefix)
          {
            int prefix_length = strlen (opt.dir_prefix);
            bool add_slash = (opt.dir_prefix[prefix_length - 1] != '/');
            int total_length;

            if (add_slash)
              ++prefix_length;
            total_length = prefix_length + (value.e - value.b);
            *filename = xmalloc (total_length + 1);
            strcpy (*filename, opt.dir_prefix);
            if (add_slash)
              (*filename)[prefix_length - 1] = '/';
            memcpy (*filename + prefix_length, value.b, (value.e - value.b));
            (*filename)[total_length] = '\0';
          }
        else
          *filename = strdupdelim (value.b, value.e);
        return true;
      }
  return false;
}

/* Persistent connections.  Currently, we cache the most recently used
   connection as persistent, provided that the HTTP server agrees to
   make it such.  The persistence data is stored in the variables
   below.  Ideally, it should be possible to cache an arbitrary fixed
   number of these connections.  */

/* Whether a persistent connection is active. */
static bool pconn_active;

static struct {
  /* The socket of the connection.  */
  int socket;

  /* Host and port of the currently active persistent connection. */
  char *host;
  int port;

  /* Whether a ssl handshake has occoured on this connection.  */
  bool ssl;

  /* Whether the connection was authorized.  This is only done by
     NTLM, which authorizes *connections* rather than individual
     requests.  (That practice is peculiar for HTTP, but it is a
     useful optimization.)  */
  bool authorized;

#ifdef ENABLE_NTLM
  /* NTLM data of the current connection.  */
  struct ntlmdata ntlm;
#endif
} pconn;

/* Mark the persistent connection as invalid and free the resources it
   uses.  This is used by the CLOSE_* macros after they forcefully
   close a registered persistent connection.  */

static void
invalidate_persistent (void)
{
  DEBUGP (("Disabling further reuse of socket %d.\n", pconn.socket));
  pconn_active = false;
  fd_close (pconn.socket);
  xfree (pconn.host);
  xzero (pconn);
}

/* Register FD, which should be a TCP/IP connection to HOST:PORT, as
   persistent.  This will enable someone to use the same connection
   later.  In the context of HTTP, this must be called only AFTER the
   response has been received and the server has promised that the
   connection will remain alive.

   If a previous connection was persistent, it is closed. */

static void
register_persistent (const char *host, int port, int fd, bool ssl)
{
  if (pconn_active)
    {
      if (pconn.socket == fd)
        {
          /* The connection FD is already registered. */
          return;
        }
      else
        {
          /* The old persistent connection is still active; close it
             first.  This situation arises whenever a persistent
             connection exists, but we then connect to a different
             host, and try to register a persistent connection to that
             one.  */
          invalidate_persistent ();
        }
    }

  pconn_active = true;
  pconn.socket = fd;
  pconn.host = xstrdup (host);
  pconn.port = port;
  pconn.ssl = ssl;
  pconn.authorized = false;

  DEBUGP (("Registered socket %d for persistent reuse.\n", fd));
}

/* Return true if a persistent connection is available for connecting
   to HOST:PORT.  */

static bool
persistent_available_p (const char *host, int port, bool ssl,
                        bool *host_lookup_failed)
{
  /* First, check whether a persistent connection is active at all.  */
  if (!pconn_active)
    return false;

  /* If we want SSL and the last connection wasn't or vice versa,
     don't use it.  Checking for host and port is not enough because
     HTTP and HTTPS can apparently coexist on the same port.  */
  if (ssl != pconn.ssl)
    return false;

  /* If we're not connecting to the same port, we're not interested. */
  if (port != pconn.port)
    return false;

  /* If the host is the same, we're in business.  If not, there is
     still hope -- read below.  */
  if (0 != strcasecmp (host, pconn.host))
    {
      /* Check if pconn.socket is talking to HOST under another name.
         This happens often when both sites are virtual hosts
         distinguished only by name and served by the same network
         interface, and hence the same web server (possibly set up by
         the ISP and serving many different web sites).  This
         admittedly unconventional optimization does not contradict
         HTTP and works well with popular server software.  */

      bool found;
      ip_address ip;
      struct address_list *al;

      if (ssl)
        /* Don't try to talk to two different SSL sites over the same
           secure connection!  (Besides, it's not clear that
           name-based virtual hosting is even possible with SSL.)  */
        return false;

      /* If pconn.socket's peer is one of the IP addresses HOST
         resolves to, pconn.socket is for all intents and purposes
         already talking to HOST.  */

      if (!socket_ip_address (pconn.socket, &ip, ENDPOINT_PEER))
        {
          /* Can't get the peer's address -- something must be very
             wrong with the connection.  */
          invalidate_persistent ();
          return false;
        }
      al = lookup_host (host, 0);
      if (!al)
        {
          *host_lookup_failed = true;
          return false;
        }

      found = address_list_contains (al, &ip);
      address_list_release (al);

      if (!found)
        return false;

      /* The persistent connection's peer address was found among the
         addresses HOST resolved to; therefore, pconn.sock is in fact
         already talking to HOST -- no need to reconnect.  */
    }

  /* Finally, check whether the connection is still open.  This is
     important because most servers implement liberal (short) timeout
     on persistent connections.  Wget can of course always reconnect
     if the connection doesn't work out, but it's nicer to know in
     advance.  This test is a logical followup of the first test, but
     is "expensive" and therefore placed at the end of the list.

     (Current implementation of test_socket_open has a nice side
     effect that it treats sockets with pending data as "closed".
     This is exactly what we want: if a broken server sends message
     body in response to HEAD, or if it sends more than conent-length
     data, we won't reuse the corrupted connection.)  */

  if (!test_socket_open (pconn.socket))
    {
      /* Oops, the socket is no longer open.  Now that we know that,
         let's invalidate the persistent connection before returning
         0.  */
      invalidate_persistent ();
      return false;
    }

  return true;
}

/* The idea behind these two CLOSE macros is to distinguish between
   two cases: one when the job we've been doing is finished, and we
   want to close the connection and leave, and two when something is
   seriously wrong and we're closing the connection as part of
   cleanup.

   In case of keep_alive, CLOSE_FINISH should leave the connection
   open, while CLOSE_INVALIDATE should still close it.

   Note that the semantics of the flag `keep_alive' is "this
   connection *will* be reused (the server has promised not to close
   the connection once we're done)", while the semantics of
   `pc_active_p && (fd) == pc_last_fd' is "we're *now* using an
   active, registered connection".  */

#define CLOSE_FINISH(fd) do {                   \
  if (!keep_alive)                              \
    {                                           \
      if (pconn_active && (fd) == pconn.socket) \
        invalidate_persistent ();               \
      else                                      \
        {                                       \
          fd_close (fd);                        \
          fd = -1;                              \
        }                                       \
    }                                           \
} while (0)

#define CLOSE_INVALIDATE(fd) do {               \
  if (pconn_active && (fd) == pconn.socket)     \
    invalidate_persistent ();                   \
  else                                          \
    fd_close (fd);                              \
  fd = -1;                                      \
} while (0)

struct http_stat
{
  wgint len;                    /* received length */
  wgint contlen;                /* expected length */
  wgint restval;                /* the restart value */
  int res;                      /* the result of last read */
  char *rderrmsg;               /* error message from read error */
  char *newloc;                 /* new location (redirection) */
  char *remote_time;            /* remote time-stamp string */
  char *error;                  /* textual HTTP error */
  int statcode;                 /* status code */
  char *message;                /* status message */
  wgint rd_size;                /* amount of data read from socket */
  double dltime;                /* time it took to download the data */
  const char *referer;          /* value of the referer header. */
  char *local_file;             /* local file name. */
  bool existence_checked;       /* true if we already checked for a file's
                                   existence after having begun to download
                                   (needed in gethttp for when connection is
                                   interrupted/restarted. */
  bool timestamp_checked;       /* true if pre-download time-stamping checks
                                 * have already been performed */
  char *orig_file_name;         /* name of file to compare for time-stamping
                                 * (might be != local_file if -K is set) */
  wgint orig_file_size;         /* size of file to compare for time-stamping */
  time_t orig_file_tstamp;      /* time-stamp of file to compare for
                                 * time-stamping */
};

static void
free_hstat (struct http_stat *hs)
{
  xfree_null (hs->newloc);
  xfree_null (hs->remote_time);
  xfree_null (hs->error);
  xfree_null (hs->rderrmsg);
  xfree_null (hs->local_file);
  xfree_null (hs->orig_file_name);
  xfree_null (hs->message);

  /* Guard against being called twice. */
  hs->newloc = NULL;
  hs->remote_time = NULL;
  hs->error = NULL;
}

#define BEGINS_WITH(line, string_constant)                               \
  (!strncasecmp (line, string_constant, sizeof (string_constant) - 1)    \
   && (c_isspace (line[sizeof (string_constant) - 1])                      \
       || !line[sizeof (string_constant) - 1]))

#ifdef __VMS
#define SET_USER_AGENT(req) do {                                         \
  if (!opt.useragent)                                                    \
    request_set_header (req, "User-Agent",                               \
                        aprintf ("Wget/%s (VMS %s %s)",                  \
                        version_string, vms_arch(), vms_vers()),         \
                        rel_value);                                      \
  else if (*opt.useragent)                                               \
    request_set_header (req, "User-Agent", opt.useragent, rel_none);     \
} while (0)
#else /* def __VMS */
#define SET_USER_AGENT(req) do {                                         \
  if (!opt.useragent)                                                    \
    request_set_header (req, "User-Agent",                               \
                        aprintf ("Wget/%s (%s)",                         \
                        version_string, OS_TYPE),                        \
                        rel_value);                                      \
  else if (*opt.useragent)                                               \
    request_set_header (req, "User-Agent", opt.useragent, rel_none);     \
} while (0)
#endif /* def __VMS [else] */

/* The flags that allow clobbering the file (opening with "wb").
   Defined here to avoid repetition later.  #### This will require
   rework.  */
#define ALLOW_CLOBBER (opt.noclobber || opt.always_rest || opt.timestamping \
                       || opt.dirstruct || opt.output_document)

/* Retrieve a document through HTTP protocol.  It recognizes status
   code, and correctly handles redirections.  It closes the network
   socket.  If it receives an error from the functions below it, it
   will print it if there is enough information to do so (almost
   always), returning the error to the caller (i.e. http_loop).

   Various HTTP parameters are stored to hs.

   If PROXY is non-NULL, the connection will be made to the proxy
   server, and u->url will be requested.  */
static uerr_t
gethttp (struct url *u, struct http_stat *hs, int *dt, struct url *proxy,
         struct iri *iri)
{
  struct request *req;

  char *type;
  char *user, *passwd;
  char *proxyauth;
  int statcode;
  int write_error;
  wgint contlen, contrange;
  struct url *conn;
  FILE *fp;

  int sock = -1;
  int flags;

  /* Set to 1 when the authorization has already been sent and should
     not be tried again. */
  bool auth_finished = false;

  /* Set to 1 when just globally-set Basic authorization has been sent;
   * should prevent further Basic negotiations, but not other
   * mechanisms. */
  bool basic_auth_finished = false;

  /* Whether NTLM authentication is used for this request. */
  bool ntlm_seen = false;

  /* Whether our connection to the remote host is through SSL.  */
  bool using_ssl = false;

  /* Whether a HEAD request will be issued (as opposed to GET or
     POST). */
  bool head_only = !!(*dt & HEAD_ONLY);

  char *head;
  struct response *resp;
  char hdrval[256];
  char *message;

  /* Whether this connection will be kept alive after the HTTP request
     is done. */
  bool keep_alive;

  /* Whether keep-alive should be inhibited.

     RFC 2068 requests that 1.0 clients not send keep-alive requests
     to proxies.  This is because many 1.0 proxies do not interpret
     the Connection header and transfer it to the remote server,
     causing it to not close the connection and leave both the proxy
     and the client hanging.  */
  bool inhibit_keep_alive =
    !opt.http_keep_alive || opt.ignore_length || proxy != NULL;

  /* Headers sent when using POST. */
  wgint post_data_size = 0;

  bool host_lookup_failed = false;

#ifdef HAVE_SSL
  if (u->scheme == SCHEME_HTTPS)
    {
      /* Initialize the SSL context.  After this has once been done,
         it becomes a no-op.  */
      if (!ssl_init ())
        {
          scheme_disable (SCHEME_HTTPS);
          logprintf (LOG_NOTQUIET,
                     _("Disabling SSL due to encountered errors.\n"));
          return SSLINITFAILED;
        }
    }
#endif /* HAVE_SSL */

  /* Initialize certain elements of struct http_stat.  */
  hs->len = 0;
  hs->contlen = -1;
  hs->res = -1;
  hs->rderrmsg = NULL;
  hs->newloc = NULL;
  hs->remote_time = NULL;
  hs->error = NULL;
  hs->message = NULL;

  conn = u;

  /* Prepare the request to send. */

  req = request_new ();
  {
    char *meth_arg;
    const char *meth = "GET";
    if (head_only)
      meth = "HEAD";
    else if (opt.post_file_name || opt.post_data)
      meth = "POST";
    /* Use the full path, i.e. one that includes the leading slash and
       the query string.  E.g. if u->path is "foo/bar" and u->query is
       "param=value", full_path will be "/foo/bar?param=value".  */
    if (proxy
#ifdef HAVE_SSL
        /* When using SSL over proxy, CONNECT establishes a direct
           connection to the HTTPS server.  Therefore use the same
           argument as when talking to the server directly. */
        && u->scheme != SCHEME_HTTPS
#endif
        )
      meth_arg = xstrdup (u->url);
    else
      meth_arg = url_full_path (u);
    request_set_method (req, meth, meth_arg);
  }

  request_set_header (req, "Referer", (char *) hs->referer, rel_none);
  if (*dt & SEND_NOCACHE)
    request_set_header (req, "Pragma", "no-cache", rel_none);
  if (hs->restval)
    request_set_header (req, "Range",
                        aprintf ("bytes=%s-",
                                 number_to_static_string (hs->restval)),
                        rel_value);
  SET_USER_AGENT (req);
  request_set_header (req, "Accept", "*/*", rel_none);

  /* Find the username and password for authentication. */
  user = u->user;
  passwd = u->passwd;
  search_netrc (u->host, (const char **)&user, (const char **)&passwd, 0);
  user = user ? user : (opt.http_user ? opt.http_user : opt.user);
  passwd = passwd ? passwd : (opt.http_passwd ? opt.http_passwd : opt.passwd);

  /* We only do "site-wide" authentication with "global" user/password
   * values unless --auth-no-challange has been requested; URL user/password
   * info overrides. */
  if (user && passwd && (!u->user || opt.auth_without_challenge))
    {
      /* If this is a host for which we've already received a Basic
       * challenge, we'll go ahead and send Basic authentication creds. */
      basic_auth_finished = maybe_send_basic_creds(u->host, user, passwd, req);
    }

  /* Generate the Host header, HOST:PORT.  Take into account that:

     - Broken server-side software often doesn't recognize the PORT
       argument, so we must generate "Host: www.server.com" instead of
       "Host: www.server.com:80" (and likewise for https port).

     - IPv6 addresses contain ":", so "Host: 3ffe:8100:200:2::2:1234"
       becomes ambiguous and needs to be rewritten as "Host:
       [3ffe:8100:200:2::2]:1234".  */
  {
    /* Formats arranged for hfmt[add_port][add_squares].  */
    static const char *hfmt[][2] = {
      { "%s", "[%s]" }, { "%s:%d", "[%s]:%d" }
    };
    int add_port = u->port != scheme_default_port (u->scheme);
    int add_squares = strchr (u->host, ':') != NULL;
    request_set_header (req, "Host",
                        aprintf (hfmt[add_port][add_squares], u->host, u->port),
                        rel_value);
  }

  if (!inhibit_keep_alive)
    request_set_header (req, "Connection", "Keep-Alive", rel_none);

  if (opt.cookies)
    request_set_header (req, "Cookie",
                        cookie_header (wget_cookie_jar,
                                       u->host, u->port, u->path,
#ifdef HAVE_SSL
                                       u->scheme == SCHEME_HTTPS
#else
                                       0
#endif
                                       ),
                        rel_value);

  if (opt.post_data || opt.post_file_name)
    {
      request_set_header (req, "Content-Type",
                          "application/x-www-form-urlencoded", rel_none);
      if (opt.post_data)
        post_data_size = strlen (opt.post_data);
      else
        {
          post_data_size = file_size (opt.post_file_name);
          if (post_data_size == -1)
            {
              logprintf (LOG_NOTQUIET, _("POST data file %s missing: %s\n"),
                         quote (opt.post_file_name), strerror (errno));
              post_data_size = 0;
            }
        }
      request_set_header (req, "Content-Length",
                          xstrdup (number_to_static_string (post_data_size)),
                          rel_value);
    }

  /* Add the user headers. */
  if (opt.user_headers)
    {
      int i;
      for (i = 0; opt.user_headers[i]; i++)
        request_set_user_header (req, opt.user_headers[i]);
    }

 retry_with_auth:
  /* We need to come back here when the initial attempt to retrieve
     without authorization header fails.  (Expected to happen at least
     for the Digest authorization scheme.)  */

  proxyauth = NULL;
  if (proxy)
    {
      char *proxy_user, *proxy_passwd;
      /* For normal username and password, URL components override
         command-line/wgetrc parameters.  With proxy
         authentication, it's the reverse, because proxy URLs are
         normally the "permanent" ones, so command-line args
         should take precedence.  */
      if (opt.proxy_user && opt.proxy_passwd)
        {
          proxy_user = opt.proxy_user;
          proxy_passwd = opt.proxy_passwd;
        }
      else
        {
          proxy_user = proxy->user;
          proxy_passwd = proxy->passwd;
        }
      /* #### This does not appear right.  Can't the proxy request,
         say, `Digest' authentication?  */
      if (proxy_user && proxy_passwd)
        proxyauth = basic_authentication_encode (proxy_user, proxy_passwd);

      /* If we're using a proxy, we will be connecting to the proxy
         server.  */
      conn = proxy;

      /* Proxy authorization over SSL is handled below. */
#ifdef HAVE_SSL
      if (u->scheme != SCHEME_HTTPS)
#endif
        request_set_header (req, "Proxy-Authorization", proxyauth, rel_value);
    }

  keep_alive = false;

  /* Establish the connection.  */

  if (!inhibit_keep_alive)
    {
      /* Look for a persistent connection to target host, unless a
         proxy is used.  The exception is when SSL is in use, in which
         case the proxy is nothing but a passthrough to the target
         host, registered as a connection to the latter.  */
      struct url *relevant = conn;
#ifdef HAVE_SSL
      if (u->scheme == SCHEME_HTTPS)
        relevant = u;
#endif

      if (persistent_available_p (relevant->host, relevant->port,
#ifdef HAVE_SSL
                                  relevant->scheme == SCHEME_HTTPS,
#else
                                  0,
#endif
                                  &host_lookup_failed))
        {
          sock = pconn.socket;
          using_ssl = pconn.ssl;
          logprintf (LOG_VERBOSE, _("Reusing existing connection to %s:%d.\n"),
                     quotearg_style (escape_quoting_style, pconn.host),
                     pconn.port);
          DEBUGP (("Reusing fd %d.\n", sock));
          if (pconn.authorized)
            /* If the connection is already authorized, the "Basic"
               authorization added by code above is unnecessary and
               only hurts us.  */
            request_remove_header (req, "Authorization");
        }
      else if (host_lookup_failed)
        {
          request_free (req);
          logprintf(LOG_NOTQUIET,
                    _("%s: unable to resolve host address %s\n"),
                    exec_name, quote (relevant->host));
          return HOSTERR;
        }
    }

  if (sock < 0)
    {
      sock = connect_to_host (conn->host, conn->port);
      if (sock == E_HOST)
        {
          request_free (req);
          return HOSTERR;
        }
      else if (sock < 0)
        {
          request_free (req);
          return (retryable_socket_connect_error (errno)
                  ? CONERROR : CONIMPOSSIBLE);
        }

#ifdef HAVE_SSL
      if (proxy && u->scheme == SCHEME_HTTPS)
        {
          /* When requesting SSL URLs through proxies, use the
             CONNECT method to request passthrough.  */
          struct request *connreq = request_new ();
          request_set_method (connreq, "CONNECT",
                              aprintf ("%s:%d", u->host, u->port));
          SET_USER_AGENT (connreq);
          if (proxyauth)
            {
              request_set_header (connreq, "Proxy-Authorization",
                                  proxyauth, rel_value);
              /* Now that PROXYAUTH is part of the CONNECT request,
                 zero it out so we don't send proxy authorization with
                 the regular request below.  */
              proxyauth = NULL;
            }
          /* Examples in rfc2817 use the Host header in CONNECT
             requests.  I don't see how that gains anything, given
             that the contents of Host would be exactly the same as
             the contents of CONNECT.  */

          write_error = request_send (connreq, sock);
          request_free (connreq);
          if (write_error < 0)
            {
              CLOSE_INVALIDATE (sock);
              return WRITEFAILED;
            }

          head = read_http_response_head (sock);
          if (!head)
            {
              logprintf (LOG_VERBOSE, _("Failed reading proxy response: %s\n"),
                         fd_errstr (sock));
              CLOSE_INVALIDATE (sock);
              return HERR;
            }
          message = NULL;
          if (!*head)
            {
              xfree (head);
              goto failed_tunnel;
            }
          DEBUGP (("proxy responded with: [%s]\n", head));

          resp = resp_new (head);
          statcode = resp_status (resp, &message);
          hs->message = xstrdup (message);
          resp_free (resp);
          xfree (head);
          if (statcode != 200)
            {
            failed_tunnel:
              logprintf (LOG_NOTQUIET, _("Proxy tunneling failed: %s"),
                         message ? quotearg_style (escape_quoting_style, message) : "?");
              xfree_null (message);
              return CONSSLERR;
            }
          xfree_null (message);

          /* SOCK is now *really* connected to u->host, so update CONN
             to reflect this.  That way register_persistent will
             register SOCK as being connected to u->host:u->port.  */
          conn = u;
        }

      if (conn->scheme == SCHEME_HTTPS)
        {
          if (!ssl_connect_wget (sock))
            {
              fd_close (sock);
              return CONSSLERR;
            }
          else if (!ssl_check_certificate (sock, u->host))
            {
              fd_close (sock);
              return VERIFCERTERR;
            }
          using_ssl = true;
        }
#endif /* HAVE_SSL */
    }

  /* Send the request to server.  */
  write_error = request_send (req, sock);

  if (write_error >= 0)
    {
      if (opt.post_data)
        {
          DEBUGP (("[POST data: %s]\n", opt.post_data));
          write_error = fd_write (sock, opt.post_data, post_data_size, -1);
        }
      else if (opt.post_file_name && post_data_size != 0)
        write_error = post_file (sock, opt.post_file_name, post_data_size);
    }

  if (write_error < 0)
    {
      CLOSE_INVALIDATE (sock);
      request_free (req);
      return WRITEFAILED;
    }
  logprintf (LOG_VERBOSE, _("%s request sent, awaiting response... "),
             proxy ? "Proxy" : "HTTP");
  contlen = -1;
  contrange = 0;
  *dt &= ~RETROKF;

  head = read_http_response_head (sock);
  if (!head)
    {
      if (errno == 0)
        {
          logputs (LOG_NOTQUIET, _("No data received.\n"));
          CLOSE_INVALIDATE (sock);
          request_free (req);
          return HEOF;
        }
      else
        {
          logprintf (LOG_NOTQUIET, _("Read error (%s) in headers.\n"),
                     fd_errstr (sock));
          CLOSE_INVALIDATE (sock);
          request_free (req);
          return HERR;
        }
    }
  DEBUGP (("\n---response begin---\n%s---response end---\n", head));

  resp = resp_new (head);

  /* Check for status line.  */
  message = NULL;
  statcode = resp_status (resp, &message);
  hs->message = xstrdup (message);
  if (!opt.server_response)
    logprintf (LOG_VERBOSE, "%2d %s\n", statcode,
               message ? quotearg_style (escape_quoting_style, message) : "");
  else
    {
      logprintf (LOG_VERBOSE, "\n");
      print_server_response (resp, "  ");
    }

  if (!opt.ignore_length
      && resp_header_copy (resp, "Content-Length", hdrval, sizeof (hdrval)))
    {
      wgint parsed;
      errno = 0;
      parsed = str_to_wgint (hdrval, NULL, 10);
      if (parsed == WGINT_MAX && errno == ERANGE)
        {
          /* Out of range.
             #### If Content-Length is out of range, it most likely
             means that the file is larger than 2G and that we're
             compiled without LFS.  In that case we should probably
             refuse to even attempt to download the file.  */
          contlen = -1;
        }
      else if (parsed < 0)
        {
          /* Negative Content-Length; nonsensical, so we can't
             assume any information about the content to receive. */
          contlen = -1;
        }
      else
        contlen = parsed;
    }

  /* Check for keep-alive related responses. */
  if (!inhibit_keep_alive && contlen != -1)
    {
      if (resp_header_copy (resp, "Keep-Alive", NULL, 0))
        keep_alive = true;
      else if (resp_header_copy (resp, "Connection", hdrval, sizeof (hdrval)))
        {
          if (0 == strcasecmp (hdrval, "Keep-Alive"))
            keep_alive = true;
        }
    }

  /* Handle (possibly multiple instances of) the Set-Cookie header. */
  if (opt.cookies)
    {
      int scpos;
      const char *scbeg, *scend;
      /* The jar should have been created by now. */
      assert (wget_cookie_jar != NULL);
      for (scpos = 0;
           (scpos = resp_header_locate (resp, "Set-Cookie", scpos,
                                        &scbeg, &scend)) != -1;
           ++scpos)
        {
          char *set_cookie; BOUNDED_TO_ALLOCA (scbeg, scend, set_cookie);
          cookie_handle_set_cookie (wget_cookie_jar, u->host, u->port,
                                    u->path, set_cookie);
        }
    }

  if (keep_alive)
    /* The server has promised that it will not close the connection
       when we're done.  This means that we can register it.  */
    register_persistent (conn->host, conn->port, sock, using_ssl);

  if (statcode == HTTP_STATUS_UNAUTHORIZED)
    {
      /* Authorization is required.  */
      if (keep_alive && !head_only && skip_short_body (sock, contlen))
        CLOSE_FINISH (sock);
      else
        CLOSE_INVALIDATE (sock);
      pconn.authorized = false;
      if (!auth_finished && (user && passwd))
        {
          /* IIS sends multiple copies of WWW-Authenticate, one with
             the value "negotiate", and other(s) with data.  Loop over
             all the occurrences and pick the one we recognize.  */
          int wapos;
          const char *wabeg, *waend;
          char *www_authenticate = NULL;
          for (wapos = 0;
               (wapos = resp_header_locate (resp, "WWW-Authenticate", wapos,
                                            &wabeg, &waend)) != -1;
               ++wapos)
            if (known_authentication_scheme_p (wabeg, waend))
              {
                BOUNDED_TO_ALLOCA (wabeg, waend, www_authenticate);
                break;
              }

          if (!www_authenticate)
            {
              /* If the authentication header is missing or
                 unrecognized, there's no sense in retrying.  */
              logputs (LOG_NOTQUIET, _("Unknown authentication scheme.\n"));
            }
          else if (!basic_auth_finished
                   || !BEGINS_WITH (www_authenticate, "Basic"))
            {
              char *pth;
              pth = url_full_path (u);
              request_set_header (req, "Authorization",
                                  create_authorization_line (www_authenticate,
                                                             user, passwd,
                                                             request_method (req),
                                                             pth,
                                                             &auth_finished),
                                  rel_value);
              if (BEGINS_WITH (www_authenticate, "NTLM"))
                ntlm_seen = true;
              else if (!u->user && BEGINS_WITH (www_authenticate, "Basic"))
                {
                  /* Need to register this host as using basic auth,
                   * so we automatically send creds next time. */
                  register_basic_auth_host (u->host);
                }
              xfree (pth);
              xfree_null (message);
              resp_free (resp);
              xfree (head);
              goto retry_with_auth;
            }
          else
            {
              /* We already did Basic auth, and it failed. Gotta
               * give up. */
            }
        }
      logputs (LOG_NOTQUIET, _("Authorization failed.\n"));
      request_free (req);
      xfree_null (message);
      resp_free (resp);
      xfree (head);
      return AUTHFAILED;
    }
  else /* statcode != HTTP_STATUS_UNAUTHORIZED */
    {
      /* Kludge: if NTLM is used, mark the TCP connection as authorized. */
      if (ntlm_seen)
        pconn.authorized = true;
    }

  /* Determine the local filename if needed. Notice that if -O is used
   * hstat.local_file is set by http_loop to the argument of -O. */
  if (!hs->local_file)
    {
      /* Honor Content-Disposition whether possible. */
      if (!opt.content_disposition
          || !resp_header_copy (resp, "Content-Disposition",
                                hdrval, sizeof (hdrval))
          || !parse_content_disposition (hdrval, &hs->local_file))
        {
          /* The Content-Disposition header is missing or broken.
           * Choose unique file name according to given URL. */
          hs->local_file = url_file_name (u);
        }
    }

  /* TODO: perform this check only once. */
  if (!hs->existence_checked && file_exists_p (hs->local_file))
    {
      if (opt.noclobber && !opt.output_document)
        {
          /* If opt.noclobber is turned on and file already exists, do not
             retrieve the file. But if the output_document was given, then this
             test was already done and the file didn't exist. Hence the !opt.output_document */
          logprintf (LOG_VERBOSE, _("\
File %s already there; not retrieving.\n\n"), quote (hs->local_file));
          /* If the file is there, we suppose it's retrieved OK.  */
          *dt |= RETROKF;

          /* #### Bogusness alert.  */
          /* If its suffix is "html" or "htm" or similar, assume text/html.  */
          if (has_html_suffix_p (hs->local_file))
            *dt |= TEXTHTML;

          xfree (head);
          xfree_null (message);
          return RETRUNNEEDED;
        }
      else if (!ALLOW_CLOBBER)
        {
          char *unique = unique_name (hs->local_file, true);
          if (unique != hs->local_file)
            xfree (hs->local_file);
          hs->local_file = unique;
        }
    }
  hs->existence_checked = true;

  /* Support timestamping */
  /* TODO: move this code out of gethttp. */
  if (opt.timestamping && !hs->timestamp_checked)
    {
      size_t filename_len = strlen (hs->local_file);
      char *filename_plus_orig_suffix = alloca (filename_len + sizeof (ORIG_SFX));
      bool local_dot_orig_file_exists = false;
      char *local_filename = NULL;
      struct_stat st;

      if (opt.backup_converted)
        /* If -K is specified, we'll act on the assumption that it was specified
           last time these files were downloaded as well, and instead of just
           comparing local file X against server file X, we'll compare local
           file X.orig (if extant, else X) against server file X.  If -K
           _wasn't_ specified last time, or the server contains files called
           *.orig, -N will be back to not operating correctly with -k. */
        {
          /* Would a single s[n]printf() call be faster?  --dan

             Definitely not.  sprintf() is horribly slow.  It's a
             different question whether the difference between the two
             affects a program.  Usually I'd say "no", but at one
             point I profiled Wget, and found that a measurable and
             non-negligible amount of time was lost calling sprintf()
             in url.c.  Replacing sprintf with inline calls to
             strcpy() and number_to_string() made a difference.
             --hniksic */
          memcpy (filename_plus_orig_suffix, hs->local_file, filename_len);
          memcpy (filename_plus_orig_suffix + filename_len,
                  ORIG_SFX, sizeof (ORIG_SFX));

          /* Try to stat() the .orig file. */
          if (stat (filename_plus_orig_suffix, &st) == 0)
            {
              local_dot_orig_file_exists = true;
              local_filename = filename_plus_orig_suffix;
            }
        }

      if (!local_dot_orig_file_exists)
        /* Couldn't stat() <file>.orig, so try to stat() <file>. */
        if (stat (hs->local_file, &st) == 0)
          local_filename = hs->local_file;

      if (local_filename != NULL)
        /* There was a local file, so we'll check later to see if the version
           the server has is the same version we already have, allowing us to
           skip a download. */
        {
          hs->orig_file_name = xstrdup (local_filename);
          hs->orig_file_size = st.st_size;
          hs->orig_file_tstamp = st.st_mtime;
#ifdef WINDOWS
          /* Modification time granularity is 2 seconds for Windows, so
             increase local time by 1 second for later comparison. */
          ++hs->orig_file_tstamp;
#endif
        }
    }

  request_free (req);

  hs->statcode = statcode;
  if (statcode == -1)
    hs->error = xstrdup (_("Malformed status line"));
  else if (!*message)
    hs->error = xstrdup (_("(no description)"));
  else
    hs->error = xstrdup (message);
  xfree_null (message);

  type = resp_header_strdup (resp, "Content-Type");
  if (type)
    {
      char *tmp = strchr (type, ';');
      if (tmp)
        {
          /* sXXXav: only needed if IRI support is enabled */
          char *tmp2 = tmp + 1;

          while (tmp > type && c_isspace (tmp[-1]))
            --tmp;
          *tmp = '\0';

          /* Try to get remote encoding if needed */
          if (opt.enable_iri && !opt.encoding_remote)
            {
              tmp = parse_charset (tmp2);
              if (tmp)
                set_content_encoding (iri, tmp);
            }
        }
    }
  hs->newloc = resp_header_strdup (resp, "Location");
  hs->remote_time = resp_header_strdup (resp, "Last-Modified");

  if (resp_header_copy (resp, "Content-Range", hdrval, sizeof (hdrval)))
    {
      wgint first_byte_pos, last_byte_pos, entity_length;
      if (parse_content_range (hdrval, &first_byte_pos, &last_byte_pos,
                               &entity_length))
        {
          contrange = first_byte_pos;
          contlen = last_byte_pos - first_byte_pos + 1;
        }
    }
  resp_free (resp);

  /* 20x responses are counted among successful by default.  */
  if (H_20X (statcode))
    *dt |= RETROKF;

  /* Return if redirected.  */
  if (H_REDIRECTED (statcode) || statcode == HTTP_STATUS_MULTIPLE_CHOICES)
    {
      /* RFC2068 says that in case of the 300 (multiple choices)
         response, the server can output a preferred URL through
         `Location' header; otherwise, the request should be treated
         like GET.  So, if the location is set, it will be a
         redirection; otherwise, just proceed normally.  */
      if (statcode == HTTP_STATUS_MULTIPLE_CHOICES && !hs->newloc)
        *dt |= RETROKF;
      else
        {
          logprintf (LOG_VERBOSE,
                     _("Location: %s%s\n"),
                     hs->newloc ? escnonprint_uri (hs->newloc) : _("unspecified"),
                     hs->newloc ? _(" [following]") : "");
          if (keep_alive && !head_only && skip_short_body (sock, contlen))
            CLOSE_FINISH (sock);
          else
            CLOSE_INVALIDATE (sock);
          xfree_null (type);
          xfree (head);
          return NEWLOCATION;
        }
    }

  /* If content-type is not given, assume text/html.  This is because
     of the multitude of broken CGI's that "forget" to generate the
     content-type.  */
  if (!type ||
        0 == strncasecmp (type, TEXTHTML_S, strlen (TEXTHTML_S)) ||
        0 == strncasecmp (type, TEXTXHTML_S, strlen (TEXTXHTML_S)))
    *dt |= TEXTHTML;
  else
    *dt &= ~TEXTHTML;

  if (type &&
      0 == strncasecmp (type, TEXTCSS_S, strlen (TEXTCSS_S)))
    *dt |= TEXTCSS;
  else
    *dt &= ~TEXTCSS;

  if (opt.adjust_extension)
    {
      if (*dt & TEXTHTML)
        /* -E / --adjust-extension / adjust_extension = on was specified,
           and this is a text/html file.  If some case-insensitive
           variation on ".htm[l]" isn't already the file's suffix,
           tack on ".html". */
        {
          ensure_extension (hs, ".html", dt);
        }
      else if (*dt & TEXTCSS)
        {
          ensure_extension (hs, ".css", dt);
        }
    }

  if (statcode == HTTP_STATUS_RANGE_NOT_SATISFIABLE
      || (hs->restval > 0 && statcode == HTTP_STATUS_OK
          && contrange == 0 && hs->restval >= contlen)
     )
    {
      /* If `-c' is in use and the file has been fully downloaded (or
         the remote file has shrunk), Wget effectively requests bytes
         after the end of file and the server response with 416
         (or 200 with a <= Content-Length.  */
      logputs (LOG_VERBOSE, _("\
\n    The file is already fully retrieved; nothing to do.\n\n"));
      /* In case the caller inspects. */
      hs->len = contlen;
      hs->res = 0;
      /* Mark as successfully retrieved. */
      *dt |= RETROKF;
      xfree_null (type);
      CLOSE_INVALIDATE (sock);        /* would be CLOSE_FINISH, but there
                                   might be more bytes in the body. */
      xfree (head);
      return RETRUNNEEDED;
    }
  if ((contrange != 0 && contrange != hs->restval)
      || (H_PARTIAL (statcode) && !contrange))
    {
      /* The Range request was somehow misunderstood by the server.
         Bail out.  */
      xfree_null (type);
      CLOSE_INVALIDATE (sock);
      xfree (head);
      return RANGEERR;
    }
  if (contlen == -1)
    hs->contlen = -1;
  else
    hs->contlen = contlen + contrange;

  if (opt.verbose)
    {
      if (*dt & RETROKF)
        {
          /* No need to print this output if the body won't be
             downloaded at all, or if the original server response is
             printed.  */
          logputs (LOG_VERBOSE, _("Length: "));
          if (contlen != -1)
            {
              logputs (LOG_VERBOSE, number_to_static_string (contlen + contrange));
              if (contlen + contrange >= 1024)
                logprintf (LOG_VERBOSE, " (%s)",
                           human_readable (contlen + contrange));
              if (contrange)
                {
                  if (contlen >= 1024)
                    logprintf (LOG_VERBOSE, _(", %s (%s) remaining"),
                               number_to_static_string (contlen),
                               human_readable (contlen));
                  else
                    logprintf (LOG_VERBOSE, _(", %s remaining"),
                               number_to_static_string (contlen));
                }
            }
          else
            logputs (LOG_VERBOSE,
                     opt.ignore_length ? _("ignored") : _("unspecified"));
          if (type)
            logprintf (LOG_VERBOSE, " [%s]\n", quotearg_style (escape_quoting_style, type));
          else
            logputs (LOG_VERBOSE, "\n");
        }
    }
  xfree_null (type);
  type = NULL;                        /* We don't need it any more.  */

  /* Return if we have no intention of further downloading.  */
  if (!(*dt & RETROKF) || head_only)
    {
      /* In case the caller cares to look...  */
      hs->len = 0;
      hs->res = 0;
      xfree_null (type);
      if (head_only)
        /* Pre-1.10 Wget used CLOSE_INVALIDATE here.  Now we trust the
           servers not to send body in response to a HEAD request, and
           those that do will likely be caught by test_socket_open.
           If not, they can be worked around using
           `--no-http-keep-alive'.  */
        CLOSE_FINISH (sock);
      else if (keep_alive && skip_short_body (sock, contlen))
        /* Successfully skipped the body; also keep using the socket. */
        CLOSE_FINISH (sock);
      else
        CLOSE_INVALIDATE (sock);
      xfree (head);
      return RETRFINISHED;
    }

/* 2005-06-17 SMS.
   For VMS, define common fopen() optional arguments.
*/
#ifdef __VMS
# define FOPEN_OPT_ARGS "fop=sqo", "acc", acc_cb, &open_id
# define FOPEN_BIN_FLAG 3
#else /* def __VMS */
# define FOPEN_BIN_FLAG true
#endif /* def __VMS [else] */

  /* Open the local file.  */
  if (!output_stream)
    {
      mkalldirs (hs->local_file);
      if (opt.backups)
        rotate_backups (hs->local_file);
      if (hs->restval)
        {
#ifdef __VMS
          int open_id;

          open_id = 21;
          fp = fopen (hs->local_file, "ab", FOPEN_OPT_ARGS);
#else /* def __VMS */
          fp = fopen (hs->local_file, "ab");
#endif /* def __VMS [else] */
        }
      else if (ALLOW_CLOBBER)
        {
#ifdef __VMS
          int open_id;

          open_id = 22;
          fp = fopen (hs->local_file, "wb", FOPEN_OPT_ARGS);
#else /* def __VMS */
          fp = fopen (hs->local_file, "wb");
#endif /* def __VMS [else] */
        }
      else
        {
          fp = fopen_excl (hs->local_file, FOPEN_BIN_FLAG);
          if (!fp && errno == EEXIST)
            {
              /* We cannot just invent a new name and use it (which is
                 what functions like unique_create typically do)
                 because we told the user we'd use this name.
                 Instead, return and retry the download.  */
              logprintf (LOG_NOTQUIET,
                         _("%s has sprung into existence.\n"),
                         hs->local_file);
              CLOSE_INVALIDATE (sock);
              xfree (head);
              return FOPEN_EXCL_ERR;
            }
        }
      if (!fp)
        {
          logprintf (LOG_NOTQUIET, "%s: %s\n", hs->local_file, strerror (errno));
          CLOSE_INVALIDATE (sock);
          xfree (head);
          return FOPENERR;
        }
    }
  else
    fp = output_stream;

  /* Print fetch message, if opt.verbose.  */
  if (opt.verbose)
    {
      logprintf (LOG_NOTQUIET, _("Saving to: %s\n"),
                 HYPHENP (hs->local_file) ? quote ("STDOUT") : quote (hs->local_file));
    }

  /* This confuses the timestamping code that checks for file size.
     #### The timestamping code should be smarter about file size.  */
  if (opt.save_headers && hs->restval == 0)
    fwrite (head, 1, strlen (head), fp);

  /* Now we no longer need to store the response header. */
  xfree (head);

  /* Download the request body.  */
  flags = 0;
  if (contlen != -1)
    /* If content-length is present, read that much; otherwise, read
       until EOF.  The HTTP spec doesn't require the server to
       actually close the connection when it's done sending data. */
    flags |= rb_read_exactly;
  if (hs->restval > 0 && contrange == 0)
    /* If the server ignored our range request, instruct fd_read_body
       to skip the first RESTVAL bytes of body.  */
    flags |= rb_skip_startpos;
  hs->len = hs->restval;
  hs->rd_size = 0;
  hs->res = fd_read_body (sock, fp, contlen != -1 ? contlen : 0,
                          hs->restval, &hs->rd_size, &hs->len, &hs->dltime,
                          flags);

  if (hs->res >= 0)
    CLOSE_FINISH (sock);
  else
    {
      if (hs->res < 0)
        hs->rderrmsg = xstrdup (fd_errstr (sock));
      CLOSE_INVALIDATE (sock);
    }

  if (!output_stream)
    fclose (fp);
  if (hs->res == -2)
    return FWRITEERR;
  return RETRFINISHED;
}

/* The genuine HTTP loop!  This is the part where the retrieval is
   retried, and retried, and retried, and...  */
uerr_t
http_loop (struct url *u, char **newloc, char **local_file, const char *referer,
           int *dt, struct url *proxy, struct iri *iri)
{
  int count;
  bool got_head = false;         /* used for time-stamping and filename detection */
  bool time_came_from_head = false;
  bool got_name = false;
  char *tms;
  const char *tmrate;
  uerr_t err, ret = TRYLIMEXC;
  time_t tmr = -1;               /* remote time-stamp */
  struct http_stat hstat;        /* HTTP status */
  struct_stat st;
  bool send_head_first = true;
  char *file_name;

  /* Assert that no value for *LOCAL_FILE was passed. */
  assert (local_file == NULL || *local_file == NULL);

  /* Set LOCAL_FILE parameter. */
  if (local_file && opt.output_document)
    *local_file = HYPHENP (opt.output_document) ? NULL : xstrdup (opt.output_document);

  /* Reset NEWLOC parameter. */
  *newloc = NULL;

  /* This used to be done in main(), but it's a better idea to do it
     here so that we don't go through the hoops if we're just using
     FTP or whatever. */
  if (opt.cookies)
    load_cookies();

  /* Warn on (likely bogus) wildcard usage in HTTP. */
  if (opt.ftp_glob && has_wildcards_p (u->path))
    logputs (LOG_VERBOSE, _("Warning: wildcards not supported in HTTP.\n"));

  /* Setup hstat struct. */
  xzero (hstat);
  hstat.referer = referer;

  if (opt.output_document)
    {
      hstat.local_file = xstrdup (opt.output_document);
      got_name = true;
    }
  else if (!opt.content_disposition)
    {
      hstat.local_file = url_file_name (u);
      got_name = true;
    }

  /* TODO: Ick! This code is now in both gethttp and http_loop, and is
   * screaming for some refactoring. */
  if (got_name && file_exists_p (hstat.local_file) && opt.noclobber && !opt.output_document)
    {
      /* If opt.noclobber is turned on and file already exists, do not
         retrieve the file. But if the output_document was given, then this
         test was already done and the file didn't exist. Hence the !opt.output_document */
      logprintf (LOG_VERBOSE, _("\
File %s already there; not retrieving.\n\n"),
                 quote (hstat.local_file));
      /* If the file is there, we suppose it's retrieved OK.  */
      *dt |= RETROKF;

      /* #### Bogusness alert.  */
      /* If its suffix is "html" or "htm" or similar, assume text/html.  */
      if (has_html_suffix_p (hstat.local_file))
        *dt |= TEXTHTML;

      ret = RETROK;
      goto exit;
    }

  /* Reset the counter. */
  count = 0;

  /* Reset the document type. */
  *dt = 0;

  /* Skip preliminary HEAD request if we're not in spider mode AND
   * if -O was given or HTTP Content-Disposition support is disabled. */
  if (!opt.spider
      && (got_name || !opt.content_disposition))
    send_head_first = false;

  /* Send preliminary HEAD request if -N is given and we have an existing
   * destination file. */
  file_name = url_file_name (u);
  if (opt.timestamping
      && !opt.content_disposition
      && file_exists_p (file_name))
    send_head_first = true;
  xfree (file_name);

  /* THE loop */
  do
    {
      /* Increment the pass counter.  */
      ++count;
      sleep_between_retrievals (count);

      /* Get the current time string.  */
      tms = datetime_str (time (NULL));

      if (opt.spider && !got_head)
        logprintf (LOG_VERBOSE, _("\
Spider mode enabled. Check if remote file exists.\n"));

      /* Print fetch message, if opt.verbose.  */
      if (opt.verbose)
        {
          char *hurl = url_string (u, URL_AUTH_HIDE_PASSWD);

          if (count > 1)
            {
              char tmp[256];
              sprintf (tmp, _("(try:%2d)"), count);
              logprintf (LOG_NOTQUIET, "--%s--  %s  %s\n",
                         tms, tmp, hurl);
            }
          else
            {
              logprintf (LOG_NOTQUIET, "--%s--  %s\n",
                         tms, hurl);
            }

#ifdef WINDOWS
          ws_changetitle (hurl);
#endif
          xfree (hurl);
        }

      /* Default document type is empty.  However, if spider mode is
         on or time-stamping is employed, HEAD_ONLY commands is
         encoded within *dt.  */
      if (send_head_first && !got_head)
        *dt |= HEAD_ONLY;
      else
        *dt &= ~HEAD_ONLY;

      /* Decide whether or not to restart.  */
      if (opt.always_rest
          && got_name
          && stat (hstat.local_file, &st) == 0
          && S_ISREG (st.st_mode))
        /* When -c is used, continue from on-disk size.  (Can't use
           hstat.len even if count>1 because we don't want a failed
           first attempt to clobber existing data.)  */
        hstat.restval = st.st_size;
      else if (count > 1)
        /* otherwise, continue where the previous try left off */
        hstat.restval = hstat.len;
      else
        hstat.restval = 0;

      /* Decide whether to send the no-cache directive.  We send it in
         two cases:
           a) we're using a proxy, and we're past our first retrieval.
              Some proxies are notorious for caching incomplete data, so
              we require a fresh get.
           b) caching is explicitly inhibited. */
      if ((proxy && count > 1)        /* a */
          || !opt.allow_cache)        /* b */
        *dt |= SEND_NOCACHE;
      else
        *dt &= ~SEND_NOCACHE;

      /* Try fetching the document, or at least its head.  */
      err = gethttp (u, &hstat, dt, proxy, iri);

      /* Time?  */
      tms = datetime_str (time (NULL));

      /* Get the new location (with or without the redirection).  */
      if (hstat.newloc)
        *newloc = xstrdup (hstat.newloc);

      switch (err)
        {
        case HERR: case HEOF: case CONSOCKERR: case CONCLOSED:
        case CONERROR: case READERR: case WRITEFAILED:
        case RANGEERR: case FOPEN_EXCL_ERR:
          /* Non-fatal errors continue executing the loop, which will
             bring them to "while" statement at the end, to judge
             whether the number of tries was exceeded.  */
          printwhat (count, opt.ntry);
          continue;
        case FWRITEERR: case FOPENERR:
          /* Another fatal error.  */
          logputs (LOG_VERBOSE, "\n");
          logprintf (LOG_NOTQUIET, _("Cannot write to %s (%s).\n"),
                     quote (hstat.local_file), strerror (errno));
        case HOSTERR: case CONIMPOSSIBLE: case PROXERR: case AUTHFAILED:
        case SSLINITFAILED: case CONTNOTSUPPORTED: case VERIFCERTERR:
          /* Fatal errors just return from the function.  */
          ret = err;
          goto exit;
        case CONSSLERR:
          /* Another fatal error.  */
          logprintf (LOG_NOTQUIET, _("Unable to establish SSL connection.\n"));
          ret = err;
          goto exit;
        case NEWLOCATION:
          /* Return the new location to the caller.  */
          if (!*newloc)
            {
              logprintf (LOG_NOTQUIET,
                         _("ERROR: Redirection (%d) without location.\n"),
                         hstat.statcode);
              ret = WRONGCODE;
            }
          else
            {
              ret = NEWLOCATION;
            }
          goto exit;
        case RETRUNNEEDED:
          /* The file was already fully retrieved. */
          ret = RETROK;
          goto exit;
        case RETRFINISHED:
          /* Deal with you later.  */
          break;
        default:
          /* All possibilities should have been exhausted.  */
          abort ();
        }

      if (!(*dt & RETROKF))
        {
          char *hurl = NULL;
          if (!opt.verbose)
            {
              /* #### Ugly ugly ugly! */
              hurl = url_string (u, URL_AUTH_HIDE_PASSWD);
              logprintf (LOG_NONVERBOSE, "%s:\n", hurl);
            }

          /* Fall back to GET if HEAD fails with a 500 or 501 error code. */
          if (*dt & HEAD_ONLY
              && (hstat.statcode == 500 || hstat.statcode == 501))
            {
              got_head = true;
              continue;
            }
          /* Maybe we should always keep track of broken links, not just in
           * spider mode.
           * Don't log error if it was UTF-8 encoded because we will try
           * once unencoded. */
          else if (opt.spider && !iri->utf8_encode)
            {
              /* #### Again: ugly ugly ugly! */
              if (!hurl)
                hurl = url_string (u, URL_AUTH_HIDE_PASSWD);
              nonexisting_url (hurl);
              logprintf (LOG_NOTQUIET, _("\
Remote file does not exist -- broken link!!!\n"));
            }
          else
            {
              logprintf (LOG_NOTQUIET, _("%s ERROR %d: %s.\n"),
                         tms, hstat.statcode,
                         quotearg_style (escape_quoting_style, hstat.error));
            }
          logputs (LOG_VERBOSE, "\n");
          ret = WRONGCODE;
          xfree_null (hurl);
          goto exit;
        }

      /* Did we get the time-stamp? */
      if (!got_head)
        {
          got_head = true;    /* no more time-stamping */

          if (opt.timestamping && !hstat.remote_time)
            {
              logputs (LOG_NOTQUIET, _("\
Last-modified header missing -- time-stamps turned off.\n"));
            }
          else if (hstat.remote_time)
            {
              /* Convert the date-string into struct tm.  */
              tmr = http_atotm (hstat.remote_time);
              if (tmr == (time_t) (-1))
                logputs (LOG_VERBOSE, _("\
Last-modified header invalid -- time-stamp ignored.\n"));
              if (*dt & HEAD_ONLY)
                time_came_from_head = true;
            }

          if (send_head_first)
            {
              /* The time-stamping section.  */
              if (opt.timestamping)
                {
                  if (hstat.orig_file_name) /* Perform the following
                                               checks only if the file
                                               we're supposed to
                                               download already exists.  */
                    {
                      if (hstat.remote_time &&
                          tmr != (time_t) (-1))
                        {
                          /* Now time-stamping can be used validly.
                             Time-stamping means that if the sizes of
                             the local and remote file match, and local
                             file is newer than the remote file, it will
                             not be retrieved.  Otherwise, the normal
                             download procedure is resumed.  */
                          if (hstat.orig_file_tstamp >= tmr)
                            {
                              if (hstat.contlen == -1
                                  || hstat.orig_file_size == hstat.contlen)
                                {
                                  logprintf (LOG_VERBOSE, _("\
Server file no newer than local file %s -- not retrieving.\n\n"),
                                             quote (hstat.orig_file_name));
                                  ret = RETROK;
                                  goto exit;
                                }
                              else
                                {
                                  logprintf (LOG_VERBOSE, _("\
The sizes do not match (local %s) -- retrieving.\n"),
                                             number_to_static_string (hstat.orig_file_size));
                                }
                            }
                          else
                            logputs (LOG_VERBOSE,
                                     _("Remote file is newer, retrieving.\n"));

                          logputs (LOG_VERBOSE, "\n");
                        }
                    }

                  /* free_hstat (&hstat); */
                  hstat.timestamp_checked = true;
                }

              if (opt.spider)
                {
                  bool finished = true;
                  if (opt.recursive)
                    {
                      if (*dt & TEXTHTML)
                        {
                          logputs (LOG_VERBOSE, _("\
Remote file exists and could contain links to other resources -- retrieving.\n\n"));
                          finished = false;
                        }
                      else
                        {
                          logprintf (LOG_VERBOSE, _("\
Remote file exists but does not contain any link -- not retrieving.\n\n"));
                          ret = RETROK; /* RETRUNNEEDED is not for caller. */
                        }
                    }
                  else
                    {
                      if (*dt & TEXTHTML)
                        {
                          logprintf (LOG_VERBOSE, _("\
Remote file exists and could contain further links,\n\
but recursion is disabled -- not retrieving.\n\n"));
                        }
                      else
                        {
                          logprintf (LOG_VERBOSE, _("\
Remote file exists.\n\n"));
                        }
                      ret = RETROK; /* RETRUNNEEDED is not for caller. */
                    }

                  if (finished)
                    {
                      logprintf (LOG_NONVERBOSE,
                                 _("%s URL: %s %2d %s\n"),
                                 tms, u->url, hstat.statcode,
                                 hstat.message ? quotearg_style (escape_quoting_style, hstat.message) : "");
                      goto exit;
                    }
                }

              got_name = true;
              *dt &= ~HEAD_ONLY;
              count = 0;          /* the retrieve count for HEAD is reset */
              continue;
            } /* send_head_first */
        } /* !got_head */

      if ((tmr != (time_t) (-1))
          && ((hstat.len == hstat.contlen) ||
              ((hstat.res == 0) && (hstat.contlen == -1))))
        {
          const char *fl = NULL;
          set_local_file (&fl, hstat.local_file);
          if (fl)
            {
              time_t newtmr = -1;
              /* Reparse time header, in case it's changed. */
              if (time_came_from_head
                  && hstat.remote_time && hstat.remote_time[0])
                {
                  newtmr = http_atotm (hstat.remote_time);
                  if (newtmr != (time_t)-1)
                    tmr = newtmr;
                }
              touch (fl, tmr);
            }
        }
      /* End of time-stamping section. */

      tmrate = retr_rate (hstat.rd_size, hstat.dltime);
      total_download_time += hstat.dltime;

      if (hstat.len == hstat.contlen)
        {
          if (*dt & RETROKF)
            {
              bool write_to_stdout = (opt.output_document && HYPHENP (opt.output_document));

              logprintf (LOG_VERBOSE,
                         write_to_stdout
                         ? _("%s (%s) - written to stdout %s[%s/%s]\n\n")
                         : _("%s (%s) - %s saved [%s/%s]\n\n"),
                         tms, tmrate,
                         write_to_stdout ? "" : quote (hstat.local_file),
                         number_to_static_string (hstat.len),
                         number_to_static_string (hstat.contlen));
              logprintf (LOG_NONVERBOSE,
                         "%s URL:%s [%s/%s] -> \"%s\" [%d]\n",
                         tms, u->url,
                         number_to_static_string (hstat.len),
                         number_to_static_string (hstat.contlen),
                         hstat.local_file, count);
            }
          ++numurls;
          total_downloaded_bytes += hstat.len;

          /* Remember that we downloaded the file for later ".orig" code. */
          if (*dt & ADDED_HTML_EXTENSION)
            downloaded_file(FILE_DOWNLOADED_AND_HTML_EXTENSION_ADDED, hstat.local_file);
          else
            downloaded_file(FILE_DOWNLOADED_NORMALLY, hstat.local_file);

          ret = RETROK;
          goto exit;
        }
      else if (hstat.res == 0) /* No read error */
        {
          if (hstat.contlen == -1)  /* We don't know how much we were supposed
                                       to get, so assume we succeeded. */
            {
              if (*dt & RETROKF)
                {
                  bool write_to_stdout = (opt.output_document && HYPHENP (opt.output_document));

                  logprintf (LOG_VERBOSE,
                             write_to_stdout
                             ? _("%s (%s) - written to stdout %s[%s]\n\n")
                             : _("%s (%s) - %s saved [%s]\n\n"),
                             tms, tmrate,
                             write_to_stdout ? "" : quote (hstat.local_file),
                             number_to_static_string (hstat.len));
                  logprintf (LOG_NONVERBOSE,
                             "%s URL:%s [%s] -> \"%s\" [%d]\n",
                             tms, u->url, number_to_static_string (hstat.len),
                             hstat.local_file, count);
                }
              ++numurls;
              total_downloaded_bytes += hstat.len;

              /* Remember that we downloaded the file for later ".orig" code. */
              if (*dt & ADDED_HTML_EXTENSION)
                downloaded_file(FILE_DOWNLOADED_AND_HTML_EXTENSION_ADDED, hstat.local_file);
              else
                downloaded_file(FILE_DOWNLOADED_NORMALLY, hstat.local_file);

              ret = RETROK;
              goto exit;
            }
          else if (hstat.len < hstat.contlen) /* meaning we lost the
                                                 connection too soon */
            {
              logprintf (LOG_VERBOSE,
                         _("%s (%s) - Connection closed at byte %s. "),
                         tms, tmrate, number_to_static_string (hstat.len));
              printwhat (count, opt.ntry);
              continue;
            }
          else if (hstat.len != hstat.restval)
            /* Getting here would mean reading more data than
               requested with content-length, which we never do.  */
            abort ();
          else
            {
              /* Getting here probably means that the content-length was
               * _less_ than the original, local size. We should probably
               * truncate or re-read, or something. FIXME */
              ret = RETROK;
              goto exit;
            }
        }
      else /* from now on hstat.res can only be -1 */
        {
          if (hstat.contlen == -1)
            {
              logprintf (LOG_VERBOSE,
                         _("%s (%s) - Read error at byte %s (%s)."),
                         tms, tmrate, number_to_static_string (hstat.len),
                         hstat.rderrmsg);
              printwhat (count, opt.ntry);
              continue;
            }
          else /* hstat.res == -1 and contlen is given */
            {
              logprintf (LOG_VERBOSE,
                         _("%s (%s) - Read error at byte %s/%s (%s). "),
                         tms, tmrate,
                         number_to_static_string (hstat.len),
                         number_to_static_string (hstat.contlen),
                         hstat.rderrmsg);
              printwhat (count, opt.ntry);
              continue;
            }
        }
      /* not reached */
    }
  while (!opt.ntry || (count < opt.ntry));

exit:
  if (ret == RETROK)
    *local_file = xstrdup (hstat.local_file);
  free_hstat (&hstat);

  return ret;
}

/* Check whether the result of strptime() indicates success.
   strptime() returns the pointer to how far it got to in the string.
   The processing has been successful if the string is at `GMT' or
   `+X', or at the end of the string.

   In extended regexp parlance, the function returns 1 if P matches
   "^ *(GMT|[+-][0-9]|$)", 0 otherwise.  P being NULL (which strptime
   can return) is considered a failure and 0 is returned.  */
static bool
check_end (const char *p)
{
  if (!p)
    return false;
  while (c_isspace (*p))
    ++p;
  if (!*p
      || (p[0] == 'G' && p[1] == 'M' && p[2] == 'T')
      || ((p[0] == '+' || p[0] == '-') && c_isdigit (p[1])))
    return true;
  else
    return false;
}

/* Convert the textual specification of time in TIME_STRING to the
   number of seconds since the Epoch.

   TIME_STRING can be in any of the three formats RFC2616 allows the
   HTTP servers to emit -- RFC1123-date, RFC850-date or asctime-date,
   as well as the time format used in the Set-Cookie header.
   Timezones are ignored, and should be GMT.

   Return the computed time_t representation, or -1 if the conversion
   fails.

   This function uses strptime with various string formats for parsing
   TIME_STRING.  This results in a parser that is not as lenient in
   interpreting TIME_STRING as I would like it to be.  Being based on
   strptime, it always allows shortened months, one-digit days, etc.,
   but due to the multitude of formats in which time can be
   represented, an ideal HTTP time parser would be even more
   forgiving.  It should completely ignore things like week days and
   concentrate only on the various forms of representing years,
   months, days, hours, minutes, and seconds.  For example, it would
   be nice if it accepted ISO 8601 out of the box.

   I've investigated free and PD code for this purpose, but none was
   usable.  getdate was big and unwieldy, and had potential copyright
   issues, or so I was informed.  Dr. Marcus Hennecke's atotm(),
   distributed with phttpd, is excellent, but we cannot use it because
   it is not assigned to the FSF.  So I stuck it with strptime.  */

time_t
http_atotm (const char *time_string)
{
  /* NOTE: Solaris strptime man page claims that %n and %t match white
     space, but that's not universally available.  Instead, we simply
     use ` ' to mean "skip all WS", which works under all strptime
     implementations I've tested.  */

  static const char *time_formats[] = {
    "%a, %d %b %Y %T",          /* rfc1123: Thu, 29 Jan 1998 22:12:57 */
    "%A, %d-%b-%y %T",          /* rfc850:  Thursday, 29-Jan-98 22:12:57 */
    "%a %b %d %T %Y",           /* asctime: Thu Jan 29 22:12:57 1998 */
    "%a, %d-%b-%Y %T"           /* cookies: Thu, 29-Jan-1998 22:12:57
                                   (used in Set-Cookie, defined in the
                                   Netscape cookie specification.) */
  };
  const char *oldlocale;
  char savedlocale[256];
  size_t i;
  time_t ret = (time_t) -1;

  /* Solaris strptime fails to recognize English month names in
     non-English locales, which we work around by temporarily setting
     locale to C before invoking strptime.  */
  oldlocale = setlocale (LC_TIME, NULL);
  if (oldlocale)
    {
      size_t l = strlen (oldlocale);
      if (l >= sizeof savedlocale)
        savedlocale[0] = '\0';
      else
        memcpy (savedlocale, oldlocale, l);
    }
  else savedlocale[0] = '\0';

  setlocale (LC_TIME, "C");

  for (i = 0; i < countof (time_formats); i++)
    {
      struct tm t;

      /* Some versions of strptime use the existing contents of struct
         tm to recalculate the date according to format.  Zero it out
         to prevent stack garbage from influencing strptime.  */
      xzero (t);

      if (check_end (strptime (time_string, time_formats[i], &t)))
        {
          ret = timegm (&t);
          break;
        }
    }

  /* Restore the previous locale. */
  if (savedlocale[0])
    setlocale (LC_TIME, savedlocale);

  return ret;
}

/* Authorization support: We support three authorization schemes:

   * `Basic' scheme, consisting of base64-ing USER:PASSWORD string;

   * `Digest' scheme, added by Junio Hamano <junio@twinsun.com>,
   consisting of answering to the server's challenge with the proper
   MD5 digests.

   * `NTLM' ("NT Lan Manager") scheme, based on code written by Daniel
   Stenberg for libcurl.  Like digest, NTLM is based on a
   challenge-response mechanism, but unlike digest, it is non-standard
   (authenticates TCP connections rather than requests), undocumented
   and Microsoft-specific.  */

/* Create the authentication header contents for the `Basic' scheme.
   This is done by encoding the string "USER:PASS" to base64 and
   prepending the string "Basic " in front of it.  */

static char *
basic_authentication_encode (const char *user, const char *passwd)
{
  char *t1, *t2;
  int len1 = strlen (user) + 1 + strlen (passwd);

  t1 = (char *)alloca (len1 + 1);
  sprintf (t1, "%s:%s", user, passwd);

  t2 = (char *)alloca (BASE64_LENGTH (len1) + 1);
  base64_encode (t1, len1, t2);

  return concat_strings ("Basic ", t2, (char *) 0);
}

#define SKIP_WS(x) do {                         \
  while (c_isspace (*(x)))                        \
    ++(x);                                      \
} while (0)

#ifdef ENABLE_DIGEST
/* Dump the hexadecimal representation of HASH to BUF.  HASH should be
   an array of 16 bytes containing the hash keys, and BUF should be a
   buffer of 33 writable characters (32 for hex digits plus one for
   zero termination).  */
static void
dump_hash (char *buf, const unsigned char *hash)
{
  int i;

  for (i = 0; i < MD5_HASHLEN; i++, hash++)
    {
      *buf++ = XNUM_TO_digit (*hash >> 4);
      *buf++ = XNUM_TO_digit (*hash & 0xf);
    }
  *buf = '\0';
}

/* Take the line apart to find the challenge, and compose a digest
   authorization header.  See RFC2069 section 2.1.2.  */
static char *
digest_authentication_encode (const char *au, const char *user,
                              const char *passwd, const char *method,
                              const char *path)
{
  static char *realm, *opaque, *nonce;
  static struct {
    const char *name;
    char **variable;
  } options[] = {
    { "realm", &realm },
    { "opaque", &opaque },
    { "nonce", &nonce }
  };
  char *res;
  param_token name, value;

  realm = opaque = nonce = NULL;

  au += 6;                      /* skip over `Digest' */
  while (extract_param (&au, &name, &value, ','))
    {
      size_t i;
      size_t namelen = name.e - name.b;
      for (i = 0; i < countof (options); i++)
        if (namelen == strlen (options[i].name)
            && 0 == strncmp (name.b, options[i].name,
                             namelen))
          {
            *options[i].variable = strdupdelim (value.b, value.e);
            break;
          }
    }
  if (!realm || !nonce || !user || !passwd || !path || !method)
    {
      xfree_null (realm);
      xfree_null (opaque);
      xfree_null (nonce);
      return NULL;
    }

  /* Calculate the digest value.  */
  {
    ALLOCA_MD5_CONTEXT (ctx);
    unsigned char hash[MD5_HASHLEN];
    char a1buf[MD5_HASHLEN * 2 + 1], a2buf[MD5_HASHLEN * 2 + 1];
    char response_digest[MD5_HASHLEN * 2 + 1];

    /* A1BUF = H(user ":" realm ":" password) */
    gen_md5_init (ctx);
    gen_md5_update ((unsigned char *)user, strlen (user), ctx);
    gen_md5_update ((unsigned char *)":", 1, ctx);
    gen_md5_update ((unsigned char *)realm, strlen (realm), ctx);
    gen_md5_update ((unsigned char *)":", 1, ctx);
    gen_md5_update ((unsigned char *)passwd, strlen (passwd), ctx);
    gen_md5_finish (ctx, hash);
    dump_hash (a1buf, hash);

    /* A2BUF = H(method ":" path) */
    gen_md5_init (ctx);
    gen_md5_update ((unsigned char *)method, strlen (method), ctx);
    gen_md5_update ((unsigned char *)":", 1, ctx);
    gen_md5_update ((unsigned char *)path, strlen (path), ctx);
    gen_md5_finish (ctx, hash);
    dump_hash (a2buf, hash);

    /* RESPONSE_DIGEST = H(A1BUF ":" nonce ":" A2BUF) */
    gen_md5_init (ctx);
    gen_md5_update ((unsigned char *)a1buf, MD5_HASHLEN * 2, ctx);
    gen_md5_update ((unsigned char *)":", 1, ctx);
    gen_md5_update ((unsigned char *)nonce, strlen (nonce), ctx);
    gen_md5_update ((unsigned char *)":", 1, ctx);
    gen_md5_update ((unsigned char *)a2buf, MD5_HASHLEN * 2, ctx);
    gen_md5_finish (ctx, hash);
    dump_hash (response_digest, hash);

    res = xmalloc (strlen (user)
                   + strlen (user)
                   + strlen (realm)
                   + strlen (nonce)
                   + strlen (path)
                   + 2 * MD5_HASHLEN /*strlen (response_digest)*/
                   + (opaque ? strlen (opaque) : 0)
                   + 128);
    sprintf (res, "Digest \
username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"",
             user, realm, nonce, path, response_digest);
    if (opaque)
      {
        char *p = res + strlen (res);
        strcat (p, ", opaque=\"");
        strcat (p, opaque);
        strcat (p, "\"");
      }
  }
  return res;
}
#endif /* ENABLE_DIGEST */

/* Computing the size of a string literal must take into account that
   value returned by sizeof includes the terminating \0.  */
#define STRSIZE(literal) (sizeof (literal) - 1)

/* Whether chars in [b, e) begin with the literal string provided as
   first argument and are followed by whitespace or terminating \0.
   The comparison is case-insensitive.  */
#define STARTS(literal, b, e)                           \
  ((e > b) \
   && ((size_t) ((e) - (b))) >= STRSIZE (literal)   \
   && 0 == strncasecmp (b, literal, STRSIZE (literal))  \
   && ((size_t) ((e) - (b)) == STRSIZE (literal)          \
       || c_isspace (b[STRSIZE (literal)])))

static bool
known_authentication_scheme_p (const char *hdrbeg, const char *hdrend)
{
  return STARTS ("Basic", hdrbeg, hdrend)
#ifdef ENABLE_DIGEST
    || STARTS ("Digest", hdrbeg, hdrend)
#endif
#ifdef ENABLE_NTLM
    || STARTS ("NTLM", hdrbeg, hdrend)
#endif
    ;
}

#undef STARTS

/* Create the HTTP authorization request header.  When the
   `WWW-Authenticate' response header is seen, according to the
   authorization scheme specified in that header (`Basic' and `Digest'
   are supported by the current implementation), produce an
   appropriate HTTP authorization request header.  */
static char *
create_authorization_line (const char *au, const char *user,
                           const char *passwd, const char *method,
                           const char *path, bool *finished)
{
  /* We are called only with known schemes, so we can dispatch on the
     first letter. */
  switch (c_toupper (*au))
    {
    case 'B':                   /* Basic */
      *finished = true;
      return basic_authentication_encode (user, passwd);
#ifdef ENABLE_DIGEST
    case 'D':                   /* Digest */
      *finished = true;
      return digest_authentication_encode (au, user, passwd, method, path);
#endif
#ifdef ENABLE_NTLM
    case 'N':                   /* NTLM */
      if (!ntlm_input (&pconn.ntlm, au))
        {
          *finished = true;
          return NULL;
        }
      return ntlm_output (&pconn.ntlm, user, passwd, finished);
#endif
    default:
      /* We shouldn't get here -- this function should be only called
         with values approved by known_authentication_scheme_p.  */
      abort ();
    }
}

static void
load_cookies (void)
{
  if (!wget_cookie_jar)
    wget_cookie_jar = cookie_jar_new ();
  if (opt.cookies_input && !cookies_loaded_p)
    {
      cookie_jar_load (wget_cookie_jar, opt.cookies_input);
      cookies_loaded_p = true;
    }
}

void
save_cookies (void)
{
  if (wget_cookie_jar)
    cookie_jar_save (wget_cookie_jar, opt.cookies_output);
}

void
http_cleanup (void)
{
  xfree_null (pconn.host);
  if (wget_cookie_jar)
    cookie_jar_delete (wget_cookie_jar);
}

void
ensure_extension (struct http_stat *hs, const char *ext, int *dt)
{
  char *last_period_in_local_filename = strrchr (hs->local_file, '.');
  char shortext[8];
  int len = strlen (ext);
  if (len == 5)
    {
      strncpy (shortext, ext, len - 1);
      shortext[len - 2] = '\0';
    }

  if (last_period_in_local_filename == NULL
      || !(0 == strcasecmp (last_period_in_local_filename, shortext)
           || 0 == strcasecmp (last_period_in_local_filename, ext)))
    {
      int local_filename_len = strlen (hs->local_file);
      /* Resize the local file, allowing for ".html" preceded by
         optional ".NUMBER".  */
      hs->local_file = xrealloc (hs->local_file,
                                 local_filename_len + 24 + len);
      strcpy (hs->local_file + local_filename_len, ext);
      /* If clobbering is not allowed and the file, as named,
         exists, tack on ".NUMBER.html" instead. */
      if (!ALLOW_CLOBBER && file_exists_p (hs->local_file))
        {
          int ext_num = 1;
          do
            sprintf (hs->local_file + local_filename_len,
                     ".%d%s", ext_num++, ext);
          while (file_exists_p (hs->local_file));
        }
      *dt |= ADDED_HTML_EXTENSION;
    }
}


#ifdef TESTING

const char *
test_parse_content_disposition()
{
  int i;
  struct {
    char *hdrval;
    char *opt_dir_prefix;
    char *filename;
    bool result;
  } test_array[] = {
    { "filename=\"file.ext\"", NULL, "file.ext", true },
    { "filename=\"file.ext\"", "somedir", "somedir/file.ext", true },
    { "attachment; filename=\"file.ext\"", NULL, "file.ext", true },
    { "attachment; filename=\"file.ext\"", "somedir", "somedir/file.ext", true },
    { "attachment; filename=\"file.ext\"; dummy", NULL, "file.ext", true },
    { "attachment; filename=\"file.ext\"; dummy", "somedir", "somedir/file.ext", true },
    { "attachment", NULL, NULL, false },
    { "attachment", "somedir", NULL, false },
  };

  for (i = 0; i < sizeof(test_array)/sizeof(test_array[0]); ++i)
    {
      char *filename;
      bool res;

      opt.dir_prefix = test_array[i].opt_dir_prefix;
      res = parse_content_disposition (test_array[i].hdrval, &filename);

      mu_assert ("test_parse_content_disposition: wrong result",
                 res == test_array[i].result
                 && (res == false
                     || 0 == strcmp (test_array[i].filename, filename)));
    }

  return NULL;
}

#endif /* TESTING */

/*
 * vim: et sts=2 sw=2 cino+={s
 */

