/* icmp.c
 */
 
#ifndef _KERNEL
#include <stdio.h>
#include <string.h>
#endif

#include "net_misc.h"
#include "sys/socket.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"
#include "netinet/ip_icmp.h"
#include "protocols.h"
#include "sys/protosw.h"
#include "sys/domain.h"
#include "netinet/icmp_var.h"
#include "netinet/in_var.h"
#include "net/if.h"

#include "core_module.h"
#include "net_module.h"
#include "core_funcs.h"
#include "raw/raw_module.h"
#include "icmp_module.h"
#include "../ipv4/ipv4_module.h"

#ifdef _KERNEL_
#include <KernelExport.h>
static status_t icmp_ops(int32 op, ...);
#else
#define icmp_ops NULL
#endif

/* private variables */
static struct core_module_info *core = NULL;
static struct raw_module_info *raw = NULL;
static struct protosw* proto[IPPROTO_MAX];
static struct in_ifaddr *ic_ifaddr = NULL;
static struct ipv4_module_info *ipm = NULL;
#ifndef _KERNEL_
static image_id ipid = -1;
#endif

static struct route icmprt;

#if SHOW_DEBUG
static void dump_icmp(struct mbuf *buf)
{
	struct ip *ip = mtod(buf, struct ip *);
	struct icmp *ic =  (struct icmp*)((caddr_t)ip + (ip->hl * 4));

	printf("ICMP: ");
	switch (ic->icmp_type) {
		case ICMP_ECHORQST:
			printf ("Echo request\n");
			break;
		case ICMP_ECHORPLY:
			printf("echo reply\n");
			break;
		default:
			printf("?? type = %d\n", ic->type);
	}
}
#endif

static void icmp_send(struct mbuf *m, struct mbuf *opts)
{
	struct ip *ip = mtod(m, struct ip*);
	int hlen;
	struct icmp *icp;

	hlen = ip->ip_hl << 2;
	m->m_data += hlen;
	m->m_len -= hlen;
	icp = mtod(m, struct icmp *);
	icp->icmp_cksum = 0;
	icp->icmp_cksum = in_cksum(m, ip->ip_len - hlen, 0);
	m->m_data -= hlen;
	m->m_len += hlen;
	proto[IPPROTO_IP]->pr_output(m, opts, NULL, 0, NULL);
}

