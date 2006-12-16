/* Host name resolution and matching.
   Copyright (C) 1995, 1996, 1997, 2000, 2001 Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

In addition, as a special exception, the Free Software Foundation
gives permission to link the code of its release of Wget with the
OpenSSL project's "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.  */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif
#include <assert.h>
#include <sys/types.h>

#ifndef WINDOWS
# include <sys/socket.h>
# include <netinet/in.h>
# ifndef __BEOS__
#  include <arpa/inet.h>
# endif
# include <netdb.h>
# define SET_H_ERRNO(err) ((void)(h_errno = (err)))
#else  /* WINDOWS */
# define SET_H_ERRNO(err) WSASetLastError (err)
#endif /* WINDOWS */

#include <errno.h>

#include "wget.h"
#include "utils.h"
#include "host.h"
#include "url.h"
#include "hash.h"

#ifndef errno
extern int errno;
#endif

#ifndef h_errno
# ifndef __CYGWIN__
extern int h_errno;
# endif
#endif

#ifndef NO_ADDRESS
# define NO_ADDRESS NO_DATA
#endif

/* Lists of IP addresses that result from running DNS queries.  See
   lookup_host for details.  */

struct address_list {
  int count;			/* number of adrresses */
  ip_address *addresses;	/* pointer to the string of addresses */

  int faulty;			/* number of addresses known not to work. */
  int connected;		/* whether we were able to connect to
				   one of the addresses in the list,
				   at least once. */

  int refcount;			/* reference count; when it drops to
				   0, the entry is freed. */
};

/* Get the bounds of the address list.  */

void
address_list_get_bounds (const struct address_list *al, int *start, int *end)
{
  *start = al->faulty;
  *end   = al->count;
}

/* Return a pointer to the address at position POS.  */

const ip_address *
address_list_address_at (const struct address_list *al, int pos)
{
  assert (pos >= al->faulty && pos < al->count);
  return al->addresses + pos;
}

/* Return non-zero if AL contains IP, zero otherwise.  */

int
address_list_contains (const struct address_list *al, const ip_address *ip)
{
  int i;
  switch (ip->type)
    {
    case IPV4_ADDRESS:
      for (i = 0; i < al->count; i++)
	{
	  ip_address *cur = al->addresses + i;
	  if (cur->type == IPV4_ADDRESS
	      && (ADDRESS_IPV4_IN_ADDR (cur).s_addr
		  ==
		  ADDRESS_IPV4_IN_ADDR (ip).s_addr))
	    return 1;
	}
      return 0;
#ifdef ENABLE_IPV6
    case IPV6_ADDRESS:
      for (i = 0; i < al->count; i++)
	{
	  ip_address *cur = al->addresses + i;
	  if (cur->type == IPV6_ADDRESS
#ifdef HAVE_SOCKADDR_IN6_SCOPE_ID
	      && ADDRESS_IPV6_SCOPE (cur) == ADDRESS_IPV6_SCOPE (ip)
#endif
	      && IN6_ARE_ADDR_EQUAL (&ADDRESS_IPV6_IN6_ADDR (cur),
				     &ADDRESS_IPV6_IN6_ADDR (ip)))
	    return 1;
	}
      return 0;
#endif /* ENABLE_IPV6 */
    default:
      abort ();
    }
}

/* Mark the INDEXth element of AL as faulty, so that the next time
   this address list is used, the faulty element will be skipped.  */

void
address_list_set_faulty (struct address_list *al, int index)
{
  /* We assume that the address list is traversed in order, so that a
     "faulty" attempt is always preceded with all-faulty addresses,
     and this is how Wget uses it.  */
  assert (index == al->faulty);

  ++al->faulty;
  if (al->faulty >= al->count)
    /* All addresses have been proven faulty.  Since there's not much
       sense in returning the user an empty address list the next
       time, we'll rather make them all clean, so that they can be
       retried anew.  */
    al->faulty = 0;
}

/* Set the "connected" flag to true.  This flag used by connect.c to
   see if the host perhaps needs to be resolved again.  */

void
address_list_set_connected (struct address_list *al)
{
  al->connected = 1;
}

/* Return the value of the "connected" flag. */

int
address_list_connected_p (const struct address_list *al)
{
  return al->connected;
}

#ifdef ENABLE_IPV6

