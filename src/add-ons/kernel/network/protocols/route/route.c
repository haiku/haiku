/* route.c */

#ifndef _KERNEL_MODE
#include <kernel/OS.h>
#include <stdio.h>
#endif

#include "net_malloc.h"

#include "net/route.h" /* includes net/radix.h */
#include "protocols.h"
#include "net/if.h"
#include "sys/protosw.h"
#include "sys/domain.h"
#include "net/raw_cb.h"
#include "sys/socketvar.h"

#include "core_module.h"
#include "net_module.h"
#include "core_funcs.h"

#include <KernelExport.h>
status_t std_ops(int32 op, ...);
#define NET_ROUTE_MODULE_NAME	NETWORK_MODULES_ROOT "protocols/route"

static struct core_module_info *core = NULL;
static struct rawcb rawcb;
static struct route_cb route_cb;
static struct pool_ctl *rawcbpool = NULL;

static int32 raw_sendspace = 8192;
static int32 raw_recvspace = 8192;

static struct sockaddr route_src = { 2, PF_ROUTE, };
static struct sockaddr route_dst = { 2, PF_ROUTE, };
static struct sockproto route_proto = { PF_ROUTE, };

#define ROUNDUP(a) (a >0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

/* BSD used these, we don't, we expand them fully! */
//#define dst     info.rti_info[RTAX_DST]
//#define gate    info.rti_info[RTAX_GATEWAY]
//#define netmask info.rti_info[RTAX_NETMASK]
//#define genmask info.rti_info[RTAX_GENMASK]
//#define ifpaddr info.rti_info[RTAX_IFP]
//#define ifaaddr info.rti_info[RTAX_IFA]
//#define brdaddr info.rti_info[RTAX_BRD]

/* Routing socket support */

static void rt_setmetrics(uint32 which, struct rt_metrics *in, 
                   struct rt_metrics *out)
{
#define metric(f, e) if (which & f) out->e = in->e;
	metric(RTV_RPIPE, rmx_recvpipe);
#undef metric
}

static void rt_xaddrs(caddr_t cp, caddr_t cplim, struct rt_addrinfo *rtinfo)
{
	struct sockaddr *sa;
	int i;
	
	memset(rtinfo->rti_info, 0, sizeof(rtinfo->rti_info));
	for (i = 0;(i < RTAX_MAX) && (cp < cplim);i++) {
		if ((rtinfo->rti_addrs & (1 << i)) == 0)
			continue;
		rtinfo->rti_info[i] = sa = (struct sockaddr*)cp;
		ADVANCE(cp, sa);
	}
}

static int rt_msg2(int type, struct rt_addrinfo *rtinfo, caddr_t cp,
                   struct walkarg *w)
{
	int i;
	int len, dlen, second_time = 0;
	caddr_t cp0;
	
	rtinfo->rti_addrs = 0;
again:
	switch(type) {
		case RTM_DELADDR:
		case RTM_NEWADDR:
			len = sizeof(struct ifa_msghdr);
			break;
		case RTM_IFINFO:
			len = sizeof(struct if_msghdr);
			break;
		default:
			len = sizeof(struct rt_msghdr);
	}
	
	if ((cp0 = cp))
		cp += len;
	for (i=0; i < RTAX_MAX; i++) {
		struct sockaddr *sa;
		
		if ((sa = rtinfo->rti_info[i]) == NULL)
			continue;
		rtinfo->rti_addrs |= (1 << i);
		dlen = ROUNDUP(sa->sa_len);
		if (cp) {
			memcpy(cp, (caddr_t)sa, dlen);
			cp += dlen;
		}
		len += dlen;
	}
	
