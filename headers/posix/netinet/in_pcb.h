/* in_pcb.h
 * internet protcol control blocks
 */

#include "sys/socketvar.h"
#include "sys/socket.h"
#include "net_misc.h"
#include "pools.h"
#include "netinet/ip.h"
#include "netinet/in.h"
#include "net/route.h"

#ifndef IN_PCB_H
#define IN_PCB_H

enum {
	INP_HDRINCL	= 0x01,
	INP_RECVOPTS	= 0x02,
	INP_RECVRETOPTS = 0x04,
	INP_RECVDSTADDR	= 0x08	/* receive IP destination as control inf. */
};
#define INP_CONTROLOPT (INP_RECVOPTS | INP_RECVRETOPTS | INP_RECVDSTADDR)

/* Constants for in_pcblookup */
enum {
	INPLOOKUP_WILDCARD 	= 1,
	INPLOOKUP_SETLOCAL	= 2,
	INPLOOKUP_IPV6		= 4
};

struct inpcb {
	struct inpcb *inp_next;
	struct inpcb *inp_prev;
	struct inpcb *inp_head;

	struct in_addr faddr;       /* foreign address */
	uint16  fport;              /* foreign port # */	
	struct in_addr laddr;       /* local address */
	uint16  lport;              /* local port # */

	struct socket *inp_socket;
	caddr_t inp_ppcb;           /* pointer to a per protocol pcb*/
	struct ip inp_ip;	        /* header prototype */	
	int inp_flags;		        /* flags */
	struct mbuf *inp_options;   /* IP options */
	/* more will be required */
	struct route inp_route;	    /* the route to host */
};

int      in_pcballoc      (struct socket *, struct inpcb *);
int      in_pcbbind       (struct inpcb *, struct mbuf *);
int      in_pcbconnect    (struct inpcb *, struct mbuf *);
void     in_pcbdetach     (struct inpcb *);
int      in_pcbdisconnect (struct inpcb *);
void     in_losing        (struct inpcb *);
void     in_setpeeraddr   (struct inpcb *, struct mbuf *);
void     in_setsockaddr   (struct inpcb *, struct mbuf *);

struct inpcb *in_pcblookup(struct inpcb *head, struct in_addr faddr,
			   uint16 fport_a, struct in_addr laddr,
			   uint16 lport_a, int flags);
struct rtentry *in_pcbrtentry (struct inpcb *);
void     in_pcbnotify (struct inpcb *, struct sockaddr *,
                      uint16, struct in_addr, uint16, 
                      int, void (*)(struct inpcb *, int));

/* helpful macro's */
#define     sotoinpcb(so)   ((struct inpcb *)(so)->so_pcb)

#endif /* IN_PCB_H */

