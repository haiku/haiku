/*
 * Copyright 2002-2020, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
 * Copyright (c) 1980, 1983, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
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

/*
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies, and that
 * the name of Digital Equipment Corporation not be used in advertising or
 * publicity pertaining to distribution of the document or software without
 * specific, written prior permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#ifndef _NETDB_H_
#define _NETDB_H_

#include <stdint.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _PATH_NETWORKS
#define	_PATH_NETWORKS	"/etc/networks"
#endif

extern int * __h_errno(void);
#define	h_errno (*__h_errno())

/*
 * Structures returned by network data base library.  All addresses are
 * supplied in host order, and returned in network order (suitable for
 * use in system calls).
 */
struct hostent {
	char		*h_name;			/* official name of host */
	char		**h_aliases;		/* alias list */
	int			h_addrtype;			/* host address type */
	int			h_length;			/* length of address */
	char		**h_addr_list;		/* list of addresses from name server */
#define	h_addr	h_addr_list[0]	/* address, for backward compatibility */
};

struct netent {
	char		*n_name;			/* official name of net */
	char		**n_aliases;		/* alias list */
	int			n_addrtype;			/* net address type */
	in_addr_t	n_net;				/* network # */
};

struct servent {
	char		*s_name;			/* official service name */
	char		**s_aliases;		/* alias list */
	int			s_port;				/* port # */
	char		*s_proto;			/* protocol to use */
};

struct protoent {
	char		*p_name;			/* official protocol name */
	char		**p_aliases;		/* alias list */
	int			p_proto;			/* protocol # */
};

struct addrinfo {
	int			ai_flags;			/* AI_PASSIVE, AI_CANONNAME */
	int			ai_family;			/* PF_xxx */
	int			ai_socktype;		/* SOCK_xxx */
	int			ai_protocol;		/* 0 or IPPROTO_xxx for IPv4 and IPv6 */
	socklen_t	ai_addrlen;			/* length of ai_addr */
	char		*ai_canonname;		/* canonical name for hostname */
	struct sockaddr	*ai_addr; 		/* binary address */
	struct addrinfo	*ai_next; 		/* next structure in linked list */
};

/*
 * Error return codes from gethostbyname() and gethostbyaddr()
 * (left in extern int h_errno).
 */
#define	NETDB_INTERNAL	-1		/* see errno */
#define	NETDB_SUCCESS	0		/* no problem */
#define	HOST_NOT_FOUND	1		/* Authoritative Answer Host not found */
#define	TRY_AGAIN		2
	/* Non-Authoritive Host not found, or SERVERFAIL */
#define	NO_RECOVERY		3
	/* Non recoverable errors, FORMERR, REFUSED, NOTIMP */
#define	NO_DATA			4
	/* Valid name, no data record of requested type */
#define	NO_ADDRESS		NO_DATA	/* no address, look for MX record */

/*
 * Error return codes from getaddrinfo()
 */
#define	EAI_ADDRFAMILY	1	/* address family for hostname not supported */
#define	EAI_AGAIN	 	2	/* temporary failure in name resolution */
#define	EAI_BADFLAGS	3	/* invalid value for ai_flags */
#define	EAI_FAIL		4	/* non-recoverable failure in name resolution */
#define	EAI_FAMILY		5	/* ai_family not supported */
#define	EAI_MEMORY		6	/* memory allocation failure */
#define	EAI_NODATA		7	/* no address associated with hostname */
#define	EAI_NONAME		8	/* hostname nor servname provided, or not known */
#define	EAI_SERVICE		9	/* servname not supported for ai_socktype */
#define	EAI_SOCKTYPE	10	/* ai_socktype not supported */
#define	EAI_SYSTEM		11	/* system error returned in errno */
#define EAI_BADHINTS	12
#define EAI_PROTOCOL	13
#define EAI_OVERFLOW	14
#define EAI_MAX			15

/*
 * Flag values for getaddrinfo()
 */
