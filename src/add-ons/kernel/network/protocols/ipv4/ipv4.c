/* ipv4.c
 * simple ipv4 implementation
 */

#ifndef _KERNEL_
#include <stdio.h>
#endif
#include <unistd.h>
#include <kernel/OS.h>
#include <sys/time.h>
#include <string.h>

#include "netinet/in.h"
#include "netinet/in_var.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"
#include "netinet/ip_var.h"
#include "netinet/in_pcb.h"
#include "netinet/ip_icmp.h"
#include "protocols.h"
#include "sys/protosw.h"
#include "sys/domain.h"

#include "core_module.h"
#include "core_funcs.h"
#include "net_module.h"
#include "ipv4_module.h"
#include "../icmp/icmp_module.h"

#ifdef _KERNEL_
#include <KernelExport.h>
static status_t ipv4_ops(int32 op, ...);
#else
#define ipv4_ops NULL
#endif	/* _KERNEL_ */

#define INA struct in_ifaddr *
#define SA  struct sockaddr *

/* private variables */
static struct core_module_info *core = NULL;
static struct protosw *proto[IPPROTO_MAX];
static int ipforwarding = 0;
static int ipsendredirects = 1;
static uint16 ip_id = 0;
static sem_id id_lock = -1;
static struct in_ifaddr *ip_ifaddr = NULL;
static struct icmp_module_info *icmp = NULL;
#ifndef _KERNEL_
static image_id icmpid = -1;
#endif
static struct ipq ipq;
static net_timer_id timerid;

/* ??? - Globals we need to remove... TLS storage? */
struct route ipforward_rt;

/* Forward prototypes */
int ipv4_output(struct mbuf *, struct mbuf *, struct route *, int, void *);
                
#if SHOW_DEBUG
static void dump_ipv4_header(struct mbuf *buf)
{
	struct ip *ip = mtod(buf, struct ip *);

	printf("IPv4 Header :\n");
	printf("            : version       : %d\n", ip->ip_v);
	printf("            : header length : %d\n", ip->ip_hl * 4);
	printf("            : tos           : %d\n", ip->ip_tos);
	printf("            : total length  : %d\n", ntohs(ip->ip_len));
	printf("            : id            : %d\n", ntohs(ip->ip_id));
	printf("            : ttl           : %d\n", ip->ip_ttl);
	dump_ipv4_addr("            : src address   :", &ip->ip_src);
	dump_ipv4_addr("            : dst address   :", &ip->ip_dst);

	printf("            : protocol      : ");

	switch(ip->ip_p) {
		case IPPROTO_ICMP:
			printf("ICMP\n");
			break;
		case IPPROTO_UDP:
			printf("UDP\n");
			break;
		case IPPROTO_TCP:
			printf("TCP\n");
			break;
		default:
			printf("unknown (0x%02x)\n", ip->ip_p);
	}
}
#endif


/* IP Options variables and structures... */
int ip_nhops = 0;
static struct ip_srcrt {
	struct in_addr dst;
	char nop;
	char srcopt[IPOPT_OFFSET + 1];
	struct in_addr route[MAX_IPOPTLEN / sizeof(struct in_addr)];
} ip_srcrt;

/*
 * Strip out IP options, at higher
 * level protocol in the kernel.
 * Second argument is buffer to which options
 * will be moved, and return value is their length.
 * XXX should be deleted; last arg currently ignored.
 */
void ip_stripoptions(struct mbuf *m, struct mbuf *mopt)
{
	int i;
	struct ip *ip = mtod(m, struct ip *);
	caddr_t opts;
	int olen;

	olen = (ip->ip_hl<<2) - sizeof (struct ip);
	opts = (caddr_t)(ip + 1);
	i = m->m_len - (sizeof (struct ip) + olen);
	memcpy(opts, opts  + olen, (unsigned)i);
	m->m_len -= olen;
	if (m->m_flags & M_PKTHDR)
		m->m_pkthdr.len -= olen;
	ip->ip_hl = sizeof(struct ip) >> 2;
}

static struct mbuf *ip_insertoptions(struct mbuf *m, struct mbuf *opt, int *phlen)
{
	struct ipoption * p = mtod(opt, struct ipoption*);
	struct mbuf *n;
	struct ip *ip = mtod(m, struct ip*);
	uint optlen;
	
	optlen = opt->m_len - sizeof(p->ipopt_dst);
	if (optlen + ip->ip_len > IP_MAXPACKET)
		return (m);
	if (p->ipopt_dst.s_addr)
		ip->ip_dst = p->ipopt_dst;
	if (m->m_flags & M_EXT || m->m_data - optlen < m->m_pktdat) {
		n = m_get(MT_HEADER);
		if (!n)
			return (m);
		n->m_pkthdr.len = m->m_pkthdr.len + optlen;
		m->m_len -= sizeof(struct ip);
		m->m_data += sizeof(struct ip);
		n->m_next = m;
		m = n;
		m->m_len = optlen + sizeof(struct ip);
		m->m_data += max_linkhdr;
		memcpy(mtod(m, void *), ip, sizeof(struct ip));
	} else {
		m->m_data -= optlen;
		m->m_len += optlen;
		m->m_pkthdr.len += optlen;
		memmove(mtod(m, void *), ip, sizeof(struct ip));
	}
	ip = mtod(m, struct ip*);
	memcpy((void*)(ip + 1), p->ipopt_list, optlen);
	*phlen = sizeof(struct ip) + optlen;
	ip->ip_len += optlen;
	return (m);
}

