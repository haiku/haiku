/* inpcb.c
 *
 * implementation of internet control blocks code
 */

#include <stdio.h>

#ifdef _KERNEL_
#include <KernelExport.h>
#endif

#include "pools.h"
#include "sys/socketvar.h"
#include "netinet/in.h"
#include "netinet/in_pcb.h"
#include "net/if.h"
#include "netinet/in_var.h"
#include "sys/protosw.h"

static struct pool_ctl *pcbpool = NULL;
static struct in_addr zeroin_addr;

int inetctlerrmap[PRC_NCMDS] = {
        0,              0,              0,              0,
        0,              EMSGSIZE,       EHOSTDOWN,      EHOSTUNREACH,
        EHOSTUNREACH,   EHOSTUNREACH,   ECONNREFUSED,   ECONNREFUSED,
        EMSGSIZE,       EHOSTUNREACH,   0,              0,
        0,              0,              0,              0,
        ENOPROTOOPT
};

int inpcb_init(void)
{
	in_ifaddr = NULL;
	
	if (!pcbpool)
		pool_init(&pcbpool, sizeof(struct inpcb));

	if (!pcbpool) {
		printf("inpcb_init: ENOMEM\n");
		return ENOMEM;
	}

	zeroin_addr.s_addr = 0;
	return 0;
}

int in_pcballoc(struct socket *so, struct inpcb *head)
{
	struct inpcb *inp;

	inp = (struct inpcb *)pool_get(pcbpool);

	if (!inp) {
		printf("in_pcballoc: ENOMEM\n");
		return ENOMEM;
	}

	memset(inp, 0, sizeof(*inp));

	inp->inp_head = head;
	/* associate ourselves with the socket */
	inp->inp_socket = so;
	insque(inp, head);
	so->so_pcb = (caddr_t)inp;
	return 0;
}

void in_pcbdetach(struct inpcb *inp) 
{
	struct socket *so = inp->inp_socket;

	so->so_pcb = NULL;
	/* BSD sockets would call sofree here - we can't.
	 * The first thing that sofree does in BSD is check whether
	 * there are still file system references to the socket
	 * (SS_NOFDREF) and if there are it doesn't free. We don't have
	 * the same relationship to our sockets, as we use the socket for
	 * the kernel cookie, and freeing it here would lead to real problems,
	 * so we leave the socket until we call soclose()
	 * This may need to be reviewed and an extra layer of abstraction
	 * added at some point if we find it's using too much system resource.
	 */

	if (inp->inp_options)
		m_free(inp->inp_options);
	if (inp->inp_route.ro_rt)
		rtfree(inp->inp_route.ro_rt);
	
	remque(inp);
	pool_put(pcbpool, inp);
}

