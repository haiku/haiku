/* udp.c
 */

#ifndef _KERNEL_
#include <stdio.h>
#include <string.h>
#endif

#include "net_misc.h"
#include "protocols.h"
#include "netinet/in_systm.h"
#include "netinet/in_var.h"
#include "netinet/in_pcb.h"
#include "netinet/ip.h"
#include "sys/domain.h"
#include "sys/protosw.h"
#include "netinet/ip_var.h"
#include "netinet/udp.h"
#include "netinet/udp_var.h"
#include "netinet/ip_icmp.h"

#include "core_module.h"
#include "net_module.h"
#include "core_funcs.h"
#include "../icmp/icmp_module.h"

#ifdef _KERNEL_
#include <KernelExport.h>
static status_t udp_ops(int32 op, ...);
#define UDP_MODULE_PATH		"network/protocol/udp"
#else	/* _KERNEL */
#define udp_ops NULL
#define UDP_MODULE_PATH	    "modules/protocol/udp"
#endif

/* Private variables */
static struct core_module_info *core = NULL;
static struct protosw *proto[IPPROTO_MAX];
static struct inpcb udb;	/* head of the UDP PCB list! */
static int udpcksum = 1;	/* do we calculate the UDP checksum? */
static struct udpstat udpstat;
static uint32 udp_sendspace;	/* size of send buffer */
static uint32 udp_recvspace;	/* size of recieve buffer */
static struct icmp_module_info *icmp = NULL;
#ifndef _KERNEL_
static image_id icmpid = -1;
#endif

static struct in_addr zeroin_addr = {0};

/* Private but used globally, need to make thread safe... tls??? */
static struct inpcb *udp_last_inpcb = NULL;
static struct sockaddr_in udp_in;

#if SHOW_DEBUG
static void dump_udp(struct mbuf *buf)
{
	struct ip *ip = mtod(buf, struct ip*);
	struct udphdr *udp = (struct udphdr*)((caddr_t)ip + (ip->ip_hl * 4));

	printf("udp_header  :\n");
	printf("            : src_port      : %d\n", ntohs(udp->src_port));
	printf("            : dst_port      : %d\n", ntohs(udp->dst_port));
	printf("            : udp length    : %d bytes\n", ntohs(udp->length));
}
#endif /* SHOW_DEBUG */

int udp_output(struct inpcb *inp, struct mbuf *m, struct mbuf *addr,struct mbuf *control)
{
	struct udpiphdr *ui;
	uint16 len = m->m_pkthdr.len;
	uint16 hdrlen = len + (uint16)sizeof(struct udpiphdr);
	struct in_addr laddr;
	int error = 0;

	if (control)
		m_freem(control);

	if (addr) {
		laddr = inp->laddr;
		if (inp->faddr.s_addr != INADDR_ANY) {
			error = EISCONN;
			goto release;
		}
		error = in_pcbconnect(inp, addr);
		if (error)
			goto release;

	} else {
		if (inp->faddr.s_addr == INADDR_ANY) {
			error = ENOTCONN;
			goto release;
		}
	}

	M_PREPEND(m, sizeof(*ui));
	if (!m) {
		error = ENOMEM;
		goto release;
	}

	ui = mtod(m, struct udpiphdr *);
	ui->ui_next = ui->ui_prev = NULL;
	ui->ui_x1 = 0;
	ui->ui_pr = IPPROTO_UDP;
	ui->ui_len = htons(len + sizeof(struct udphdr));	
	ui->ui_src = inp->laddr;
	ui->ui_dst = inp->faddr;
	ui->ui_sport = inp->lport;
	ui->ui_dport = inp->fport;
	ui->ui_ulen = ui->ui_len;
	ui->ui_sum = 0;

	if (udpcksum)
		ui->ui_sum = in_cksum(m, hdrlen, 0);

	if (ui->ui_sum == 0)
		ui->ui_sum = 0xffff;

	((struct ip*)ui)->ip_len = hdrlen;
	((struct ip*)ui)->ip_ttl = 64;	/* XXX - Fix this! */
	((struct ip*)ui)->ip_tos = 0;	/* XXX - Fix this! */


	/* XXX - add multicast options when available! */
	error = proto[IPPROTO_IP]->pr_output(m,
					inp->inp_options, &inp->inp_route,
					inp->inp_socket->so_options & (SO_DONTROUTE | SO_BROADCAST),
					NULL /* inp->inp_moptions */ );

	if (addr) {
		in_pcbdisconnect(inp); /* remove temporary route */
		inp->laddr = laddr;
	}

	return error;

release:
	m_freem(m);
	return error;
}

