/* raw.c */

#ifndef _KERNEL_
#include <stdio.h>
#include <string.h>
#endif

#include "sys/protosw.h"
#include "sys/domain.h"
#include "sys/socket.h"
#include "netinet/in_pcb.h"
#include "netinet/in.h"
#include "netinet/in_var.h"
#include "netinet/ip_var.h"

#include "core_module.h"
#include "net_module.h"
#include "core_funcs.h"
#include "raw/raw_module.h"
#include "ipv4/ipv4_module.h"

#ifdef _KERNEL_
#include <KernelExport.h>
static status_t raw_ops(int32 op, ...);
#else	/* _KERNEL_ */
#define raw_ops NULL
static image_id ipid;
#endif

static struct core_module_info *core = NULL;
static struct ipv4_module_info *ipm = NULL;

static struct inpcb rawinpcb;
static struct sockaddr_in ripsrc;
static int rip_sendspace = 8192;
static int rip_recvspace = 8192;

void rip_init(void)
{
	rawinpcb.inp_next = rawinpcb.inp_prev = &rawinpcb;
	memset(&ripsrc, 0, sizeof(ripsrc));
	ripsrc.sin_family = AF_INET;
	ripsrc.sin_len = sizeof(ripsrc);
}

void rip_input(struct mbuf *m, int hdrlen)
{
	struct ip *ip = mtod(m, struct ip*);
	struct inpcb *inp;
	struct socket *last = NULL;
	
	ripsrc.sin_addr = ip->ip_src;
	for (inp = rawinpcb.inp_next; inp != &rawinpcb; inp=inp->inp_next) {
		if (inp->inp_ip.ip_p && inp->inp_ip.ip_p != ip->ip_p)
			continue;
		if (inp->laddr.s_addr && inp->laddr.s_addr == ip->ip_dst.s_addr)
			continue;
		if (inp->faddr.s_addr && inp->faddr.s_addr == ip->ip_src.s_addr)
			continue;
		if (last) {
			struct mbuf *n;
			if ((n = m_copym(m, 0, (int)M_COPYALL))) {
				if (sbappendaddr(&last->so_rcv, (struct sockaddr*)&ripsrc, 
				                 n, NULL) == 0)
					m_freem(n);
				else
					sorwakeup(last);
			}
		}
		last = inp->inp_socket;
	}
	if (last) {
		if (sbappendaddr(&last->so_rcv, (struct sockaddr*)&ripsrc, 
		                 m, NULL) == 0)
			m_freem(m);
		else
			sorwakeup(last);
	} else {
		m_freem(m);
		ipstat.ips_noproto++;
		ipstat.ips_delivered--;
	}
	return;
}

int rip_output(struct mbuf *m, struct socket *so, uint32 dst)
{
	struct ip *ip;
	struct inpcb *inp = sotoinpcb(so);
	struct mbuf *opts;
	int flags = (so->so_options & SO_DONTROUTE) | IP_ALLOWBROADCAST;

	if ((inp->inp_flags & INP_HDRINCL) == 0) {
		M_PREPEND(m, sizeof(struct ip));
		ip = mtod(m, struct ip *);
		ip->ip_p = inp->inp_ip.ip_p;
		ip->ip_len = m->m_pkthdr.len;
		ip->ip_src = inp->laddr;
		ip->ip_dst.s_addr = dst;
		ip->ip_ttl = MAXTTL;
		opts = inp->inp_options;
		ip->ip_off = 0;
		ip->ip_tos = 0;
	} else {
		ip = mtod(m, struct ip *);
		/* ip_output relies on having the ip->ip_len in host
		 * order...this is lame... */
		ip->ip_len = ntohs(ip->ip_len);
		if (ip->ip_id == 0)
			if (ipm)
				ip->ip_id = htons(ipm->ip_id());

		opts = NULL;
		flags |= IP_RAWOUTPUT;
		ipstat.ips_rawout++;
	}

	if (ipm) {
		return ipm->output(m, opts, &inp->inp_route, flags, NULL);
	}
	/* XXX - last arg should be inp->inp_moptions when we have multicast */

	return 0;
}

int rip_userreq(struct socket *so, int req, struct mbuf *m, struct mbuf *nam,
                struct mbuf *control)
{
	int error = 0;
	struct inpcb *inp = sotoinpcb(so);
	struct ifnet *interfaces = get_interfaces();
	