int in_pcbbind(struct inpcb *inp, struct mbuf *nam)
{
	struct socket *so = inp->inp_socket;
	struct inpcb *head = inp->inp_head;
	struct sockaddr_in *sin;
	uint16 lport = 0;
	int wild = 0;
	int reuseport = (so->so_options & SO_REUSEPORT);

	if (inp->lport || inp->laddr.s_addr != INADDR_ANY) {
		printf("in_pcbbind: EINVAL (%08lx:%d)\n", inp->laddr.s_addr, inp->lport);
		return EINVAL;
	}

	/* XXX - yuck! Try to format this better */
	/* This basically checks all the options that might be set that allow
	 * us to use wildcard searches.
	 */
	if (((so->so_options & (SO_REUSEADDR | SO_REUSEPORT)) == 0) &&
	    ((so->so_proto->pr_flags & PR_CONNREQUIRED) == 0 ||
	    (so->so_options & SO_ACCEPTCONN) == 0))
		wild = INPLOOKUP_WILDCARD;

	if (nam) {
		sin = mtod(nam, struct sockaddr_in *);
		if (nam->m_len != sizeof(*sin)) {
			printf("in_pcbind: EINVAL (m_len = %ld vs %ld)\n",
			       nam->m_len, sizeof(*sin));
			/* whoops, too much data! */
			return EINVAL;
		}

		/* Apparently this may not be correctly 
		 * in older programs, so this may need
		 * to be commented out...
		 */
		if (sin->sin_family != AF_INET) {
			printf("in_pcbbind: EAFNOSUPPORT\n");
			return EAFNOSUPPORT;
		}

		lport = sin->sin_port;

		if (IN_MULTICAST(ntohl(sin->sin_addr.s_addr))) {
			/* need special case for multicast. We'll
			 * allow the complete binding to be duplicated if
			 * SO_REUSEPORT is set, or if we have 
			 * SO_REUSEADDR set and both sockets have
			 * a multicast address bound.
			 * We'll exit with the reuseport variable set
			 * correctly.
			 */
			if (so->so_options & SO_REUSEADDR)
				reuseport = SO_REUSEADDR | SO_REUSEPORT;
		} else if (sin->sin_addr.s_addr != INADDR_ANY) {
			sin->sin_port = 0; /* must be zero for next step */
			if (ifa_ifwithaddr((struct sockaddr*)sin) == NULL) {
				printf("in_pcbbind: EADDRNOTAVAIL\n");
				return EADDRNOTAVAIL;
			}

		}
		if (lport) {
			struct inpcb *t;
			/* we have something to work with... */
			/* XXX - reserved ports have no meaning for us */
			/* XXX - fix me if we ever have multi-user */
			t = in_pcblookup(head, zeroin_addr, 0,
					 sin->sin_addr, lport, wild);
			if (t && (reuseport & t->inp_socket->so_options) == 0) {
				printf("in_pcbbind: EADDRINUSE\n");
				return EADDRINUSE;
			}
		}
		inp->laddr = sin->sin_addr;
	}
	/* if we have an ephemereal port, find a suitable port to use */
	if (lport == 0) {
		/* ephemereal port!! */
		do {
			if (head->lport++ < IPPORT_RESERVED ||
		  	    head->lport > IPPORT_USERRESERVED) {
				head->lport = IPPORT_RESERVED;
			}
			lport = htons(head->lport);
		} while (in_pcblookup(head, zeroin_addr, 0, 
					inp->laddr, lport, wild));
	}

	inp->lport = lport;
	return 0;
}

struct inpcb *in_pcblookup(struct inpcb *head, struct in_addr faddr,
			   uint16 fport_a, struct in_addr laddr,
			   uint16 lport_a, int flags)
{
	struct inpcb *inp;
	struct inpcb *match = NULL;
	int matchwild = 3;
	int wildcard;
	uint16 fport = fport_a;
	uint16 lport = lport_a;

	for (inp = head->inp_next; inp != head; inp = inp->inp_next) {
		if (inp->lport != lport)
			continue; /* local ports don't match */
		wildcard = 0;
		/* Here we try to find the best match. wildcard is set to 0
		 * and bumped by one every time we find something that doesn't match
		 * so we can have a suitable match at the end
		 */
		if (inp->laddr.s_addr != INADDR_ANY) {
			if (laddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->laddr.s_addr != laddr.s_addr)
				continue;
		} else {
			if (laddr.s_addr != INADDR_ANY)
				wildcard++;
		}
		
		if (inp->faddr.s_addr != INADDR_ANY) {
			if (faddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->faddr.s_addr != faddr.s_addr ||
				 inp->fport != fport)
				continue;
		} else {
			if (faddr.s_addr != INADDR_ANY)
				wildcard++;
		}
 		if (wildcard && ((flags & INPLOOKUP_WILDCARD) == 0)) {
			continue; /* wildcard match is not allowed!! */
		}
		
		if (wildcard < matchwild) {
			match = inp;
			matchwild = wildcard;
			if (matchwild == 0)
				break; /* exact match!! */
		}
	}
	return match;
}

