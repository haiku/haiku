/* in.c */

#include <stdio.h>

#ifdef _KERNEL_
#include <KernelExport.h>
#endif

#include "netinet/in.h"
#include "netinet/in_var.h"
#include "sys/socketvar.h"
#include "net/if.h"
#include "sys/sockio.h"
#include "net/route.h"

extern struct ifnet **ifnet_addrs;

struct in_ifaddr *get_primary_addr(void)
{
	return in_ifaddr;
}

/*
 * Trim a mask in a sockaddr
 */
void in_socktrim(struct sockaddr_in *ap)
{
	char *cplim = (char *) &ap->sin_addr;
	char *cp = (char *) (&ap->sin_addr + 1);

	ap->sin_len = 0;
	while (--cp >= cplim)
		if (*cp) {
			(ap)->sin_len = cp - (char *) (ap) + 1;
			break;
		}
}

#define rtinitflags(x) \
        ((((x)->ia_ifp->if_flags & (IFF_LOOPBACK | IFF_POINTOPOINT)) != 0) \
            ? RTF_HOST : 0)
/*
 * remove a route to prefix ("connected route" in cisco terminology).
 * re-installs the route by using another interface address, if there's one
 * with the same prefix (otherwise we lose the route mistakenly).
 */
static int in_scrubprefix(struct in_ifaddr *target)
{
        struct in_ifaddr *ia;
        struct in_addr prefix, mask, p;
        int error;

        if ((target->ia_flags & IFA_ROUTE) == 0)
                return 0;

        if (rtinitflags(target))
                prefix = target->ia_dstaddr.sin_addr;
        else
                prefix = target->ia_addr.sin_addr;
        mask = target->ia_sockmask.sin_addr;
        prefix.s_addr &= mask.s_addr;

        for (ia = in_ifaddr; ia; ia = ia->ia_next) {
                /* easy one first */
                if (mask.s_addr != ia->ia_sockmask.sin_addr.s_addr)
                        continue;

                if (rtinitflags(ia))
                        p = ia->ia_dstaddr.sin_addr;
                else
                        p = ia->ia_addr.sin_addr;
                p.s_addr &= ia->ia_sockmask.sin_addr.s_addr;
                if (prefix.s_addr != p.s_addr)
                        continue;

                /*
                 * if we got a matching prefix route, move IFA_ROUTE to him
                 */
                if ((ia->ia_flags & IFA_ROUTE) == 0) {
                        rtinit(&(target->ia_ifa), (int)RTM_DELETE,
                            rtinitflags(target));
                        target->ia_flags &= ~IFA_ROUTE;

                        error = rtinit(&ia->ia_ifa, (int)RTM_ADD,
                            rtinitflags(ia) | RTF_UP);
                        if (error == 0)
                                ia->ia_flags |= IFA_ROUTE;
                        return error;
                }
        }

        /*
         * noone seem to have prefix route.  remove it.
         */
        rtinit(&(target->ia_ifa), (int)RTM_DELETE, rtinitflags(target));
        target->ia_flags &= ~IFA_ROUTE;
        return 0;
}

#undef rtinitflags