/* Create an address_list from the addresses in the given struct
   addrinfo.  */

static struct address_list *
address_list_from_addrinfo (const struct addrinfo *ai)
{
  struct address_list *al;
  const struct addrinfo *ptr;
  int cnt;
  ip_address *ip;

  cnt = 0;
  for (ptr = ai; ptr != NULL ; ptr = ptr->ai_next)
    if (ptr->ai_family == AF_INET || ptr->ai_family == AF_INET6)
      ++cnt;
  if (cnt == 0)
    return NULL;

  al = xnew0 (struct address_list);
  al->addresses = xnew_array (ip_address, cnt);
  al->count     = cnt;
  al->refcount  = 1;

  ip = al->addresses;
  for (ptr = ai; ptr != NULL; ptr = ptr->ai_next)
    if (ptr->ai_family == AF_INET6) 
      {
	const struct sockaddr_in6 *sin6 =
	  (const struct sockaddr_in6 *)ptr->ai_addr;
	ip->type = IPV6_ADDRESS;
	ADDRESS_IPV6_IN6_ADDR (ip) = sin6->sin6_addr;
#ifdef HAVE_SOCKADDR_IN6_SCOPE_ID
	ADDRESS_IPV6_SCOPE (ip) = sin6->sin6_scope_id;
#endif
	++ip;
      } 
    else if (ptr->ai_family == AF_INET)
      {
	const struct sockaddr_in *sin =
	  (const struct sockaddr_in *)ptr->ai_addr;
	ip->type = IPV4_ADDRESS;
	ADDRESS_IPV4_IN_ADDR (ip) = sin->sin_addr;
	++ip;
      }
  assert (ip - al->addresses == cnt);
  return al;
}

#define IS_IPV4(addr) (((const ip_address *) addr)->type == IPV4_ADDRESS)

/* Compare two IP addresses by type, giving preference to the IPv4
   address (sorting it first).  In other words, return -1 if ADDR1 is
   IPv4 and ADDR2 is IPv6, +1 if ADDR1 is IPv6 and ADDR2 is IPv4, and
   0 otherwise.

   This is intended to be used as the comparator arg to a qsort-like
   sorting function, which is why it accepts generic pointers.  */

static int
cmp_prefer_ipv4 (const void *addr1, const void *addr2)
{
  return !IS_IPV4 (addr1) - !IS_IPV4 (addr2);
}

#define IS_IPV6(addr) (((const ip_address *) addr)->type == IPV6_ADDRESS)

/* Like the above, but give preference to the IPv6 address.  */

static int
cmp_prefer_ipv6 (const void *addr1, const void *addr2)
{
  return !IS_IPV6 (addr1) - !IS_IPV6 (addr2);
}

#else  /* not ENABLE_IPV6 */

/* Create an address_list from a NULL-terminated vector of IPv4
   addresses.  This kind of vector is returned by gethostbyname.  */

static struct address_list *
address_list_from_ipv4_addresses (char **vec)
{
  int count, i;
  struct address_list *al = xnew0 (struct address_list);

  count = 0;
  while (vec[count])
    ++count;
  assert (count > 0);

  al->addresses = xnew_array (ip_address, count);
  al->count     = count;
  al->refcount  = 1;

  for (i = 0; i < count; i++)
    {
      ip_address *ip = &al->addresses[i];
      ip->type = IPV4_ADDRESS;
      memcpy (ADDRESS_IPV4_DATA (ip), vec[i], 4);
    }

  return al;
}

#endif /* not ENABLE_IPV6 */

static void
address_list_delete (struct address_list *al)
{
  xfree (al->addresses);
  xfree (al);
}

/* Mark the address list as being no longer in use.  This will reduce
   its reference count which will cause the list to be freed when the
   count reaches 0.  */

void
address_list_release (struct address_list *al)
{
  --al->refcount;
  DEBUGP (("Releasing 0x%0*lx (new refcount %d).\n", PTR_FORMAT (al),
	   al->refcount));
  if (al->refcount <= 0)
    {
      DEBUGP (("Deleting unused 0x%0*lx.\n", PTR_FORMAT (al)));
      address_list_delete (al);
    }
}

/* Versions of gethostbyname and getaddrinfo that support timeout. */

#ifndef ENABLE_IPV6

