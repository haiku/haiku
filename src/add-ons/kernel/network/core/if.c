/* if helper functions */

#include <kernel/OS.h>
#include <stdio.h>
#include <string.h>

#ifdef _KERNEL_
#include <KernelExport.h>
#endif

#include "netinet/in.h"
#include "sys/socketvar.h"
#include "net/if.h"
#include "net/if_dl.h"
#include "sys/sockio.h"
#include "netinet/in_var.h"
#include "net/route.h"
#include "sys/protosw.h"

extern struct ifnet *devices;
extern int ndevs;

/* Variables used outside this file */
struct ifaddr **ifnet_addrs;

/* Private variables */
static int if_index;
static int if_indexlim;

struct ifnet *get_interfaces(void)
{
	return devices;
}

void if_init(void)
{
	ifnet_addrs = NULL;
	if_index = 0;
	if_indexlim = 8; /* initial value */
}

int ifioctl(struct socket *so, int cmd, caddr_t data)
{
	struct ifnet *ifp;
	struct ifreq *ifr;
	
	if (cmd == SIOCGIFCONF)
		return (ifconf(cmd, data));

	ifr = (struct ifreq*) data;
	ifp = ifunit(ifr->ifr_name);
	if (ifp == NULL)
		return ENXIO;

	switch(cmd) {
		case SIOCGIFFLAGS:
			/* get interface flags */
			ifr->ifr_flags = ifp->if_flags;
			break;
		case SIOCGIFMETRIC:
			/* get interface metric */
			ifr->ifr_metric = ifp->if_metric;
			break;
		case SIOCGIFMTU:
			/* get interface MTU */
			ifr->ifr_mtu = ifp->if_mtu;
			break;
		case SIOCSIFFLAGS:
			ifp->if_flags = ifr->ifr_flags;
			/* restart card with new settings... */
			break;
		case SIOCSIFMETRIC:
			/* set interface metric */
			ifp->if_metric = ifr->ifr_metric;
			break;
		default:
			if (so->so_proto == NULL)
				return EOPNOTSUPP;
			return (*so->so_proto->pr_userreq)(so, PRU_CONTROL,
				(struct mbuf*)cmd, (struct mbuf*)data, (struct mbuf*)ifp);
				
	}
	
	
	return 0;
}
	
struct ifnet *ifunit(char *name)
{
	ifnet *d = devices;

	for (d=devices;d;d = d->if_next)
		if (strcmp(d->if_name, name) == 0)
			return d;
	return NULL;
}

void dump_sockaddr(void *ptr)
{
	struct sockaddr *sa = (struct sockaddr *)ptr;
	uint8 *d = NULL;
	int i;

	switch (sa->sa_family) {
		case AF_LINK: {
			struct sockaddr_dl *sdl = (struct sockaddr_dl *)ptr;
			if (sdl->sdl_type == IFT_ETHER) {
				printf("\t\tETHERNET: ");
				printf("Interface ");
				d = (unsigned char *)&sdl->sdl_data[0];
				for (i=0;i<sdl->sdl_nlen;i++, d++) {
					printf("%c", *d);
				}
				printf(" -> ");
				for (i=0;i<sdl->sdl_alen;i++, d++) {
					printf("%02x", *d);
					if (i< 5)
						printf(":");
				}
				printf("\n");
			}
			break;
		}
		case AF_INET: {
			struct sockaddr_in *sin = (struct sockaddr_in *)ptr;
			struct in_addr ho;
			ho.s_addr = sin->sin_addr.s_addr;
			printf("\t\tIPv4: ");
			d = (uint8*)&ho.s_addr;
			for (i=0;i<4;i++, d++) {
				printf("%d", *d);
				if (i < 3)
					printf(".");
			}
			printf("\n");
			break;
		}
		default:
			printf("Unknown type... %d\n", sa->sa_family);
	}
}

void *protocol_address(struct ifnet *ifa, int family)
{
	struct ifaddr *a = ifa->if_addrlist;

	for (; a != NULL; a = a->ifa_next) {
		if (a->ifa_addr->sa_family == family) {
			if (family == AF_INET) {
				return &((struct sockaddr_in*)a->ifa_addr)->sin_addr;
			} else {
				return &a->ifa_addr->sa_data;
			}
		}
	}
	return NULL;
}

#define equal(a1, a2) \
  (memcmp((caddr_t)(a1), (caddr_t)(a2), ((struct sockaddr *)(a1))->sa_len) == 0)

/*
 * Find an interface address specific to an interface best matching
 * a given address.
 */
struct ifaddr *ifaof_ifpforaddr(struct sockaddr *addr, 
				struct ifnet *ifp)
{
	struct ifaddr *ifa;
	char *cp, *cp2, *cp3;
	char *cplim;
	struct ifaddr *ifa_maybe = 0;
        uint af = addr->sa_family;

        if (af >= AF_MAX)
                return (NULL);