int in_pcbconnect(struct inpcb *inp, struct mbuf *nam)
{
	struct in_ifaddr *ia = NULL;
	struct sockaddr_in *ifaddr = NULL;
	struct sockaddr_in *sin = mtod(nam, struct sockaddr_in *);
	
	if (nam->m_len != sizeof(*sin)) {
		printf("in_pcbconnect: EINVAL\n");
		return EINVAL;
	}
	if (sin->sin_family != AF_INET) {
		printf("in_pcbconnect: EAFNOSUPPORT (sin_family = %d, not %d)\n",
		       sin->sin_family, AF_INET);
		return EAFNOSUPPORT;
	}
	if (sin->sin_port == 0) {
		printf("in_pcbconnect: EADDRNOTAVAIL\n");
		return EADDRNOTAVAIL;
	}
	
	if (in_ifaddr) {
		if (sin->sin_addr.s_addr == INADDR_ANY)
			sin->sin_addr = IA_SIN(in_ifaddr)->sin_addr;

		/* we need to handle INADDR_BROADCAST here as well */
		
	}
	
	if (inp->laddr.s_addr == INADDR_ANY) {
		struct route *ro;

		ro = &inp->inp_route;

		if (ro && ro->ro_rt &&
		    (satosin(&ro->ro_dst)->sin_addr.s_addr != sin->sin_addr.s_addr
		    || inp->inp_socket->so_options & SO_DONTROUTE)) {
			RTFREE(ro->ro_rt);
			ro->ro_rt = NULL;
		}
		if ((inp->inp_socket->so_options & SO_DONTROUTE) == 0
		    && (ro->ro_rt == NULL 
			|| ro->ro_rt->rt_ifp == NULL)) {
			/* we don't have a route, try to get one */
			memset(&ro->ro_dst, 0, sizeof(ro->ro_dst));
			ro->ro_dst.sa_family = AF_INET;
			ro->ro_dst.sa_len = sizeof(struct sockaddr_in);
			((struct sockaddr_in*)&ro->ro_dst)->sin_addr = sin->sin_addr;
			rtalloc(ro);
		}
		/* did we find a route?? */
		if (ro->ro_rt && (ro->ro_rt->rt_ifp->if_flags & IFF_LOOPBACK))
			ia = ifatoia(ro->ro_rt->rt_ifa);

		if (ia == NULL) {
			uint16 fport = sin->sin_port;

			sin->sin_port = 0;
			ia = ifatoia(ifa_ifwithdstaddr(sintosa(sin)));
			if (ia == NULL)
				ia = ifatoia(ifa_ifwithnet(sintosa(sin)));
			sin->sin_port = fport;
			if (ia == NULL)
				ia = in_ifaddr;
			if (ia == NULL) {
				printf("in_pcbconnect: EADDRNOTAVAIL\n");
				return EADDRNOTAVAIL;
			}
		}
		/* XXX - handle multicast */
		ifaddr = (struct sockaddr_in*) &ia->ia_addr;
	}

	if (in_pcblookup(inp->inp_head, sin->sin_addr, sin->sin_port, 
			 inp->laddr.s_addr ? inp->laddr : ifaddr->sin_addr,
			 inp->lport, 0)) {
		printf("in_pcbconnect: EADDRINUSE\n"); 
		return EADDRINUSE;
	}

	if (inp->laddr.s_addr == INADDR_ANY) {
		if (inp->lport == 0)
			in_pcbbind(inp, NULL);
		inp->laddr = ifaddr->sin_addr;
	}
	inp->faddr = sin->sin_addr;
	inp->fport = sin->sin_port;
	return 0;
}

/* XXX - why is this an int? */
int in_pcbdisconnect(struct inpcb *inp)
{
	inp->faddr.s_addr = INADDR_ANY;
	inp->fport = 0;
	if (inp->inp_socket->so_state & SS_NOFDREF)
		in_pcbdetach(inp);
	return 0;
}

