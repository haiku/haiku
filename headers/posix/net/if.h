/* if.h
 * Interface definitions for beos
 */

#ifndef OBOS_IF_H
#define OBOS_IF_H

#include <kernel/OS.h>
#include <Drivers.h>
#include "sys/socketvar.h"
#include "net/if_types.h"
#include "netinet/in.h"
#include "net/route.h"
 
enum {
	IF_GETADDR = B_DEVICE_OP_CODES_END,
	IF_INIT,
	IF_NONBLOCK,
	IF_ADDMULTI,
	IF_REMMULTI,
	IF_SETPROMISC,
	IF_GETFRAMESIZE
};

/* Media types are now listed in net/if_types.h */

/* Interface flags */
enum {
	IFF_UP          = 0x0001,
	IFF_DOWN        = 0x0002,
	IFF_PROMISC     = 0x0004,
	IFF_RUNNING     = 0x0008,
	IFF_MULTICAST   = 0x0010,
	IFF_BROADCAST   = 0x0020,
	IFF_POINTOPOINT = 0x0040,
	IFF_NOARP       = 0x0080, 
	IFF_LOOPBACK	= 0x0100,
	IFF_DEBUG       = 0x0200,
	IFF_LINK0       = 0x0400,
	IFF_LINK1       = 0x0800,
	IFF_LINK2       = 0x1000,
	IFF_SIMPLEX     = 0x2000
};

struct ifq {
	struct mbuf *head;
	struct mbuf *tail;
	int maxlen;
	int len;
	sem_id lock;
	sem_id pop;
};

#define IFQ_FULL(ifq)   (ifq->len >= ifq->maxlen)

#define IFQ_ENQUEUE(ifq, m) { \
	acquire_sem((ifq)->lock); \
	(m)->m_nextpkt = 0; \
	if ((ifq)->tail == 0) \
		(ifq)->head = m; \
	else \
		(ifq)->tail->m_nextpkt = m; \
	(ifq)->tail = m; \
	(ifq)->len++; \
	release_sem((ifq)->lock); \
	release_sem((ifq)->pop); \
}

#define IFQ_DEQUEUE(ifq, m) { \
	acquire_sem((ifq)->lock); \
	(m) = (ifq)->head; \
	if (m) { \
		if (((ifq)->head = (m)->m_nextpkt) == 0) \
			(ifq)->tail = 0; \
		(m)->m_nextpkt = 0; \
		(ifq)->len--; \
	} \
	release_sem((ifq)->lock); \
}

struct ifaddr {
	struct ifaddr 	*ifa_next;      /* the next address for the interface */
	struct ifnet 	*ifa_ifp;           /* pointer to the interface structure */

	struct sockaddr *ifa_addr;	    /* the address - cast to be a suitable type, so we
		                             * use this structure to store any type of address that
		                             * we have a struct sockaddr_? for. e.g.
		                             * link level address via sockaddr_dl and
		                             * ipv4 via sockeddr_in
		                             * same for next 2 pointers as well
		                             */ 
	struct sockaddr	*ifa_dstaddr;	/* if we're on a point-to-point link this
					                 * is the other end */
	struct sockaddr	*ifa_netmask;	/* The netmask we're using */

	/* check or clean routes... */
	void (*ifa_rtrequest)(int, struct rtentry *, struct sockaddr *);
	uint8	ifa_flags;              /* flags (mainly routing */
	uint16	ifa_refcnt;             /* how many references are there to
	                                 * this structure? */
	int	ifa_metric;                 /* the metirc for this interface/address */
};
#define ifa_broadaddr	ifa_dstaddr

struct if_data {
	uint8	ifi_type;              /* type of media */
	uint8	ifi_addrlen;           /* length of media address length */
	uint8	ifi_hdrlen;            /* size of media header */
	uint32	ifi_mtu;               /* mtu */
	uint32	ifi_metric;            /* routing metric */
	uint32	ifi_baudrate;          /* baudrate of line */
	/* statistics!! */
	int32  ifi_ipackets;           /* packets received on interface */
	int32  ifi_ierrors;            /* input errors on interface */
	int32  ifi_opackets;           /* packets sent on interface */
	int32  ifi_oerrors;            /* output errors on interface */
	int32  ifi_collisions;         /* collisions on csma interfaces */
	int32  ifi_ibytes;             /* total number of octets received */
	int32  ifi_obytes;             /* total number of octets sent */
	int32  ifi_imcasts;            /* packets received via multicast */
	int32  ifi_omcasts;            /* packets sent via multicast */
	int32  ifi_iqdrops;            /* dropped on input, this interface */
	int32  ifi_noproto;            /* destined for unsupported protocol */
};
	
struct ifnet {
	struct ifnet *if_next;       /* next device */
	struct ifaddr *if_addrlist;  /* linked list of addresses */
	int devid;                   /* our device id if we have one... */
	int id;                      /* id within the stack's device list */
	char *name;                  /* name of driver e.g. tulip */
	int if_unit;                 /* number of unit e.g  0 */
	char *if_name;               /* full name, e.g. tulip0 */
	struct if_data ifd;	         /* if_data structure, shortcuts below */
	int if_flags;                /* if flags */
	int if_index;                /* our index in ifnet_addrs and interfaces */

