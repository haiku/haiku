/* route.c */

#ifndef _KERNEL_
#include <stdio.h>
#endif
#include <kernel/OS.h>

#ifdef _KERNEL_
#include <KernelExport.h>
#endif

#include "net_malloc.h"

#include "sys/domain.h"
#include "net/route.h" /* includes net/radix.h */
#include "protocols.h"
#include "net/if.h"

int rttrash = 0;	/* routes in table that should have been freed but hevn't been */

#define SA(p) ((struct sockaddr *)(p))
#define ROUNDUP(a) (a >0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

struct  radix_node_head **get_rt_tables(void)
{
	return (struct radix_node_head**)rt_tables;
}

struct rtentry *rtalloc1(struct sockaddr *dst, int report)
{
	struct radix_node_head *rnh = rt_tables[dst->sa_family];
	struct rtentry *rt;
	struct radix_node *rn;
	struct rtentry *newrt = NULL;
	struct rt_addrinfo info;
	int msgtype = RTM_MISS;
	int err;

	if (rnh && (rn = rnh->rnh_matchaddr((caddr_t)dst, rnh))
	    && ((rn->rn_flags & RNF_ROOT) == 0)) {
		newrt = rt = (struct rtentry*) rn;
		if (report && (rt->rt_flags & RTF_CLONING)) {
			err = rtrequest(RTM_RESOLVE, dst, NULL,
				        NULL, 0, &newrt);
			if (err) {
				newrt = rt;
				rt->rt_refcnt++;
				goto miss;
			}
			if ((rt = newrt) && (rt->rt_flags & RTF_XRESOLVE)) {
				msgtype = RTM_RESOLVE;
				goto miss;
			}
		} else
			rt->rt_refcnt++;
	}
	/* XXX - stats? */
		
miss:
	if (report) {
		memset((caddr_t)&info, 0, sizeof(info));
		info.rti_info[RTAX_DST] = dst;
		//rt_missmsg(msgtype, &info, 0, err);
	}
	return (newrt);
}

void rtalloc(struct route *ro) 
{
	/* can we use what we have?? */
	if (ro && ro->ro_rt && ro->ro_rt->rt_ifp &&
	    (ro->ro_rt->rt_flags & RTF_UP)) {
		/* yes */
		return;
	}
	/* no, get a new route */
	ro->ro_rt = rtalloc1(&ro->ro_dst, 1);
}

void rtfree(struct rtentry *rt)
{
	struct ifaddr *ifa;

	if (!rt) {
		printf("rtfree on a NULL pointer!\n");
		return;
	}

	rt->rt_refcnt--;
	if (rt->rt_refcnt <= 0 && (rt->rt_flags && RTF_UP) == 0) {
		if (rt->rt_nodes->rn_flags & (RNF_ACTIVE | RNF_ROOT)) {
			printf("Trying to free nodes we shouldn't be!\n");
			return;
		}
		rttrash--;
		if (rt->rt_refcnt < 0) {
			printf("rtfree: %p not freed as the refcnt is negative!\n", rt);
			return;
		}
		ifa = rt->rt_ifa;
		IFAFREE(ifa);
		Free(rt_key(rt));
		Free(rt);
	}
}

int rtrequest(int req,  struct sockaddr *dst,
			struct sockaddr *gateway,
			struct sockaddr *netmask,
			int flags,
			struct rtentry **ret_nrt)
{
	int error = 0;
	struct rtentry *rt;
	struct radix_node *rn = NULL;
	struct radix_node_head *rnh;
	struct ifaddr *ifa;
	struct sockaddr *ndst;

#define snderr(x)	{error = x; goto bad; }

	if ((rnh = rt_tables[dst->sa_family]) == NULL)
		snderr(ESRCH);
	if (flags & RTF_HOST)
		netmask = NULL;

