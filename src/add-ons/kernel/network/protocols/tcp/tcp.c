/* udp.c
 */

#ifndef _KERNEL_
#include <stdio.h>
#include <string.h>
#endif

#include "pools.h"
#include "net_misc.h"
#include "protocols.h"
#include "netinet/in_systm.h"
#include "netinet/in_var.h"
#include "netinet/in_pcb.h"
#include "netinet/ip.h"
#include "sys/domain.h"
#include "sys/protosw.h"
#include "netinet/ip_var.h"
#include "netinet/tcp.h"
#include "netinet/tcp_timer.h"
#include "netinet/tcp_fsm.h"
#include "netinet/tcp_seq.h"
#include "netinet/tcp_var.h"
#include "netinet/tcpip.h"

#include "core_module.h"
#include "net_module.h"
#include "core_funcs.h"
#include "ipv4/ipv4_module.h"
#include "net_timer.h"

#ifdef _KERNEL_
#include <KernelExport.h>
#define TCP_MODULE_PATH		"network/protocol/tcp"
static status_t tcp_ops(int32 op, ...);
#else	/* _KERNEL_ */
#define tcp_ops NULL
#define TCP_MODULE_PATH	    "modules/protocol/tcp"
static image_id ipid = -1;
#endif

struct core_module_info *core = NULL;
struct ipv4_module_info *ipm = NULL;

/* Declaration as we don't have it natively... */
uint32 arc4random();

struct protosw *proto[IPPROTO_MAX];
struct pool_ctl *tcppool = NULL;

/* patchable/settable parameters for tcp */
int	tcp_mssdflt = TCP_MSS;
int	tcp_rttdflt = TCPTV_SRTTDFLT / PR_SLOWHZ;
static net_timer_id slowtim;
static net_timer_id fasttim;

struct inpcb *tcp_last_inpcb = NULL;

static uint32 tcp_sendspace = 8192;	/* size of send buffer */
static uint32 tcp_recvspace = 8192;	/* size of recieve buffer */

void tcp_init(void)
{
	tcp_now = arc4random() / 2;
	tcp_iss = 1;
	
	tcb.inp_next = tcb.inp_prev = &tcb;
	if (max_protohdr < sizeof(struct tcpiphdr))
		max_protohdr = sizeof(struct tcpiphdr);
	memset(&tcpstat, 0, sizeof(struct tcpstat));
		
	memset(proto, 0, sizeof(struct protosw *) * IPPROTO_MAX);
	add_protosw(proto, NET_LAYER4);

	if (!tcppool)
		pool_init(&tcppool, sizeof(struct tcpcb));

	/* Add timers... */
	/* Assuming we're using usecs, then we call PR_SLOWHZ per sec
	 * which is 1,000,000 / PR_SLOWHZ
	 */
	slowtim = net_add_timer(&tcp_slowtimer, NULL, 1000000 / PR_SLOWHZ);
	fasttim = net_add_timer(&tcp_fasttimer, NULL, 1000000 / PR_FASTHZ);
}

struct tcpiphdr *tcp_template(struct tcpcb *tp)
{
	struct inpcb *inp = tp->t_inpcb;
	struct mbuf *m;
	struct tcpiphdr *n = NULL;
	
	if ((n = tp->t_template) == NULL) {
		m = m_get(MT_HEADER);
		if (m == NULL)
			return NULL;
		m->m_len = sizeof(struct tcpiphdr);
		n = mtod(m, struct tcpiphdr*);
	}
	/* ??? maybe we should just memset 0 and then fill in what we need? */
	n->ti_next = n->ti_prev = NULL;
	n->ti_x1 = 0;
	n->ti_pr = IPPROTO_TCP;
	n->ti_len = htons(sizeof(struct tcpiphdr) - sizeof(struct ip));
	n->ti_src = inp->laddr;
	n->ti_dst = inp->faddr;
	n->ti_sport = inp->lport;
	n->ti_dport = inp->fport;
	n->ti_seq = 0;
	n->ti_ack = 0;
	n->ti_x2 = 0;
	n->ti_off = 5;
	n->ti_flags = 0;
	n->ti_win = 0;
	n->ti_sum = 0;
	n->ti_urp = 0;
	return n;
}