static void icmp_reflect(struct mbuf *m)
{
	struct ip *ip = mtod(m, struct ip*);
	struct in_ifaddr *ia;
	struct in_addr t;
	struct mbuf *opts = NULL;
	int optlen = (ip->ip_hl << 2) - sizeof(struct ip);
	struct sockaddr_in icmpdst;

	if (!ic_ifaddr) {
		ic_ifaddr = get_primary_addr();
		if (!ic_ifaddr) {
			printf("icmp_reflect: no interfaces available (ic_ifaddr == NULL)\n");
			m_freem(m);
			return;
		}
	}
	
	if (!in_canforward(ip->ip_src) &&
	    ((ntohl(ip->ip_src.s_addr) & IN_CLASSA_NET) !=
	    (IN_LOOPBACKNET << IN_CLASSA_NSHIFT))) {
	    printf("icmp_reflect: can't forward packet!\n");
		m_freem(m);
		goto done;
	}
	t = ip->ip_dst;
	ip->ip_dst = ip->ip_src;
	for (ia = ic_ifaddr; ia; ia = ia->ia_next) {
		if (t.s_addr == IA_SIN(ia)->sin_addr.s_addr)
			break;
		if ((ia->ia_ifp->if_flags & IFF_BROADCAST) &&
		    t.s_addr == satosin(&ia->ia_broadaddr)->sin_addr.s_addr)
			break;
	}
	icmpdst.sin_addr = t;
	if (ia == NULL)
		ia = (struct in_ifaddr*)ifaof_ifpforaddr((struct sockaddr*)&icmpdst,
		                                         m->m_pkthdr.rcvif);
	if (ia == NULL)
		ia = in_ifaddr;
	t = IA_SIN(ia)->sin_addr;
	ip->ip_src = t;
	ip->ip_ttl = MAXTTL;
	if (optlen > 0) {
		uint8 *cp;
		int opt, cnt;
		uint len;
		
		cp = (uint8*)(ip + 1);
		if ((opts = ipm->srcroute()) == 0 &&
		    (opts = m_gethdr(MT_HEADER))) {
			opts->m_len = sizeof(struct in_addr);
			mtod(opts, struct in_addr*)->s_addr = 0;
		}
		if (opts) {
			for (cnt = optlen; cnt > 0; cnt -= len, cp+= len) {
				opt = cp[IPOPT_OPTVAL];
				if (opt == IPOPT_EOL)
					break;
				if (opt == IPOPT_NOP)
					len = 1;
				else {
					len = cp[IPOPT_OLEN];
					if (len <= 0 || len > cnt)
						break;
				}
				if (opt == IPOPT_RR || opt == IPOPT_TS || 
				    opt == IPOPT_SECURITY) {
					memcpy((void*)(mtod(opts, char*) + opts->m_len), 
					       (void*)cp, len);
					opts->m_len += len;
				}
			}
			if ((cnt = opts->m_len % 4)) {
				for (; cnt < 4; cnt++) {
					*(mtod(opts, char*) + opts->m_len) = IPOPT_EOL;
					opts->m_len++;
				}
			}
		}
		ip->ip_len -= optlen;
		ip->ip_hl = sizeof(struct ip) >> 2;
		m->m_len -= optlen;
		if (m->m_flags & M_PKTHDR)
			m->m_pkthdr.len -= optlen;
		optlen += sizeof(struct ip);
		memcpy((void*)(ip + 1), (void*)(ip + optlen), 
		       (uint)(m->m_len - sizeof(struct ip)));
	}
	m->m_flags &= ~(M_BCAST | M_MCAST);
	icmp_send(m, opts);
done:
	if (opts)
		m_free(opts);
}