	switch(req) {
		case RTM_DELETE:
			if ((rn = rnh->rnh_deladdr(dst, netmask, rnh)) == NULL)
				snderr(ESRCH);
			if (rn->rn_flags & (RNF_ACTIVE | RNF_ROOT)) {
				/* XXX - should be panic */
				printf("rtrequest: delete: cannot delete route!\n");
				return -1;
			}
			rt = (struct rtentry *)rn;
			rt->rt_flags &= ~RTF_UP; /* mark route as down */
			if (rt->rt_gwroute) {
				rt = rt->rt_gwroute;
				RTFREE(rt);
				(rt = (struct rtentry*)rn)->rt_gwroute = NULL;
			}
			if ((ifa = rt->rt_ifa) && ifa->ifa_rtrequest)
				ifa->ifa_rtrequest(RTM_DELETE, rt, NULL);
			rttrash++;
			if (ret_nrt)
				*ret_nrt = rt;
			else if (rt->rt_refcnt <= 0) {
				rt->rt_refcnt++;
				rtfree(rt);
			}
			break;
		case RTM_RESOLVE:
			if (ret_nrt == NULL || (rt = *ret_nrt) == NULL)
				snderr(EINVAL);
			ifa = rt->rt_ifa;
			flags = rt->rt_flags & ~RTF_CLONING;
			gateway = rt->rt_gateway;
			if ((netmask = rt->rt_genmask) == NULL)
				flags |= RTF_HOST;
			/* fall through */
			goto makeroute;
		case RTM_ADD:
			/* can we find a route to it? */
			if ((ifa = ifa_ifwithroute(flags, dst, gateway)) == NULL) {
				printf("ENETUNREACH!\n");
				snderr(ENETUNREACH);
			}
makeroute:
			R_Malloc(rt, struct rtentry *, sizeof(*rt));
			if (!rt)
				snderr(ENOMEM);
			Bzero(rt, sizeof(*rt));
			rt->rt_flags = RTF_UP | flags;
			if (rt_setgate(rt, dst, gateway)) {
				Free(rt);
				snderr(ENOMEM);
			}

			ndst = rt_key(rt);
			if (netmask)
				rt_maskedcopy(dst, ndst, netmask);
			else
				Bcopy(dst, ndst, dst->sa_len);

			rn = rnh->rnh_addaddr((caddr_t) ndst, (caddr_t) netmask,
						rnh, rt->rt_nodes);

			if (!rn) {
				if (rt->rt_gwroute)
					rtfree(rt->rt_gwroute);
				Free(rt_key(rt));
				Free(rt);
				snderr(EEXIST);
			}
			ifa->ifa_refcnt++;
			rt->rt_ifa = ifa;
			rt->rt_ifp = ifa->ifa_ifp;
			/* if we've fallen through - copy metrics */
			if (req == RTM_RESOLVE)
				rt->rt_rmx = (*ret_nrt)->rt_rmx;
			if (ifa->ifa_rtrequest)
				ifa->ifa_rtrequest(req, rt, SA(ret_nrt ? *ret_nrt : 0));
			if (ret_nrt) {
				*ret_nrt = rt;
				rt->rt_refcnt++;
			}
			break;
	}
bad:
        return (error);
}

struct ifaddr *ifa_ifwithroute(int flags, 
				struct sockaddr *dst, 
				struct sockaddr *gateway)
{
	struct ifaddr *ifa;

        if ((flags & RTF_GATEWAY) == 0) {
                /*
                 * If we are adding a route to an interface,
                 * and the interface is a pt to pt link
                 * we should search for the destination
                 * as our clue to the interface.  Otherwise
                 * we can use the local address.
                 */
                ifa = NULL;
                if (flags & RTF_HOST)
                        ifa = ifa_ifwithdstaddr(dst);
                if (ifa == NULL)
                        ifa = ifa_ifwithaddr(gateway);
        } else {
                /*
                 * If we are adding a route to a remote net
                 * or host, the gateway may still be on the
                 * other end of a pt to pt link.
                 */
                ifa = ifa_ifwithdstaddr(gateway);
        }
        if (ifa == NULL)
                ifa = ifa_ifwithnet(gateway);
        if (ifa == NULL) {
                struct rtentry *rt = rtalloc1(gateway, 0);
                if (rt == NULL)
                        return (NULL);
                rt->rt_refcnt--;
                /* The gateway must be local if the same address family. */
                if ((rt->rt_flags & RTF_GATEWAY) &&
                    rt_key(rt)->sa_family == dst->sa_family)
                        return (0);
                if ((ifa = rt->rt_ifa) == NULL)
                        return (NULL);
        }
        if (ifa->ifa_addr->sa_family != dst->sa_family) {
                struct ifaddr *oifa = ifa;
                ifa = ifaof_ifpforaddr(dst, ifa->ifa_ifp);
                if (ifa == NULL)
                        ifa = oifa;
        }
        return (ifa);
}

int rt_setgate(struct rtentry *rt0, 
		struct sockaddr *dst, 
		struct sockaddr *gate)
{
        caddr_t new, old = NULL;
        int dlen = ROUNDUP(dst->sa_len), glen = ROUNDUP(gate->sa_len);
        struct rtentry *rt = rt0;


        if (rt->rt_gateway == NULL || glen > ROUNDUP(rt->rt_gateway->sa_len)) {
                old = (caddr_t)rt_key(rt);
                R_Malloc(new, caddr_t, dlen + glen);
                if (new == NULL)
                        return 1;
                rt->rt_nodes->rn_key = new;
        } else {
                new = rt->rt_nodes->rn_key;
                old = NULL;
        }

        Bcopy(gate, (rt->rt_gateway = (struct sockaddr *)(new + dlen)), glen);
        if (old) {
                Bcopy(dst, new, dlen);
                Free(old);
        }

        if (rt->rt_gwroute != NULL) {
                rt = rt->rt_gwroute;
                RTFREE(rt);
                rt = rt0;
                rt->rt_gwroute = NULL;
        }