	if (cp == NULL && w!= NULL && !second_time) {
		struct walkarg *rw = w;
		
		rw->w_needed += len;
		if (rw->w_needed <= 0 && rw->w_where) {
			if (rw->w_tmemsize < len) {
				if (rw->w_tmem)
					free(rw->w_tmem);
				if ((rw->w_tmem = (caddr_t)malloc(len))) {
					memset(rw->w_tmem, 0, len);
					rw->w_tmemsize = len;
				}
			}
			if (rw->w_tmem) {
				cp = rw->w_tmem;
				second_time = 1;
				goto again;
			} else
				rw->w_where = NULL;
		}
	}
	if (cp) {
		struct rt_msghdr *rtm = (struct rt_msghdr*)cp0;
		rtm->rtm_version = RTM_VERSION;
		rtm->rtm_type = type;
		rtm->rtm_msglen = len;
	}
	return len;
}

static void raw_init(void)
{
	rawcb.rcb_next = rawcb.rcb_prev = &rawcb;
	memset(&route_cb, 0, sizeof(struct route_cb));
	if (!rawcbpool)
		pool_init(&rawcbpool, sizeof(struct rawcb));
}

static int raw_attach(struct socket *so, int proto)
{
	struct rawcb *rp = sotorawcb(so);
	int error;
	
	if (rp == NULL)
		return ENOBUFS;
	if ((error = soreserve(so, raw_sendspace, raw_recvspace)))
		return error;
	rp->rcb_socket = so;
	rp->rcb_proto.sp_family = so->so_proto->pr_domain->dom_family;
	rp->rcb_proto.sp_protocol = proto;

	rp->rcb_prev = rawcb.rcb_next->rcb_prev;
	rp->rcb_next = rawcb.rcb_next;
	rawcb.rcb_next->rcb_prev = rp;
	rawcb.rcb_next = rp;
	
	return 0;
}

static int raw_detach(struct rawcb *rp)
{
	struct socket *so = rp->rcb_socket;
	
	so->so_pcb = NULL;
	rp->rcb_prev->rcb_next = rp->rcb_next;
	rp->rcb_next->rcb_prev = rp->rcb_prev;

	pool_put(rawcbpool, rp);
	return 0;
}

static int raw_input(struct mbuf *m0, struct sockproto *proto, struct sockaddr *src,
              struct sockaddr *dst)
{
	struct rawcb *rp;
	struct mbuf *m = m0;
	int sockets = 0;
	struct socket *last;
	
	last = NULL;
	for (rp = rawcb.rcb_next; rp != &rawcb; rp = rp->rcb_next) {
		if (rp->rcb_proto.sp_family != proto->sp_family)
			continue;
		if (rp->rcb_proto.sp_protocol &&
		    rp->rcb_proto.sp_protocol != proto->sp_protocol)
			continue;
#define equal(a1, a2) (memcmp((caddr_t)a1, (caddr_t)a2, a1->sa_len) == 0)
		if (rp->rcb_laddr && !equal(rp->rcb_laddr, dst))
			continue;
		if (rp->rcb_faddr && !equal(rp->rcb_faddr, src))
			continue;
		if (last) {
			struct mbuf *n;
			if ((n = m_copym(m, 0, M_COPYALL))) {
				if (sockbuf_appendaddr(&last->so_rcv, src, n, NULL) == 0)
					m_freem(n);
				else {
					sorwakeup(last);
					sockets++;
				}
			}
		}
		last = rp->rcb_socket;
	}
	if (last) {
		if (sockbuf_appendaddr(&last->so_rcv, src, m, NULL) == 0)
			m_freem(m);
		else {
			sorwakeup(last);
			sockets++;
		}
	} else 
		m_freem(m);
	return 0;
}