struct tcpcb *tcp_close(struct tcpcb *tp)
{
	struct tcpiphdr *t;
	struct inpcb *inp = tp->t_inpcb;
	struct socket *so = inp->inp_socket;
	struct mbuf *m;
	struct rtentry *rt;

	/* did we send enough data to get some meaningful iformation?
	 * If we did save it in the routing entry.
	 * We define enough as being the sendpipesize (default 8k) x 16
	 * which should give us 16 rtt samples, assuming of course we only
	 * have one sample per frame.
	 * 16 samples is enough for the srtt filter to converge to within 5%
	 * of the correct value.
	 *
	 * We don't however update the default route or anything else that the
	 * user has locked.
	 */
	if (SEQ_LT(tp->iss + so->so_snd.sb_hiwat * 16, tp->snd_max) &&
	    (rt = inp->inp_route.ro_rt) &&
	    ((struct sockaddr_in*)rt_key(rt))->sin_addr.s_addr != INADDR_ANY) {
		uint32 i;
		
		if ((rt->rt_rmx.rmx_locks & RTV_RTT) == 0) {
			i = tp->t_srtt * (RTM_RTTUNIT / (PR_SLOWHZ * TCP_RTT_SCALE));
			if (rt->rt_rmx.rmx_rtt && i)
				/*
				 * update this value with half the old and new values,
				 * converting scale.
				 */
				rt->rt_rmx.rmx_rtt = (rt->rt_rmx.rmx_rtt + i) / 2;
			else
				rt->rt_rmx.rmx_rtt = i;
		}
		if ((rt->rt_rmx.rmx_locks & RTV_RTTVAR) == 0) {
			i = tp->t_rttvar * (RTM_RTTUNIT / (PR_SLOWHZ * TCP_RTTVAR_SCALE));
			if (rt->rt_rmx.rmx_rttvar && i)
				rt->rt_rmx.rmx_rttvar = (rt->rt_rmx.rmx_rttvar + i) / 2;
			else
				rt->rt_rmx.rmx_rttvar = i;
		}
		/* update the pipelimit (ssthresh) */
		if ((rt->rt_rmx.rmx_locks & RTV_SSTHRESH) == 0 &&
		    ((i = tp->snd_ssthresh) && (rt->rt_rmx.rmx_ssthresh ||
		    i < (rt->rt_rmx.rmx_sendpipe / 2)))) {
			/* convert the limit from user data bytes to
			 * packets and then to packet data bytes
			 */
			i = (i + tp->t_maxseg / 2) / tp->t_maxseg;
			if (i < 2)
				i = 2;
			i *= (u_long)(tp->t_maxseg + sizeof(struct tcpiphdr));
			if (rt->rt_rmx.rmx_ssthresh)
				rt->rt_rmx.rmx_ssthresh = (rt->rt_rmx.rmx_ssthresh + i) / 2;
			else
				rt->rt_rmx.rmx_ssthresh = i;
		}
	}
	/* free our reassembly queue */
	t = tp->seg_next;
	while (t != (struct tcpiphdr*)tp) {
		t = (struct tcpiphdr*)t->ti_next;
		m = REASS_MBUF((struct tcpiphdr*)t->ti_prev);
		remque(t->ti_prev);
		m_freem(m);
	}
	if (tp->t_template)
		(void)m_free(dtom(tp->t_template));

	pool_put(tcppool, tp);
	inp->inp_ppcb = NULL;
	soisdisconnected(so);
	if (inp == tcp_last_inpcb)
		tcp_last_inpcb = &tcb;
	in_pcbdetach(inp);
	tcpstat.tcps_closed++;

