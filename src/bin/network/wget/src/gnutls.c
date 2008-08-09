/* SSL support via GnuTLS library.
   Copyright (C) 2005, 2006, 2007, 2008 Free Software Foundation, Inc.

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

#include <assert.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <stdio.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "wget.h"
#include "utils.h"
#include "connect.h"
#include "url.h"
#include "ssl.h"

/* Note: some of the functions private to this file have names that
   begin with "wgnutls_" (e.g. wgnutls_read) so that they wouldn't be
   confused with actual gnutls functions -- such as the gnutls_read
   preprocessor macro.  */

static gnutls_certificate_credentials credentials;

bool
ssl_init ()
{
  gnutls_global_init ();
  gnutls_certificate_allocate_credentials (&credentials);
  if (opt.ca_cert)
    gnutls_certificate_set_x509_trust_file (credentials, opt.ca_cert,
                                            GNUTLS_X509_FMT_PEM);
  return true;
}

struct wgnutls_transport_context {
  gnutls_session session;       /* GnuTLS session handle */
  int last_error;               /* last error returned by read/write/... */

  /* Since GnuTLS doesn't support the equivalent to recv(...,
     MSG_PEEK) or SSL_peek(), we have to do it ourselves.  Peeked data
     is stored to PEEKBUF, and wgnutls_read checks that buffer before
     actually reading.  */
  char peekbuf[512];
  int peekstart, peeklen;
};

#ifndef MIN
# define MIN(i, j) ((i) <= (j) ? (i) : (j))
#endif

static int
wgnutls_read (int fd, char *buf, int bufsize, void *arg)
{
  int ret;
  struct wgnutls_transport_context *ctx = arg;

  if (ctx->peeklen)
    {
      /* If we have any peek data, simply return that. */
      int copysize = MIN (bufsize, ctx->peeklen);
      memcpy (buf, ctx->peekbuf + ctx->peekstart, copysize);
      ctx->peeklen -= copysize;
      if (ctx->peeklen != 0)
        ctx->peekstart += copysize;
      else
        ctx->peekstart = 0;
      return copysize;
    }

  do
    ret = gnutls_record_recv (ctx->session, buf, bufsize);
  while (ret == GNUTLS_E_INTERRUPTED);

  if (ret < 0)
    ctx->last_error = ret;
  return ret;
}

static int
wgnutls_write (int fd, char *buf, int bufsize, void *arg)
{
  int ret;
  struct wgnutls_transport_context *ctx = arg;
  do
    ret = gnutls_record_send (ctx->session, buf, bufsize);
  while (ret == GNUTLS_E_INTERRUPTED);
  if (ret < 0)
    ctx->last_error = ret;
  return ret;
}

static int
wgnutls_poll (int fd, double timeout, int wait_for, void *arg)
{
  return 1;
}

static int
wgnutls_peek (int fd, char *buf, int bufsize, void *arg)
{
  int ret;
  struct wgnutls_transport_context *ctx = arg;

  /* We don't support peeks following peeks: the reader must drain all
     peeked data before the next peek.  */
  assert (ctx->peeklen == 0);
  if (bufsize > sizeof ctx->peekbuf)
    bufsize = sizeof ctx->peekbuf;

  do
    ret = gnutls_record_recv (ctx->session, buf, bufsize);
  while (ret == GNUTLS_E_INTERRUPTED);

  if (ret >= 0)
    {
      memcpy (ctx->peekbuf, buf, ret);
      ctx->peeklen = ret;
    }
  return ret;
}

static const char *
wgnutls_errstr (int fd, void *arg)
{
  struct wgnutls_transport_context *ctx = arg;
  return gnutls_strerror (ctx->last_error);
}

static void
wgnutls_close (int fd, void *arg)
{
  struct wgnutls_transport_context *ctx = arg;
  /*gnutls_bye (ctx->session, GNUTLS_SHUT_RDWR);*/
  gnutls_deinit (ctx->session);
  xfree (ctx);
#ifndef WINDOWS
  close (fd);
#else
  closesocket (fd);
#endif
}

/* gnutls_transport is the singleton that describes the SSL transport
   methods provided by this file.  */

static struct transport_implementation wgnutls_transport = {
  wgnutls_read, wgnutls_write, wgnutls_poll,
  wgnutls_peek, wgnutls_errstr, wgnutls_close
};