static struct in_ifaddr * ip_rtaddr(struct in_addr dst)
{
	struct sockaddr_in *sin;
	
	sin = (struct sockaddr_in*)&ipforward_rt.ro_dst;
	
	if (ipforward_rt.ro_rt == NULL || dst.s_addr != sin->sin_addr.s_addr) {
		if (ipforward_rt.ro_rt) {
			RTFREE(ipforward_rt.ro_rt);
			ipforward_rt.ro_rt = NULL;
		}
		memset(&sin, 0, sizeof(*sin));
		sin->sin_family = AF_INET;
		sin->sin_len = sizeof(*sin);
		sin->sin_addr = dst;
		
		rtalloc(&ipforward_rt);
	}
	if (ipforward_rt.ro_rt == NULL)
		return NULL;
	return ((struct in_ifaddr*) ipforward_rt.ro_rt->rt_ifa);
}

static void save_rte(uchar *option, struct in_addr dst)
{
	uint olen;

	olen = option[IPOPT_OLEN];
	if (olen > sizeof(struct ip_srcrt) - (1 + sizeof(dst)))
		return;
	memcpy((caddr_t) ip_srcrt.srcopt, (caddr_t)option, olen);
	ip_nhops = (olen - IPOPT_OFFSET - 1) / sizeof(struct in_addr);

	ip_srcrt.dst = dst;
}

static void ip_forward(struct mbuf *m, int srcrt)
{
	struct ip *ip = mtod(m, struct ip*);
	struct sockaddr_in *sin;
	struct rtentry *rt;
	struct mbuf *mcopy;
	n_long dest;
	int code, type, error = 0;
	struct ifnet *destifp = NULL;

	memset(sin, 0, sizeof(*sin));	
	dest = 0;
	if (m->m_flags & M_BCAST || in_canforward(ip->ip_dst) == 0) {
		ipstat.ips_cantforward++;
		m_freem(m);
		return;
	}
	ip->ip_id = htons(ip->ip_id);
	if (ip->ip_ttl <= IPTTLDEC) {
		icmp->error(m, ICMP_TIMXCEED, ICMP_TIMXCEED_INTRANS, dest, 0);
		return;
	}
	ip->ip_ttl -= IPTTLDEC;
	
	sin = (struct sockaddr_in *)&ipforward_rt.ro_dst;
	if ((rt = ipforward_rt.ro_rt) == NULL ||
	    ip->ip_dst.s_addr != sin->sin_addr.s_addr) {
		if (ipforward_rt.ro_rt) {
			RTFREE(ipforward_rt.ro_rt);
			ipforward_rt.ro_rt = NULL;
		}
		sin->sin_family = AF_INET;
		sin->sin_len = sizeof(*sin);
		sin->sin_addr = ip->ip_dst;
		
		rtalloc(&ipforward_rt);
		if (ipforward_rt.ro_rt == NULL) {
			icmp->error(m, ICMP_UNREACH, ICMP_UNREACH_HOST, dest, 0);
			return;
		}
		rt = ipforward_rt.ro_rt;
	} 
	mcopy = m_copym(m, 0, (int)min(ip->ip_len, 64));

	if (rt->rt_ifp == m->m_pkthdr.rcvif &&
	    (rt->rt_flags & (RTF_DYNAMIC | RTF_MODIFIED)) == 0 &&
	    satosin(rt_key(rt))->sin_addr.s_addr != 0 &&
	    ipsendredirects && !srcrt) {
#define RTA(rt) ((struct in_ifaddr *)(rt->rt_ifa))
		uint32 src = ntohl(ip->ip_src.s_addr);
		if (RTA(rt) &&
		    (src & RTA(rt)->ia_subnetmask) == RTA(rt)->ia_subnet) {
			if (rt->rt_flags & RTF_GATEWAY)
				dest = satosin(rt->rt_gateway)->sin_addr.s_addr;
			else
				dest = ip->ip_dst.s_addr;
			type = ICMP_REDIRECT;
			code = ICMP_REDIRECT_HOST;
		}
	}
	error = ipv4_output(m, NULL, &ipforward_rt, IP_FORWARDING | IP_ALLOWBROADCAST, 0);
	if (error)
		ipstat.ips_cantforward++;
	else {
		ipstat.ips_forward++;
		if (type)
			ipstat.ips_redirectsent++;
		else {
			if (mcopy)
				m_freem(mcopy);
			return;
		}
	}
	if (!mcopy)
		return;
	destifp = NULL;
	
	switch(error) {
		case 0:
			break;
		case ENETUNREACH:
		case EHOSTUNREACH:
		case ENETDOWN:
		case EHOSTDOWN:
		default:
			type = ICMP_UNREACH;
			code = ICMP_UNREACH_HOST;
			break;
		case EMSGSIZE:
			type = ICMP_UNREACH;
			code = ICMP_UNREACH_NEEDFRAG;
			if (ipforward_rt.ro_rt)
				destifp = ipforward_rt.ro_rt->rt_ifp;
			ipstat.ips_cantfrag++;
			break;
		case ENOBUFS:
			type = ICMP_SOURCEQUENCH;
			code = 0;
			break;
	}
	icmp->error(mcopy, type, code, dest, destifp);
}