void icmp_input(struct mbuf *buf, int hdrlen)
{
	struct ip *ip = mtod(buf, struct ip *);	
	struct icmp *ic;
	int icl = ip->ip_len;
	int i;
	uint16 rv;
	int code;
	struct sockaddr_in icmpsrc = {sizeof(struct sockaddr_in), AF_INET};

	if (icl < ICMP_MINLEN) {
		icmpstat.icps_tooshort++;
		goto freeit;
	}
#if SHOW_DEBUG
	dump_icmp(buf);
#endif
	i = hdrlen + min(icl, ICMP_ADVLENMIN);
	if (buf->m_len < i && (buf = m_pullup(buf, i)) == NULL) {
		icmpstat.icps_tooshort++;
		return;
	}
	ip = mtod(buf, struct ip*);	
	buf->m_len -= hdrlen;
	buf->m_data += hdrlen;	
	ic = mtod(buf, struct icmp*);

	if ((rv = in_cksum(buf, icl, 0)) != 0) {
		printf("icmp_input: checksum failed over %d bytes (%d)!\n", icl, rv);
		icmpstat.icps_checksum++;
		goto freeit;
	}	
	buf->m_len += hdrlen;
	buf->m_data -= hdrlen;
	if (ic->icmp_type > ICMP_MAXTYPE)
		goto raw;

	icmpstat.icps_inhist[ic->icmp_type]++;
	code = ic->icmp_code;	
	switch (ic->icmp_type) {
		case ICMP_UNREACH:
			switch (code) {
				case ICMP_UNREACH_NET:
				case ICMP_UNREACH_HOST:
				case ICMP_UNREACH_PROTOCOL:
				case ICMP_UNREACH_PORT:
				case ICMP_UNREACH_SRCFAIL:
					code += PRC_UNREACH_NET;
					break;
				case ICMP_UNREACH_NEEDFRAG:
					code = PRC_MSGSIZE;
					break;
				case ICMP_UNREACH_NET_UNKNOWN:
				case ICMP_UNREACH_NET_PROHIB:
				case ICMP_UNREACH_TOSNET:
					code = PRC_UNREACH_NET;
					break;
				case ICMP_UNREACH_HOST_UNKNOWN:
				case ICMP_UNREACH_ISOLATED:
				case ICMP_UNREACH_HOST_PROHIB:
				case ICMP_UNREACH_TOSHOST:
					code = PRC_UNREACH_HOST;
					break;
				default:
					goto badcode;
			}
			goto deliver;
		case ICMP_TIMXCEED:
			if (code > 1)
				goto badcode;
			code += PRC_TIMXCEED_INTRANS;
			goto deliver;
		case ICMP_PARAMPROB:
			if (code > 1)
				goto badcode;
			code = PRC_PARAMPROB;
			goto deliver;
		case ICMP_SOURCEQUENCH:
			if (code)
				goto badcode;
			code = PRC_QUENCH;
deliver:
			if (icl < ICMP_ADVLENMIN || icl < ICMP_ADVLEN(ic) ||
			    (ic->icmp_ip.ip_hl < sizeof(struct ip) >> 2)) {
				icmpstat.icps_badlen++;
				goto freeit;
			}
			ic->icmp_ip.ip_len = htons(ic->icmp_ip.ip_len);
			icmpsrc.sin_addr = ic->icmp_ip.ip_dst;
			if (proto[ic->icmp_ip.ip_p]->pr_ctlinput)
				proto[ic->icmp_ip.ip_p]->pr_ctlinput(code, 
				       (struct sockaddr*)&icmpsrc, (void*)&ic->icmp_ip);
			break;
badcode:
			icmpstat.icps_badcode++;
			break;		
		case ICMP_ECHO: {	
			ic->icmp_type = ICMP_ECHOREPLY;
			ip->ip_len += hdrlen;
			icmpstat.icps_reflect++;
			icmpstat.icps_outhist[ic->icmp_type]++;
			icmp_reflect(buf);
			return;
			break;
		}
		case ICMP_ECHOREPLY:
			break;
		default:
			break;
	}

raw:
	if (raw)
		raw->input(buf, 0);

	return;

freeit:
	m_freem(buf);
	return;
}


void icmp_error(struct mbuf *n, int type, int code, n_long dest, 
                struct ifnet *destifp)
{
	struct ip *oip = mtod(n, struct ip*), *nip;
	uint oiplen = oip->ip_hl << 2;
	struct icmp *icp;
	struct mbuf *m;
	uint icmplen;

	if (type != ICMP_REDIRECT)
		icmpstat.icps_error++;
	if (oip->ip_off & ~(IP_MF | IP_DF))
		goto freeit;
	if (oip->ip_p == IPPROTO_ICMP && type != ICMP_REDIRECT &&
	    n->m_len >= oiplen + ICMP_MINLEN &&
	    ICMP_INFOTYPE(((struct icmp*)((void*)(oip + oiplen)))->icmp_type)) {
		icmpstat.icps_oldicmp++;
		goto freeit;
	}
	/* don't send icmp errors in response to multi or broad cast */
	if (n->m_flags & (M_BCAST | M_MCAST))
		goto freeit;
		