struct ghbnwt_context {
  const char *host_name;
  struct hostent *hptr;
};

static void
gethostbyname_with_timeout_callback (void *arg)
{
  struct ghbnwt_context *ctx = (struct ghbnwt_context *)arg;
  ctx->hptr = gethostbyname (ctx->host_name);
}

/* Just like gethostbyname, except it times out after TIMEOUT seconds.
   In case of timeout, NULL is returned and errno is set to ETIMEDOUT.
   The function makes sure that when NULL is returned for reasons
   other than timeout, errno is reset.  */

static struct hostent *
gethostbyname_with_timeout (const char *host_name, double timeout)
{
  struct ghbnwt_context ctx;
  ctx.host_name = host_name;
  if (run_with_timeout (timeout, gethostbyname_with_timeout_callback, &ctx))
    {
      SET_H_ERRNO (HOST_NOT_FOUND);
      errno = ETIMEDOUT;
      return NULL;
    }
  if (!ctx.hptr)
    errno = 0;
  return ctx.hptr;
}

/* Print error messages for host errors.  */
static char *
host_errstr (int error)
{
  /* Can't use switch since some of these constants can be equal,
     which makes the compiler complain about duplicate case
     values.  */
  if (error == HOST_NOT_FOUND
      || error == NO_RECOVERY
      || error == NO_DATA
      || error == NO_ADDRESS)
    return _("Unknown host");
  else if (error == TRY_AGAIN)
    /* Message modeled after what gai_strerror returns in similar
       circumstances.  */
    return _("Temporary failure in name resolution");
  else
    return _("Unknown error");
}

#else  /* ENABLE_IPV6 */

struct gaiwt_context {
  const char *node;
  const char *service;
  const struct addrinfo *hints;
  struct addrinfo **res;
  int exit_code;
};

static void
getaddrinfo_with_timeout_callback (void *arg)
{
  struct gaiwt_context *ctx = (struct gaiwt_context *)arg;
  ctx->exit_code = getaddrinfo (ctx->node, ctx->service, ctx->hints, ctx->res);
}

/* Just like getaddrinfo, except it times out after TIMEOUT seconds.
   In case of timeout, the EAI_SYSTEM error code is returned and errno
   is set to ETIMEDOUT.  */

static int
getaddrinfo_with_timeout (const char *node, const char *service,
			  const struct addrinfo *hints, struct addrinfo **res,
			  double timeout)
{
  struct gaiwt_context ctx;
  ctx.node = node;
  ctx.service = service;
  ctx.hints = hints;
  ctx.res = res;

  if (run_with_timeout (timeout, getaddrinfo_with_timeout_callback, &ctx))
    {
      errno = ETIMEDOUT;
      return EAI_SYSTEM;
    }
  return ctx.exit_code;
}

#endif /* ENABLE_IPV6 */

/* Pretty-print ADDR.  When compiled without IPv6, this is the same as
   inet_ntoa.  With IPv6, it either prints an IPv6 address or an IPv4
   address.  */

const char *
pretty_print_address (const ip_address *addr)
{
  switch (addr->type) 
    {
    case IPV4_ADDRESS:
      return inet_ntoa (ADDRESS_IPV4_IN_ADDR (addr));
#ifdef ENABLE_IPV6
    case IPV6_ADDRESS:
      {
        static char buf[128];
	inet_ntop (AF_INET6, &ADDRESS_IPV6_IN6_ADDR (addr), buf, sizeof (buf));
#if 0
#ifdef HAVE_SOCKADDR_IN6_SCOPE_ID
	{
	  /* append "%SCOPE_ID" for all ?non-global? addresses */
	  char *p = buf + strlen (buf);
	  *p++ = '%';
	  number_to_string (p, ADDRESS_IPV6_SCOPE (addr));
	}
#endif
#endif
        buf[sizeof (buf) - 1] = '\0';
        return buf;
      }
#endif
    }
  abort ();
}

/* The following two functions were adapted from glibc. */