static int route_output(struct mbuf *m, struct socket *so)
{
	struct rt_msghdr *rtm = NULL;
	struct rtentry *rt = NULL;
	struct rtentry *saved_nrt = NULL;
	struct rt_addrinfo info;
	int len;
	int error = 0;
	struct ifnet *ifp = NULL;
	struct ifaddr *ifa = NULL;
	
#define snderr(e) { error = e; goto flush; }

	if (m == NULL || ((m->m_len < sizeof(int32)) &&
	    ((m = m_pullup(m, sizeof(int32))) == NULL))) {
		printf("route_output: ENOBUFS (m_pullup)\n");
		if (m)
			printf("m = %p, m_len = %ld\n", m, m->m_len);
		return ENOBUFS;
	}	
	if ((m->m_flags & M_PKTHDR) == 0) {
		printf("route_output: Not packet header\n");
		return -1;
	}
	len = m->m_pkthdr.len;
	if (len < (int) sizeof(*rtm) || len != (mtod(m, struct rt_msghdr*))->rtm_msglen) {
		info.rti_info[RTAX_DST] = NULL;
		printf("route_output: EINVAL\n");
		snderr(EINVAL);
	}
	R_Malloc(rtm, struct rt_msghdr *, len);
	if (rtm == NULL) {
		info.rti_info[RTAX_DST] = NULL;
		printf("route_output: ENOBUFS (rtm == NULL)\n");
		snderr(ENOBUFS);
	}
	m_copydata(m, 0, len, (caddr_t)rtm);
	if (rtm->rtm_version != RTM_VERSION) {
		info.rti_info[RTAX_DST] = NULL;
		printf("route_output: EPROTONOSUPPORT\n");
		snderr(EPROTONOSUPPORT);
	}
	
	info.rti_addrs = rtm->rtm_addrs;
	rt_xaddrs((caddr_t)(rtm + 1), len + (caddr_t)rtm, &info);
	
	if (info.rti_info[RTAX_DST] == NULL)
		snderr(EINVAL);

	if (info.rti_info[RTAX_GENMASK]) {
		struct radix_node *t;
		t = rn_addmask((caddr_t)info.rti_info[RTAX_GENMASK], 1, 2);
		if (t && Bcmp(info.rti_info[RTAX_GENMASK], t->rn_key, *(uint8*)info.rti_info[RTAX_GENMASK]) == 0)
			info.rti_info[RTAX_GENMASK] = (struct sockaddr *)t->rn_key;
		else
			snderr(ENOBUFS);
	}
	
	switch (rtm->rtm_type) {
		case RTM_ADD:
			if (info.rti_info[RTAX_GATEWAY] == NULL) {
				printf("route_output: EINVAL\n");
				snderr(EINVAL);
			}
			error = rtrequest(RTM_ADD, info.rti_info[RTAX_DST], info.rti_info[RTAX_GATEWAY],
			                  info.rti_info[RTAX_NETMASK], rtm->rtm_flags, &saved_nrt);
			if (error == 0 && saved_nrt) {
				rt_setmetrics(rtm->rtm_inits, &rtm->rtm_rmx, &saved_nrt->rt_rmx);
				saved_nrt->rt_refcnt--;
				saved_nrt->rt_genmask = info.rti_info[RTAX_GENMASK];
			}
			break;
		case RTM_DELETE:
			error = rtrequest(RTM_DELETE, info.rti_info[RTAX_DST], info.rti_info[RTAX_GATEWAY],
			                  info.rti_info[RTAX_NETMASK], rtm->rtm_flags, NULL);
			break;
		case RTM_GET:
		case RTM_CHANGE:
		case RTM_LOCK:
			rt = rtalloc1(info.rti_info[RTAX_DST], 0);
			if (rt == NULL) {
				printf("route_output: ESRCH\n");
				snderr(ESRCH);
			}
			if (rtm->rtm_type != RTM_GET) {
				struct radix_node *rn;
				if (Bcmp(info.rti_info[RTAX_DST], rt_key(rt), 
				         info.rti_info[RTAX_DST]->sa_len) != 0)
					snderr(ESRCH);
				if (info.rti_info[RTAX_NETMASK] &&
				    (rn = rn_head_search(info.rti_info[RTAX_NETMASK])))
					info.rti_info[RTAX_NETMASK] = (struct sockaddr*)rn->rn_key;
				for (rn = rt->rt_nodes; rn; rn = rn->rn_dupedkey)
					if (info.rti_info[RTAX_NETMASK] == (struct sockaddr*)rn->rn_mask)
						break;
				if (rn == NULL) {
					printf("route_output: ETOOMANYREFS\n");
					snderr(EINVAL);//ETOOMANYREFS;
				}
				rt = (struct rtentry *)rn;
			}
			switch (rtm->rtm_type) {
				case RTM_GET:
					info.rti_info[RTAX_DST] = rt_key(rt);
					info.rti_info[RTAX_GATEWAY] = rt->rt_gateway;
					info.rti_info[RTAX_NETMASK] = rt_mask(rt);
					info.rti_info[RTAX_GENMASK] = rt->rt_genmask;
					if (rtm->rtm_addrs && (RTA_IFP | RTA_IFA)) {
						if ((ifp = rt->rt_ifp)) {
							info.rti_info[RTAX_IFA] = ifp->if_addrlist->ifa_addr;
							info.rti_info[RTAX_IFA] = rt->rt_ifa->ifa_addr;
							rtm->rtm_index = ifp->if_index;
						} else {
							info.rti_info[RTAX_IFA] = NULL;
							info.rti_info[RTAX_IFA] = NULL;
						}
					}
					len = rt_msg2(RTM_GET, &info, NULL, NULL);
					if (len > rtm->rtm_msglen) {
						struct rt_msghdr *new_rtm ;
						R_Malloc(new_rtm, struct rt_msghdr *, len);
						if (new_rtm == NULL)
							snderr(ENOBUFS);
						Bcopy(rtm, new_rtm, rtm->rtm_msglen);
						Free(rtm);
						rtm = new_rtm;
					}
					(void)rt_msg2(RTM_GET, &info, (caddr_t)rtm, NULL);
					rtm->rtm_flags = rt->rt_flags;
					rtm->rtm_rmx = rt->rt_rmx;
					rtm->rtm_addrs = info.rti_addrs;
					break;
				case RTM_CHANGE:
					if (info.rti_info[RTAX_GATEWAY] && rt_setgate(rt, rt_key(rt), info.rti_info[RTAX_GATEWAY])) {
						printf("route_output: EDQUOT\n");
						snderr(EINVAL); //EDQUOT ???
					}
					/* new gateway could require a new ifaddr and ifp, flags might also
					 * be different. ifp may be specified by link level sockaddr when
					 * protocol address is ambiguous...
					 */
					if (info.rti_info[RTAX_IFA] && 
					    (ifa = ifa_ifwithnet(info.rti_info[RTAX_IFA])) &&
					    (ifp = ifa->ifa_ifp))
						ifa = ifaof_ifpforaddr(info.rti_info[RTAX_IFA] ?
						                        info.rti_info[RTAX_IFA] : 
						                        info.rti_info[RTAX_GATEWAY],
						                       ifp);
					else if ((info.rti_info[RTAX_IFA] && (ifa = ifa_ifwithaddr(info.rti_info[RTAX_IFA]))) ||
					         (ifa = ifa_ifwithroute(rt->rt_flags, rt_key(rt), info.rti_info[RTAX_GATEWAY])))
						ifp = ifa->ifa_ifp;
					if (ifa) {
						struct ifaddr *oifa = rt->rt_ifa;
						if (oifa != ifa) {
							if (oifa && oifa->ifa_rtrequest)
								oifa->ifa_rtrequest(RTM_DELETE, rt, info.rti_info[RTAX_GATEWAY]);
							IFAFREE(rt->rt_ifa);
							rt->rt_ifa = ifa;
							ifa->ifa_refcnt++;
							rt->rt_ifp = ifp;
						}
					}
					rt_setmetrics(rtm->rtm_inits, &rtm->rtm_rmx, &rt->rt_rmx);
					if (rt->rt_ifa && rt->rt_ifa->ifa_rtrequest)
						rt->rt_ifa->ifa_rtrequest(RTM_ADD, rt, info.rti_info[RTAX_GATEWAY]);
					if (info.rti_info[RTAX_GENMASK])
						rt->rt_genmask = info.rti_info[RTAX_GENMASK];
					/* fall through... */
				case RTM_LOCK:
					rt->rt_rmx.rmx_locks &= ~(rtm->rtm_inits);
					rt->rt_rmx.rmx_locks |= (rtm->rtm_inits & rtm->rtm_rmx.rmx_locks);
					break;
			}
			break;
		default:
			printf("route_output: EOPNOTSUPP\n");
			snderr(EINVAL); //EOPNOTSUPP;
	}
	
flush:	
	if (rtm) {
		if (error)
			rtm->rtm_errno = error;
		else
			rtm->rtm_flags |= RTF_DONE;
	}
	if (rt)
		rtfree(rt);
	{
		struct rawcb *rp = NULL;
		/* check if it's our own message! */
		if ((so->so_options & SO_USELOOPBACK) == 0) {
			if (route_cb.any_count <= 1) {
				if (rtm)
					Free(rtm);
				m_freem(m);
				return error;
			}
			rp = sotorawcb(so);
		}
		if (rtm) {
			m_copyback(m, 0, rtm->rtm_msglen, (caddr_t)rtm);
			Free(rtm);
		}
		if (rp)
			rp->rcb_proto.sp_family = 0;
		if (info.rti_info[RTAX_DST])
			route_proto.sp_protocol = info.rti_info[RTAX_DST]->sa_family;
		raw_input(m, &route_proto, &route_src, &route_dst);
		if (rp)
			rp->rcb_proto.sp_family = PF_ROUTE;
	}
	return error;
}