int in_ifinit(struct ifnet *dev, struct in_ifaddr *ia, struct sockaddr_in *sin,
		int scrub)
{
	uint32 i = sin->sin_addr.s_addr;
	struct sockaddr_in oldsin;
	int error;
	int flags = RTF_UP;

	oldsin = ia->ia_addr;
	ia->ia_addr = *sin;

	if (dev && dev->ioctl) {
		error = (*dev->ioctl)(dev, SIOCSIFADDR, (caddr_t)ia);
		if (error) {
			ia->ia_addr = oldsin;
			return error;
		}
	}

	if (dev->if_type == IFT_ETHER) {
		ia->ia_ifa.ifa_flags |= RTF_CLONING;
	}
	
	if (scrub) {
		ia->ia_ifa.ifa_addr = (struct sockaddr*)&oldsin;
		in_scrubprefix(ia);
		ia->ia_ifa.ifa_addr = (struct sockaddr*)&ia->ia_addr;
	}

	if (IN_CLASSA(i))
		ia->ia_netmask = IN_CLASSA_NET;
	else if (IN_CLASSB(i))
		ia->ia_netmask = IN_CLASSB_NET;
	else
		ia->ia_netmask = IN_CLASSC_NET;
	
	if (ia->ia_subnetmask == 0) {
		ia->ia_subnetmask = ia->ia_netmask;
		ia->ia_sockmask.sin_addr.s_addr = ia->ia_subnetmask;
	} else
		ia->ia_netmask &= ia->ia_subnetmask;

	ia->ia_net = i & ia->ia_netmask;
	ia->ia_subnet = i & ia->ia_subnetmask;
	in_socktrim(&ia->ia_sockmask);

	ia->ia_ifa.ifa_metric = dev->if_metric;
	if (dev->if_flags & IFF_BROADCAST) {
		ia->ia_broadaddr.sin_addr.s_addr = ia->ia_subnet | ~ia->ia_subnetmask;
		ia->ia_netbroadcast.s_addr = ia->ia_net | ~ia->ia_netmask;
	} else if (dev->if_flags & IFF_LOOPBACK) {
		ia->ia_ifa.ifa_dstaddr = ia->ia_ifa.ifa_addr;
		flags |= RTF_HOST;
	} else if (dev->if_flags & IFF_POINTOPOINT) {
		if (ia->ia_dstaddr.sin_family != AF_INET)
			return 0;
		flags |= RTF_HOST;
	}

	/* This is really useful debugging code, but not needed at the moment... */
#if 0
	printf("in_ifaddr:\n");
	printf("         : ia_net          : %08lx\n", ia->ia_net);
	printf("         : ia_netmask      : %08lx\n", ia->ia_netmask);
	printf("         : ia_subnet       : %08lx\n", ia->ia_subnet);
	printf("         : ia_subnetmask   : %08lx\n", ia->ia_subnetmask);
	printf("         : ia_netbroadcast : %08lx\n", ia->ia_netbroadcast.s_addr);
	printf("         : ia_addr         : %08lx\n", ia->ia_addr.sin_addr.s_addr);
	printf("         : ia_dstaddr      : %08lx\n", ia->ia_dstaddr.sin_addr.s_addr);
	printf("         : ia_sockmask     : %08lx\n", ia->ia_sockmask.sin_addr.s_addr);
#endif

	error = rtinit(&(ia->ia_ifa), (int)RTM_ADD, flags);
	if (error == 0)
		ia->ia_flags |= IFA_ROUTE;

	/* XXX - Multicast address list */

	return error;
}