static int
is_valid_ipv4_address (const char *str, const char *end)
{
  int saw_digit = 0;
  int octets = 0;
  int val = 0;

  while (str < end)
    {
      int ch = *str++;

      if (ch >= '0' && ch <= '9')
	{
	  val = val * 10 + (ch - '0');

	  if (val > 255)
	    return 0;
	  if (saw_digit == 0)
	    {
	      if (++octets > 4)
		return 0;
	      saw_digit = 1;
	    }
	}
      else if (ch == '.' && saw_digit == 1)
	{
	  if (octets == 4)
	    return 0;
	  val = 0;
	  saw_digit = 0;
	}
      else
	return 0;
    }
  if (octets < 4)
    return 0;
  
  return 1;
}

int
is_valid_ipv6_address (const char *str, const char *end)
{
  /* Use lower-case for these to avoid clash with system headers.  */
  enum {
    ns_inaddrsz  = 4,
    ns_in6addrsz = 16,
    ns_int16sz   = 2
  };

  const char *curtok;
  int tp;
  const char *colonp;
  int saw_xdigit;
  unsigned int val;

  tp = 0;
  colonp = NULL;

  if (str == end)
    return 0;
  
  /* Leading :: requires some special handling. */
  if (*str == ':')
    {
      ++str;
      if (str == end || *str != ':')
	return 0;
    }

  curtok = str;
  saw_xdigit = 0;
  val = 0;

  while (str < end)
    {
      int ch = *str++;

      /* if ch is a number, add it to val. */
      if (ISXDIGIT (ch))
	{
	  val <<= 4;
	  val |= XDIGIT_TO_NUM (ch);
	  if (val > 0xffff)
	    return 0;
	  saw_xdigit = 1;
	  continue;
	}

      /* if ch is a colon ... */
      if (ch == ':')
	{
	  curtok = str;
	  if (saw_xdigit == 0)
	    {
	      if (colonp != NULL)
		return 0;
	      colonp = str + tp;
	      continue;
	    }
	  else if (str == end)
	    return 0;
	  if (tp > ns_in6addrsz - ns_int16sz)
	    return 0;
	  tp += ns_int16sz;
	  saw_xdigit = 0;
	  val = 0;
	  continue;
	}

      /* if ch is a dot ... */
      if (ch == '.' && (tp <= ns_in6addrsz - ns_inaddrsz)
	  && is_valid_ipv4_address (curtok, end) == 1)
	{
	  tp += ns_inaddrsz;
	  saw_xdigit = 0;
	  break;
	}
    
      return 0;
    }

  if (saw_xdigit == 1)
    {
      if (tp > ns_in6addrsz - ns_int16sz) 
	return 0;
      tp += ns_int16sz;
    }

  if (colonp != NULL)
    {
      if (tp == ns_in6addrsz) 
	return 0;
      tp = ns_in6addrsz;
    }

  if (tp != ns_in6addrsz)
    return 0;

  return 1;
}

/* Simple host cache, used by lookup_host to speed up resolving.  The
   cache doesn't handle TTL because Wget is a fairly short-lived
   application.  Refreshing is attempted when connect fails, though --
   see connect_to_host.  */

/* Mapping between known hosts and to lists of their addresses. */
static struct hash_table *host_name_addresses_map;


/* Return the host's resolved addresses from the cache, if
   available.  */

static struct address_list *
cache_query (const char *host)
{
  struct address_list *al;
  if (!host_name_addresses_map)
    return NULL;
  al = hash_table_get (host_name_addresses_map, host);
  if (al)
    {
      DEBUGP (("Found %s in host_name_addresses_map (%p)\n", host, al));
      ++al->refcount;
      return al;
    }
  return NULL;
}

/* Cache the DNS lookup of HOST.  Subsequent invocations of
   lookup_host will return the cached value.  */

static void
cache_store (const char *host, struct address_list *al)
{
  if (!host_name_addresses_map)
    host_name_addresses_map = make_nocase_string_hash_table (0);

  ++al->refcount;
  hash_table_put (host_name_addresses_map, xstrdup_lower (host), al);

#ifdef ENABLE_DEBUG
  if (opt.debug)
    {
      int i;
      debug_logprintf ("Caching %s =>", host);
      for (i = 0; i < al->count; i++)
	debug_logprintf (" %s", pretty_print_address (al->addresses + i));
      debug_logprintf ("\n");
    }
#endif
}

/* Remove HOST from the DNS cache.  Does nothing is HOST is not in
   the cache.  */

