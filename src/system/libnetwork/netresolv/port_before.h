/*
 * Copyright (C) 2005-2008  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef port_before_h
#define port_before_h

#include <config.h>

#ifdef __HAIKU__
#	include <sys/sockio.h>
#	define ETOOMANYREFS EBADF

// inet_*() are just weak symbols
#	define	inet_addr		__inet_addr
#	define	inet_aton		__inet_aton
#	define	inet_lnaof		__inet_lnaof
#	define	inet_makeaddr	__inet_makeaddr
#	define	inet_neta		__inet_neta
#	define	inet_netof		__inet_netof
#	define	inet_network	__inet_network
#	define	inet_net_ntop	__inet_net_ntop
#	define	inet_net_pton	__inet_net_pton
#	define	inet_cidr_ntop	__inet_cidr_ntop
#	define	inet_cidr_pton	__inet_cidr_pton
#	define	inet_ntoa		__inet_ntoa
#	define	inet_pton		__inet_pton
#	define	inet_ntop		__inet_ntop
#	define	inet_nsap_addr	__inet_nsap_addr
#	define	inet_nsap_ntoa	__inet_nsap_ntoa

#define	__weak_alias(alias, sym) __asm(".weak " #alias "\n" #alias " = " #sym);

/* From nameser.h: Private data structure - do not use from outside library. */
struct _ns_flagdata {  int mask, shift;  };
extern struct _ns_flagdata _ns_flagdata[];
#endif

#define ISC_SOCKLEN_T socklen_t

#ifdef __GNUC__
#define ISC_FORMAT_PRINTF(fmt, args) \
	__attribute__((__format__(__printf__, fmt, args)))
#else
#define ISC_FORMAT_PRINTF(fmt, args)
#endif

#define DE_CONST(konst, var) \
	do { \
		union { const void *k; void *v; } _u; \
		_u.k = konst; \
		var = _u.v; \
	} while (0)

#define UNUSED(x) (x) = (x)

#endif