	return NULL;
}

struct tcpcb *tcp_drop(struct tcpcb *tp, int error)
{
	struct socket *so = tp->t_inpcb->inp_socket;

	if (TCPS_HAVERCVDSYN(tp->t_state)) {
		tp->t_state = TCPS_CLOSED;
		(void) tcp_output(tp);
		tcpstat.tcps_drops++;
	} else
		tcpstat.tcps_conndrops++;
	
	if (error == ETIMEDOUT && tp->t_softerror)
		error = tp->t_softerror;
	so->so_error = error;
	return tcp_close(tp);
}

void tcp_respond(struct tcpcb *tp, struct tcpiphdr *ti, struct mbuf *m,
                 tcp_seq ack, tcp_seq seq, int flags)
{
	int tlen;
	int win = 0;
	struct route *ro = NULL;

	if (tp) {
		win = sbspace(&tp->t_inpcb->inp_socket->so_rcv);
		ro = &tp->t_inpcb->inp_route;
	}
	if (m == NULL) {
		m = m_gethdr(MT_HEADER);
		if (!m)
			return;
		tlen = 0;
		m->m_data += max_linkhdr;
		*mtod(m, struct tcpiphdr*) = *ti;
		ti = mtod(m, struct tcpiphdr*);
		flags = TH_ACK;
	} else {
		m_freem(m->m_next);
		m->m_next = NULL;
		m->m_data = (caddr_t) ti;
		m->m_len = sizeof(struct tcpiphdr);
		tlen = 0;
#define xchng(a,b,type) { type t; t=a; a=b; b= t; }
		xchng(ti->ti_dst.s_addr, ti->ti_src.s_addr, uint32);
		xchng(ti->ti_dport, ti->ti_sport, uint16);
#undef  xchng
	}
	ti->ti_len = htons((uint16)(sizeof(struct tcphdr) + tlen));
	tlen += sizeof(struct tcpiphdr);
	m->m_len = tlen;
	m->m_pkthdr.len = tlen;
	m->m_pkthdr.rcvif = NULL;
	ti->ti_next = ti->ti_prev = NULL;
	ti->ti_x1 = 0;
	ti->ti_seq = htonl(seq);
	ti->ti_ack = htonl(ack);
	ti->ti_x2 = 0;
	ti->ti_off = sizeof(struct tcphdr) >> 2;
	ti->ti_flags = flags;
	if (tp)
		ti->ti_win = htons((uint16)(win >> tp->rcv_scale));
	else
		ti->ti_win = htons((uint16)win);
	ti->ti_urp = 0;
	ti->ti_sum = 0;
	ti->ti_sum = in_cksum(m, tlen, 0);
	((struct ip*)ti)->ip_len = tlen;
	((struct ip*)ti)->ip_ttl = 64;/* XXX - ip_defttl; */
	ipm->output(m, NULL, ro, 0, NULL);
}
	
struct tcpcb *tcp_usrclosed(struct tcpcb *tp)
{
	switch(tp->t_state) {
		case TCPS_CLOSED:
		case TCPS_LISTEN:
		case TCPS_SYN_SENT:
			tp->t_state = TCPS_CLOSED;
			tp = tcp_close(tp);
			break;
		case TCPS_SYN_RECEIVED:
		case TCPS_ESTABLISHED:
			tp->t_state = TCPS_FIN_WAIT_1;
			break;
		case TCPS_CLOSE_WAIT:
			tp->t_state = TCPS_LAST_ACK;
			break;
	}
	if (tp && tp->t_state >= TCPS_FIN_WAIT_2)
		soisdisconnected(tp->t_inpcb->inp_socket);
	return tp;
}

static struct tcpcb *tcp_disconnect(struct tcpcb *tp)
{
	struct socket *so = tp->t_inpcb->inp_socket;
	