#define	AI_PASSIVE		0x00000001
#define	AI_CANONNAME	0x00000002
#define AI_NUMERICHOST	0x00000004
#define AI_NUMERICSERV	0x00000008
#define	AI_MASK			\
	(AI_PASSIVE | AI_CANONNAME | AI_NUMERICHOST | AI_NUMERICSERV \
		| AI_ADDRCONFIG)

/*
 * Flag values for getipnodebyname()
 */
#define AI_ALL			0x00000100
#define AI_V4MAPPED_CFG	0x00000200
#define AI_ADDRCONFIG	0x00000400
#define AI_V4MAPPED		0x00000800
#define AI_DEFAULT		(AI_V4MAPPED_CFG | AI_ADDRCONFIG)

/*
 * Constants for getnameinfo()
 */
#define	NI_MAXHOST		1025
#define	NI_MAXSERV		32

/*
 * Flag values for getnameinfo()
 */
#define	NI_NOFQDN		0x00000001
#define	NI_NUMERICHOST	0x00000002
#define	NI_NAMEREQD		0x00000004
#define	NI_NUMERICSERV	0x00000008
#define	NI_DGRAM		0x00000010
#define	NI_WITHSCOPEID	0x00000020
#define NI_NUMERICSCOPE	0x00000040

/*
 * Scope delimit character
 */
#define SCOPE_DELIMITER	'%'

void			endhostent(void);
void			endnetent(void);
void			endprotoent(void);
void			endservent(void);
void			freehostent(struct hostent *host);
struct hostent	*gethostbyaddr(const void *address, socklen_t length, int type);
struct hostent	*gethostbyname(const char *name);
struct hostent	*gethostbyname2(const char *name, int type);
struct hostent	*gethostent(void);
struct hostent	*getipnodebyaddr(const void *, size_t, int, int *);
struct hostent	*getipnodebyname(const char *, int, int, int *);
struct netent	*getnetbyaddr(uint32_t, int);
struct netent	*getnetbyname(const char *);
struct netent	*getnetent(void);
struct protoent	*getprotobyname(const char *);
struct protoent	*getprotobynumber(int);
struct protoent	*getprotoent(void);
struct servent	*getservbyname(const char *, const char *);
struct servent	*getservbyport(int, const char *);
struct servent	*getservent(void);
void			herror(const char *);
const char		*hstrerror(int);
void			sethostent(int);
void			setnetent(int);
void			setprotoent(int);
void			setservent(int);
int				getaddrinfo(const char *, const char *,
					const struct addrinfo *, struct addrinfo **);
int				getnameinfo(const struct sockaddr *, socklen_t, char *,
					socklen_t, char *, socklen_t, int);
struct addrinfo	*allocaddrinfo(socklen_t);
void			freeaddrinfo(struct addrinfo *);
const char		*gai_strerror(int);

struct hostent	*gethostent_r(struct hostent *, char *, int, int *);
void			sethostent_r(int);
void			endhostent_r(void);

struct netent	*getnetbyname_r(const char *, struct netent *,
					char *, int);
struct netent	*getnetbyaddr_r(long, int, struct netent *,
					char *, int);
struct netent	*getnetent_r(struct netent *, char *, int);
void			setnetent_r(int);
void			endnetent_r(void);

struct protoent_data;
struct protoent	*getprotobyname_r(const char *,
				struct protoent *, struct protoent_data *);
struct protoent	*getprotobynumber_r(int,
				struct protoent *, struct protoent_data *);
struct protoent	*getprotoent_r(struct protoent *, struct protoent_data *);
void			setprotoent_r(int, struct protoent_data *);
void			endprotoent_r(struct protoent_data *);

struct servent_data;
struct servent	*getservbyname_r(const char *name, const char *,
					struct servent *, struct servent_data *);
struct servent	*getservbyport_r(int port, const char *,
					struct servent *, struct servent_data *);
struct servent	*getservent_r(struct servent *, struct servent_data *);
void			setservent_r(int, struct servent_data *);
void			endservent_r(struct servent_data *);

#ifdef __cplusplus
}
#endif

#endif /* _NETDB_H_ */