int udp_userreq(struct socket *so, int req,
			    struct mbuf *m, struct mbuf *addr, struct mbuf *ctrl)
{
	struct inpcb *inp = sotoinpcb(so);
	int error = 0;

	if (req == PRU_CONTROL)
		return in_control(so, (int)m, (caddr_t)addr, (struct ifnet *)ctrl);

	if (inp == NULL && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}

	switch (req) {
		case PRU_ATTACH:
			/* we don't replace an existing inpcb! */
			if (inp != NULL) {
				error = EINVAL;
				break;
			}
			error = in_pcballoc(so, &udb); /* udp head */
			if (error)
				break;
			error = soreserve(so, udp_sendspace, udp_recvspace);
			if (error)
				break;
			/* XXX - this is a hack! This should be the default ip TTL */
			((struct inpcb*) so->so_pcb)->inp_ip.ip_ttl = 64;
			break;
		case PRU_DETACH:
			/* This should really be protected when in kernel... */
			if (inp == udp_last_inpcb)
				udp_last_inpcb = &udb;
			in_pcbdetach(inp);
			break;
		case PRU_BIND:
			/* XXX - locking */
			error = in_pcbbind(inp, addr);
			break;
		case PRU_SEND:
			/* we can use this as we're in the same module... */
			return udp_output(inp, m, addr, ctrl);
		case PRU_LISTEN:
			error = EINVAL;//EOPNOTSUPP;
			break;
		case PRU_CONNECT:
			if (inp->faddr.s_addr != INADDR_ANY) {
				error = EISCONN;
				break;
			}
			error = in_pcbconnect(inp, addr);
			if (error == 0)
				soisconnected(so);
			break;
		case PRU_CONNECT2:
			error = EINVAL;//EOPNOTSUPP;
			break;
		case PRU_ACCEPT:
			error = EINVAL;//EOPNOTSUPP;
			break;
		case PRU_DISCONNECT:
			if (inp->faddr.s_addr == INADDR_ANY) {
				error = ENOTCONN;
				break;
			}
			in_pcbdisconnect(inp);
			inp->laddr.s_addr = INADDR_ANY;
			so->so_state &= ~SS_ISCONNECTED;
			break;
		case PRU_SOCKADDR:
			in_setsockaddr(inp, addr);
			break;
		case PRU_PEERADDR:
			in_setpeeraddr(inp, addr);
			break;
		case PRU_SENSE:
			/* will we ever see one of these???? */
			/* generated by an fstat on bsd... */
			return 0;
		default:
			printf("Unknown options passed to udp_userreq (%d)\n", req);
	}

release:
	if (ctrl) {
		printf("UDP control retained!\n");
		m_freem(ctrl);
	}
	if (m)
		m_freem(m);

	return error;
}

void udp_input(struct mbuf *buf, int hdrlen)
{
	struct ip *ip = mtod(buf, struct ip*);
	struct udphdr *udp = (struct udphdr*)((caddr_t)ip + hdrlen);
	uint16 ck = 0;
	int len;
	struct ip saved_ip;
	struct mbuf *opts = NULL;
	struct inpcb *inp = NULL;

#if SHOW_DEBUG
        dump_udp(buf);
#endif
	/* check and adjust sizes as required... */
	len = ntohs(udp->uh_ulen) + hdrlen;
	saved_ip = *ip;

	if (udpcksum && udp->uh_sum) {
		((struct ipovly*)ip)->ih_next = ((struct ipovly*)ip)->ih_prev = NULL;
		((struct ipovly*)ip)->ih_x1 = 0;
		((struct ipovly*)ip)->ih_len = udp->uh_ulen;
		/* XXX - if we have options we need to be careful when calculating the
		 * checksum here...
		 */
		if ((ck = in_cksum(buf, len, 0)) != 0) {
			udpstat.udps_badsum++;
			m_freem(buf);
			printf("udp_input: UDP Checksum check failed. (%d over %ld bytes)\n", ck, len + sizeof(*ip));
			return;
		}
	}
	inp = udp_last_inpcb;

	if (inp == NULL ||
	    inp->lport != udp->uh_dport ||
		inp->fport != udp->uh_sport ||
		inp->faddr.s_addr != ip->ip_src.s_addr ||
		inp->laddr.s_addr != ip->ip_dst.s_addr) {

		inp = in_pcblookup(&udb, ip->ip_src, udp->uh_sport,
				   ip->ip_dst, udp->uh_dport, INPLOOKUP_WILDCARD);
		if (inp)
			udp_last_inpcb = inp;
	}
	if (!inp) {
		atomic_add((vint32 *)&udpstat.udps_noport, 1);
		if (buf->m_flags & (M_BCAST | M_MCAST)) {
			atomic_add((vint32 *)&udpstat.udps_noportbcast, 1);
			goto bad;
		}
		*ip = saved_ip;
		ip->ip_len += hdrlen;
		icmp->error(buf, ICMP_UNREACH, ICMP_UNREACH_PORT, 0, 0);
		return;
	}

	udp_in.sin_port = udp->uh_sport;
	udp_in.sin_addr = ip->ip_src;

	if (inp->inp_flags & INP_CONTROLOPT) {
		printf("INP Control Options to process!\n");
		/* XXX - add code to do this... */
	}

	hdrlen += sizeof(struct udphdr);
	buf->m_len -= hdrlen;
	buf->m_pkthdr.len -= hdrlen;
	buf->m_data += hdrlen;
	
	if (sbappendaddr(&inp->inp_socket->so_rcv, (struct sockaddr*)&udp_in,
			buf, opts) == 0) {
		goto bad;
	}
	sorwakeup(inp->inp_socket);
	return;

bad:
	if (opts)
		m_freem(opts);
	m_freem(buf);
	return;
}