bool
ssl_connect (int fd) 
{
  static const int cert_type_priority[] = {
    GNUTLS_CRT_X509, GNUTLS_CRT_OPENPGP, 0
  };
  struct wgnutls_transport_context *ctx;
  gnutls_session session;
  int err;
  gnutls_init (&session, GNUTLS_CLIENT);
  gnutls_set_default_priority (session);
  gnutls_certificate_type_set_priority (session, cert_type_priority);
  gnutls_credentials_set (session, GNUTLS_CRD_CERTIFICATE, credentials);
  gnutls_transport_set_ptr (session, (gnutls_transport_ptr) fd);
  err = gnutls_handshake (session);
  if (err < 0)
    {
      logprintf (LOG_NOTQUIET, "GnuTLS: %s\n", gnutls_strerror (err));
      gnutls_deinit (session);
      return false;
    }
  ctx = xnew0 (struct wgnutls_transport_context);
  ctx->session = session;
  fd_register_transport (fd, &wgnutls_transport, ctx);
  return true;
}

bool
ssl_check_certificate (int fd, const char *host)
{
  struct wgnutls_transport_context *ctx = fd_transport_context (fd);

  unsigned int status;
  int err;

  /* If the user has specified --no-check-cert, we still want to warn
     him about problems with the server's certificate.  */
  const char *severity = opt.check_cert ? _("ERROR") : _("WARNING");
  bool success = true;

  err = gnutls_certificate_verify_peers2 (ctx->session, &status);
  if (err < 0)
    {
      logprintf (LOG_NOTQUIET, _("%s: No certificate presented by %s.\n"),
                 severity, escnonprint (host));
      success = false;
      goto out;
    }

  if (status & GNUTLS_CERT_INVALID)
    {
      logprintf (LOG_NOTQUIET, _("%s: The certificate of `%s' is not trusted.\n"),
                 severity, escnonprint (host));
      success = false;
    }
  if (status & GNUTLS_CERT_SIGNER_NOT_FOUND)
    {
      logprintf (LOG_NOTQUIET, _("%s: The certificate of `%s' hasn't got a known issuer.\n"),
                 severity, escnonprint (host));
      success = false;
    }
  if (status & GNUTLS_CERT_REVOKED)
    {
      logprintf (LOG_NOTQUIET, _("%s: The certificate of `%s' has been revoked.\n"),
                 severity, escnonprint (host));
      success = false;
    }

  if (gnutls_certificate_type_get (ctx->session) == GNUTLS_CRT_X509)
    {
      time_t now = time (NULL);
      gnutls_x509_crt cert;
      const gnutls_datum *cert_list;
      unsigned int cert_list_size;

      if ((err = gnutls_x509_crt_init (&cert)) < 0)
        {
          logprintf (LOG_NOTQUIET, _("Error initializing X509 certificate: %s\n"),
                     gnutls_strerror (err));
          success = false;
          goto out;
        }

      cert_list = gnutls_certificate_get_peers (ctx->session, &cert_list_size);
      if (!cert_list)
        {
          logprintf (LOG_NOTQUIET, _("No certificate found\n"));
          success = false;
          goto out;
        }
      err = gnutls_x509_crt_import (cert, cert_list, GNUTLS_X509_FMT_DER);
      if (err < 0)
        {
          logprintf (LOG_NOTQUIET, _("Error parsing certificate: %s\n"),
                     gnutls_strerror (err));
          success = false;
          goto out;
        }
      if (now < gnutls_x509_crt_get_activation_time (cert))
        {
          logprintf (LOG_NOTQUIET, _("The certificate has not yet been activated\n"));
          success = false;
        }
      if (now >= gnutls_x509_crt_get_expiration_time (cert))
        {
          logprintf (LOG_NOTQUIET, _("The certificate has expired\n"));
          success = false;
        }
      if (!gnutls_x509_crt_check_hostname (cert, host))
        {
          logprintf (LOG_NOTQUIET,
                     _("The certificate's owner does not match hostname '%s'\n"),
                     host);
          success = false;
        }
      gnutls_x509_crt_deinit (cert);
   }

 out:
  return opt.check_cert ? success : true;
}
