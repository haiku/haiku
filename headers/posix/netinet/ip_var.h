/* Parts of this file are covered under the following copyright */
/*
 * Copyright (c) 1982, 1986, 1993
 *      The Regents of the University of California.  All rights reserved.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
 *
 *      @(#)ip_var.h    8.1 (Berkeley) 6/10/93
 */

#ifndef NETINET_IP_VAR_H
#define NETINET_IP_VAR_H

#include "sys/socket.h"

/*
 * Overlay for ip header used by other protocols (tcp, udp).
 */
struct ipovly {
	caddr_t   ih_next;
	caddr_t   ih_prev;
	uint8     ih_x1;        /* (unused) */
	uint8     ih_pr;           /* protocol */
	uint16    ih_len;          /* protocol length */
	struct    in_addr ih_src;  /* source internet address */
	struct    in_addr ih_dst;  /* destination internet address */
}; 

/*
 * Structure stored in mbuf in inpcb.ip_options
 * and passed to ip_output when ip options are in use.
 * The actual length of the options (including ipopt_dst)
 * is in m_len.
 */
#define MAX_IPOPTLEN    40

struct ipoption {
	struct  in_addr ipopt_dst;         /* first-hop dst if source routed */
	int8    ipopt_list[MAX_IPOPTLEN];  /* options proper */
};  

/*
 * Structure attached to inpcb.ip_moptions and
 * passed to ip_output when IP multicast options are in use.
 */
struct ip_moptions {
	struct    ifnet *imo_multicast_ifp; /* ifp for outgoing multicasts */
	uint8  imo_multicast_ttl;           /* TTL for outgoing multicasts */
	uint8  imo_multicast_loop;          /* 1 => here sends if a member */
	uint16 imo_num_memberships;         /* no. memberships this socket */
	struct    in_multi *imo_membership[IP_MAX_MEMBERSHIPS];
};

struct ipasfrag {
#if B_HOST_IS_BENDIAN
	uint8  ip_v:4;
	uint8  ip_hl:4;
#else
	uint8  ip_hl:4;
	uint8  ip_v:4;
#endif
	uint8  ipf_mff;
	int16  ip_len;
	uint16 ip_id;
	int16  ip_off;
	uint8  ip_ttl;
	uint8  ip_p;
	struct ipasfrag *ipf_next;
	struct ipasfrag *ipf_prev;
};

struct ipq {
	struct ipq *next, *prev;
	uint8  ipq_ttl;
	uint8  ipq_p;
	uint16 ipq_id;
	struct ipasfrag *ipq_next, *ipq_prev;
	struct in_addr ipq_src, ipq_dst;
};

struct  ipstat {
        int32  ips_total;              /* total packets received */
        int32  ips_badsum;             /* checksum bad */
        int32  ips_tooshort;           /* packet too short */
        int32  ips_toosmall;           /* not enough data */
        int32  ips_badhlen;            /* ip header length < data size */
        int32  ips_badlen;             /* ip length < ip header length */
        int32  ips_fragments;          /* fragments received */
        int32  ips_fragdropped;        /* frags dropped (dups, out of space) */
        int32  ips_fragtimeout;        /* fragments timed out */
        int32  ips_forward;            /* packets forwarded */
        int32  ips_cantforward;        /* packets rcvd for unreachable dest */
        int32  ips_redirectsent;       /* packets forwarded on same net */
        int32  ips_noproto;            /* unknown or unsupported protocol */
        int32  ips_delivered;          /* datagrams delivered to upper level*/
        int32  ips_localout;           /* total ip packets generated here */
        int32  ips_odropped;           /* lost packets due to nobufs, etc. */
        int32  ips_reassembled;        /* total packets reassembled ok */
        int32  ips_fragmented;         /* datagrams sucessfully fragmented */
        int32  ips_ofragments;         /* output fragments created */
        int32  ips_cantfrag;           /* don't fragment flag was set, etc. */
        int32  ips_badoptions;         /* error in option processing */
        int32  ips_noroute;            /* packets discarded due to no route */
        int32  ips_badvers;            /* ip version != 4 */
        int32  ips_rawout;             /* total raw ip packets generated */
        int32  ips_badfrags;           /* malformed fragments (bad length) */
        int32  ips_rcvmemdrop;         /* frags dropped for lack of memory */
        int32  ips_toolong;            /* ip length > max ip packet size */
        int32  ips_nogif;              /* no match gif found */
        int32  ips_badaddr;            /* invalid address on header */
        int32  ips_inhwcsum;           /* hardware checksummed on input */
        int32  ips_outhwcsum;          /* hardware checksummed on output */
};

#ifdef _NETWORK_STACK

#define IP_FORWARDING           0x1             /* most of ip header exists */
#define IP_RAWOUTPUT            0x2             /* raw ip header exists */
#define IP_ROUTETOIF            SO_DONTROUTE    /* bypass routing tables */
#define IP_ALLOWBROADCAST       SO_BROADCAST    /* can send broadcast packets */
#define IP_MTUDISC              0x0400          /* pmtu discovery, set DF */

struct ipstat ipstat;

void    ip_stripoptions (struct mbuf *, struct mbuf *);

#endif /* _NETWORK_STACK */

#endif /* NETINET_IP_VAR_H */