int ip_dooptions(struct mbuf *m)
{
	struct ip *ip = mtod(m, struct ip*);
	uint8 *cp;
	struct in_addr dst, *sin;
	struct sockaddr_in ipaddr;
	int cnt, optlen, opt, code, off;
	int type = ICMP_PARAMPROB;
	int forward = 0;
	struct in_ifaddr *ia;
	struct ip_timestamp*ipt;
	n_time ntime;

printf("ip_dooptions\n");
	
	dst = ip->ip_dst;
	cp = (uint8*)(ip + 1);
	cnt = (ip->ip_hl << 2) - sizeof(struct ip);
	for (; cnt > 0; cnt -= optlen, cp+= optlen) {
		opt = cp[IPOPT_OPTVAL];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[IPOPT_OLEN];
			if (optlen <= 0 || optlen > cnt) {
				code = &cp[IPOPT_OLEN] - (uint8 *)ip;
				goto bad;
			}
		}
		switch (opt) {
			case IPOPT_LSRR:
			case IPOPT_SSRR:
				if ((off = cp[IPOPT_OFFSET]) < IPOPT_MINOFF) {
					code = &cp[IPOPT_OFFSET] - (uint8 *)ip;
					goto bad;
				}
				ipaddr.sin_addr = ip->ip_dst;
				ia = (struct in_ifaddr*)ifa_ifwithaddr((struct sockaddr*)&ipaddr);
				if (ia == NULL) {
					if (opt == IPOPT_SSRR) {
						type = ICMP_UNREACH;
						code = ICMP_UNREACH_SRCFAIL;
						goto bad;
					}
					break;
				}
				off--;
				if (off > optlen - sizeof(struct in_addr)) {
					save_rte(cp, ip->ip_src);
					break;
				}
				memcpy(&ipaddr.sin_addr, cp+off, sizeof(ipaddr.sin_addr));
				if (opt == IPOPT_SSRR) {
					if ((ia = (INA) ifa_ifwithdstaddr((SA)&ipaddr)) == NULL)
						ia = (INA) ifa_ifwithnet((SA)&ipaddr);
				} else 
					ia = ip_rtaddr(ipaddr.sin_addr);
				if (ia == NULL) {
					type = ICMP_UNREACH;
					code = ICMP_UNREACH_SRCFAIL;
					goto bad;
				}
				ip->ip_dst = ipaddr.sin_addr;
				memcpy(cp+off, &(IA_SIN(ia)->sin_addr), sizeof(struct in_addr));
				cp[IPOPT_OFFSET] += sizeof(struct in_addr);
				forward = !IN_MULTICAST(ntohl(ip->ip_dst.s_addr));
				break;
			case IPOPT_RR:
				if ((off = cp[IPOPT_OFFSET]) < IPOPT_MINOFF) {
					code = &cp[IPOPT_OFFSET] - (uint8*)ip;
					goto bad;
				}
				off--;
				if (off > optlen - sizeof(struct in_addr))
					break;
				memcpy(&ipaddr.sin_addr, &ip->ip_dst, sizeof(ipaddr.sin_addr));
				if ((ia = (INA)ifa_ifwithaddr((SA)&ipaddr)) == NULL &&
				    (ia = ip_rtaddr(ipaddr.sin_addr)) == NULL) {
					type = ICMP_UNREACH;
					code = ICMP_UNREACH_HOST;
					goto bad;
				}
				memcpy(cp+off, &(IA_SIN(ia)->sin_addr), sizeof(struct in_addr));
				cp[IPOPT_OFFSET] += sizeof(struct in_addr);
				break;
			case IPOPT_TS:
				code = cp - (uint8*)ip;
				ipt = (struct ip_timestamp *)cp;
				if (ipt->ipt_len < 5)
					goto bad;
				if (ipt->ipt_ptr > ipt->ipt_len - sizeof(uint32)) {
					if (++ipt->ipt_oflw == 0)
						goto bad;
					break;
				}
				sin = (struct in_addr *)(cp + ipt->ipt_ptr - 1);
				switch (ipt->ipt_flg) {
					case IPOPT_TS_TSONLY:
						break;
					case IPOPT_TS_TSANDADDR:
						if (ipt->ipt_ptr + sizeof(n_time) + 
						    sizeof(struct in_addr) > ipt->ipt_len)
							goto bad;
						ipaddr.sin_addr = dst;
						ia = (INA)ifaof_ifpforaddr((SA) &ipaddr, m->m_pkthdr.rcvif);
						if (!ia)
							continue;
						memcpy(sin, &IA_SIN(ia)->sin_addr, sizeof(struct in_addr));
						ipt->ipt_ptr += sizeof(struct in_addr);
						break;
					case IPOPT_TS_PRESPEC:
						if (ipt->ipt_ptr + sizeof(n_time) +
						    sizeof(struct in_addr) > ipt->ipt_len)
							goto bad;
						memcpy(&ipaddr.sin_addr, sin, sizeof(struct in_addr));
						if (ifa_ifwithaddr((SA)&ipaddr) == 0)
							continue;
						ipt->ipt_ptr += sizeof(struct in_addr);
						break;
					default:
						goto bad;
				}
				ntime = iptime();
				memcpy(cp+ipt->ipt_ptr - 1, &ntime, sizeof(n_time));
				ipt->ipt_ptr += sizeof(n_time);
			default:
				break;
		}
	}
	if (forward) {
		ip_forward(m, 1);
		return 1;
	}
	return 0;
