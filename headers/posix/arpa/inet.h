/*
 * ++Copyright++ 1983, 1993
 * -
 * Copyright (c) 1983, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the University of
 * 	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 * -
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
 * -
 * --Copyright--
 */

/*
 *	@(#)inet.h	8.1 (Berkeley) 6/2/93
 *	$Id: inet.h,v 1.6 2004/05/13 00:56:00 phoudoin Exp $
 */

#ifndef _INET_H_
#define	_INET_H_

#include <sys/types.h>
#include <netinet/in.h>

/* External definitions for functions in inet(3) */

/* R5 libnet.so does not have "__" prefix. */
#ifndef BUILDING_R5_LIBNET
#define	inet_addr		__inet_addr
#define	inet_aton		__inet_aton
#define	inet_lnaof		__inet_lnaof
#define	inet_makeaddr		__inet_makeaddr
#define	inet_neta		__inet_neta
#define	inet_netof		__inet_netof
#define	inet_network		__inet_network
#define	inet_net_ntop		__inet_net_ntop
#define	inet_net_pton		__inet_net_pton
#define	inet_cidr_ntop		__inet_cidr_ntop
#define	inet_cidr_pton		__inet_cidr_pton
#define	inet_ntoa		__inet_ntoa
#define	inet_pton		__inet_pton
#define	inet_ntop		__inet_ntop
#define	inet_nsap_addr		__inet_nsap_addr
#define	inet_nsap_ntoa		__inet_nsap_ntoa
#endif

#ifdef __cplusplus
extern "C" {
#endif

unsigned long	 inet_addr(const char *);
int		 inet_aton(const char *, struct in_addr *);
unsigned long	 inet_lnaof(struct in_addr);
struct in_addr	 inet_makeaddr(u_long , u_long);
char *		 inet_neta(u_long, char *, size_t);
unsigned long	 inet_netof(struct in_addr);
unsigned long	 inet_network(const char *);
char		*inet_net_ntop(int, const void *, int, char *, size_t);
int		 inet_net_pton(int, const char *, void *, size_t);
char		*inet_cidr_ntop(int, const void *, int, char *, size_t);
int		 inet_cidr_pton(int, const char *, void *, int *);
/*const*/ char	*inet_ntoa(struct in_addr);
int		 inet_pton(int, const char *, void *);
const char	*inet_ntop(int, const void *, char *, size_t);
u_int		 inet_nsap_addr(const char *, u_char *, int);
char		*inet_nsap_ntoa(int, const u_char *, char *);

#ifdef __cplusplus
}
#endif


#endif /* !_INET_H_ */
