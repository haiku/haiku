/* tcp_output.c */

#ifndef _KERNEL_
#include <stdio.h>
#endif

#include "sys/socketvar.h"
#include "sys/protosw.h"
#include "netinet/in.h"
#include "netinet/in_pcb.h"
#include "netinet/ip_var.h"
#include "netinet/tcp.h"
#include "netinet/tcp_timer.h"
#include "netinet/tcp_var.h"
#include "netinet/tcpip.h"
#include "netinet/tcp_seq.h"
#define TCPOUTFLAGS
#include "netinet/tcp_fsm.h"
#include "netinet/tcp_debug.h"

#include "core_module.h"
#include "core_funcs.h"
#include "ipv4/ipv4_module.h"

#ifdef _KERNEL_
#include <KernelExport.h>

#endif

extern struct core_module_info *core;
extern struct ipv4_module_info *ipm;
extern struct pool_ctl *tcppool;

#define  roundup(x, y)   ((((x)+((y)-1))/(y))*(y))
extern uint32 sb_max; /* defined in socketvar.h */

void tcp_setpersist(struct tcpcb *tp)
{
	int t = ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1;
	
	if (tp->t_timer[TCPT_REXMT]) {
		printf("PANIC: tcp_output REXMT\n");
		return;
	}
	/* Start/reset the persistance timer */
	TCPT_RANGESET(tp->t_timer[TCPT_PERSIST],
	              t * tcp_backoff[tp->t_rxtshift],
	              TCPTV_PERSMIN, TCPTV_PERSMAX);
	if (tp->t_rxtshift < TCP_MAXRXTSHIFT)
		tp->t_rxtshift++;
}

int tcp_mss(struct tcpcb *tp, uint offer)
{
	struct route *ro;
	struct rtentry *rt;
	struct ifnet *ifp;
	int rtt, mss;
	uint32 bufsize;
	struct inpcb *inp;
	struct socket *so;
	
	inp = tp->t_inpcb;
	ro = &inp->inp_route;

	if ((rt = ro->ro_rt) == NULL) {
		/* don't have a route, get one if we can */
		if (inp->faddr.s_addr != INADDR_ANY) {
			memset(&ro->ro_dst, 0, sizeof(ro->ro_dst));
			ro->ro_dst.sa_family = AF_INET;
			ro->ro_dst.sa_len = sizeof(ro->ro_dst);
			((struct sockaddr_in *)&ro->ro_dst)->sin_addr = inp->faddr;
			rtalloc(ro);
		}
		if ((rt = ro->ro_rt) == NULL)
			return tcp_mssdflt;
	}
	ifp=rt->rt_ifp;
	so = inp->inp_socket;
	
	if (tp->t_srtt == 0 && (rtt = rt->rt_rmx.rmx_rtt)) {
		if (rt->rt_rmx.rmx_locks & RTV_RTT)
			tp->t_rttmin = rtt / (RTM_RTTUNIT / PR_SLOWHZ);
		tp->t_srtt = rtt / (RTM_RTTUNIT / (PR_SLOWHZ * TCP_RTT_SCALE));
		
		if (rt->rt_rmx.rmx_rttvar)
			tp->t_rttvar = rt->rt_rmx.rmx_rttvar / 
			               (RTM_RTTUNIT / (PR_SLOWHZ * TCP_RTTVAR_SCALE));
		else
			tp->t_rttvar = tp->t_srtt * TCP_RTTVAR_SCALE / TCP_RTT_SCALE;
		
		TCPT_RANGESET(tp->t_rxtcur,
		              ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1,
		              tp->t_rttmin, TCPTV_REXMTMAX);
	}
	
	if (rt->rt_rmx.rmx_mtu)
		mss = rt->rt_rmx.rmx_mtu - sizeof(struct tcpiphdr);
	else {
		mss = ifp->if_mtu - sizeof(struct tcpiphdr);
#if (MCLBYTES & (MCLBYTES - 1)) == 0
		if (mss > MCLBYTES)
			mss &=~ (MCLBYTES - 1);
else
		if (mss > MCLBYTES)
			mss = mss / MCLBYTES * MCLBYTES;
#endif
		if (!in_localaddr(inp->faddr))
			mss = min(mss, tcp_mssdflt);
	}
	
	if (offer) 
		mss = min(mss, offer);
	
	mss = max(mss, 32);
	if (mss < tp->t_maxseg || offer != 0) {
		if ((bufsize = rt->rt_rmx.rmx_sendpipe) == 0)
			bufsize = so->so_snd.sb_hiwat;
		if (bufsize < mss)
			mss = bufsize;
		else {
			bufsize = roundup(bufsize, mss);
			if (bufsize > sb_max)
				bufsize = sb_max;
			sbreserve(&so->so_snd, bufsize);
		}
		tp->t_maxseg = mss;
		
		if ((bufsize = rt->rt_rmx.rmx_recvpipe) == 0)
			bufsize = so->so_rcv.sb_hiwat;
		if (bufsize > mss) {
			bufsize = roundup(bufsize, mss);
			if (bufsize > sb_max)
				bufsize = sb_max;
			sbreserve(&so->so_rcv, bufsize);
		}
	}
	tp->snd_cwnd = mss;
	if (rt->rt_rmx.rmx_ssthresh) {
		tp->snd_ssthresh = max(2 * mss, rt->rt_rmx.rmx_ssthresh);
	}
	return mss;
}