static int sysctl_dumpentry(struct radix_node *rn, void *data)
{
	struct rtentry *rt = (struct rtentry *)rn;
	int error = 0, size;
	struct rt_addrinfo info;
	struct walkarg *w = (struct walkarg *)data;
	
	if (w->w_op == NET_RT_FLAGS && !(rt->rt_flags & w->w_arg))
		return 0;
	memset((caddr_t)&info, 0, sizeof(info));
	info.rti_info[RTAX_DST] = rt_key(rt);
	info.rti_info[RTAX_GATEWAY] = rt->rt_gateway;
	info.rti_info[RTAX_NETMASK] = rt_mask(rt);
	info.rti_info[RTAX_GENMASK] = rt->rt_genmask;
	size = rt_msg2(RTM_GET, &info, 0, w);
	if (w->w_where && w->w_tmem) {
		struct rt_msghdr *rtm = (struct rt_msghdr*)w->w_tmem;
		
		rtm->rtm_flags = rt->rt_flags;
		rtm->rtm_use = rt->rt_use;
		rtm->rtm_rmx = rt->rt_rmx;
		rtm->rtm_index = rt->rt_ifp->if_index;
		rtm->rtm_errno = rtm->rtm_seq = 0;
		rtm->rtm_addrs = info.rti_addrs;
		if (memcpy(w->w_where, (caddr_t)rtm, size) == NULL) {
			error = ENOMEM;
			w->w_where = NULL;
		} else
			w->w_where += size;
	}
	return error;
}