bad:
	ip->ip_len -= ip->ip_hl << 2;
	icmp->error(m, type, code, 0, 0);
	ipstat.ips_badoptions++;
	return 1;
}


struct mbuf * ip_srcroute(void)
{
	struct in_addr *p, *q;
	struct mbuf *m;

	if (ip_nhops == 0)
		return NULL;

	m = m_get(MT_SOOPTS);
	if (!m)
		return NULL;

#define OPTSIZ (sizeof(ip_srcrt.nop) + sizeof(ip_srcrt.srcopt))

	m->m_len = ip_nhops * sizeof(struct in_addr) + sizeof(struct in_addr) + OPTSIZ;
	
	p = &ip_srcrt.route[ip_nhops-1];
	*(mtod(m, struct in_addr*)) = *p--;
	ip_srcrt.nop = IPOPT_NOP;
	ip_srcrt.srcopt[IPOPT_OFFSET] = IPOPT_MINOFF;
	memcpy(mtod(m, caddr_t) + sizeof(struct in_addr), &ip_srcrt.nop, OPTSIZ);
	q = (struct in_addr*)(mtod(m, caddr_t) + sizeof(struct in_addr) + OPTSIZ);

#undef OPTSIZ

	while (p >= ip_srcrt.route) {
		*q++ = *p--;
	}
	
	*q = ip_srcrt.dst;
	return m;
}

/* XXX - don't think these are thread safe somehow!
 * look at adding locking or making them thread safe 
 */
static void ip_enq(struct ipasfrag *p, struct ipasfrag *prev)
{
	p->ipf_prev = prev;
	p->ipf_next = prev->ipf_next;
	prev->ipf_next->ipf_prev = p;
	prev->ipf_next = p;
}

static void ip_deq(struct ipasfrag *p)
{
	p->ipf_prev->ipf_next = p->ipf_next;
	p->ipf_next->ipf_prev = p->ipf_prev;
}

static void ip_freef(struct ipq *fp)
{
	struct ipasfrag *q, *p;
	
	for (q = fp->ipq_next; q!= (struct ipasfrag*)fp; q = p) {
		p = q->ipf_next;
		ip_deq(q);
		m_freem(dtom(q));
	}
	remque(fp);
	m_free(dtom(fp));
}

static struct ip *ip_reass(struct ipasfrag *ip, struct ipq *fp)
{
	struct mbuf *m = dtom(ip);
	struct ipasfrag *q;
	struct mbuf *t;
	int hlen = ip->ip_hl << 2;
	int i, next;
	
	m->m_data += hlen;
	m->m_len -= hlen;

	if (!fp) {
		if ((t = m_get(MT_FTABLE)) == NULL)
			goto dropfrag;
		fp = mtod(t, struct ipq*);
		insque(fp, &ipq);
		fp->ipq_ttl = IPFRAGTTL;
		fp->ipq_p = ip->ip_p;
		fp->ipq_id = ip->ip_id;
		fp->ipq_next = fp->ipq_prev = (struct ipasfrag*)fp;
		fp->ipq_src = ((struct ip*)ip)->ip_src;
		fp->ipq_dst = ((struct ip*)ip)->ip_dst;
		q = (struct ipasfrag*)fp;
		goto insert;
	}
	/* Find a fragment that begins after the one we're trying to insert */
	for (q = fp->ipq_next; q != (struct ipasfrag*)fp; q = q->ipf_next)
		if (q->ip_off > ip->ip_off)
			break;
	/* If we have a preceeding fragment, check for overlaps and discard
	 * the overlapped data from the new fragment
	 */
	if (q->ipf_prev != (struct ipasfrag*)fp) {
		i = q->ipf_prev->ip_off + q->ipf_prev->ip_len - ip->ip_off;
		if (i > 0) {
			/* overlapped! */
			if (i >= ip->ip_len)
				goto dropfrag;
			m_adj(dtom(ip), i);
			ip->ip_off += i;
			ip->ip_len -= i;
		}
	}
	/* Trim overlapping fragments or if they overlap totally simply drop
	 * them 
	 */
	while (q != (struct ipasfrag*)fp && ip->ip_off + ip->ip_len > q->ip_off) {
		i = (ip->ip_off + ip->ip_len) - q->ip_off;
		if (i < q->ip_len) {
			q ->ip_len -= i;
			q->ip_off += i;
			m_adj(dtom(q), i);
			break;
		}
		q = q->ipf_next;
		m_freem(dtom(q->ipf_prev));
		ip_deq(q->ipf_prev);
	}
insert:
	ip_enq(ip, q->ipf_prev);
	next = 0;
	for (q = fp->ipq_next; q != (struct ipasfrag*)fp; q = q->ipf_next) {
		if (q->ip_off != next)
			return NULL;
		next += q->ip_len;
	}
	if (q->ipf_prev->ipf_mff & 1)
		return NULL;
	/* we're the last fragment */
	q = fp->ipq_next;
	m = dtom(q);
	t = m->m_next;
	m->m_next = NULL;
	m_cat(m, t);
	q = q->ipf_next;
	while (q != (struct ipasfrag*)fp) {
		t = dtom(q);
		q = q->ipf_next;
		m_cat(m, t);
	}
	/* create new header */
	ip = fp->ipq_next;
	ip->ip_len = next;
	ip->ipf_mff &= ~1;
	((struct ip*)ip)->ip_src = fp->ipq_src;
	((struct ip*)ip)->ip_dst = fp->ipq_dst;
	remque(fp);
	m_free(dtom(fp));
	m = dtom(ip);
	m->m_len += (ip->ip_hl << 2);
	m->m_data -= (ip->ip_hl << 2);
	if (m->m_flags & M_PKTHDR) {
		int plen = 0;
		for (t=m;m;m = m->m_next)
			plen += m->m_len;
		t->m_pkthdr.len = plen;
	}
	return ((struct ip*)ip);
	
dropfrag:
	ipstat.ips_fragdropped++;
	m_freem(m);
	return NULL;
}

