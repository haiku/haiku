/* udp_var.h */

#ifndef UDP_VAR_H
#define UDP_VAR_H

#include "netinet/ip_var.h"

struct udpiphdr {
	struct ipovly ui_i;
	struct udphdr ui_u;
};
#define ui_next         ui_i.ih_next
#define ui_prev         ui_i.ih_prev
#define ui_x1           ui_i.ih_x1 /* NB _x1 (one) */
#define ui_pr           ui_i.ih_pr
#define ui_len          ui_i.ih_len
#define ui_src          ui_i.ih_src
#define ui_dst          ui_i.ih_dst
#define ui_sport        ui_u.uh_sport
#define ui_dport        ui_u.uh_dport
#define ui_ulen         ui_u.uh_ulen
#define ui_sum          ui_u.uh_sum  

struct  udpstat {
                                /* input statistics: */
        uint32  udps_ipackets;          /* total input packets */
        uint32  udps_hdrops;            /* packet shorter than header */
        uint32  udps_badsum;            /* checksum error */
        uint32  udps_nosum;             /* no checksum */
        uint32  udps_badlen;            /* data length larger than packet */
        uint32  udps_noport;            /* no socket on port */
        uint32  udps_noportbcast;       /* of above, arrived as broadcast */
        uint32  udps_nosec;             /* dropped for lack of ipsec */
        uint32  udps_fullsock;          /* not delivered, input socket full */
        uint32  udps_pcbhashmiss;       /* input packets missing pcb hash */
        uint32  udps_inhwcsum;          /* input hardware-csummed packets */
                                /* output statistics: */
        uint32  udps_opackets;          /* total output packets */
        uint32  udps_outhwcsum;         /* output hardware-csummed packets */
};

/*
 * Names for UDP sysctl objects
 */
#define UDPCTL_CHECKSUM           1 /* checksum UDP packets */
#define UDPCTL_BADDYNAMIC         2 /* return bad dynamic port bitmap */
#define UDPCTL_RECVSPACE          3 /* receive buffer space */
#define UDPCTL_SENDSPACE          4 /* send buffer space */
#define UDPCTL_MAXID              5  

#endif