static int sysctl_iflist(int af, struct walkarg *w)
{
	struct ifnet *ifp;
	struct ifaddr *ifa;
	struct rt_addrinfo info;
	int len;
	struct ifnet *interfaces = get_interfaces();

	memset(&info, 0, sizeof(info));
	for (ifp = interfaces; ifp; ifp = ifp->if_next) {
		if (w->w_arg && w->w_arg != ifp->if_index)
			continue;
		ifa = ifp->if_addrlist;
		info.rti_info[RTAX_IFA] = ifa->ifa_addr;
		len = rt_msg2(RTM_IFINFO, &info, NULL, w);
		info.rti_info[RTAX_IFA] = NULL;
		if (w->w_where && w->w_tmem) {
			struct if_msghdr *ifm;
			
			ifm = (struct if_msghdr*)w->w_tmem;
			ifm->ifm_index = ifp->if_index;		
			ifm->ifm_flags = ifp->if_flags;
			ifm->ifm_data = ifp->ifd;
			ifm->ifm_addrs = info.rti_addrs;
			if (memcpy(w->w_where, (caddr_t)ifm, len) == NULL)
				return ENOMEM;
			w->w_where += len;
		}
		while ((ifa = ifa->ifa_next) != NULL) {
			if (af && af != ifa->ifa_addr->sa_family)
				continue;
			info.rti_info[RTAX_IFA] = ifa->ifa_addr;
			info.rti_info[RTAX_NETMASK] = ifa->ifa_netmask;
			info.rti_info[RTAX_BRD] = ifa->ifa_dstaddr;
			len = rt_msg2(RTM_NEWADDR, &info, NULL, w);
			if (w->w_where && w->w_tmem) {
				struct ifa_msghdr *ifam;
				
				ifam = (struct ifa_msghdr *)w->w_tmem;
				ifam->ifam_index = ifa->ifa_ifp->if_index;
				ifam->ifam_flags = ifa->ifa_flags;
				ifam->ifam_metric = ifa->ifa_metric;
				ifam->ifam_addrs = info.rti_addrs;
				if (memcpy(w->w_where, w->w_tmem, len) == NULL)
					return ENOMEM;
				w->w_where += len;
			}
		}
		info.rti_info[RTAX_IFA] = info.rti_info[RTAX_NETMASK] = info.rti_info[RTAX_BRD] = NULL;
	}
	return 0;
}