void ipv4_input(struct mbuf *m, int hdrlen)
{
	struct ip *ip;
	struct in_ifaddr *ia = NULL;
	int hlen;
	struct ipq *fp;
	
#if SHOW_DEBUG
	dump_ipv4_header(buf);
#endif
	if (!m)
		return;
	/* If we don't have a pointer to our IP addresses we can't go on */
	if (!ip_ifaddr) {
		ip_ifaddr = get_primary_addr();
		if (!ip_ifaddr) {
			printf("ipv4_input: no interfaces available! (ip_ifaddr == NULL)\n");
			goto bad;
		}
	}

	ipstat.ips_total++;
	/* Get the whole header in the first mbuf */
	if (m->m_len < sizeof(struct ip) && 
	    (m = m_pullup(m, sizeof(struct ip))) == NULL) {
		ipstat.ips_toosmall++;
		return;
	}
	ip = mtod(m, struct ip *);

	/* Check IP version... */
	if (ip->ip_v != IPVERSION) {
		printf("Wrong IP version! %d\n", ip->ip_v);
		ipstat.ips_badvers++;
		goto bad;
	}
	/* Figure out of header length */
	hlen = ip->ip_hl << 2;
	/* Check we're at least the minimum possible length */
	if (hlen < sizeof(struct ip)) {
		ipstat.ips_badhlen++;
		goto bad;
	}
	/* Check again we have the entire header in the first mbuf */
	if (hlen > m->m_len) {
		if ((m = m_pullup(m, hlen)) == NULL) {
			ipstat.ips_badhlen++;
			goto bad;
		}
		ip = mtod(m, struct ip *);
	}

	/* Checksum (should be 0) */	
	if ((ip->ip_sum = in_cksum(m, hlen, 0)) != 0) {
		printf("ipv4_input: checksum failed\n");
		ipstat.ips_badsum++;
		goto bad;
	}

	/* we put the length into host order here... */
	ip->ip_len = ntohs(ip->ip_len);
	/* sanity check. Datagram MUST be longer than the header! */
	if (ip->ip_len < hlen) {
		ipstat.ips_badhlen++;
		goto bad;
	}
	ip->ip_id = ntohs(ip->ip_id);
	ip->ip_off = ntohs(ip->ip_off);
	
	/* the first mbuf should be the packet hdr, so check it's length */
	if (m->m_pkthdr.len < ip->ip_len) {
		ipstat.ips_tooshort++;
		goto bad;
	}

	/* Strip excess data from mbuf */
	if (m->m_pkthdr.len > ip->ip_len) {
		if (m->m_len == m->m_pkthdr.len) {
			m->m_len = ip->ip_len;
			m->m_pkthdr.len = ip->ip_len;
		} else 
			m_adj(m, ip->ip_len - m->m_pkthdr.len);
	}

	/* options processing */	
	ip_nhops = 0;
	if (hlen > sizeof(struct ip) && ip_dooptions(m))
		return;
	
	for (ia = ip_ifaddr;ia; ia = ia->ia_next) {
		if (IA_SIN(ia)->sin_addr.s_addr == ip->ip_dst.s_addr)
			goto ours;
		
		if (ia->ia_ifp == m->m_pkthdr.rcvif && 
		    (ia->ia_ifp->if_flags & IFF_BROADCAST)) {
			uint32 t;
			
			if (satosin(&ia->ia_broadaddr)->sin_addr.s_addr == ip->ip_dst.s_addr)
				goto ours;
			if (ip->ip_dst.s_addr == ia->ia_netbroadcast.s_addr)
				goto ours;
			t = ntohl(ip->ip_dst.s_addr);
			if (t == ia->ia_subnet)
				goto ours;
			if (t == ia->ia_net)
				goto ours;
		}
	}
	
	if (ip->ip_dst.s_addr == (uint32)INADDR_BROADCAST)
		goto ours;
	if (ip->ip_dst.s_addr == INADDR_ANY)
		goto ours;
	
	if (ipforwarding == 0) {
		ipstat.ips_cantforward++;
		m_freem(m);
	} else 
		ip_forward(m, 0);
	return;
ours:
	if (ip->ip_off & ~IP_DF) {
		if (m->m_flags & M_EXT) {
			if ((m = m_pullup(m, sizeof(struct ip))) == NULL) {
				ipstat.ips_toosmall++;
				return;
			}
			ip = mtod(m, struct ip*);
		}
		for (fp = ipq.next; fp != &ipq; fp = fp->next) {
			if (ip->ip_id == fp->ipq_id &&
			    ip->ip_src.s_addr == fp->ipq_src.s_addr &&
			    ip->ip_dst.s_addr == fp->ipq_dst.s_addr &&
			    ip->ip_p == fp->ipq_p)
				goto found;
		}
		fp = NULL;
found:
		ip->ip_len -= hlen;
		((struct ipasfrag*)ip)->ipf_mff &= ~1;
		if (ip->ip_off & IP_MF)
			((struct ipasfrag*)ip)->ipf_mff |= 1;
		ip->ip_off <<= 3;
		if (((struct ipasfrag*)ip)->ipf_mff & 1 || ip->ip_off) {
			ipstat.ips_fragments++;
			ip = ip_reass((struct ipasfrag*)ip, fp);
			if (ip == NULL)
				return;
			ipstat.ips_reassembled++;
			m = dtom(ip);
		} else if (fp)
			ip_freef(fp);
	} else
		ip->ip_len -= hlen;
		
#if SHOW_ROUTE
	/* This just shows which interface we're planning on using */
	printf("Accepting packet [%d] to address %08lx via device %s from src addr %08lx\n",
	       ip->ip_p, ntohl(ip->ip_dst.s_addr), m->m_pkthdr.rcvif->if_name, 
	       ntohl(ip->ip_src.s_addr));
#endif

	ipstat.ips_delivered++;
	if (proto[ip->ip_p] && proto[ip->ip_p]->pr_input) {
		proto[ip->ip_p]->pr_input(m, hlen);
		return;
	} else {
		printf("proto[%d] = %p\n", ip->ip_p, proto[ip->ip_p]);
		goto bad;
	}
	
	return;
bad:
	m_freem(m);
	return;
}