        for (ifa = ifp->if_addrlist; ifa != NULL; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr->sa_family != af)
                        continue;
                ifa_maybe = ifa;
                if (ifa->ifa_netmask == 0) {
                        if (equal(addr, ifa->ifa_addr) ||
                            (ifa->ifa_dstaddr && equal(addr, ifa->ifa_dstaddr)))
                                return (ifa);
                        continue;
                }
                cp = (char *)addr->sa_data;
                cp2 = (char *)ifa->ifa_addr->sa_data;
                cp3 = (char *)ifa->ifa_netmask->sa_data;
                cplim = ifa->ifa_netmask->sa_len + (char *)ifa->ifa_netmask;
                for (; cp3 < cplim; cp3++)
                        if ((*cp++ ^ *cp2++) & *cp3)
                                break;
                if (cp3 == cplim)
                        return (ifa);
        }
        return (ifa_maybe);
}

struct ifaddr *ifa_ifwithdstaddr(struct sockaddr *addr)
{
	struct ifnet *ifp;
	struct ifaddr *ifa;

        for (ifp = devices; ifp != NULL; ifp = ifp->if_next)
            if (ifp->if_flags & IFF_POINTOPOINT)
                for (ifa = ifp->if_addrlist; ifa != NULL; ifa = ifa->ifa_next) {
                        if (ifa->ifa_addr->sa_family != addr->sa_family ||
                            ifa->ifa_dstaddr == NULL)
                                continue;
                        if (equal(addr, ifa->ifa_dstaddr))
                                return (ifa);
        }
        return (NULL);
}

struct ifaddr *ifa_ifwithaddr(struct sockaddr *addr)
{
	struct ifnet *ifp;
	struct ifaddr *ifa;

        for (ifp = devices; ifp != NULL; ifp = ifp->if_next) {
            for (ifa = ifp->if_addrlist; ifa != NULL; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr->sa_family != addr->sa_family)
                        continue;
                if (equal(addr, ifa->ifa_addr))
                        return (ifa);
                if ((ifp->if_flags & IFF_BROADCAST) && ifa->ifa_broadaddr &&
                    /* IPv6 doesn't have broadcast */
                    ifa->ifa_broadaddr->sa_len != 0 &&
                    equal(ifa->ifa_broadaddr, addr))
                        return (ifa);
        	}
        }
        return (NULL);
}

/*
 * Find an interface on a specific network.  If many, choice
 * is most specific found.
 */
struct ifaddr *ifa_ifwithnet(struct sockaddr *addr)
{
	struct ifnet *ifp;
	struct ifaddr *ifa;
	struct ifaddr *ifa_maybe = NULL;
	uint af = addr->sa_family;
	char *addr_data = (char *)addr->sa_data, *cplim;

	if (af == AF_LINK) {
		struct sockaddr_dl *sdl = (struct sockaddr_dl *)addr;
		if (sdl->sdl_index && sdl->sdl_index <= ndevs)
                	return (ifnet_addrs[sdl->sdl_index]);
	}
        for (ifp = devices; ifp != NULL; ifp = ifp->if_next)
                for (ifa = ifp->if_addrlist; ifa != NULL; ifa = ifa->ifa_next) {
                        register char *cp, *cp2, *cp3;

                        if (ifa->ifa_addr->sa_family != af ||
                            ifa->ifa_netmask == 0)
next: 
								continue;
                        cp = addr_data;
                        cp2 = (char *)ifa->ifa_addr->sa_data;
                        cp3 = (char *)ifa->ifa_netmask->sa_data;
                        cplim = (char *)ifa->ifa_netmask +
                                ifa->ifa_netmask->sa_len;
                        while (cp3 < cplim)
                                if ((*cp++ ^ *cp2++) & *cp3++)
                                    /* want to continue for() loop */
                                        goto next;
                        if (ifa_maybe == 0 ||
                            rn_refines((caddr_t)ifa->ifa_netmask,
                            (caddr_t)ifa_maybe->ifa_netmask))
                                ifa_maybe = ifa;
                }
        return (ifa_maybe);
}

/* XXX - we have a memory leak here! When we clean up we need to free the memory
 *       that is malloc'd here
 */
