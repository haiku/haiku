/*
 * Copyright (C) 2009  Internet Systems Consortium, Inc. ("ISC")
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

#ifndef lint
static const char rcsid[] = "$Id: ns_rdata.c,v 1.2 2009/01/23 23:49:15 tbox Exp $";
#endif

#include "port_before.h"

#if __OpenBSD__
#include <sys/types.h>
#endif
#include <netinet/in.h>
#include <arpa/nameser.h>

#include <errno.h>
#include <resolv.h>
#include <string.h>

#include "port_after.h"

#define CONSUME_SRC \
	do { \
		rdata += n, rdlen -= n; \
	} while (0)

#define CONSUME_DST \
	do { \
		nrdata += n, nrdsiz -= n, nrdlen += n; \
	} while (0)

#define UNPACK_DNAME \
	do { \
		size_t t; \
		\
		if ((n = ns_name_unpack2(msg,eom,rdata,nrdata,nrdsiz,&t))<0) {\
			errno = EMSGSIZE; \
			return (-1); \
		} \
		CONSUME_SRC; \
		n = t; \
		CONSUME_DST; \
	} while (0)

#define UNPACK_SOME(x) \
	do { \
		n = (x); \
		if ((size_t)n > rdlen || (size_t)n > nrdsiz) { \
			errno = EMSGSIZE; \
			return (-1); \
		} \
		memcpy(nrdata, rdata, n); \
		CONSUME_SRC; CONSUME_DST; \
	} while (0)

#define	UNPACK_REST(x) \
	do { \
		n = (x); \
		if ((size_t)n != rdlen) { \
			errno = EMSGSIZE; \
			return (-1); \
		} \
		memcpy(nrdata, rdata, n); \
		CONSUME_SRC; CONSUME_DST; \
	} while (0)

ssize_t
ns_rdata_unpack(const u_char *msg, const u_char *eom,
		ns_type type, const u_char *rdata, size_t rdlen,
		u_char *nrdata, size_t nrdsiz)
{
	size_t nrdlen = 0;
	int n;

	switch (type) {
	case ns_t_a:
		UNPACK_REST(NS_INADDRSZ);
		break;
	case ns_t_aaaa:
		UNPACK_REST(NS_IN6ADDRSZ);
		break;
	case ns_t_cname:
	case ns_t_mb:
	case ns_t_mg:
	case ns_t_mr:
	case ns_t_ns:
	case ns_t_ptr:
	case ns_t_dname:
		UNPACK_DNAME;
		break;
	case ns_t_soa:
		UNPACK_DNAME;
		UNPACK_DNAME;
		UNPACK_SOME(NS_INT32SZ * 5);
		break;
	case ns_t_mx:
	case ns_t_afsdb:
	case ns_t_rt:
		UNPACK_SOME(NS_INT16SZ);
		UNPACK_DNAME;
		break;
	case ns_t_px:
		UNPACK_SOME(NS_INT16SZ);
		UNPACK_DNAME;
		UNPACK_DNAME;
		break;
	case ns_t_srv:
		UNPACK_SOME(NS_INT16SZ * 3);
		UNPACK_DNAME;
		break;
	case ns_t_minfo:
	case ns_t_rp:
		UNPACK_DNAME;
		UNPACK_DNAME;
		break;
	default:
		UNPACK_SOME(rdlen);
		break;
	}
	if (rdlen > 0) {
		errno = EMSGSIZE;
		return (-1);
	}
	return (nrdlen);
}

#define	EQUAL_CONSUME \
	do { \
		rdata1 += n, rdlen1 -= n; \
		rdata2 += n, rdlen2 -= n; \
	} while (0)

#define	EQUAL_DNAME \
	do { \
		ssize_t n; \
		\
		if (rdlen1 != rdlen2) \
			return (0); \
		n = ns_name_eq(rdata1, rdlen1, rdata2, rdlen2); \
		if (n <= 0) \
			return (n); \
		n = rdlen1; \
		EQUAL_CONSUME; \
	} while (0)

#define	EQUAL_SOME(x) \
	do { \
		size_t n = (x); \
		\
		if (n > rdlen1 || n > rdlen2) { \
			errno = EMSGSIZE; \
			return (-1); \
		} \
		if (memcmp(rdata1, rdata2, n) != 0) \
			return (0); \
		EQUAL_CONSUME; \
	} while (0)

int
ns_rdata_equal(ns_type type,
	       const u_char *rdata1, size_t rdlen1,
	       const u_char *rdata2, size_t rdlen2)
{
	switch (type) {
	case ns_t_cname:
	case ns_t_mb:
	case ns_t_mg:
	case ns_t_mr:
	case ns_t_ns:
	case ns_t_ptr:
	case ns_t_dname:
		EQUAL_DNAME;
		break;
	case ns_t_soa:
		/* "There can be only one." --Highlander */
		break;
	case ns_t_mx:
	case ns_t_afsdb:
	case ns_t_rt:
		EQUAL_SOME(NS_INT16SZ);
		EQUAL_DNAME;
		break;
	case ns_t_px:
		EQUAL_SOME(NS_INT16SZ);
		EQUAL_DNAME;
		EQUAL_DNAME;
		break;
	case ns_t_srv:
		EQUAL_SOME(NS_INT16SZ * 3);
		EQUAL_DNAME;
		break;
	case ns_t_minfo:
	case ns_t_rp:
		EQUAL_DNAME;
		EQUAL_DNAME;
		break;
	default:
		EQUAL_SOME(rdlen1);
		break;
	}
	if (rdlen1 != 0 || rdlen2 != 0)
		return (0);
	return (1);
}

#define REFERS_DNAME \
	do { \
		int n; \
		\
		n = ns_name_eq(rdata, rdlen, nname, NS_MAXNNAME); \
		if (n < 0) \
			return (-1); \
		if (n > 0) \
			return (1); \
		n = dn_skipname(rdata, rdata + rdlen); \
		if (n < 0) \
			return (-1); \
		CONSUME_SRC; \
	} while (0)

#define	REFERS_SOME(x) \
	do { \
		size_t n = (x); \
		\
		if (n > rdlen) { \
			errno = EMSGSIZE; \
			return (-1); \
		} \
		CONSUME_SRC; \
	} while (0)

int
ns_rdata_refers(ns_type type,
		const u_char *rdata, size_t rdlen,
		const u_char *nname)
{
	switch (type) {
	case ns_t_cname:
	case ns_t_mb:
	case ns_t_mg:
	case ns_t_mr:
	case ns_t_ns:
	case ns_t_ptr:
	case ns_t_dname:
		REFERS_DNAME;
		break;
	case ns_t_soa:
		REFERS_DNAME;
		REFERS_DNAME;
		REFERS_SOME(NS_INT32SZ * 5);
		break;
	case ns_t_mx:
	case ns_t_afsdb:
	case ns_t_rt:
		REFERS_SOME(NS_INT16SZ);
		REFERS_DNAME;
		break;
	case ns_t_px:
		REFERS_SOME(NS_INT16SZ);
		REFERS_DNAME;
		REFERS_DNAME;
		break;
	case ns_t_srv:
		REFERS_SOME(NS_INT16SZ * 3);
		REFERS_DNAME;
		break;
	case ns_t_minfo:
	case ns_t_rp:
		REFERS_DNAME;
		REFERS_DNAME;
		break;
	default:
		REFERS_SOME(rdlen);
		break;
	}
	if (rdlen != 0) {
		errno = EMSGSIZE;
		return (-1);
	}
	return (0);
}
