/* netinet/ip.h
 * definitions for ipv4 protocol
 */
 
#ifndef NETINET_IP_H
#define NETINET_IP_H

#include <ByteOrder.h>

#include "sys/mbuf.h"
#include "net_misc.h"
#include "netinet/in_systm.h"
#include "netinet/in.h"

/* Based on RFC 791 */

#define IPVERSION 4

struct ip {
#if B_HOST_IS_BENDIAN
	uint8	ip_v:4;
	uint8	ip_hl:4;
#else
	uint8	ip_hl:4;
	uint8	ip_v:4;
#endif
	uint8	ip_tos;
	uint16	ip_len;
	uint16	ip_id;
	int16	ip_off;
	uint8	ip_ttl;
	uint8	ip_p;
	uint16	ip_sum;
	struct in_addr	ip_src;
	struct in_addr 	ip_dst;
} _PACKED;

#define IP_MAXPACKET   65535/* Maximum packet size */

/* IP Type of Service */
#define IPTOS_RELIABILITY   0x04
#define IPTOS_THROUGHPUT    0x08
#define IPTOS_LOWDELAY      0x10

/*
 * Definitions for options.
 */
#define IPOPT_COPIED(o)((o)&0x80)
#define IPOPT_CLASS(o)((o)&0x60)
#define IPOPT_NUMBER(o)((o)&0x1f)

#define IPOPT_CONTROL            0x00
#define IPOPT_RESERVED1          0x20
#define IPOPT_DEBMEAS            0x40
#define IPOPT_RESERVED2          0x60

#define IPOPT_EOL                   0/* end of option list */
#define IPOPT_NOP                   1/* no operation */

#define IPOPT_RR                    7/* record packet route */
#define IPOPT_TS                   68/* timestamp */
#define IPOPT_SECURITY            130/* provide s,c,h,tcc */
#define IPOPT_LSRR                131/* loose source route */
#define IPOPT_SATID               136/* satnet id */
#define IPOPT_SSRR                137/* strict source route */  

/*
 * Offsets to fields in options other than EOL and NOP.
 */
#define IPOPT_OPTVAL                0/* option ID */
#define IPOPT_OLEN                  1/* option length */
#define IPOPT_OFFSET                2/* offset within option */
#define IPOPT_MINOFF                4/* min value of above */   

struct  ip_timestamp {
	uint8 ipt_code;/* IPOPT_TS */
	uint8 ipt_len;/* size of structure (variable) */
	uint8 ipt_ptr;/* index of current entry */
#if B_HOST_IS_BENDIAN
	uint8 ipt_oflw:4,
	      ipt_flg:4;
#else
	uint8 ipt_flg:4,
	      ipt_oflw:4;
#endif
	union ipt_timestamp {
		n_time ipt_time[1];
		struct ipt_ta {
			struct in_addr ipt_addr;
			n_time ipt_time;
		} ipt_ta;
	} ipt_timestamp;
}; 

/* flag bits for ipt_flg */
#define IPOPT_TS_TSONLY      0/* timestamps only */
#define IPOPT_TS_TSANDADDR   1/* timestamps and addresses */
#define IPOPT_TS_PRESPEC     3/* specified modules only */

/* bits for security (not byte swapped) */
#define IPOPT_SECUR_UNCLASS     0x0000
#define IPOPT_SECUR_CONFID      0xf135
#define IPOPT_SECUR_EFTO        0x789a
#define IPOPT_SECUR_MMMM        0xbc4d
#define IPOPT_SECUR_RESTR       0xaf13
#define IPOPT_SECUR_SECRET      0xd788
#define IPOPT_SECUR_TOPSECRET   0x6bc5 

#define MAXTTL                  255/* maximum time to live (seconds) */
#define IPDEFTTL                64/* default ttl, from RFC 1340 */
#define IPFRAGTTL               60/* time to live for frags, slowhz */
#define IPTTLDEC                1/* subtracted when forwarding */

#define IP_MSS                  576/* default maximum segment size */  

struct ippseudo {
	struct    in_addr ippseudo_src; /* source internet address */
	struct    in_addr ippseudo_dst; /* destination internet address */
	uint8  ippseudo_pad;/* pad, must be zero */
	uint8  ippseudo_p;/* protocol */
	uint16 ippseudo_len;/* protocol length */
};  

/* Fragment flags */
#define IP_DF 0x4000        /* don't fragment */
#define IP_MF 0x2000        /* more fragments */
#define IP_OFFMASK 0x1fff

#endif /* NETINET_IP_H */