void if_attach(struct ifnet *ifp)
{
	uint socksize, ifasize;
	int namelen, masklen;
	struct ifnet **p = &devices;
	struct sockaddr_dl *sdl;
	struct ifaddr *ifa;
	char dname[IFNAMSIZ];
	
	if (!ifp)
		return;

	sprintf(dname, "%s%d", ifp->name, ifp->if_unit);
	ifp->if_name = strdup(dname);

	while (*p)
		p = &((*p)->if_next);
	
	*p = ifp;
	ifp->if_index = ++if_index; /* atomic add ? */

	/* allocate memory for ifnet_addrs if required... */
	if (ifnet_addrs == NULL || if_index >= if_indexlim) {
		uint n = (if_indexlim <<= 1) * sizeof(*ifa);
		struct ifaddr **q = (struct ifaddr**)malloc(n);
		if (ifnet_addrs) {
			memcpy((caddr_t)q, (caddr_t)ifnet_addrs, n / 2);
			free(ifnet_addrs);
		}
		ifnet_addrs = q;
	}

	/* get the unit # as a string */
	namelen = strlen(ifp->if_name);

	/* memory: we need to allocate enough memory for the following...
	 * struct ifaddr
	 * struct sockaddr_dl that will hold the link level address and name
	 * struct sockaddr_dl that will hold the mask
	 */

#define _offsetof(t, m) ((int)((caddr_t)&((t *)0)->m))
	masklen = _offsetof(struct sockaddr_dl, sdl_data[0]) + namelen;
	socksize = masklen + ifp->if_addrlen;
#define ROUNDUP(a) (1 + (((a) - 1) | (sizeof(int32) -1)))
	socksize = ROUNDUP(socksize);

	if (socksize < sizeof(*sdl))
		socksize = sizeof(*sdl);
	ifasize = sizeof(*ifa) + 2 * socksize;
	if ((ifa = (struct ifaddr*)malloc(ifasize))) {
		memset(ifa, 0, ifasize);

		sdl = (struct sockaddr_dl *)(ifa + 1);
		sdl->sdl_len = socksize;
		sdl->sdl_family = AF_LINK;
		memcpy(&sdl->sdl_data, ifp->if_name, namelen);
		sdl->sdl_nlen = namelen;
		sdl->sdl_index = ifp->if_index;
		sdl->sdl_type = ifp->if_type;
		ifnet_addrs[if_index - 1] = ifa;
		ifa->ifa_ifp = ifp;
		ifa->ifa_next = ifp->if_addrlist;
		ifp->if_addrlist = ifa;
		ifa->ifa_addr = (struct sockaddr*)sdl;

		/* now do mask... */
		sdl = (struct sockaddr_dl *)(socksize + (caddr_t)sdl);
		ifa->ifa_netmask = (struct sockaddr*)sdl;
		sdl->sdl_len = masklen;
		/* build the mask */
		while (namelen != 0)
			sdl->sdl_data[--namelen] = 0xff;
	}
}

void if_detach(struct ifnet *ifp)
{
	struct ifnet **p = &devices, *q;

	for (; (*p)->if_next != ifp ; (*p) = (*p)->if_next)
		continue;
	q = (*p)->if_next->if_next;
	(*p)->if_next = q;
}
	
/* XXX - memcpy used as copyin / copyout not available. I did look
 * for the source for them but was unable to find them in the
 * code jungle that is OpenBSD and FreeBSD!
 *
 * copyin / copyout should maybe be added if they'd add a speed improvement as
 * they're used in a lot of other places as well
 */
int ifconf(int cmd, caddr_t data)
{
	struct ifconf *ifc = (struct ifconf*)data;
	struct ifnet *ifp = devices;
	struct ifaddr *ifa = NULL;
	char *cp, *ep;
	struct ifreq ifr, *ifrp;
	int space = ifc->ifc_len; /* how big the buffer is */
	void *copyptr = NULL;
	
	ifrp = ifc->ifc_req;
	ep = ifr.ifr_name + sizeof(ifr.ifr_name) - 2;
	
	for (; space > sizeof(ifr) && ifp; ifp = ifp->if_next) {
		strncpy(ifr.ifr_name, ifp->if_name, sizeof(ifr.ifr_name) - 2);
		for (cp = ifr.ifr_name;cp < ep && *cp; cp++)
			continue;
		*cp = '\0';
		if ((ifa = ifp->if_addrlist) == NULL) {
			memset((caddr_t)&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
			copyptr = memcpy((caddr_t) ifrp, (caddr_t) &ifr, sizeof(ifr));
			if (copyptr == NULL)
				break;
			space -= sizeof(ifr), ifrp++;
		} else {
			for (; space > sizeof(ifr) && ifa; ifa = ifa->ifa_next) {
				struct sockaddr *sa = ifa->ifa_addr;
				if (sa->sa_len <= sizeof(*sa)) {
					printf("sa->sa_len = %d compared to %ld, sa->sa_family = %d\n", 
						sa->sa_len, sizeof(*sa), sa->sa_family);
						
					ifr.ifr_addr = *sa;
					copyptr = memcpy((caddr_t)ifrp, (caddr_t)&ifr, sizeof(ifr));
					ifrp++;
				} else {
					space -= sa->sa_len - sizeof(*sa);
					if (space < sizeof(ifr))
						break;
					copyptr = memcpy((caddr_t)ifrp, (caddr_t)&ifr, sizeof(ifr.ifr_name));
					if (copyptr != NULL)
						copyptr = memcpy((caddr_t)&ifrp->ifr_addr, (caddr_t)sa, sa->sa_len);
					ifrp = (struct ifreq*)(sa->sa_len + (caddr_t)&ifrp->ifr_addr);
				}
				if (copyptr == NULL)
					break;
				space -= sizeof(ifr);
			}
		}
	}
	ifc->ifc_len -= space;
	/* Yuck! */
	return (copyptr == NULL ? -1 : 0);
}