void in_losing(struct inpcb *inp)
{
	struct rtentry *rt;
	struct rt_addrinfo info;
	
	if ((rt = inp->inp_route.ro_rt)) {
		inp->inp_route.ro_rt = NULL;
		memset(&info, 0, sizeof(info));
		info.rti_info[RTAX_DST] = (struct sockaddr*)&inp->inp_route.ro_dst;
		info.rti_info[RTAX_GATEWAY] = rt->rt_gateway;
		info.rti_info[RTAX_NETMASK] = rt_mask(rt);
		//rt_missmsg 
		
		if (rt->rt_flags & RTF_DYNAMIC)
			rtrequest(RTM_DELETE, rt_key(rt), rt->rt_gateway, rt_mask(rt),
			           rt->rt_flags, NULL);
		else
			rtfree(rt);
	}
}

struct rtentry *in_pcbrtentry(struct inpcb *inp)
{
	struct route *ro;

	ro = &inp->inp_route;

	/*
	 * No route yet, so try to acquire one.
	 */
	if (ro->ro_rt == NULL) {
		memset(ro, 0, sizeof(struct route));

		if (inp->faddr.s_addr != INADDR_ANY) {
			/* this probably isn't needed, but better safe than sorry */
			memset(&ro->ro_dst, 0, sizeof(ro->ro_dst));
			ro->ro_dst.sa_family = AF_INET;
			ro->ro_dst.sa_len = sizeof(ro->ro_dst);
			satosin(&ro->ro_dst)->sin_addr = inp->faddr;
			rtalloc(ro);
		}
	}
	return (ro->ro_rt);
}

int inetctlerr(int cmd)
{
	return inetctlerrmap[cmd];
}

/* remove the route associated with a control block (if there is one)
 * forcing the route to be allocated next time it's used
 */
static void in_rtchange(struct inpcb *inp, int err)
{
	if (inp->inp_route.ro_rt) {
		rtfree(inp->inp_route.ro_rt);
		inp->inp_route.ro_rt = NULL;
	}
}

void in_pcbnotify(struct inpcb *head, struct sockaddr *dst, 
                  uint16 fport_arg, struct in_addr laddr, 
                  uint16 lport_arg, int cmd, 
                  void (*notify)(struct inpcb *, int))
{
	struct inpcb *inp, *oinp;
	struct in_addr faddr;
	uint16 fport = fport_arg, lport = lport_arg;
	int err = 0;
	
	if ((uint)cmd > PRC_NCMDS || dst->sa_family != AF_INET)
		return;
	faddr = satosin(dst)->sin_addr;
	if (faddr.s_addr == INADDR_ANY)
		return;

	if (PRC_IS_REDIRECT(cmd) || cmd == PRC_HOSTDEAD) {
		fport = lport = 0;
		laddr.s_addr = 0;
		if (cmd != PRC_HOSTDEAD)
			notify = in_rtchange;
	}
	err = inetctlerrmap[cmd];
	for (inp = head->inp_next; inp != head;) {
		if (inp->faddr.s_addr != faddr.s_addr ||
		    inp->inp_socket == NULL ||
		    inp->fport != fport ||
		    inp->lport != lport ||
		    (laddr.s_addr && inp->laddr.s_addr != laddr.s_addr)) {
			inp = inp->inp_next;
			continue;
		}
		oinp = inp;
		inp = inp->inp_next;
		if (notify)
			(*notify)(oinp, err);
	}
}

void in_setsockaddr(struct inpcb *inp, struct mbuf *nam)
{
	struct sockaddr_in *sin;
	
	nam->m_len = sizeof(*sin);
	sin = mtod(nam, struct sockaddr_in *);
	memset(sin, 0, sizeof(*sin));
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(*sin);
	sin->sin_port = inp->lport;
	sin->sin_addr = inp->laddr;
}

void in_setpeeraddr(struct inpcb *inp, struct mbuf *nam)
{
	struct sockaddr_in *sin;
	
	nam->m_len = sizeof(*sin);
	sin = mtod(nam, struct sockaddr_in *);
	memset(sin, 0, sizeof(*sin));
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(*sin);
	sin->sin_port = inp->fport;
	sin->sin_addr = inp->faddr;
}