	switch(req) {
		case PRU_ATTACH:
			if (inp) {
				printf("Trying to attach to a socket already attached!\n");
				return EINVAL;
			}
			if ((error = soreserve(so, rip_sendspace, rip_recvspace)) ||
			    (error = in_pcballoc(so, &rawinpcb)))
				break;
			inp = (struct inpcb*)so->so_pcb;
			inp->inp_ip.ip_p = (int)nam;
			break;
		case PRU_DISCONNECT:
			if ((so->so_state & SS_ISCONNECTED) == 0) {
				error = ENOTCONN;
				break;
			}
		case PRU_ABORT:
			soisdisconnected(so);
		case PRU_DETACH:
			if (inp == NULL) {
				printf("Can't detach from NULL protocol block!\n");
				error = EINVAL;
				break;
			}
			in_pcbdetach(inp);
			break;
		case PRU_SEND: {
			uint32 dst;
			if ((so->so_state & SS_ISCONNECTED)) {
				if (nam) {
					error = EISCONN;
					break;
				}
				dst = inp->faddr.s_addr;
			} else {
				if (!nam) {
					error = ENOTCONN;
					break;
				}
				dst = mtod(nam, struct sockaddr_in *)->sin_addr.s_addr;
			}
			error = rip_output(m, so, dst);
			m = NULL;
			break;
		}
		case PRU_BIND: {
			struct sockaddr_in *addr = mtod(nam, struct sockaddr_in *);
			if (nam->m_len != sizeof(*addr)) {
				error = EINVAL;
				break;
			}
			if ((interfaces) ||
			    ((addr->sin_family != AF_INET) &&
			    (addr->sin_family != AF_IMPLINK)) ||
			    (addr->sin_addr.s_addr && 
			    ifa_ifwithaddr((struct sockaddr*)addr) == 0)) {
				error = EADDRNOTAVAIL;
				break;
			}
			inp->laddr = addr->sin_addr;
			break;
		}
		case PRU_CONNECT: {
			struct sockaddr_in *addr = mtod(nam, struct sockaddr_in *);
			
			if (nam->m_len != sizeof(*addr)) {
				error = EINVAL;
				break;
			}
			if ((interfaces == NULL)) {
				error = EADDRNOTAVAIL;
				break;
			}
			if ((addr->sin_family != AF_INET) &&
			    (addr->sin_family != AF_IMPLINK)) {
				error = EAFNOSUPPORT;
				break;
			}
			inp->faddr = addr->sin_addr;
			soisconnected(so);
			break;
		}
		case PRU_CONNECT2:
			error = EOPNOTSUPP;
			break;
		case PRU_SHUTDOWN:
			socantsendmore(so);
			break;
		case PRU_RCVOOB:
		case PRU_RCVD:
		case PRU_LISTEN:
		case PRU_ACCEPT:
		case PRU_SENDOOB:
			error = EINVAL;//EOPNOTSUPP;
			break;			
		/* add remaining cases */
	}
	
	return error;
}

int rip_ctloutput(int op, struct socket *so, int level,
                  int optnum, struct mbuf **m)
{
	struct inpcb *inp = sotoinpcb(so);
	
	if (level != IPPROTO_IP)
		return EINVAL;
	
	switch (optnum) {
		case IP_HDRINCL:
			if (op == PRCO_SETOPT || op == PRCO_GETOPT) {
				if (m == NULL || *m == NULL || (*m)->m_len < sizeof(int))
					return EINVAL;
				if (op == PRCO_SETOPT) {
					if (*mtod(*m, int*))
						inp->inp_flags |= INP_HDRINCL;
					else
						inp->inp_flags &= ~INP_HDRINCL;
					m_free(*m);
				} else {
					(*m)->m_len = sizeof(int);
					*mtod(*m, int*) = inp->inp_flags & INP_HDRINCL;
				}
				return 0;
			}
			break;
		/* XXX - Add other options here */
	}
#ifdef _KERNEL_
	return ipm->ctloutput(op, so, level, optnum, m);
#else
/* XXX - get this working for app...? */
	return 0;
#endif
}
	
static struct protosw my_protocol = {
	"Raw IP module",
	RAW_MODULE_PATH,
	SOCK_RAW,
	NULL,
	0,
	PR_ATOMIC | PR_ADDR,
	NET_LAYER4,
	
	&rip_init,
	&rip_input,
	NULL,
	&rip_userreq,
	NULL,                    /* pr_sysctl */
	NULL,
	&rip_ctloutput,
	
	NULL,
	NULL
};

static int raw_module_init(void *cpp)
{
	if (cpp)
		core = cpp;

	add_domain(NULL, AF_INET);
	add_protocol(&my_protocol, AF_INET);

#ifndef _KERNEL_
	if (!ipm) {
		char path[PATH_MAX];
		getcwd(path, PATH_MAX);
		strcat(path, "/" IPV4_MODULE_PATH);

		ipid = load_add_on(path);
		if (ipid > 0) {
			status_t rv = get_image_symbol(ipid, "protocol_info",
								B_SYMBOL_TYPE_DATA, (void**)&ipm);
			if (rv < 0) {
				printf("Failed to get access to IPv4 information!\n");
				return -1;
			}
			ipm->set_core(cpp);
		} else { 
			printf("Failed to load the IPv4 module...%ld [%s]\n", 
			       ipid, strerror(ipid));
			return -1;
		}
	}
#else
	if (!ipm)
		get_module(IPV4_MODULE_PATH, (module_info**)&ipm);
#endif

	return 0;	
}

static int raw_module_stop(void)
{
#ifndef _KERNEL_
	unload_add_on(ipid);
#else
	put_module(IPV4_MODULE_PATH);
#endif

	remove_protocol(&my_protocol);
	remove_domain(AF_INET);
	
	return 0;
}

_EXPORT struct raw_module_info protocol_info = {
	{
		{
			RAW_MODULE_PATH,
			0,
			raw_ops
		},
		raw_module_init,
		raw_module_stop
	},
	&rip_input
};

#ifdef _KERNEL_
static status_t raw_ops(int32 op, ...)
{
	switch(op) {
		case B_MODULE_INIT:
			get_module(CORE_MODULE_PATH, (module_info**)&core);
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
	(module_info*)&protocol_info,
	NULL
};

#endif