	struct ifq *rxq;
	thread_id rx_thread;
	struct ifq *txq;
	thread_id tx_thread;
	struct ifq *devq;

	int	 (*start) (struct ifnet *);
	int	 (*stop)  (struct ifnet *);	
	void (*input) (struct mbuf*);
	int	 (*output)(struct ifnet *, struct mbuf*, 
			  struct sockaddr*, struct rtentry *); 
	int	 (*ioctl) (struct ifnet *, int, caddr_t);
};
#define if_mtu          ifd.ifi_mtu
#define if_type         ifd.ifi_type
#define if_addrlen      ifd.ifi_addrlen
#define if_hdrlen       ifd.ifi_hdrlen
#define if_metric       ifd.ifi_metric
#define if_baudrate	    ifd.ifi_baurdate
#define if_ipackets     ifd.ifi_ipackets
#define if_ierrors      ifd.ifi_ierrors
#define if_opackets     ifd.ifi_opackets
#define if_oerrors      ifd.ifi_oerrors
#define if_collisions   ifd.ifi_collisions
#define if_ibytes       ifd.ifi_ibytes
#define if_obytes       ifd.ifi_obytes
#define if_imcasts      ifd.ifi_imcasts
#define if_omcasts      ifd.ifi_omcasts
#define if_iqdrops      ifd.ifi_iqdrops
#define if_noproto      ifd.ifi_noproto

#define IFNAMSIZ	16

/* This structure is used for passing interface requests via ioctl */
struct ifreq {
	char	ifr_name[IFNAMSIZ];	/* name of interface */
	union {
		struct sockaddr ifru_addr;
		struct sockaddr ifru_dstaddr;
		struct sockaddr ifru_broadaddr;
		uint16 ifru_flags;
		int ifru_metric;
		caddr_t ifru_data;
	} ifr_ifru;
};
#define ifr_addr        ifr_ifru.ifru_addr
#define ifr_dstaddr     ifr_ifru.ifru_dstaddr
#define ifr_broadaddr	ifr_ifru.ifru_broadaddr
#define ifr_flags       ifr_ifru.ifru_flags
#define ifr_metric      ifr_ifru.ifru_metric
#define ifr_mtu         ifr_ifru.ifru_metric /* sneaky overload :) */
#define	ifr_data        ifr_ifru.ifru_data

struct ifconf {
	int ifc_len;	/* length of associated buffer */
	union {
		caddr_t ifcu_buf;
		struct ifreq *ifcu_req;
	} ifc_ifcu;
};
#define ifc_buf		ifc_ifcu.ifcu_buf
#define ifc_req		ifc_ifcu.ifcu_req


#define IFAFREE(ifa) \
do { \
        if ((ifa)->ifa_refcnt <= 0) \
                ifafree(ifa); \
        else \
                (ifa)->ifa_refcnt--; \
} while (0)

/* used to pass in additional information, such as aliases */
struct ifaliasreq {
	char ifa_name[IFNAMSIZ];
	struct sockaddr ifra_addr;
	struct sockaddr ifra_broadaddr;
#define ifra_dstaddr	ifra_broadaddr
	struct sockaddr ifra_mask;
};

#define IFA_ROUTE	RTF_UP

/*
 * Message format for use in obtaining information about interfaces
 * from sysctl and the routing socket.
 */
struct if_msghdr {
        u_short ifm_msglen;     /* to skip over non-understood messages */
        u_char  ifm_version;    /* future binary compatability */
        u_char  ifm_type;       /* message type */
        int     ifm_addrs;      /* like rtm_addrs */
        int     ifm_flags;      /* value of if_flags */
        u_short ifm_index;      /* index for associated ifp */
        struct  if_data ifm_data;/* statistics and other data about if */
};

/*
 * Message format for use in obtaining information about interface addresses
 * from sysctl and the routing socket.
 */
struct ifa_msghdr {
        u_short ifam_msglen;    /* to skip over non-understood messages */
        u_char  ifam_version;   /* future binary compatability */
        u_char  ifam_type;      /* message type */
        int     ifam_addrs;     /* like rtm_addrs */
        int     ifam_flags;     /* value of ifa_flags */
        u_short ifam_index;     /* index for associated ifp */
        int     ifam_metric;    /* value of ifa_metric */
};

#ifdef _NETWORK_STACK

/* function declaration */
struct  ifq    *start_ifq(void);
void            stop_ifq(struct ifq *);
struct  ifnet  *get_interfaces(void); 
struct	ifnet  *ifunit(char *name);
struct	ifaddr *ifa_ifwithaddr(struct sockaddr *);
struct	ifaddr *ifa_ifwithaf(int);
struct	ifaddr *ifa_ifwithdstaddr(struct sockaddr *);
struct	ifaddr *ifa_ifwithnet(struct sockaddr *);
struct	ifaddr *ifa_ifwithroute(int, struct sockaddr *,
                                struct sockaddr *);
struct	ifaddr *ifaof_ifpforaddr(struct sockaddr *, struct ifnet *);
void	ifafree(struct ifaddr *);

void    if_attach(struct ifnet *ifp);
void    if_detach(struct ifnet *ifp);

int     ifioctl(struct socket *so, int cmd, caddr_t data);
int     ifconf(int cmd, caddr_t data);
void    if_init(void);

#endif

#endif /* OBOS_IF_H */