static int sysctl_rtable(int *name, uint namelen, void *where, size_t *given, void *newp, 
                  size_t newlen) 
{
	int i, error = EINVAL;
	uchar af;
	struct walkarg w;
	struct radix_node_head *rnh;
	struct radix_node_head **rt_tables;

	if (newp)
		return EPERM;
	
	if (namelen != 3) {
		printf("sysctl_rtable: EINVAL (namelen != 3)\n");
		return EINVAL;
	}

	rt_tables = get_rt_tables();

	af = name[0];
	memset(&w, 0, sizeof(w));
	w.w_where = where;
	w.w_given = *given;
	w.w_needed = 0 - w.w_given;
	w.w_op = name[1];
	w.w_arg = name[2];
	
	switch(w.w_op) {
		case NET_RT_DUMP:
		case NET_RT_FLAGS:
			for(i=1 ; i <= AF_MAX; i++) {
				rnh = rt_tables[i];
				if (rnh && (af == 0 || af == i) &&
				    (error = rnh->rnh_walktree(rnh, sysctl_dumpentry, &w)))
					break;
			}
			break;
		case NET_RT_IFLIST:
			error = sysctl_iflist(af, &w);
	}
	
	if (w.w_tmem)
		free(w.w_tmem);
	w.w_needed += w.w_given;
	if (where) {
		*given = (caddr_t)w.w_where - (caddr_t)where;
		if (*given < (size_t) w.w_needed) {
			printf("sysctl_rtable: ENOMEM\n");
			return ENOMEM;
		}
	} else {
		*given = (11 * w.w_needed) / 10;
	}
	return error;
}
 
static int raw_userreq(struct socket *so, int req, struct mbuf *m, struct mbuf *nam,
                struct mbuf *control)
{
	struct rawcb *rp = sotorawcb(so);
	int error = 0;
	
	if (req == PRU_CONTROL) {
		return EINVAL;//EOPNOTSUPP;
	}
	if (control && control->m_len) {
		error = EINVAL;//EOPNOTSUPP;
		goto release;
	}
	if (rp == NULL) {
		error = EINVAL;
		goto release;
	}
	
