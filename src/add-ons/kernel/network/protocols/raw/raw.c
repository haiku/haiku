/* raw.c */

#ifndef _KERNEL_MODE
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
#include "raw_module.h"
#include "ipv4_module.h"

#include <KernelExport.h>
status_t std_ops(int32 op, ...);

static struct core_module_info *core = NULL;
static struct ipv4_module_info *ipm = NULL;

static struct inpcb rawinpcb;
static struct sockaddr_in ripsrc;
static int rip_sendspace = 8192;
static int rip_recvspace = 8192;

struct ipstat ipstat; //XXX might need to be shared

static void rip_init(void)
{
	rawinpcb.inp_next = rawinpcb.inp_prev = &rawinpcb;
	memset(&ripsrc, 0, sizeof(ripsrc));
	ripsrc.sin_family = AF_INET;
	ripsrc.sin_len = sizeof(ripsrc);
}

static void rip_input(struct mbuf *m, int hdrlen)
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
				if (sockbuf_appendaddr(&last->so_rcv, (struct sockaddr*)&ripsrc, 
				                 n, NULL) == 0)
					m_freem(n);
				else
					sorwakeup(last);
			}
		}
		last = inp->inp_socket;
	}
	if (last) {
		if (sockbuf_appendaddr(&last->so_rcv, (struct sockaddr*)&ripsrc, 
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

static int rip_output(struct mbuf *m, struct socket *so, uint32 dst)
{
	struct ip *ip;
	struct inpcb *inp = sotoinpcb(so);
	struct mbuf *opts;
	int flags = (so->so_options & SO_DONTROUTE) | IP_ALLOWBROADCAST;

	if ((inp->inp_flags & INP_HDRINCL) == 0) {
		M_PREPEND(m, (int) sizeof(struct ip));
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

static int rip_userreq(struct socket *so, int req, struct mbuf *m, struct mbuf *nam,
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
			socket_set_disconnected(so);
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
			socket_set_connected(so);
			break;
		}
		case PRU_CONNECT2:
			error = EOPNOTSUPP;
			break;
		case PRU_SHUTDOWN:
			socket_set_cantsendmore(so);
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

static int rip_ctloutput(int op, struct socket *so, int level,
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

	return ipm->ctloutput(op, so, level, optnum, m);
}
	
static struct protosw my_protocol = {
	"Raw IP module",
	NET_RAW_MODULE_NAME,
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

	if (!ipm)
		get_module(NET_IPV4_MODULE_NAME, (module_info**) &ipm);

	return 0;	
}

static int raw_module_stop(void)
{
	put_module(NET_IPV4_MODULE_NAME);

	remove_protocol(&my_protocol);
	remove_domain(AF_INET);
	
	return 0;
}

struct raw_module_info protocol_info = {
	{
		{
			NET_RAW_MODULE_NAME,
			0,
			std_ops
		},
		raw_module_init,
		raw_module_stop,
		NULL
	},
	&rip_input
};

// #pragma mark -

_EXPORT status_t std_ops(int32 op, ...) 
{
	switch(op) {
		case B_MODULE_INIT:
			get_module(NET_CORE_MODULE_NAME, (module_info **) &core);
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
	(module_info *) &protocol_info,
	NULL
};