int in_control(struct socket *so, int cmd, caddr_t data, struct ifnet *ifp)
{
	struct ifreq *ifr = (struct ifreq*)data;
	struct in_ifaddr *ia = NULL;
	struct ifaddr *ifa;
	struct in_ifaddr *oia;
	struct in_aliasreq *ifra = (struct in_aliasreq*)data;
	struct sockaddr_in oldaddr;
	int error = 0, hostIsNew, maskIsNew;
	long i;

	if (ifp) /* we need to find the in_ifaddr */
		for (ia = in_ifaddr;ia; ia = ia->ia_next)
			if (ia->ia_ifp == ifp)
				break;
		
	switch (cmd) {
		case SIOCAIFADDR:
			printf("SIOCAIFADDR\n");
			/* add an address */
		case SIOCDIFADDR:
			printf("SIODIFADDR\n");
			/* delete an address */
			if (ifra->ifra_addr.sin_family == AF_INET)
				for (oia = ia; ia; ia = ia->ia_next) {
					if (ia->ia_ifp == ifp &&
					    ia->ia_addr.sin_addr.s_addr == ifra->ifra_addr.sin_addr.s_addr)
						break;
				}
			if (cmd == SIOCDIFADDR && ia == NULL)
				return EADDRNOTAVAIL;
		case SIOCSIFADDR:
			printf("SIOCSIFADDR\n");
			/* set an address */
		case SIOCSIFNETMASK:
			printf("SIOCSIFNETMASK #1 (%d)\n", cmd);
			/* set a net mask */
		case SIOCSIFDSTADDR:
			/* set the destination address of a point to point link */
			if (!ifp) {
				printf("No interface pointer!\n");
				return EINVAL;
			}
			if (ia == NULL) {
				oia = (struct in_ifaddr*)malloc(sizeof(struct in_ifaddr));
				if (oia == NULL)
					return ENOMEM;
				memset(oia, 0, sizeof(*oia));
				if ((ia = in_ifaddr)) {
					/* we've got other structures - add at end */
					for (; ia->ia_next; ia = ia->ia_next)
						continue;
					ia->ia_next = oia;
				} else
					in_ifaddr = oia;
				
				ia = oia;
				if ((ifa = ifp->if_addrlist)) {
					for (; ifa->ifa_next; ifa = ifa->ifa_next)
						continue;
					ifa->ifa_next = (struct ifaddr*)ia;
				} else
					ifp->if_addrlist =  (struct ifaddr*)ia;
				
				ia->ia_ifa.ifa_addr     = (struct sockaddr*) &ia->ia_addr;
				ia->ia_ifa.ifa_dstaddr  = (struct sockaddr*) &ia->ia_dstaddr;
				ia->ia_ifa.ifa_netmask  = (struct sockaddr*) &ia->ia_sockmask;
				ia->ia_sockmask.sin_len = 8;
				if (ifp->if_flags & IFF_BROADCAST) {
					ia->ia_broadaddr.sin_len = sizeof(ia->ia_addr);
					ia->ia_broadaddr.sin_family = AF_INET;
				}
				ia->ia_ifp = ifp;
			}
			break;
		case SIOCSIFBRDADDR:
		case SIOCGIFADDR:
		case SIOCGIFNETMASK:
		case SIOCGIFDSTADDR:
		case SIOCGIFBRDADDR:
			if (ia == NULL)
				return EADDRNOTAVAIL;
			break;
				
	}
	
	printf("loop #2 : %d [%ld]\n", cmd, SIOCSIFNETMASK);
	
	switch(cmd) {
		case SIOCGIFADDR:
			/* get interface address */
			*((struct sockaddr_in*) &ifr->ifr_addr) = ia->ia_addr;
			break;
		case SIOCGIFDSTADDR:
			/* get interface point to point destination address */
			if ((ifp->if_flags & IFF_POINTOPOINT) == 0)
				/* we're not a point to point interface */
				return EINVAL;
			*((struct sockaddr_in*) &ifr->ifr_dstaddr) = ia->ia_dstaddr;
			break;
		case SIOCGIFBRDADDR:
			/* get interface broadcast address */
			if ((ifp->if_flags & IFF_BROADCAST) == 0)
				/* we're not a broadcast capable interface */
				return EINVAL;
			*((struct sockaddr_in*) &ifr->ifr_dstaddr) = ia->ia_broadaddr;
			break;
		case SIOCGIFNETMASK:
			/* get interface netmask */
			*((struct sockaddr_in*) &ifr->ifr_addr) = ia->ia_sockmask;
			break;
		case SIOCSIFADDR:
			printf("SIOCSIFADDR #2\n");
			return in_ifinit(ifp, ia, (struct sockaddr_in*)&ifr->ifr_addr, 1);
		case SIOCSIFNETMASK:
			printf("Setting netmask\n");
			/* set the netmask for the interface... */
			/* set i to the network netmask (network host order) */
			i = ifra->ifra_addr.sin_addr.s_addr;
			/* set the host byte order netmask into ia_subnetmask */
			ia->ia_subnetmask = ntohl((ia->ia_sockmask.sin_addr.s_addr = i));
printf("ia->ia_subnetmask: %08lx\n", ia->ia_subnetmask);
			break;
		case SIOCSIFDSTADDR:
			if ((ifp->if_flags & IFF_POINTOPOINT) == 0)
				return EINVAL;
			oldaddr = ia->ia_dstaddr;
			ia->ia_dstaddr = *(struct sockaddr_in*)&ifr->ifr_dstaddr;
			/* update the interface if required */
			if (ifp->ioctl) {
				error = ifp->ioctl(ifp, SIOCSIFDSTADDR, (caddr_t) ia);
				if (error) {
					ia->ia_dstaddr = oldaddr;
					return error;
				}
			}
			/* change the routing info if it's been set */
			if (ia->ia_flags & IFA_ROUTE) {
				ia->ia_ifa.ifa_dstaddr = (struct sockaddr*)&oldaddr;
				rtinit(&(ia->ia_ifa), RTM_DELETE, RTF_HOST);
				ia->ia_ifa.ifa_dstaddr = (struct sockaddr*)&ia->ia_dstaddr;
				rtinit(&(ia->ia_ifa), RTM_ADD, RTF_HOST|RTF_UP);
			}
			break;

		case SIOCSIFBRDADDR:
			/* set the broadcast address if interface supports it */
			if ((ifp->if_flags & IFF_BROADCAST) == 0)
				/* we don't support broadcast on that interface */
				return EINVAL;
			ia->ia_broadaddr = *(struct sockaddr_in*) &ifr->ifr_broadaddr;
			break;
		case SIOCAIFADDR:
			maskIsNew = 0;
			hostIsNew = 1;
			error = 0;
			if (ia->ia_addr.sin_family == AF_INET) {
				if (ifra->ifra_addr.sin_len == 0) {
					ifra->ifra_addr = ia->ia_addr;
					hostIsNew = 0;
				} else if (ifra->ifra_addr.sin_addr.s_addr == 
				           ia->ia_addr.sin_addr.s_addr)
					hostIsNew = 0;
			}
			if (ifra->ifra_mask.sin_len) {
				in_scrubprefix(ia);
				ia->ia_sockmask = ifra->ifra_mask;
				ia->ia_subnetmask = ia->ia_sockmask.sin_addr.s_addr;
				maskIsNew = 1;
			}
			if ((ifp->if_flags & IFF_POINTOPOINT) &&
			    (ifra->ifra_dstaddr.sin_family == AF_INET)) {
				in_scrubprefix(ia);
				ia->ia_dstaddr = ifra->ifra_dstaddr;
				maskIsNew = 1;
			}
			if (ifra->ifra_addr.sin_family == AF_INET &&
			    (hostIsNew || maskIsNew))
				error = in_ifinit(ifp, ia, &ifra->ifra_addr, 0);
			if ((ifp->if_flags & IFF_BROADCAST) &&
			    (ifra->ifra_broadaddr.sin_family == AF_INET))
				ia->ia_broadaddr = ifra->ifra_broadaddr;
			return error;
		default:
		printf("2nd iteration: default (%d)\n", cmd);
			/* if we don't have enough to do the default, return */
			if (ifp == NULL || ifp->ioctl == NULL)
				return EINVAL; /* XXX - should be EOPNOTSUPP */
			/* send to the card and let it process it */
			return ifp->ioctl(ifp, cmd, data);
	}
	return 0;
}