	switch(req) {
		case PRU_ATTACH:
			error = raw_attach(so, (int) nam);
			break;
		case PRU_DETACH:
			if (rp == NULL) {
				error = ENOTCONN;
				break;
			}
			raw_detach(rp);
			break;
		case PRU_CONNECT2:
			error = EOPNOTSUPP;
			break;
		case PRU_DISCONNECT:
			if (rp->rcb_faddr == NULL) {
				error = ENOTCONN;
				break;
			}
			raw_detach(rp);
			socket_set_disconnected(so);
			break;
		case PRU_SHUTDOWN:
			socket_set_cantsendmore(so);
			break;
		case PRU_LISTEN:
		case PRU_ACCEPT:
		case PRU_SENDOOB:
			error = EOPNOTSUPP;
			break;
		case PRU_ABORT:
			raw_detach(rp);
			socket_set_disconnected(so);
			break;
		case PRU_SEND:
			if (nam) {
				if (rp->rcb_faddr) {
					error = EISCONN;
					break;
				}
				rp->rcb_faddr = mtod(nam, struct sockaddr *);
			} else if (rp->rcb_faddr == NULL) {
				error = ENOTCONN;
				break;
			}
			/* XXX - Hmm, should we be hardcoding this??? */
			error = route_output(m, so);
			m = NULL;
			if (nam)
				rp->rcb_faddr = NULL;
			break;
		default:
			printf("raw_userreq: unsupported request.\n");
	}
	
release:
	if (m != NULL)
		m_freem(m);
	return error;
}

static int route_userreq(struct socket *so, int req, struct mbuf *m, struct mbuf *nam,
                  struct mbuf *control)
{
	int error = 0;
	struct rawcb *rp = sotorawcb(so);
	
	if (req == PRU_ATTACH) {
		rp = (struct rawcb*)pool_get(rawcbpool);
		if ((so->so_pcb = (caddr_t)rp))
			memset(so->so_pcb, 0, sizeof(*rp));
	}
	if (req == PRU_DETACH && rp) {
		int af = rp->rcb_proto.sp_protocol;
		if (af == AF_INET)
			route_cb.ip_count--;
		route_cb.any_count--;
	}

	error = raw_userreq(so, req, m, nam, control);
	rp = sotorawcb(so);
	if (req == PRU_ATTACH && rp) {
		int af = rp->rcb_proto.sp_protocol;
		if (error) {
			pool_put(rawcbpool, rp);
			return error;
		}
		if (af == AF_INET)
			route_cb.ip_count++;
		route_cb.any_count++;
		
		rp->rcb_faddr = &route_src;
		socket_set_connected(so);
		so->so_options |= SO_USELOOPBACK;
	}
	return error;
}		
			


static struct protosw my_protocol = {
	"Routing Sockets",
	NET_ROUTE_MODULE_NAME,
	SOCK_RAW,
	NULL,
	0,
	PR_ATOMIC | PR_ADDR,
	NET_LAYER4,
	
	&raw_init,
	NULL,
	NULL,
	&route_userreq,
	&sysctl_rtable,                    /* pr_sysctl */
	NULL,
	NULL,                              /* pr_ctloutput */
	
	NULL,
	NULL
};

static struct domain pf_route_domain = {
	PF_ROUTE,
	"route",
	NULL,
	NULL,
	NULL,
	NULL,
	0,
	0
};

static int route_module_init(void *cpp)
{
	if (cpp)
		core = cpp;

	add_domain(&pf_route_domain, PF_ROUTE);
	add_protocol(&my_protocol, PF_ROUTE);

	return 0;
}

static int route_module_stop(void)
{
	remove_protocol(&my_protocol);
	remove_domain(PF_ROUTE);
	return 0;
}

struct kernel_net_module_info protocol_info = {
	{
		NET_ROUTE_MODULE_NAME,
		0,
		std_ops
	},
	route_module_init,
	route_module_stop,
	NULL
};

// #pragma mark -

_EXPORT status_t std_ops(int32 op, ...)
{
	switch(op) {
		case B_MODULE_INIT:
			get_module(NET_CORE_MODULE_NAME, (module_info**) &core);
			if (!core)
				return B_ERROR;
			return B_OK;

		case B_MODULE_UNINIT:
			break;

		default:
			return B_ERROR;
	}
	return B_OK;
}

_EXPORT module_info *modules[] = {
	(module_info *)&protocol_info,
	NULL
};