        if (rt->rt_flags & RTF_GATEWAY) {
                rt->rt_gwroute = rtalloc1(gate, 1);
                /*
                 * If we switched gateways, grab the MTU from the new
                 * gateway route if the current MTU is 0 or greater
                 * than the MTU of gateway.
                 */
                if (rt->rt_gwroute && !(rt->rt_rmx.rmx_locks & RTV_MTU) &&
                    (rt->rt_rmx.rmx_mtu == 0 ||
                     rt->rt_rmx.rmx_mtu > rt->rt_gwroute->rt_rmx.rmx_mtu)) {
                        rt->rt_rmx.rmx_mtu = rt->rt_gwroute->rt_rmx.rmx_mtu;
                }
        }

        return 0;
}

void rt_maskedcopy(struct sockaddr *src, 
		struct sockaddr *dst, 
		struct sockaddr *netmask)
{
        uchar *cp1 = (uchar *)src;
        uchar *cp2 = (uchar *)dst;
        uchar *cp3 = (uchar *)netmask;
        uchar *cplim = cp2 + *cp3;
        uchar *cplim2 = cp2 + *cp1;

        *cp2++ = *cp1++; *cp2++ = *cp1++; /* copies sa_len & sa_family */
        cp3 += 2;
        if (cplim > cplim2)
                cplim = cplim2;
        while (cp2 < cplim)
                *cp2++ = *cp1++ & *cp3++;
        if (cp2 < cplim2)
                memset((caddr_t)cp2, 0, (unsigned)(cplim2 - cp2));
}

void ifafree(struct ifaddr *ifa)
{
	if (ifa == NULL) {
		printf("ifafree");
		return;
	}		
	if (ifa->ifa_refcnt == 0)
		free(ifa);
	else
		ifa->ifa_refcnt--;
}

/*
 * Set up a routing table entry, normally
 * for an interface.
 */
int rtinit(struct ifaddr *ifa, int cmd, int flags)
{
	struct rtentry *rt;
	struct sockaddr *dst;
	struct sockaddr *deldst;
	struct mbuf *m = NULL;
	struct rtentry *nrt = NULL;
	int error;

	dst = flags & RTF_HOST ? ifa->ifa_dstaddr : ifa->ifa_addr;
	if (cmd == RTM_DELETE) {
		if ((flags & RTF_HOST) == 0 && ifa->ifa_netmask) {
			m = m_get(MT_SONAME);
			if (m == NULL)
				return(ENOBUFS);
			deldst = mtod(m, struct sockaddr *);
			rt_maskedcopy(dst, deldst, ifa->ifa_netmask);
			dst = deldst;
		}
		if ((rt = rtalloc1(dst, 0)) != NULL) {
			rt->rt_refcnt--;
			if (rt->rt_ifa != ifa) {
				if (m != NULL)
					(void) m_free(m);
				return (flags & RTF_HOST ? EHOSTUNREACH : ENETUNREACH);
			}
		}
	}

	error = rtrequest(cmd, dst, ifa->ifa_addr, ifa->ifa_netmask,
	                  flags | ifa->ifa_flags, &nrt);

	if (cmd == RTM_DELETE && error == 0 && (rt = nrt) != NULL) {
		/* XXX - add this when we have routing sockets!
		rt_newaddrmsg(cmd, ifa, error, nrt);
		*/
		if (rt->rt_refcnt <= 0) {
			rt->rt_refcnt++;
			rtfree(rt);
		}
	}
	if (cmd == RTM_ADD && error == 0 && (rt = nrt) != NULL) {
		rt->rt_refcnt--;
		if (rt->rt_ifa != ifa) {
			printf("rtinit: wrong ifa (%p) was (%p)\n", ifa, rt->rt_ifa);

			if (rt->rt_ifa->ifa_rtrequest)
				rt->rt_ifa->ifa_rtrequest(RTM_DELETE, rt, NULL);
			IFAFREE(rt->rt_ifa);
			rt->rt_ifa = ifa;
			rt->rt_ifp = ifa->ifa_ifp;
			rt->rt_rmx.rmx_mtu = ifa->ifa_ifp->if_mtu;
			ifa->ifa_refcnt++;
			if (ifa->ifa_rtrequest)
				ifa->ifa_rtrequest(RTM_ADD, rt, NULL);
		}
		/* XXX - add this when we have routing sockets!
		rt_newaddrmsg(cmd, ifa, error, nrt);
		*/
	}
	return (error);
}

void rtable_init(void **table)
{
	struct domain *dom;
	
	for (dom = domains; dom; dom = dom->dom_next)
		if (dom->dom_rtattach)
			dom->dom_rtattach(&table[dom->dom_family], dom->dom_rtoffset);
}

void route_init(void)
{
	rn_init();

	rtable_init((void**)rt_tables);
}