static int ip_optcopy(struct ip *ip, struct ip *jp)
{
	uint8 *cp, *dp;
	int opt, optlen, cnt;
	
	cp =(uint8*)(ip + 1);
	dp = (uint8*)(jp + 1);
	cnt = (ip->ip_hl << 2) - sizeof(struct ip);
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[0];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP) {
			*dp++ = IPOPT_NOP;
			optlen = 1;
			continue;
		} else 
			optlen = cp[IPOPT_OLEN];
		if (optlen > cnt)
			optlen = cnt;
		if (IPOPT_COPIED(opt)) {
			memcpy(dp, cp, optlen);
			dp += optlen;
		}
	}
	for (optlen = dp - (uint8*)(jp + 1); optlen & 0x3; optlen ++)
		*dp++ = IPOPT_EOL;
	return (optlen);
}
		
int ipv4_output(struct mbuf *m0, struct mbuf *opt, struct route *ro,
                int flags, void *optp)
{
	struct mbuf *m = m0;
	struct ip *ip = mtod(m, struct ip*), *mhip;
	struct route iproute; /* temporary route we may need */
	struct sockaddr_in *dst; /* destination address */
	struct in_ifaddr *ia;
	int error = 0, hlen = sizeof(struct ip);
	struct ifnet *ifp = NULL;
	int len, off;

	/* handle options... */
	if (opt) {
		m = ip_insertoptions(m, opt, &len);
		hlen = len;
	}

	ip = mtod(m, struct ip*);
	
	if ((flags & (IP_FORWARDING | IP_RAWOUTPUT)) == 0) {
		ip->ip_v = IPVERSION;
		ip->ip_off &= IP_DF;
		ip->ip_id = htons(ip_id++);
		ip->ip_hl = hlen >> 2;
		ipstat.ips_localout++;
	} else
		hlen = ip->ip_hl << 2;
		
	/* route the packet! */
	if (!ro) {
		ro = &iproute;
		memset(ro, 0, sizeof(iproute));
	}
	dst = (struct sockaddr_in *)&ro->ro_dst;

	if (ro && ro->ro_rt && 
	    ((ro->ro_rt->rt_flags & RTF_UP) == 0 || /* route isn't available */
	     dst->sin_addr.s_addr != ip->ip_dst.s_addr)) { /* not same ip address */
		RTFREE(ro->ro_rt);
		ro->ro_rt = NULL;
	}
	if (ro->ro_rt == NULL) {
		memset(&ro->ro_dst, 0, sizeof(ro->ro_dst));
		dst->sin_family = AF_INET;
		dst->sin_len = sizeof(*dst);
		dst->sin_addr = ip->ip_dst;
	}
	if (flags & IP_ROUTETOIF) {
		/* we're routing to an interface... */
		if (!(ia = ifatoia(ifa_ifwithdstaddr(sintosa(dst)))) &&
		    !(ia = ifatoia(ifa_ifwithnet(sintosa(dst))))) {
			ipstat.ips_noroute++;
			error = ENETUNREACH;
			goto bad;
		}
		ifp = ia->ia_ifp;
		ip->ip_ttl = 1;
	} else {
		/* normal routing */
		if (ro->ro_rt == NULL)
			rtalloc(ro);
		if (ro->ro_rt == NULL) {
			ipstat.ips_noroute++;
			printf("EHOSTUNREACH\n");
			error = EHOSTUNREACH;
			goto bad;
		}

		ia = ifatoia(ro->ro_rt->rt_ifa);
		ifp = ro->ro_rt->rt_ifp;
		atomic_add((volatile long *)&ro->ro_rt->rt_use, 1);
		if (ro->ro_rt->rt_flags & RTF_GATEWAY)
			dst = (struct sockaddr_in *) ro->ro_rt->rt_gateway;
	}
	/* make sure we have an outgoing address. if not yet specified, use the
	 * address of the outgoing interface
	 */
	if (ip->ip_src.s_addr == INADDR_ANY)
		ip->ip_src = IA_SIN(ia)->sin_addr;

	if ((in_broadcast(dst->sin_addr, ifp))) {
		if ((ifp->if_flags & IFF_BROADCAST) == 0) {
			error = EADDRNOTAVAIL;
			goto bad;
		}
		if ((flags & IP_ALLOWBROADCAST) == 0) {
			error = EACCES;
			goto bad;
		}
		if (ip->ip_len > ifp->if_mtu) {
			error = EMSGSIZE;
			goto bad;
		}
		m->m_flags |= M_BCAST;
	} else 
		m->m_flags &= ~M_BCAST;

#if SHOW_ROUTE
	/* This just shows which interface we're planning on using */
	printf("Sending to address %08lx via device %s using src addr %08lx\n",
	       ntohl(ip->ip_dst.s_addr), ifp->if_name, ntohl(ip->ip_src.s_addr));
#endif

	/* if we're small enough, just send the thing! */
	if (ip->ip_len <= ifp->if_mtu) {
		ip->ip_len = htons(ip->ip_len);
		ip->ip_off = htons(ip->ip_off);
		ip->ip_sum = 0;
		ip->ip_sum = in_cksum(m, hlen, 0);
		/* now send the packet! */
		error = (*ifp->output)(ifp, m, (struct sockaddr *)dst, ro->ro_rt);
		goto done;
	}

	/* datagram is too big for interface! */
	/* IP_DF = do not fragment, so if we're too big we have a problem */
	if ((ip->ip_off & IP_DF)) {
		error = EMSGSIZE;
		ipstat.ips_cantfrag++;
		goto bad;
	}
	len = (ifp->if_mtu - hlen) & ~7;
	/* we need at least 8 bytes per fragment... */
	if (len < 8) {
		error = EMSGSIZE;
		goto bad;
	}
	{
		int mhlen, firstlen = len;
		struct mbuf **mnext = &m->m_nextpkt;
		
		m0 = m;
		mhlen = sizeof(struct ip);
		for (off = hlen + len; off < (uint16)ip->ip_len; off += len) {
			m = m_gethdr(MT_HEADER);
			if (!m) {
				error = ENOBUFS;
				ipstat.ips_odropped++;
				goto sendorfree;
			}
			m->m_data += max_linkhdr;
			mhip = mtod(m, struct ip*);
			*mhip = *ip;
			if (hlen > sizeof(struct ip)) {
				mhlen = ip_optcopy(ip, mhip) + sizeof(struct ip);
				mhip->ip_hl = mhlen >> 2;
			}
			m->m_len = mhlen;
			mhip->ip_off = ((off - hlen) >> 3) + (ip->ip_off & ~IP_MF);
			if (ip->ip_off & IP_MF)
				mhip->ip_off |= IP_MF;
			if (off + len >= (uint16)ip->ip_len)
				len = (uint16) ip->ip_len - off;
			else
				mhip->ip_off |= IP_MF;
			mhip->ip_len = htons(len + mhlen);
			m->m_next = m_copym(m0, off, len);
			if (!m->m_next) {
				m_freem(m);
				error = ENOBUFS;
				ipstat.ips_odropped++;
				goto sendorfree;
			}
			m->m_pkthdr.len = mhlen + len;
			m->m_pkthdr.rcvif = NULL;
			mhip->ip_off = htons(mhip->ip_off);
			mhip->ip_sum = 0;
			mhip->ip_sum = in_cksum(m, mhlen, 0);
			*mnext = m;
			mnext = &m->m_nextpkt;
			ipstat.ips_ofragments++;
		}
		m = m0;
		m_adj(m, hlen + firstlen - ip->ip_len);
		m->m_pkthdr.len = hlen + firstlen;
		ip->ip_len = htons(m->m_pkthdr.len);
		ip->ip_off = htons((ip->ip_off | IP_MF));
		ip->ip_sum = 0;
		ip->ip_sum = in_cksum(m, hlen, 0);
sendorfree:
		for (m = m0; m; m = m0) {
			m0 = m->m_nextpkt;
			m->m_nextpkt = NULL;
			if (error == 0)
				error = (*ifp->output)(ifp, m, (struct sockaddr *)dst, ro->ro_rt);
			else
				m_freem(m);
		}
		if (error == 0)
			ipstat.ips_fragmented++;
	}

done:
	if (ro == &iproute && /* we used our own variable */
	    (flags & IP_ROUTETOIF) == 0 && /* we didn't route to an iterface */
	    ro->ro_rt) { /* we have an allocated route */
		RTFREE(ro->ro_rt); /* free the route */
	}

	return error;
bad:
	m_free(m0);
	goto done;
}

