/*! This file includes only the portions of <resolv.h> not present in the public headers. */
/*
 * Portions Copyright (C) 2004, 2005, 2008, 2009  Internet Systems Consortium, Inc. ("ISC")
 * Portions Copyright (C) 1995-2003  Internet Software Consortium.
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

/*
 * Copyright (c) 1983, 1987, 1989
 *    The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _RESOLV_H_
#include_next <resolv.h>

#include <sys/cdefs.h>

#define	nsaddr	nsaddr_list[0]		/*%< for backward compatibility */

/*%
 * This used to be defined in res_query.c, now it's in herror.c.
 * [XXX no it's not.  It's in irs/irs_data.c]
 * It was
 * never extern'd by any *.h file before it was placed here.  For thread
 * aware programs, the last h_errno value set is stored in res->h_errno.
 *
 * XXX:	There doesn't seem to be a good reason for exposing RES_SET_H_ERRNO
 *	(and __h_errno_set) to the public via <resolv.h>.
 * XXX:	__h_errno_set is really part of IRS, not part of the resolver.
 *	If somebody wants to build and use a resolver that doesn't use IRS,
 *	what do they do?  Perhaps something like
 *		#ifdef WANT_IRS
 *		# define RES_SET_H_ERRNO(r,x) __h_errno_set(r,x)
 *		#else
 *		# define RES_SET_H_ERRNO(r,x) (h_errno = (r)->res_h_errno = (x))
 *		#endif
 */

#define RES_SET_H_ERRNO(r,x) __h_errno_set(r,x)
struct __res_state; /*%< forward */
__BEGIN_DECLS
void __h_errno_set(struct __res_state *, int);
__END_DECLS

typedef enum { res_goahead, res_nextns, res_modified, res_done, res_error }
	res_sendhookact;

typedef res_sendhookact (*res_send_qhook)(struct sockaddr * const *,
						  const u_char **, int *,
						  u_char *, int, int *);

typedef res_sendhookact (*res_send_rhook)(const struct sockaddr *,
						  const u_char *, int, u_char *,
						  int, int *);

union res_sockaddr_union {
	struct sockaddr_in	sin;
#ifdef IN6ADDR_ANY_INIT
	struct sockaddr_in6	sin6;
#endif
#ifdef ISC_ALIGN64
	int64_t			__align64;	/*%< 64bit alignment */
#else
	int32_t			__align32;	/*%< 32bit alignment */
#endif
	char			__space[128];   /* max size */
};

/*%
 * Resolver flags (used to be discrete per-module statics ints).
 */
#define	RES_F__UNUSED	0x00000008	/*%< (unused) */
#define	RES_F_LASTMASK	0x000000F0	/*%< ordinal server of last res_nsend */
#define	RES_F_LASTSHIFT	4		/*%< bit position of LASTMASK "flag" */
#define	RES_GETLAST(res) (((res)._flags & RES_F_LASTMASK) >> RES_F_LASTSHIFT)

/*%
 * Resolver options (keep these in synch with res_debug.c, please)
 */
#define RES_NSID		(1 << 18)   /*%< request name server ID */
/* KAME extensions: use higher bit to avoid conflict with ISC use */
#define RES_USE_DNAME	0x10000000	/*%< use DNAME */

/* Things involving an internal (static) resolver context. */
__BEGIN_DECLS
extern struct __res_state *__res_get_state(void);
extern void __res_put_state(struct __res_state *);

/*
 * Source and Binary compatibility; _res will not work properly
 * with multi-threaded programs.
 */
__END_DECLS

#ifndef __BIND_NOSTATIC
#define	res_opt			__res_opt
#define res_sendsigned		__res_sendsigned

__BEGIN_DECLS
int		res_opt(int, u_char *, int, int);
int		res_sendsigned(const u_char *, int, ns_tsig_key *,
					u_char *, int);
__END_DECLS
#endif