static void
cache_remove (const char *host)
{
  struct address_list *al;
  if (!host_name_addresses_map)
    return;
  al = hash_table_get (host_name_addresses_map, host);
  if (al)
    {
      address_list_release (al);
      hash_table_remove (host_name_addresses_map, host);
    }
}

/* Look up HOST in DNS and return a list of IP addresses.

   This function caches its result so that, if the same host is passed
   the second time, the addresses are returned without DNS lookup.
   (Use LH_REFRESH to force lookup, or set opt.dns_cache to 0 to
   globally disable caching.)

   The order of the returned addresses is affected by the setting of
   opt.prefer_family: if it is set to prefer_ipv4, IPv4 addresses are
   placed at the beginning; if it is prefer_ipv6, IPv6 ones are placed
   at the beginning; otherwise, the order is left intact.  The
   relative order of addresses with the same family is left
   undisturbed in either case.

   FLAGS can be a combination of:
     LH_SILENT  - don't print the "resolving ... done" messages.
     LH_BIND    - resolve addresses for use with bind, which under
                  IPv6 means to use AI_PASSIVE flag to getaddrinfo.
		  Passive lookups are not cached under IPv6.
     LH_REFRESH - if HOST is cached, remove the entry from the cache
                  and resolve it anew.  */

struct address_list *
lookup_host (const char *host, int flags)
{
  struct address_list *al;
  int silent = flags & LH_SILENT;
  int use_cache;
  int numeric_address = 0;
  double timeout = opt.dns_timeout;

#ifndef ENABLE_IPV6
  /* If we're not using getaddrinfo, first check if HOST specifies a
     numeric IPv4 address.  Some implementations of gethostbyname
     (e.g. the Ultrix one and possibly Winsock) don't accept
     dotted-decimal IPv4 addresses.  */
  {
    uint32_t addr_ipv4 = (uint32_t)inet_addr (host);
    if (addr_ipv4 != (uint32_t) -1)
      {
	/* No need to cache host->addr relation, just return the
	   address.  */
	char *vec[2];
	vec[0] = (char *)&addr_ipv4;
	vec[1] = NULL;
	return address_list_from_ipv4_addresses (vec);
      }
  }
#else  /* ENABLE_IPV6 */
  /* If we're using getaddrinfo, at least check whether the address is
     already numeric, in which case there is no need to print the
     "Resolving..." output.  (This comes at no additional cost since
     the is_valid_ipv*_address are already required for
     url_parse.)  */
  {
    const char *end = host + strlen (host);
    if (is_valid_ipv4_address (host, end) || is_valid_ipv6_address (host, end))
      numeric_address = 1;
  }
#endif

  /* Cache is normally on, but can be turned off with --no-dns-cache.
     Don't cache passive lookups under IPv6.  */
  use_cache = opt.dns_cache;
#ifdef ENABLE_IPV6
  if ((flags & LH_BIND) || numeric_address)
    use_cache = 0;
#endif

  /* Try to find the host in the cache so we don't need to talk to the
     resolver.  If LH_REFRESH is requested, remove HOST from the cache
     instead.  */
  if (use_cache)
    {
      if (!(flags & LH_REFRESH))
	{
	  al = cache_query (host);
	  if (al)
	    return al;
	}
      else
	cache_remove (host);
    }

  /* No luck with the cache; resolve HOST. */

  if (!silent && !numeric_address)
    logprintf (LOG_VERBOSE, _("Resolving %s... "), escnonprint (host));

#ifdef ENABLE_IPV6
  {
    int err;
    struct addrinfo hints, *res;

    xzero (hints);
    hints.ai_socktype = SOCK_STREAM;
    if (opt.ipv4_only)
      hints.ai_family = AF_INET;
    else if (opt.ipv6_only)
      hints.ai_family = AF_INET6;
    else
      /* We tried using AI_ADDRCONFIG, but removed it because: it
	 misinterprets IPv6 loopbacks, it is broken on AIX 5.1, and
	 it's unneeded since we sort the addresses anyway.  */
	hints.ai_family = AF_UNSPEC;

    if (flags & LH_BIND)
      hints.ai_flags |= AI_PASSIVE;

#ifdef AI_NUMERICHOST
    if (numeric_address)
      {
	/* Where available, the AI_NUMERICHOST hint can prevent costly
	   access to DNS servers.  */
	hints.ai_flags |= AI_NUMERICHOST;
	timeout = 0;		/* no timeout needed when "resolving"
				   numeric hosts -- avoid setting up
				   signal handlers and such. */
      }
#endif

    err = getaddrinfo_with_timeout (host, NULL, &hints, &res, timeout);
    if (err != 0 || res == NULL)
      {
	if (!silent)
	  logprintf (LOG_VERBOSE, _("failed: %s.\n"),
		     err != EAI_SYSTEM ? gai_strerror (err) : strerror (errno));
	return NULL;
      }
    al = address_list_from_addrinfo (res);
    freeaddrinfo (res);
    if (!al)
      {
	logprintf (LOG_VERBOSE,
		   _("failed: No IPv4/IPv6 addresses for host.\n"));
	return NULL;
      }

    /* Reorder addresses so that IPv4 ones (or IPv6 ones, as per
       --prefer-family) come first.  Sorting is stable so the order of
       the addresses with the same family is undisturbed.  */
    if (al->count > 1 && opt.prefer_family != prefer_none)
      stable_sort (al->addresses, al->count, sizeof (ip_address),
		   opt.prefer_family == prefer_ipv4
		   ? cmp_prefer_ipv4 : cmp_prefer_ipv6);
  }
#else  /* not ENABLE_IPV6 */
  {
    struct hostent *hptr = gethostbyname_with_timeout (host, timeout);
    if (!hptr)
      {
	if (!silent)
	  {
	    if (errno != ETIMEDOUT)
	      logprintf (LOG_VERBOSE, _("failed: %s.\n"),
			 host_errstr (h_errno));
	    else
	      logputs (LOG_VERBOSE, _("failed: timed out.\n"));
	  }
	return NULL;
      }
    /* Do older systems have h_addr_list?  */
    al = address_list_from_ipv4_addresses (hptr->h_addr_list);
  }
#endif /* not ENABLE_IPV6 */

  /* Print the addresses determined by DNS lookup, but no more than
     three.  */
  if (!silent && !numeric_address)
    {
      int i;
      int printmax = al->count <= 3 ? al->count : 3;
      for (i = 0; i < printmax; i++)
	{
	  logprintf (LOG_VERBOSE, "%s",
		     pretty_print_address (al->addresses + i));
	  if (i < printmax - 1)
	    logputs (LOG_VERBOSE, ", ");
	}
      if (printmax != al->count)
	logputs (LOG_VERBOSE, ", ...");
      logputs (LOG_VERBOSE, "\n");
    }

  /* Cache the lookup information. */
  if (use_cache)
    cache_store (host, al);

  return al;
}