/* ??? - can we just use atomic_add() here? */
uint16 get_ip_id(void)
{
	uint16 rv = 0;
	acquire_sem_etc(id_lock, 1, B_CAN_INTERRUPT, 0);
	rv = ip_id++;
	release_sem_etc(id_lock, 1, B_CAN_INTERRUPT);
	return rv;
}

static int ipv4_ctloutput(int op, struct socket *so, int level,
                        int optnum, struct mbuf **mp)
{
	struct inpcb *inp = sotoinpcb(so);
	struct mbuf *m = *mp;
	int optval;
	int error = 0;
	
	if (level != IPPROTO_IP) {
		error = EINVAL;
		if (op == PRCO_SETOPT && *mp)
			m_free(*mp);
	} else {
		switch(op) {
			case PRCO_SETOPT:
				switch(optnum) {
					case IP_OPTIONS:
						/* process options... */
						break;
					case IP_TOS:
					case IP_TTL:
					case IP_RECVOPTS:
					case IP_RECVRETOPTS:
					case IP_RECVDSTADDR:
						if (m->m_len != sizeof(int))
							error = EINVAL;
						else {
							optval = *mtod(m, int*);
							switch (optnum) {
								case IP_TOS:
									inp->inp_ip.ip_tos = optval;
									break;
								case IP_TTL:
									inp->inp_ip.ip_ttl = optval;
									break;
#define OPTSET(bit) \
	if (optval) \
		inp->inp_flags |= bit; \
	else \
		inp->inp_flags &= ~bit;
		
								case IP_RECVOPTS:
									OPTSET(INP_RECVOPTS);
									break;
								case IP_RECVRETOPTS:
									OPTSET(INP_RECVRETOPTS);
									break;
								case IP_RECVDSTADDR:
									OPTSET(INP_RECVDSTADDR);
									break;
							}
						}
						break;
//freeit:
					default:
						error = EINVAL;
						break;
				}
				if (m)
					m_free(m);
				break;
			case PRCO_GETOPT:
				switch(optnum) {
					/* XXX - add the code here */		
					default:
						error = ENOPROTOOPT;
						break;
				}
				break;
		}
	}
	return error;
}