static void udp_notify(struct inpcb *inp, int err)
{
	inp->inp_socket->so_error = err;
	sorwakeup(inp->inp_socket);
	sowwakeup(inp->inp_socket);
}

static void udp_ctlinput(int cmd, struct sockaddr *sa, void *ipp)
{
	struct ip *ip = (struct ip*)ipp;
	struct udphdr *uh;

	if (!PRC_IS_REDIRECT(cmd) && 
	    ((uint)cmd >= PRC_NCMDS || inetctlerrmap(cmd) == 0))
		return;
	if (ip) {
		uh = (struct udphdr *)((char *) ip + (ip->ip_hl << 2));
		in_pcbnotify(&udb, sa, uh->uh_dport, ip->ip_src, uh->uh_sport, cmd, udp_notify);
	} else
		in_pcbnotify(&udb, sa, 0, zeroin_addr, 0, cmd, udp_notify);
}

void udp_init(void)
{
	udb.inp_prev = udb.inp_next = &udb;
	udp_sendspace = 9216; /* default size */
	udp_recvspace = 41600; /* default size */
	udp_in.sin_len = sizeof(udp_in);
	memset(&udpstat, 0, sizeof(udpstat));
		
	memset(proto, 0, sizeof(struct protosw *) * IPPROTO_MAX);
	add_protosw(proto, NET_LAYER2);
}

static struct protosw my_proto = {
	"UDP Module",
	UDP_MODULE_PATH,
	SOCK_DGRAM,
	NULL,
	IPPROTO_UDP,
	PR_ATOMIC | PR_ADDR,
	NET_LAYER3,
	
	&udp_init,
	&udp_input,
	NULL,                  /* pr_output */
	&udp_userreq,
	NULL,                  /* pr_sysctl */
	&udp_ctlinput,
	NULL,                  /* pr_ctloutput */
		
	NULL,
	NULL
};

static int udp_module_init(void *cpp)
{
	if (cpp)
		core = cpp;
	add_domain(NULL, AF_INET);
	add_protocol(&my_proto, AF_INET);

#ifndef _KERNEL_
	if (!icmp) {
		char path[PATH_MAX];
		getcwd(path, PATH_MAX);
		strcat(path, "/" ICMP_MODULE_PATH);

		icmpid = load_add_on(path);
		if (icmpid > 0) {
			status_t rv = get_image_symbol(icmpid, "protocol_info",
								B_SYMBOL_TYPE_DATA, (void**)&icmp);
			if (rv < 0) {
				printf("Failed to get access to IPv4 information!\n");
				return -1;
			}
		} else { 
			printf("Failed to load the IPv4 module...\n");
			return -1;
		}
		icmp->set_core(cpp);
	}
#else
	if (!icmp)
		get_module(ICMP_MODULE_PATH, (module_info**)&icmp);
#endif

	return 0;
}

static int udp_module_stop(void)
{
	remove_protocol(&my_proto);
	remove_domain(AF_INET);
	return 0;
}

_EXPORT struct kernel_net_module_info protocol_info = {
	{
		UDP_MODULE_PATH,
		B_KEEP_LOADED,
		udp_ops
	},
	udp_module_init,
	udp_module_stop
};

#ifdef _KERNEL_
static status_t udp_ops(int32 op, ...)
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
	(module_info *)&protocol_info,
	NULL
};
#endif
