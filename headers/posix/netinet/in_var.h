/* in_var.h */
#ifndef NETINET_IN_VAR_H
#define NETINET_IN_VAR_H

#include "netinet/in.h"
#include "net/if.h"

struct in_ifaddr {
	struct ifaddr 		ia_ifa;
	struct in_ifaddr 	*ia_next;

	uint32			ia_net;
	uint32			ia_netmask;
	uint32			ia_subnet;
	uint32			ia_subnetmask;
	struct in_addr 		ia_netbroadcast;
	struct sockaddr_in 	ia_addr;
	struct sockaddr_in 	ia_dstaddr;		/* broadcast address */
	struct sockaddr_in	ia_sockmask;
	/*XXX - milticast address list */
};
#define ia_ifp		ia_ifa.ifa_ifp
#define ia_flags	ia_ifa.ifa_flags
#define ia_broadaddr	ia_dstaddr

#define ifatoia(ifa)    ((struct in_ifaddr *)(ifa))
#define sintosa(sin)	((struct sockaddr *)(sin))

/* used to pass in additional information, such as aliases */
struct in_aliasreq {
	char ifa_name[IFNAMSIZ];
	struct sockaddr_in ifra_addr;
	struct sockaddr_in ifra_broadaddr;
#define ifra_dstaddr	ifra_broadaddr
	struct sockaddr_in ifra_mask;
};


struct in_multi {
	struct in_addr inm_addr;
	struct ifnet *inm_ifp;
	struct in_ifaddr *inm_ia;
	uint inm_refcount;
	uint inm_timer;
	struct in_multi *next;
	struct in_multi **prev;	
	uint inm_state;
};

/*
 * Given a pointer to an in_ifaddr (ifaddr),
 * return a pointer to the addr as a sockaddr_in.
 */
#define IA_SIN(ia) (&(((struct in_ifaddr *)(ia))->ia_addr))

struct in_ifaddr *in_ifaddr;

int in_control(struct socket *so, int cmd, caddr_t data, struct ifnet *ifp);
int in_ifinit(struct ifnet *dev, struct in_ifaddr *ia, struct sockaddr_in *sin,
                int scrub);
int inetctlerr(int cmd);
struct in_ifaddr *get_primary_addr(void);

#endif /* NETINET_IN_VAR_H */