	if (tp->t_state < TCPS_ESTABLISHED)
		tp = tcp_close(tp);
	else if ((so->so_options & SO_LINGER) && so->so_linger == 0)
		tp = tcp_drop(tp, 0);
	else {
		soisdisconnecting(so);
		sbflush(&so->so_rcv);
		tp = tcp_usrclosed(tp);
		if (tp)
			tcp_output(tp);
	}
	return tp;
}

static struct tcpcb * tcp_newtcpcb(struct inpcb *inp)
{
	struct tcpcb *tp;
	tp = (struct tcpcb*)pool_get(tcppool);
	if (!tp)
		return NULL;
	memset(tp, 0, sizeof(*tp));
	tp->seg_next = tp->seg_prev = (struct tcpiphdr*)tp;
	tp->t_maxseg = tcp_mssdflt;
	tp->t_flags = tcp_do_rfc1323 ? (TF_REQ_SCALE | TF_REQ_TSTMP) : 0;
	tp->t_inpcb = inp;
	
	tp->t_srtt = TCPTV_SRTTBASE;
	tp->t_rttvar = tcp_rttdflt * PR_SLOWHZ << 2;
	tp->t_rttmin = TCPTV_MIN;
	TCPT_RANGESET(tp->t_rxtcur, ((TCPTV_SRTTBASE >> 2) + (TCPTV_SRTTDFLT << 2)) >> 1,
	              TCPTV_MIN, TCPTV_REXMTMAX);
	tp->snd_cwnd = tp->snd_ssthresh = TCP_MAXWIN << TCP_MAX_WINSHIFT;

	inp->inp_ip.ip_ttl = 64;/* XXX - ip_defttl; */
	inp->inp_ppcb = (caddr_t)tp;
	return tp;
}
	
static int tcp_attach(struct socket *so)
{
	struct inpcb *inp;
	struct tcpcb *tp;
	int error = 0;

	if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
		error = soreserve(so, tcp_sendspace, tcp_recvspace);
		if (error)
			return error;
	}
	error = in_pcballoc(so, &tcb);
	if (error)
		return error;
	inp = sotoinpcb(so);
	tp = tcp_newtcpcb(inp);
	if (tp == NULL) {
		/* we don't want to free the socket just yet, so
		 * record the setting of SS_NOFDREF, then clear the bit,
		 * detach and then reset the bit.
		 */
		int nofd = so->so_state & SS_NOFDREF;
		so->so_state &= ~SS_NOFDREF;
		in_pcbdetach(inp);
		so->so_state |= nofd;
		return ENOBUFS;
	}
	tp->t_state = TCPS_CLOSED;
	return 0;
}

int tcp_userreq(struct socket *so, int req, struct mbuf *m, 
                struct mbuf *nam, struct mbuf *control)
{
	struct inpcb *inp;
	struct tcpcb *tp = NULL;
	int error = 0;
	int ostate;
		
	if (req == PRU_CONTROL)
		return in_control(so, (int)m, (caddr_t)nam, (struct ifnet *)control);
	if (control && control->m_len) {
		m_freem(control);
		if (m)
			m_freem(m);
		return EINVAL;
	}
	
	inp = sotoinpcb(so);
	/* When we're attached, the inpcb points at the socket */
	if (inp == NULL && req != PRU_ATTACH)
		return EINVAL;
	
	if (inp) {
		tp = intotcpcb(inp);
		ostate = tp->t_state;
	} else
		ostate = 0;
	