void tcp_quench(struct inpcb *inp, int error)
{
	struct tcpcb *tp = intotcpcb(inp);
	
	if(tp)
		tp->snd_cwnd = tp->t_maxseg;
}

int tcp_output(struct tcpcb *tp)
{
	struct socket *so = tp->t_inpcb->inp_socket;
	int32 len, win;
	int off, flags, error = 0;
	struct mbuf *m;
	struct tcpiphdr *ti;
	u_char opt[MAX_TCPOPTLEN];
	uint optlen, hdrlen;
	int idle, sendalot;

	idle = (tp->snd_max == tp->snd_una);

	if (idle && tp->t_idle >= tp->t_rxtcur)
		/* Basically have we been idle for a while?
		 * If so, slow start to get ack "clock" running again
		 */
		tp->snd_cwnd = tp->t_maxseg;
		
again:
	sendalot = 0;
	
	off = tp->snd_nxt - tp->snd_una;
	win = min(tp->snd_wnd, tp->snd_cwnd);
	
	flags = tcp_outflags[tp->t_state];
	/* if we're in a persist window of 0, send 1 byte
	 * Otherwise, if we have a small but nonzero window and
	 * the timer has expired, send what we can and go to
	 * transmit state
	 */
	if (tp->t_force) {
		if (win == 0) {
			/* If we have data to send, clear the FIN bit.
			 */
			if (off < so->so_snd.sb_cc)
				flags &= ~TH_FIN;
			win = 1;
		} else {
			tp->t_timer[TCPT_PERSIST] = 0;
			tp->t_rxtshift = 0;
		}
	}
	len = min(so->so_snd.sb_cc, win) - off;
	if (len < 0) {
		/* if FIN has been sent but not ack'd,
		 * but we haven't been asked to retransmit, len would be -1
		 * Otherwise window shrank after we sent into it. If window
		 * shrank to 0, cancel pending transmit and pull snd_nxt
		 * back to (closed) window.
		 */
		len = 0;
		if (win == 0) {
			tp->t_timer[TCPT_REXMT] = 0;
			tp->snd_nxt = tp->snd_una;
		}
	}
	if (len > tp->t_maxseg) {
		len = tp->t_maxseg;
		sendalot = 1;
	}
	if (SEQ_LT(tp->snd_nxt + len, tp->snd_una + so->so_snd.sb_cc))
		flags &= ~TH_FIN;
	
	win = sbspace(&so->so_rcv);

	/* Do we have a reason to send anything? */
	if (len) {
		if (len == tp->t_maxseg)
			goto send;
		if ((idle || tp->t_flags & TF_NODELAY) &&
		    len + off >= so->so_snd.sb_cc)
			goto send;
		if (tp->t_force)
			goto send;
		if (len >= tp->max_sndwnd / 2)
			goto send;
		if (SEQ_LT(tp->snd_nxt, tp->snd_max))
			goto send;
	}
	if (win > 0) {
		int32 adv = min(win, (int32)TCP_MAXWIN << tp->rcv_scale) - 
		            (tp->rcv_adv - tp->rcv_nxt);
		if (adv >= (int32)(2 * tp->t_maxseg))
			goto send;
		if (2 * adv >= (int32)so->so_rcv.sb_hiwat)
			goto send;
	}
	if (tp->t_flags & TF_ACKNOW)
		goto send;
	if (flags & (TH_SYN | TH_RST))
		goto send;
	if (SEQ_GT(tp->snd_up, tp->snd_una))
		goto send;
	if ((flags & TH_FIN) && ((tp->t_flags & TF_SENTFIN) == 0 ||
	    tp->snd_nxt == tp->snd_una))
		goto send;
	if (so->so_snd.sb_cc && tp->t_timer[TCPT_REXMT] == 0 &&
	    (tp->t_timer[TCPT_PERSIST] == 0)) {
		tp->t_rxtshift = 0;
		tcp_setpersist(tp);
	}
	
	/* We don't have a reason to send anything for this connection,
	 * so just return.
	 */		
	return 0;
send:
	
	optlen = 0;
	hdrlen = sizeof(struct tcpiphdr);
	if (flags & TH_SYN) {
		tp->snd_nxt = tp->iss;
		if ((tp->t_flags & TF_NOOPT) == 0) {
			uint16 mss;
			
			opt[0] = TCPOPT_MAXSEG;
			opt[1] = 4;
			mss = htons((uint16)tcp_mss(tp, 0));
			memcpy((caddr_t)opt + 2, &mss, sizeof(mss));
			optlen = 4;
			if ((tp->t_flags & TF_REQ_SCALE) &&
			    ((flags & TH_ACK) == 0 ||
			    (tp->t_flags & TF_RCVD_SCALE))) {
				*((uint32*)(opt + optlen)) = htonl (TCPOPT_NOP << 24 |
				                                    TCPOPT_WINDOW << 16 |
				                                    TCPOLEN_WINDOW << 8 |
				                                    tp->request_r_scale);
				optlen += 4;
			}
		}
	}
	if ((tp->t_flags & (TF_REQ_TSTMP | TF_NOOPT)) == TF_REQ_TSTMP &&
	    (flags & TH_RST) == 0 &&
	    ((flags & (TH_SYN | TH_ACK)) == TH_SYN ||
	     (tp->t_flags & TF_RCVD_TSTMP))) {
		uint32 *lp = (uint32*)(opt + optlen);
		*lp++ = htonl(TCPOPT_TSTAMP_HDR);
		*lp++ = htonl(tcp_now);
		*lp = htonl(tp->ts_recent);
		optlen += TCPOLEN_TSTAMP_APPA;
	}
	hdrlen += optlen;
	
	if (len > tp->t_maxseg - optlen) {
		len = tp->t_maxseg - optlen;
		sendalot = 1;
	}
	
	if (len) {
		if (tp->t_force && len == 1)
			tcpstat.tcps_sndprobe++;
		else if (SEQ_LT(tp->snd_nxt, tp->snd_max)) {
			tcpstat.tcps_sndrexmitpack++;
			tcpstat.tcps_sndrexmitbyte += len;
		} else {
			tcpstat.tcps_sndpack++;
			tcpstat.tcps_sndbyte += len;
		}
		m = m_gethdr(MT_HEADER);
		if (m == NULL) {
			printf("tcp_output: ENOBUFS\n");
			error = ENOBUFS;
			goto out;
		}
		m->m_data += max_linkhdr;
		m->m_len = hdrlen;
		if (len <= MHLEN - hdrlen - max_linkhdr) {
			m_copydata(so->so_snd.sb_mb, off, (int)len, mtod(m, caddr_t) + hdrlen);
			m->m_len += len;
		} else {
			m->m_next = m_copym(so->so_snd.sb_mb, off, (int)len);
			if (m->m_next == NULL)
				len = 0;
		}
		if (off + len == so->so_snd.sb_cc)
			flags |= TH_PUSH;
	} else {
		if (tp->t_flags & TF_ACKNOW)
			tcpstat.tcps_sndacks++;
		else if (flags & (TH_SYN | TH_FIN | TH_RST))
			tcpstat.tcps_sndctrl++;
		else if (SEQ_GT(tp->snd_up, tp->snd_una))
			tcpstat.tcps_sndurg++;
		else
			tcpstat.tcps_sndwinup++;
		
		m = m_gethdr(MT_HEADER);
		if (m == NULL) {
			printf("tcp_output: ENOBUFS\n");
			error = ENOBUFS;
			goto out;
		}
		m->m_data += max_linkhdr;
		m->m_len = hdrlen;
	}
	m->m_pkthdr.rcvif = NULL;
	ti = mtod(m, struct tcpiphdr*);
	if (tp->t_template == NULL)
		printf("tcp_output: PANIC t_template == NULL\n");
	memcpy((caddr_t)ti, (caddr_t)tp->t_template, sizeof(struct tcpiphdr));
	
	if (flags & TH_FIN && (tp->t_flags & TF_SENTFIN) &&
	    (tp->snd_nxt == tp->snd_max))
		tp->snd_nxt--;
	
	if (len || (flags & (TH_SYN | TH_FIN)) || tp->t_timer[TCPT_PERSIST])
		ti->ti_seq = htonl(tp->snd_nxt);
	else
		ti->ti_seq = htonl(tp->snd_max);

	ti->ti_ack = htonl(tp->rcv_nxt);
	
	if (optlen) {
		memcpy((caddr_t)(ti + 1), (caddr_t)opt, optlen);
		ti->ti_off = (sizeof(struct tcphdr) + optlen) >> 2;
	}
	ti->ti_flags = flags;

	if (win < (int32)(so->so_rcv.sb_hiwat / 4) &&
	    win < (int32) tp->t_maxseg)
		win = 0;
	if (win > (int32) TCP_MAXWIN << tp->rcv_scale)
		win = (int32) TCP_MAXWIN << tp->rcv_scale;
	if (win < (int32)(tp->rcv_adv - tp->rcv_nxt))
		win = (int32)(tp->rcv_adv - tp->rcv_nxt);
	ti->ti_win = htons((uint16)(win >> tp->rcv_scale));
	
	if (SEQ_GT(tp->snd_up, tp->snd_nxt)) {
		ti->ti_urp = htons((uint16)(tp->snd_up - tp->snd_nxt));
		ti->ti_flags |= TH_URG;
	} else
		tp->snd_up = tp->snd_una;
	
	if (len + optlen)
		ti->ti_len = htons((uint16)(sizeof(struct tcphdr) + optlen + len));
	ti->ti_sum = in_cksum(m, (int)(hdrlen+len), 0);
	
	if (tp->t_force == 0 || tp->t_timer[TCPT_PERSIST] == 0) {
		tcp_seq startseq = tp->snd_nxt;
		
		if (flags & (TH_SYN | TH_FIN)) {
			tp->snd_nxt++;
			if (flags & TH_FIN)
				tp->t_flags |= TF_SENTFIN;
		}
		tp->snd_nxt += len;
		if (SEQ_GT(tp->snd_nxt, tp->snd_max)) {
			tp->snd_max = tp->snd_nxt;
			if (tp->t_rtt == 0) {
				tp->t_rtt = 1;
				tp->t_rtseq = startseq;
				tcpstat.tcps_segstimed++;
			}
		}
		if (tp->t_timer[TCPT_REXMT] == 0 &&
		    tp->snd_nxt != tp->snd_una) {
			tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
			if (tp->t_timer[TCPT_PERSIST]) {
				tp->t_timer[TCPT_PERSIST] = 0;
				tp->t_rxtshift = 0;
			}
		}
	} else if (SEQ_GT(tp->snd_nxt + len, tp->snd_max))
		tp->snd_max = tp->snd_nxt + len;
	
	if (so->so_options & SO_DEBUG)
		tcp_trace(TA_OUTPUT, tp->t_state, tp, ti, 0, len);
	
	m->m_pkthdr.len = hdrlen + len;
	((struct ip*)ti)->ip_len = m->m_pkthdr.len;
	((struct ip*)ti)->ip_ttl = tp->t_inpcb->inp_ip.ip_ttl;
	((struct ip*)ti)->ip_tos = tp->t_inpcb->inp_ip.ip_tos;
	error = ipm->output(m, tp->t_inpcb->inp_options, &tp->t_inpcb->inp_route,
	                    so->so_options & SO_DONTROUTE, NULL);
	
	if (error) {
out:
		if (error == ENOBUFS) {
			tcp_quench(tp->t_inpcb, 0);
			return 0;
		}
		if ((error == EHOSTUNREACH || error == ENETDOWN) &&
		    TCPS_HAVERCVDSYN(tp->t_state)) {
			tp->t_softerror = error;
			return 0;
		}
		return error;
	}
	
	tcpstat.tcps_sndtotal++;
	
	if (win > 0 && SEQ_GT(tp->rcv_nxt + win, tp->rcv_adv))
		tp->rcv_adv = tp->rcv_nxt + win;
	tp->last_ack_sent = tp->rcv_nxt;
	tp->t_flags &= ~(TF_ACKNOW | TF_DELACK);
	
	if (sendalot)
		goto again;
	
	return 0;
}