/* Determine whether a URL is acceptable to be followed, according to
   a list of domains to accept.  */
int
accept_domain (struct url *u)
{
  assert (u->host != NULL);
  if (opt.domains)
    {
      if (!sufmatch ((const char **)opt.domains, u->host))
	return 0;
    }
  if (opt.exclude_domains)
    {
      if (sufmatch ((const char **)opt.exclude_domains, u->host))
	return 0;
    }
  return 1;
}

/* Check whether WHAT is matched in LIST, each element of LIST being a
   pattern to match WHAT against, using backward matching (see
   match_backwards() in utils.c).

   If an element of LIST matched, 1 is returned, 0 otherwise.  */
int
sufmatch (const char **list, const char *what)
{
  int i, j, k, lw;

  lw = strlen (what);
  for (i = 0; list[i]; i++)
    {
      for (j = strlen (list[i]), k = lw; j >= 0 && k >= 0; j--, k--)
	if (TOLOWER (list[i][j]) != TOLOWER (what[k]))
	  break;
      /* The domain must be first to reach to beginning.  */
      if (j == -1)
	return 1;
    }
  return 0;
}

static int
host_cleanup_mapper (void *key, void *value, void *arg_ignored)
{
  struct address_list *al;

  xfree (key);			/* host */

  al = (struct address_list *)value;
  assert (al->refcount == 1);
  address_list_delete (al);

  return 0;
}

void
host_cleanup (void)
{
  if (host_name_addresses_map)
    {
      hash_table_map (host_name_addresses_map, host_cleanup_mapper, NULL);
      hash_table_destroy (host_name_addresses_map);
      host_name_addresses_map = NULL;
    }
}