	switch(req) {
		case PRU_ATTACH:
			if (inp) {
				error = EISCONN;
				break;
			}
			error = tcp_attach(so);
			if (error)
				break;
			if ((so->so_options & SO_LINGER) && so->so_linger == 0)
				so->so_linger = TCP_LINGERTIME;
			tp = sototcpcb(so);
			break;
		case PRU_DETACH:
			if (tp->t_state > TCPS_LISTEN)
				tp = tcp_disconnect(tp);
			else
				tp = tcp_close(tp);
			break;
		case PRU_BIND:
			error = in_pcbbind(inp, nam);
			break;
		case PRU_LISTEN:
			if (inp->lport == 0)
				error = in_pcbbind(inp, NULL);
			if (error == 0)
				tp->t_state = TCPS_LISTEN;
			break;
		case PRU_CONNECT:
			if (inp->lport == 0) {
				error = in_pcbbind(inp, NULL);
				if (error)
					break;
			}
										
			error = in_pcbconnect(inp, nam);
			if (error) {
				printf("in_pcbconnect gave error %d\n", error);
				break;
			}
			tp->t_template = tcp_template(tp);
			if (tp->t_template == NULL) {
				in_pcbdisconnect(inp);
				error = ENOBUFS;
				break;
			}
			while (tp->request_r_scale < TCP_MAX_WINSHIFT &&
			       (TCP_MAXWIN << tp->request_r_scale) < so->so_rcv.sb_hiwat)
				tp->request_r_scale++;
			soisconnecting(so);
			tcpstat.tcps_connattempt++;
			tp->t_state = TCPS_SYN_SENT;
			tp->t_timer[TCPT_KEEP] = TCPTV_KEEP_INIT;
			
			tp->iss = tcp_iss;
			tcp_iss += TCP_ISSINCR / 2;
			tcp_sendseqinit(tp);
			error = tcp_output(tp);
			break;
		case PRU_CONNECT2:
			error = EOPNOTSUPP;
			break;
		case PRU_DISCONNECT:
			tp = tcp_disconnect(tp);
			break;
		case PRU_ACCEPT:
			in_setpeeraddr(inp, nam);
			break;
		case PRU_SLOWTIMO:
			tp = tcp_timers(tp, (int)nam);
			req |= (int)nam << 8;
			break;
		case PRU_SEND:
			sbappend(&so->so_snd, m);
			error = tcp_output(tp);
			break;
		case PRU_RCVD:
			(void) tcp_output(tp);
			break;
		case PRU_SHUTDOWN:
			socantsendmore(so);
			tp = tcp_usrclosed(tp);
			if (tp)
				error = tcp_output(tp);
			break;
		case PRU_SOCKADDR:
			in_setsockaddr(inp, nam);
			break;
		case PRU_PEERADDR:
			in_setpeeraddr(inp, nam);
			break;					
	}
/* XXX - add tcp_trace!	
	if (tp && (so->so_options & SO_DEBUG))
*/		
	return error;
}

static struct protosw my_proto = {
	"TCP Module",
	TCP_MODULE_PATH,
	SOCK_STREAM,
	NULL,
	IPPROTO_TCP,
	PR_CONNREQUIRED | PR_WANTRCVD,
	NET_LAYER3,
	
	&tcp_init,
	&tcp_input,            /* pr_input     */
	NULL,                  /* pr_output    */
	&tcp_userreq,          /* pr_userreq   */
	NULL,                  /* pr_sysctl    */
	NULL,                  /* pr_ctlinput  */
	NULL,                  /* pr_ctloutput */
		
	NULL,
	NULL
};


static int tcp_module_init(void *cpp)
{
	if (cpp)
		core = cpp;

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

static int tcp_module_stop(void)
{
	net_remove_timer(slowtim);
	net_remove_timer(fasttim);

#ifndef _KERNEL_
	unload_add_on(ipid);
#else
	put_module(IPV4_MODULE_PATH);
#endif

	remove_protocol(&my_proto);
	remove_domain(AF_INET);
	
	return 0;
}
	
_EXPORT struct kernel_net_module_info protocol_info = {
	{
		TCP_MODULE_PATH,
		0,
		tcp_ops
	},
	tcp_module_init,
	tcp_module_stop
};

#ifdef _KERNEL_
static status_t tcp_ops(int32 op, ...)
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