#if !defined(SHARED_LIBBIND) || defined(LIB)
/*
 * If libbind is a shared object (well, DLL anyway)
 * these externs break the linker when resolv.h is
 * included by a lib client (like named)
 * Make them go away if a client is including this
 *
 */
extern const struct res_sym __p_key_syms[];
extern const struct res_sym __p_cert_syms[];
extern const struct res_sym __p_class_syms[];
extern const struct res_sym __p_type_syms[];
extern const struct res_sym __p_rcode_syms[];
#endif /* SHARED_LIBBIND */


#define p_secstodate		__p_secstodate
#define p_section		__p_section
#define p_sockun		__p_sockun
#define putlong			__putlong
#define putshort		__putshort
#define	res_check		__res_check
#define res_findzonecut		__res_findzonecut
#define res_findzonecut2	__res_findzonecut2
#define res_pquery		__res_pquery
#define res_nsendsigned		__res_nsendsigned
#define res_nisourserver	__res_nisourserver
#define res_rndinit		__res_rndinit
#define res_nrandomid		__res_nrandomid
#define res_nopt		__res_nopt
#define res_nopt_rdata       	__res_nopt_rdata
#define res_ndestroy		__res_ndestroy
#define	res_nametoclass		__res_nametoclass
#define	res_nametotype		__res_nametotype
#define	res_setservers		__res_setservers
#define	res_getservers		__res_getservers
#define	res_buildprotolist	__res_buildprotolist
#define	res_destroyprotolist	__res_destroyprotolist
#define	res_destroyservicelist	__res_destroyservicelist
#define	res_get_nibblesuffix	__res_get_nibblesuffix
#define	res_get_nibblesuffix2	__res_get_nibblesuffix2
#define	res_ourserver_p		__res_ourserver_p
#define	res_send_setqhook	__res_send_setqhook
#define	res_send_setrhook	__res_send_setrhook
#define	res_servicename		__res_servicename
#define	res_servicenumber	__res_servicenumber

__BEGIN_DECLS
struct timespec;
int		res_check(res_state, struct timespec *);
void		putlong(uint32_t, u_char *);
void		putshort(uint16_t, u_char *);
#ifndef __ultrix__
uint16_t	_getshort(const u_char *);
uint32_t	_getlong(const u_char *);
#endif
const char *	p_sockun(union res_sockaddr_union, char *, size_t);
char *		p_secstodate(u_long);
void		res_rndinit(res_state);
u_int		res_nrandomid(res_state);
const char *	p_section(int, int);
/* Things involving a resolver context. */
void		res_pquery(const res_state, const u_char *, int, FILE *);
int		res_nisourserver(const res_state, const struct sockaddr_in *);
int		res_nsendsigned(res_state, const u_char *, int,
					 ns_tsig_key *, u_char *, int);
int		res_findzonecut(res_state, const char *, ns_class, int,
					 char *, size_t, struct in_addr *, int);
int		res_findzonecut2(res_state, const char *, ns_class, int,
					  char *, size_t,
					  union res_sockaddr_union *, int);
int		res_nopt(res_state, int, u_char *, int, int);
int		res_nopt_rdata(res_state, int, u_char *, int, u_char *,
					u_short, u_short, u_char *);
void		res_send_setqhook(res_send_qhook);
void		res_send_setrhook(res_send_rhook);
int		__res_vinit(res_state, int);
void		res_destroyservicelist(void);
const char *	res_servicename(uint16_t, const char *);
void		res_destroyprotolist(void);
void		res_buildprotolist(void);
const char *	res_get_nibblesuffix(res_state);
const char *	res_get_nibblesuffix2(res_state);
void		res_ndestroy(res_state);
uint16_t	res_nametoclass(const char *, int *);
uint16_t	res_nametotype(const char *, int *);
void		res_setservers(res_state,
					const union res_sockaddr_union *, int);
int		res_getservers(res_state,
					union res_sockaddr_union *, int);
__END_DECLS

#endif /* !_RESOLV_H_ */