/*
 * Return 1 if the address might be a local broadcast address.
 */
int in_broadcast(struct in_addr in, struct ifnet *ifp)
{
	struct ifnet *ifn, *if_first, *if_target;
	struct ifaddr *ifa;

	if (in.s_addr == INADDR_BROADCAST ||
	    in.s_addr == INADDR_ANY)
		return 1;
	if (ifp && ((ifp->if_flags & IFF_BROADCAST) == 0))
		return 0;

	if (ifp == NULL) {
		if_first = *ifnet_addrs;
		if_target = 0;
	} else {
		if_first = ifp;
		if_target = ifp->if_next;
	}

#define ia (ifatoia(ifa))
	/*
	 * Look through the list of addresses for a match
	 * with a broadcast address.
	 * If ifp is NULL, check against all the interfaces.
	 */
	for (ifn = if_first; ifn != if_target; ifn = ifn->if_next) {
		for (ifa = ifn->if_addrlist; ifa; ifa = ifa->ifa_next) {
			if (!ifp) {
				if (ifa->ifa_addr->sa_family == AF_INET &&
				    ((ia->ia_subnetmask != 0xffffffff &&
				    (((ifn->if_flags & IFF_BROADCAST) &&
				    in.s_addr == ia->ia_broadaddr.sin_addr.s_addr) ||
				    in.s_addr == ia->ia_subnet)) ||
				    /*
				     * Check for old-style (host 0) broadcast.
				     */
				    (in.s_addr == ia->ia_netbroadcast.s_addr ||
				    in.s_addr == ia->ia_net)))
					return 1;
				else
					if (ifa->ifa_addr->sa_family == AF_INET &&
					    (((ifn->if_flags & IFF_BROADCAST) &&
					    in.s_addr == ia->ia_broadaddr.sin_addr.s_addr) ||
					    in.s_addr == ia->ia_netbroadcast.s_addr ||
					    /*
					     * Check for old-style (host 0) broadcast.
					     */
					    in.s_addr == ia->ia_subnet ||
					    in.s_addr == ia->ia_net))
						return 1;
			}
		}
	}
	return (0);
#undef ia
}

#ifndef SUBNETSARELOCAL
#define SUBNETSARELOCAL 0
#endif
int subnetsarelocal = SUBNETSARELOCAL;
/*
 * Return 1 if an internet address is for a ``local'' host
 * (one to which we have a connection).  If subnetsarelocal
 * is true, this includes other subnets of the local net.
 * Otherwise, it includes only the directly-connected (sub)nets.
 */
int in_localaddr(struct in_addr in)
{
	struct in_ifaddr *ia;

	if (subnetsarelocal) {
		for (ia = in_ifaddr; ia != 0; ia = ia->ia_next)
			if ((in.s_addr & ia->ia_netmask) == ia->ia_net)
				return (1);
	} else {
		for (ia = in_ifaddr; ia != 0; ia = ia->ia_next)
			if ((in.s_addr & ia->ia_subnetmask) == ia->ia_subnet)
				return (1);
	}
	return (0);
}

int in_canforward(struct in_addr in)
{
	uint32 i = ntohl(in.s_addr);
	uint32 net;
	
	if (IN_EXPERIMENTAL(i) || IN_MULTICAST(i))
		return 0;
	if (IN_CLASSA(i)) {
		net = i & IN_CLASSA_NET;
		if (net == 0 || net == (IN_LOOPBACKNET << IN_CLASSA_NSHIFT))
			return 0;
	}
	return 1;
}