	m = m_gethdr(MT_HEADER);
	if (!m)
		goto freeit;
	icmplen = oiplen + min(8, oip->ip_len);
	m->m_len = icmplen + ICMP_MINLEN;
	MH_ALIGN(m, m->m_len);
	icp = mtod(m, struct icmp*);
	if ((uint)type > ICMP_MAXTYPE) {
		/* PANIC */
		printf("PANIC: icmp_error! type outside of range\n");
		return;
	}
	icmpstat.icps_outhist[type]++;
	icp->icmp_type = type;
	if (type == ICMP_REDIRECT)
		icp->icmp_gwaddr.s_addr = dest;
	else {
		icp->icmp_void = 0;
		if (type == ICMP_PARAMPROB) {
			icp->icmp_pptr = code;
		} else if (type == ICMP_UNREACH && 
		           code == ICMP_UNREACH_NEEDFRAG && 
		           destifp) {
			icp->icmp_nextmtu = htons(destifp->if_mtu);
		}
	}
	icp->icmp_code = code;
	memcpy((void*)&icp->icmp_ip, (void*)oip, icmplen);
	nip = &icp->icmp_ip;
	nip->ip_len = htons((nip->ip_len + oiplen));
	if (m->m_data - sizeof(struct ip) < m->m_pktdat) {
		/* PANIC */
		printf("PANIC: icmp_error: icmp len\n");
		return;
	}
	m->m_data -= sizeof(struct ip);
	m->m_len += sizeof(struct ip);
	m->m_pkthdr.len = m->m_len;
	m->m_pkthdr.rcvif = n->m_pkthdr.rcvif;
	nip = mtod(m, struct ip*);
	memcpy((void*)nip, (void*)oip, sizeof(struct ip));
	nip->ip_len = m->m_len;
	nip->ip_hl = sizeof(struct ip) >> 2;
	nip->ip_p = IPPROTO_ICMP;
	nip->ip_tos = 0;
	icmp_reflect(m);

freeit:
	m_freem(n);
}

static void icmp_init(void)
{
	memset(&icmprt, 0, sizeof(icmprt));

	memset(proto, 0, sizeof(struct protosw *) * IPPROTO_MAX);
	add_protosw(proto, NET_LAYER2);
#ifdef _KERNEL
	if (!raw)
		get_module(RAW_MODULE_PATH, (module_info**)&raw);
#endif
	ic_ifaddr = get_primary_addr();
}

struct protosw my_proto = {
	"ICMP (v4)",
	ICMP_MODULE_PATH,
	0,
	NULL,
	IPPROTO_ICMP,
	PR_ATOMIC | PR_ADDR,
	NET_LAYER2,
	
	&icmp_init,
	&icmp_input,
	NULL,         /* pr_output, */
	NULL,
	NULL,         /* pr_sysctl */
	NULL,
	NULL,         /* pr_ctloutput */
	
	NULL,
	NULL
};

static int icmp_protocol_init(void *cpp)
{
	/* we will never call this with anything but NULL when in kernel,
	 * so this should be safe.
	 */
	if (cpp)
		core = (struct core_module_info *)cpp;
	add_domain(NULL, AF_INET);
	add_protocol(&my_proto, AF_INET);

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
		} else { 
			printf("Failed to load the IPv4 module...\n");
			return -1;
		}
		ipm->set_core(cpp);
	}
#else
	if (!ipm)
		get_module(IPV4_MODULE_PATH, (module_info**)&ipm);
#endif

	return 0;
}

static int icmp_protocol_stop(void)
{
	remove_protocol(&my_proto);
	remove_domain(AF_INET);
	return 0;
}

#ifndef _KERNEL_
void set_core(struct core_module_info *cp)
{
	core = cp;
}
#endif

_EXPORT struct icmp_module_info protocol_info = {
	{
		{
			ICMP_MODULE_PATH,
			0,
			icmp_ops
		},
		icmp_protocol_init,
		icmp_protocol_stop
	},
#ifndef _KERNEL_
	set_core,
#endif
	icmp_error
};

#ifdef _KERNEL_
static status_t icmp_ops(int32 op, ...)
{
	switch(op) {
		case B_MODULE_INIT:
			if (!core)
				get_module(CORE_MODULE_PATH, (module_info**)&core);
			if (!core)
				return B_ERROR;
			load_driver_symbols("icmp");
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