static void ip_slowtimer(void *data)
{
	struct ipq *fp;
	
	fp = ipq.next;
	if (!fp)
		return;
	while (fp != &ipq) {
		--fp->ipq_ttl;
		fp = fp->next;
		if (fp->prev->ipq_ttl == 0) {
			/* timed out! remove it */
			ipstat.ips_fragtimeout++;
			ip_freef(fp->prev);
		}
	}
}

static void ipv4_init(void)
{
	if (ip_id == 0)
		ip_id = real_time_clock() & 0xffff;

	ip_ifaddr = get_primary_addr();
	
	memset(proto, 0, sizeof(struct protosw *) * IPPROTO_MAX);
	add_protosw(proto, NET_LAYER2);

	ipq.next = ipq.prev = &ipq;
	
	timerid = net_add_timer(&ip_slowtimer, NULL, 1000000 / PR_SLOWHZ);
}

struct protosw my_proto = {
	"IPv4",
	IPV4_MODULE_PATH,
	0,
	NULL,
	IPPROTO_IP,
	0,
	NET_LAYER2,
	
	&ipv4_init,
	&ipv4_input,
	&ipv4_output,
	NULL,             /* pr_userreq */
	NULL,             /* pr_sysctl */
	NULL,
	&ipv4_ctloutput,
	
	NULL,
	NULL
};


static int ipv4_module_init(void *cpp)
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

static int ipv4_module_stop(void)
{
	remove_protocol(&my_proto);
	remove_domain(AF_INET);
	
	net_remove_timer(timerid);
	return 0;
}

#ifndef _KERNEL_
void set_core(struct core_module_info *cp)
{
	core = cp;
}
#endif

_EXPORT struct ipv4_module_info protocol_info = {
	{
		{
			IPV4_MODULE_PATH,
			0,
			ipv4_ops
		},
		ipv4_module_init, 
		ipv4_module_stop
	},

#ifndef _KERNEL_
	set_core,
#endif
	
	ipv4_output,
	get_ip_id,
	ipv4_ctloutput,
	ip_srcroute,
	ip_stripoptions,
	ip_srcroute
};

#ifdef _KERNEL_
static status_t ipv4_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			get_module(CORE_MODULE_PATH, (module_info**)&core);
			if (!core)
				return B_ERROR;
			load_driver_symbols("ipv4");
			return B_OK;
		case B_MODULE_UNINIT:
			return B_OK;
		default:
			return B_ERROR;
	}
	return B_OK;
}


_EXPORT module_info *modules[] = {
	(module_info*) &protocol_info,
	NULL
};		
#endif
