/*
 * Copyright (c) 1982, 1986, 1988, 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)COPYRIGHT	1.1 (NRL) 17 January 1995
 * 
 * NRL grants permission for redistribution and use in source and binary
 * forms, with or without modification, of the software and documentation
 * created at NRL provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgements:
 * 	This product includes software developed by the University of
 * 	California, Berkeley and its contributors.
 * 	This product includes software developed at the Information
 * 	Technology Division, US Naval Research Laboratory.
 * 4. Neither the name of the NRL nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THE SOFTWARE PROVIDED BY NRL IS PROVIDED BY NRL AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL NRL OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the US Naval
 * Research Laboratory (NRL).
 */

#ifndef _KERNEL_
#include <stdio.h>
#endif

#include <sys/param.h>
#include <kernel/OS.h>

#include "sys/protosw.h"
#include "sys/socket.h"
#include "sys/socketvar.h"

#include "net/if.h"
#include "net/route.h"

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_debug.h>

#include "core_module.h"
#include "core_funcs.h"
#include "ipv4/ipv4_module.h"

#ifdef _KERNEL_
#include <KernelExport.h>
#endif

extern struct core_module_info *core;
extern struct ipv4_module_info *ipm;

int	    tcprexmtthresh = 3;
struct	tcpiphdr tcp_saveti;
int	    tcptv_keep_init = TCPTV_KEEP_INIT;

extern struct inpcb *tcp_last_inpcb;
extern uint32 sb_max;

#define  roundup(x, y)   ((((x)+((y)-1))/(y))*(y))
#define TCP_PAWS_IDLE	(24 * 24 * 60 * 60 * PR_SLOWHZ)

/* for modulo comparisons of timestamps */
#define TSTMP_LT(a,b)	((int)((a)-(b)) < 0)
#define TSTMP_GEQ(a,b)	((int)((a)-(b)) >= 0)

#define TCP_REASS(tp, ti, m, so, flags) { \
	if ((ti)->ti_seq == (tp)->rcv_nxt && \
	    (tp)->seg_next == (struct tcpiphdr*)(tp) && \
	    (tp)->t_state == TCPS_ESTABLISHED) { \
	    \
		(tp)->t_flags |= TF_DELACK; \
		(tp)->rcv_nxt += (ti)->ti_len; \
		(flags) = (ti)->ti_flags & TH_FIN; \
		tcpstat.tcps_rcvpack++; \
		tcpstat.tcps_rcvbyte += (ti)->ti_len; \
		sbappend(&(so)->so_rcv, (m)); \
		sorwakeup(so); \
	} else { \
		(flags) = tcp_reass((tp), (ti), (m)); \
		(tp)->t_flags |= TF_ACKNOW; \
	}\
}

/*
 * Insert segment ti into reassembly queue of tcp with
 * control block tp.  Return TH_FIN if reassembly now includes
 * a segment with FIN.  The macro form does the common case inline
 * (segment is the next to be received on an established connection,
 * and the queue is empty), avoiding linkage into and removal
 * from the queue and repetition of various conversions.
 * Set DELACK for segments received in order, but ack immediately
 * when segments are out of order (so fast retransmit can work).
 */

int tcp_reass(struct tcpcb *tp, struct tcpiphdr *ti, struct mbuf *m)
{
	struct tcpiphdr *q;
	struct socket *so = tp->t_inpcb->inp_socket;
	int flags;

	/*
	 * Call with ti==NULL after become established to
	 * force pre-ESTABLISHED data up to user socket.
	 */
	if (ti == NULL)
		goto present;

	/*
	 * Find a segment which begins after this one does.
	 */
	for (q = tp->seg_next; q != (struct tcpiphdr*)tp; q = (struct tcpiphdr*)q->ti_next)
		if (SEQ_GT(q->ti_seq, ti->ti_seq))
			break;
	/*
	 * If there is a preceding segment, it may provide some of
	 * our data already.  If so, drop the data from the incoming
	 * segment.  If it provides all of our data, drop us.
	 */
	if ((struct tcpiphdr *)q->ti_prev != (struct tcpiphdr*)tp) {
		int i;
		q = (struct tcpiphdr*)q->ti_prev;
		/* conversion to int (in i) handles seq wraparound */
		i = (q->ti_seq + q->ti_len) - ti->ti_seq;
		if (i > 0) {
			if (i >= ti->ti_len) {
				tcpstat.tcps_rcvduppack++;
				tcpstat.tcps_rcvdupbyte += ti->ti_len;
				//m_freem(m);
				return (0);
			}
			m_adj(m, i);
			ti->ti_len -= i;
			ti->ti_seq += i;
		}
		q = (struct tcpiphdr*)(q->ti_next);
	}
	tcpstat.tcps_rcvoopack++;
	tcpstat.tcps_rcvoobyte += ti->ti_len;
	REASS_MBUF(ti) = m;
	
	/*
	 * While we overlap succeeding segments trim them or,
	 * if they are completely covered, dequeue them.
	 */
	while (q != (struct tcpiphdr*)tp) {
		int i = (ti->ti_seq + ti->ti_len) - q->ti_seq;

		if (i <= 0)
			break;
		if (i < q->ti_len) {
			q->ti_seq += i;
			q->ti_len -= i;
			m_adj(REASS_MBUF(q), i);
			break;
		}
		q = (struct tcpiphdr*)q->ti_next;
		m = REASS_MBUF((struct tcpiphdr*)q->ti_prev);
		remque(q->ti_prev);
		//m_freem(m);
	}

	insque(ti, q->ti_prev);

present:
	/*
	 * Present data to user, advancing rcv_nxt through
	 * completed sequence space.
	 */
	if (TCPS_HAVERCVDSYN(tp->t_state) == 0) {
		return 0;
	}
	ti = tp->seg_next;
	if (ti == (struct tcpiphdr*)tp || ti->ti_seq != tp->rcv_nxt) {
		return 0;
	}
	if (tp->t_state == TCPS_SYN_RECEIVED && ti->ti_len) {
		return 0;
	}
	do {
		tp->rcv_nxt += ti->ti_len;
		flags = ti->ti_flags & TH_FIN;
		remque(ti);
		
		m = REASS_MBUF(ti);
		ti = (struct tcpiphdr*) ti->ti_next;
		/* XXX - Change
		 * BSD sockets would call m_freem(m) if this was true, but if we
		 * do that we have trouble...
		 * So, reverse the logic and only append data if we can.
		 */
		if ((so->so_state & SS_CANTRCVMORE) == 0)
			sbappend(&so->so_rcv, m);
	} while (ti != (struct tcpiphdr*)tp && ti->ti_seq == tp->rcv_nxt);
	sorwakeup(so);
	return (flags);
}

#ifdef TCP_DEBUG
static void show_ti(struct tcpiphdr *ti)
{
	printf("TCP/IP Header:\n");
	printf("             : src port : %d\n", ntohs(ti->ti_sport));
	printf("             : dst port : %d\n", ntohs(ti->ti_dport));
	printf("             : seq num  : %lu\n", ti->ti_seq);
	printf("             : ack num  : %lu\n", ti->ti_ack);
	printf("             : hdr len  : %d\n", ti->ti_len);
	printf("             : flags    : ");
	if (ti->ti_flags & TH_SYN)
		printf(" SYN ");
	if (ti->ti_flags & TH_ACK)
		printf(" ACK ");
	if (ti->ti_flags & TH_RST)
		printf(" RST ");
	if (ti->ti_flags & TH_PUSH)
		printf(" PSH ");
	if (ti->ti_flags & TH_URG)
		printf(" URG ");
	if (ti->ti_flags & TH_FIN)
		printf(" FIN ");
	printf("\n");
	printf("             : win      : %d\n", ti->ti_win);

}

static void show_tp(struct tcpcb *tp)
{
	printf("TCP Control Block:\n");
	printf("                 : snd_una       : %lu\n", tp->snd_una);
	printf("                 : snd_nxt       : %lu\n", tp->snd_nxt);
	printf("                 : snd_up        : %lu\n", tp->snd_up);
	printf("                 : snd_wl1       : %lu\n", tp->snd_wl1);
	printf("                 : snd_wl2       : %lu\n", tp->snd_wl2);
	printf("                 : iss           : %lu\n", tp->iss);
	printf("                 : rcv_wnd       : %lu\n", tp->rcv_wnd);
	printf("                 : rcv_nxt       : %lu\n", tp->rcv_nxt);
	printf("                 : rcv_up        : %lu\n", tp->rcv_up);
	printf("                 : last_ack_sent : %lu\n", tp->last_ack_sent);
	printf("                 : flags         : ");
	if (tp->t_flags & TF_ACKNOW)
		printf(" ACKNOW ");
	if (tp->t_flags & TF_DELACK)
		printf(" DELACK ");
	if (tp->t_flags & TF_SENTFIN)
		printf(" SENTFIN ");
	if (tp->t_flags & TF_NODELAY)
		printf(" NODELAY ");
	if (tp->t_flags & TF_NOOPT)
		printf(" NOOPT ");
	printf("\n");
}
#endif /* TCP_DEBUG */

/*
 * TCP input routine, follows pages 65-76 of the
 * protocol specification dated September, 1981 very closely.
 */
void tcp_input(struct mbuf *m, int iphlen)
{
	struct inpcb *inp;
	caddr_t optp = NULL;
	int optlen = 0;
	int len = 0;
	uint16 tlen = 0;
	int off = 0;
	struct tcpcb *tp = NULL;
	int tiflags = 0;
	struct socket *so = NULL;
	int todrop, acked, ourfinisacked, needoutput = 0;
	short ostate = 0;
	struct in_addr laddr;
	int dropsocket = 0;
	int iss = 0;
	u_long tiwin;
	uint32 ts_val, ts_ecr;
	int ts_present = 0;
	struct tcpiphdr *ti;

	tcpstat.tcps_rcvtotal++;
	/* Get the IP and TCP header together (the tcpiphdr struct)
	 * together in the first mbuf.
	 * NB IP layer should leave IP header in first mbuf
	 */
	ti = mtod(m, struct tcpiphdr*);

	if (iphlen > sizeof(struct ip))
		ipm->ip_stripoptions(m, NULL);

	if (m->m_len < sizeof(struct tcpiphdr)) {
		if ((m = m_pullup(m, sizeof(struct tcpiphdr))) == NULL) {
			tcpstat.tcps_rcvshort++;
			return;
		}
		ti = mtod(m, struct tcpiphdr*);
	}
	
	/*
	 * Checksum extended TCP header and data.
	 */
	tlen = ((struct ip *) ti)->ip_len;
	len = sizeof(struct ip) + tlen;

	ti->ti_next = ti->ti_prev = NULL;
	ti->ti_x1 = 0;
	ti->ti_len = htons(tlen);

	if ((ti->ti_sum = in_cksum(m, len, 0))) {
		tcpstat.tcps_rcvbadsum++;
		printf("invalid checksum %d over %d bytes!\n", ti->ti_sum, len);
		goto drop;
	}
	
	off = ti->ti_off << 2;
	if (off < sizeof(struct tcphdr) || off > tlen) {
		tcpstat.tcps_rcvbadoff++;
		printf("tcp_input: bad len\n");
		goto drop;
	}
	
	tlen -=off;
	ti->ti_len = tlen;
		
	if (off > sizeof(struct tcphdr)) {
		if (m->m_len < sizeof(struct ip) + off) {
			if ((m = m_pullup(m, sizeof(struct ip) + off)) == NULL) {
				tcpstat.tcps_rcvshort++;
				return;
			}
			ti = mtod(m, struct tcpiphdr*);
		}
		optlen = off - sizeof(struct tcphdr);
		optp = mtod(m, caddr_t) + sizeof(struct tcphdr);

		/* 
		 * Do quick retrieval of timestamp options ("options
		 * prediction?").  If timestamp is the only option and it's
		 * formatted as recommended in RFC 1323 appendix A, we
		 * quickly get the values now and not bother calling
		 * tcp_dooptions(), etc.
		 */
		if ((optlen == TCPOLEN_TSTAMP_APPA ||
		     (optlen > TCPOLEN_TSTAMP_APPA &&
		      optp[TCPOLEN_TSTAMP_APPA] == TCPOPT_EOL)) &&
		     *(uint32 *)optp == htonl(TCPOPT_TSTAMP_HDR) &&
		     (ti->ti_flags & TH_SYN) == 0) {
			ts_present = 1;
			ts_val = ntohl(*(uint32 *)(optp + 4));
			ts_ecr = ntohl(*(uint32 *)(optp + 8));
			optp = NULL;	/* we've parsed the options */
		}
	}
	tiflags = ti->ti_flags;

	/*
	 * Convert TCP protocol specific fields to host format.
	 */
	ti->ti_seq = ntohl(ti->ti_seq);
	ti->ti_ack = ntohl(ti->ti_ack);
	ti->ti_win = ntohs(ti->ti_win);
	ti->ti_urp = ntohs(ti->ti_urp);

	/*
	 * Locate pcb for segment.
	 */
findpcb:
	inp = tcp_last_inpcb;
	if (!inp || inp->lport != ti->ti_dport ||
	    inp->fport != ti->ti_sport ||
	    inp->faddr.s_addr != ti->ti_src.s_addr ||
	    inp->laddr.s_addr != ti->ti_dst.s_addr) {
		inp = in_pcblookup(&tcb, ti->ti_src, ti->ti_sport, ti->ti_dst,
		                   ti->ti_dport, INPLOOKUP_WILDCARD);
		if (inp)
			tcp_last_inpcb = inp;
		++tcpstat.tcps_pcbhashmiss;
	}
	if (inp == NULL) {
/* XXX - This is very common, but only commented out as it's useful at times
         for debugging.
		printf("tcp_input: dropwithreset: inp is NULL, line %d\n", __LINE__);
 */
		goto dropwithreset;
	}
	
	tp = intotcpcb(inp);
	if (tp == NULL) {
		printf("tcp_input: dropwithreset: tp is NULL, line %d\n", __LINE__);
		goto dropwithreset;
	}

	if (tp->t_state == TCPS_CLOSED) {
		printf("tcp_input: state is closed\n");
		goto drop;
	}

	/* Unscale the window into a 32-bit value. */
	if ((tiflags & TH_SYN) == 0)
		tiwin = ti->ti_win << tp->snd_scale;
	else
		tiwin = ti->ti_win;

	so = inp->inp_socket;
	if (so->so_options & (SO_DEBUG|SO_ACCEPTCONN)) {
		if (so->so_options & SO_DEBUG) {
			ostate = tp->t_state;
			tcp_saveti = *ti;
		}
		if (so->so_options & SO_ACCEPTCONN) {
			so = sonewconn(so, 0);
			if (so == NULL) {
				goto drop;
			}
			/*
			 * This is ugly, but ....
			 *
			 * Mark socket as temporary until we're
			 * committed to keeping it.  The code at
			 * ``drop'' and ``dropwithreset'' check the
			 * flag dropsocket to see if the temporary
			 * socket created here should be discarded.
			 * We mark the socket as discardable until
			 * we're committed to it below in TCPS_LISTEN.
			 */
			dropsocket++;
			inp = (struct inpcb *)so->so_pcb;
			inp->laddr = ti->ti_dst;
			inp->lport = ti->ti_dport;
			inp->inp_options = ipm->ip_srcroute();
			tp = intotcpcb(inp);
			tp->t_state = TCPS_LISTEN;

			tp->request_r_scale = 0;
			/* Compute proper scaling value from buffer space */
			while (tp->request_r_scale < TCP_MAX_WINSHIFT &&
			       TCP_MAXWIN << tp->request_r_scale < so->so_rcv.sb_hiwat)
				tp->request_r_scale++;
		}
	}

	/*
	 * Segment received on connection.
	 * Reset idle time and keep-alive timer.
	 */
	tp->t_idle = 0;
	tp->t_timer[TCPT_KEEP] = tcp_keepidle;
	
	/*
	 * Process options if not in LISTEN state,
	 * else do it below (after getting remote address).
	 */
	if (optp && tp->t_state != TCPS_LISTEN)
		tcp_dooptions(tp, (unsigned char *)optp, optlen, ti,	&ts_present, &ts_val, &ts_ecr);

	/* 
	 * Header prediction: check for the two common cases
	 * of a uni-directional data xfer.  If the packet has
	 * no control flags, is in-sequence, the window didn't
	 * change and we're not retransmitting, it's a
	 * candidate.  If the length is zero and the ack moved
	 * forward, we're the sender side of the xfer.  Just
	 * free the data acked & wake any higher level process
	 * that was blocked waiting for space.  If the length
	 * is non-zero and the ack didn't move, we're the
	 * receiver side.  If we're getting packets in-order
	 * (the reassembly queue is empty), add the data to
	 * the socket buffer and note that we need a delayed ack.
	 */
	if (tp->t_state == TCPS_ESTABLISHED &&
	    (tiflags & (TH_SYN|TH_FIN|TH_RST|TH_URG|TH_ACK)) == TH_ACK &&
	    (!ts_present || TSTMP_GEQ(ts_val, tp->ts_recent)) &&
	    ti->ti_seq == tp->rcv_nxt &&
	    tiwin && tiwin == tp->snd_wnd &&
	    tp->snd_nxt == tp->snd_max) {

		/* 
		 * If last ACK falls within this segment's sequence numbers,
		 *  record the timestamp.
		 * Fix from Braden, see Stevens p. 870
		 */
		if (ts_present && SEQ_LEQ(ti->ti_seq, tp->last_ack_sent)) {
			tp->ts_recent_age = tcp_now;
			tp->ts_recent = ts_val;
		}

		if (ti->ti_len == 0) {
			if (SEQ_GT(ti->ti_ack, tp->snd_una) &&
			    SEQ_LEQ(ti->ti_ack, tp->snd_max) &&
			    tp->snd_cwnd >= tp->snd_wnd) {
				/*
				 * this is a pure ack for outstanding data.
				 */
				++tcpstat.tcps_predack;
				if (ts_present)
					tcp_xmit_timer(tp, tcp_now - ts_ecr + 1);
				else if (tp->t_rtt && SEQ_GT(ti->ti_ack, tp->t_rtseq))
					tcp_xmit_timer(tp, tp->t_rtt);
				acked = ti->ti_ack - tp->snd_una;
				tcpstat.tcps_rcvackpack++;
				tcpstat.tcps_rcvackbyte += acked;
				sbdrop(&so->so_snd, acked);
				tp->snd_una = ti->ti_ack;
				m_freem(m);

				/*
				 * If all outstanding data are acked, stop
				 * retransmit timer, otherwise restart timer
				 * using current (possibly backed-off) value.
				 * If process is waiting for space,
				 * wakeup/selwakeup/signal.  If data
				 * are ready to send, let tcp_output
				 * decide between more output or persist.
				 */
				if (tp->snd_una == tp->snd_max)
					tp->t_timer[TCPT_REXMT] = 0;
				else if (tp->t_timer[TCPT_PERSIST] == 0)
					tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;

				if (so->so_snd.sb_flags & SB_NOTIFY)
					sowwakeup(so);
				if (so->so_snd.sb_cc) {
					(void) tcp_output(tp);
				}
				return;
			}
		} else if (ti->ti_ack == tp->snd_una &&
		    tp->seg_next == (struct tcpiphdr*)tp &&
		    ti->ti_len <= sbspace(&so->so_rcv)) {
			/*
			 * This is a pure, in-sequence data packet
			 * with nothing on the reassembly queue and
			 * we have enough buffer space to take it.
			 */
			++tcpstat.tcps_preddat;
			tp->rcv_nxt += ti->ti_len;
			tcpstat.tcps_rcvpack++;
			tcpstat.tcps_rcvbyte += ti->ti_len;
			/*
			 * Drop TCP, IP headers and TCP options then add data
			 * to socket buffer.
			 */
			m->m_data += sizeof(struct tcpiphdr) + off - sizeof(struct tcphdr);
			m->m_len -= sizeof(struct tcpiphdr) + off - sizeof(struct tcphdr);
			sbappend(&so->so_rcv, m);
			sorwakeup(so);
			tp->t_flags |= TF_DELACK;
			return;
		}
	}

	m->m_data += sizeof(struct tcpiphdr) + off - sizeof(struct tcphdr);
	m->m_len -= sizeof(struct tcpiphdr) + off - sizeof(struct tcphdr);

	/*
	 * Calculate amount of space in receive window,
	 * and then do TCP input processing.
	 * Receive window is amount of space in rcv queue,
	 * but not less than advertised window.
	 */
	{ 
		int win = sbspace(&so->so_rcv);
		if (win < 0)
			win = 0;
		tp->rcv_wnd = max(win, (int)(tp->rcv_adv - tp->rcv_nxt));
	}

	switch (tp->t_state) {

	/*
	 * If the state is LISTEN then ignore segment if it contains an RST.
	 * If the segment contains an ACK then it is bad and send a RST.
	 * If it does not contain a SYN then it is not interesting; drop it.
	 * If it is from this socket, drop it, it must be forged.
	 * Don't bother responding if the destination was a broadcast.
	 * Otherwise initialize tp->rcv_nxt, and tp->irs, select an initial
	 * tp->iss, and send a segment:
	 *     <SEQ=ISS><ACK=RCV_NXT><CTL=SYN,ACK>
	 * Also initialize tp->snd_nxt to tp->iss+1 and tp->snd_una to tp->iss.
	 * Fill in remote peer address fields if not previously specified.
	 * Enter SYN_RECEIVED state, and process any other fields of this
	 * segment in this state.
	 */
		case TCPS_LISTEN: {
			struct mbuf *am;
			struct sockaddr_in *sin;

			if (tiflags & TH_RST) {
				printf("tcp_input: TH_RST\n");
				goto drop;
			}
			if (tiflags & TH_ACK) {
				printf("tcp_input: dropwithreset: line %d\n", __LINE__);
				goto dropwithreset;
			}
			if ((tiflags & TH_SYN) == 0) {
				printf("tcp_input: TH_SYN\n");
				goto drop;
			}
			/*
			 * RFC1122 4.2.3.10, p. 104: discard bcast/mcast SYN
			 * in_broadcast() should never return true on a received
			 * packet with M_BCAST not set.
			 */
			if (m->m_flags & (M_BCAST | M_MCAST) ||
			    IN_MULTICAST(ti->ti_dst.s_addr)) {
				printf("tcp_input: multicast! %d\n", __LINE__);
				goto drop;
			}
			
			am = m_get(MT_SONAME);
			if (!am) {
				printf("failed to get MT_SONAME\n");
				goto drop;
			}
			am->m_len = sizeof(struct sockaddr_in);
			sin = mtod(am, struct sockaddr_in *);
			sin->sin_family = AF_INET;
			sin->sin_len = sizeof(*sin);
			sin->sin_addr = ti->ti_src;
			sin->sin_port = ti->ti_sport;
			memset((caddr_t)sin->sin_zero, 0, sizeof(sin->sin_zero));
			laddr = inp->laddr;
			if (inp->laddr.s_addr == INADDR_ANY)
				inp->laddr = ti->ti_dst;

			if (in_pcbconnect(inp, am)) {
				inp->laddr = laddr;
				(void) m_free(am);
				printf("tcp_input: in_pcbconnect failed\n");
				goto drop;
			}
			(void) m_free(am);
			tp->t_template = tcp_template(tp);
			if (tp->t_template == NULL) {
				printf("template = NULL\n");
				tp = tcp_drop(tp, ENOBUFS);
				dropsocket = 0;		/* socket is already gone */
				goto drop;
			}
			if (optp)
				tcp_dooptions(tp, (unsigned char *)optp, optlen, ti,
				              &ts_present, &ts_val, &ts_ecr);

			if (iss)
				tp->iss = iss;
			else
				tp->iss = tcp_iss;
			tcp_iss += TCP_ISSINCR / 2;
			tp->irs = ti->ti_seq;
			tcp_sendseqinit(tp);
			tcp_rcvseqinit(tp);
			tp->t_flags |= TF_ACKNOW;
			tp->t_state = TCPS_SYN_RECEIVED;
			tp->t_timer[TCPT_KEEP] = TCPTV_KEEP_INIT;
			dropsocket = 0;		/* committed to socket */
			tcpstat.tcps_accepts++;
			goto trimthenstep6;
		}

		/*
		 * If the state is SYN_RECEIVED:
		 * 	if seg contains SYN/ACK, send an RST.
		 *	if seg contains an ACK, but not for our SYN/ACK, send an RST
		 */

		case TCPS_SYN_RECEIVED:
			if (tiflags & TH_ACK) {
				if (tiflags & TH_SYN) {
					tcpstat.tcps_badsyn++;
					printf("SYN + ACK in a reply\n");
					printf("tcp_input: dropwithreset: line %d\n", __LINE__);
					goto dropwithreset;
				}
				if (SEQ_LEQ(ti->ti_ack, tp->snd_una) ||
				    SEQ_GT(ti->ti_ack, tp->snd_max)) {
				    printf("SEQ outside of boundaries...\n");
					printf("tcp_input: dropwithreset: line %d\n", __LINE__);
					goto dropwithreset;
				}
			}
			break;

		/*
		 * If the state is SYN_SENT:
		 *	if seg contains an ACK, but not for our SYN, drop the input.
		 *	if seg contains a RST, then drop the connection.
		 *	if seg does not contain SYN, then drop it.
		 * Otherwise this is an acceptable SYN segment
		 *	initialize tp->rcv_nxt and tp->irs
		 *	if seg contains ack then advance tp->snd_una
		 *	if SYN has been acked change to ESTABLISHED else SYN_RCVD state
		 *	arrange for segment to be acked (eventually)
		 *	continue processing rest of data/controls, beginning with URG
		 */
		case TCPS_SYN_SENT:
			if ((tiflags & TH_ACK) &&
		    	(SEQ_LEQ(ti->ti_ack, tp->iss) ||
		     	SEQ_GT(ti->ti_ack, tp->snd_max))) {
		     	printf("SYN_SENT but ACK was not for our request!\n");
				printf("tcp_input: dropwithreset: line %d\n", __LINE__);
				goto dropwithreset;
			}
			if (tiflags & TH_RST) {
				if (tiflags & TH_ACK)
					tp = tcp_drop(tp, ECONNREFUSED);
				printf("tcp_input: %d\n", __LINE__);
				goto drop;
			}
			if ((tiflags & TH_SYN) == 0) {
				printf("tcp_input: %d\n", __LINE__);
				goto drop;
			}
			if (tiflags & TH_ACK) {
				tp->snd_una = ti->ti_ack;
				if (SEQ_LT(tp->snd_nxt, tp->snd_una))
					tp->snd_nxt = tp->snd_una;
			}
			tp->t_timer[TCPT_REXMT] = 0;
			tp->irs = ti->ti_seq;
			tcp_rcvseqinit(tp);
			tp->t_flags |= TF_ACKNOW;
			if (tiflags & TH_ACK && SEQ_GT(tp->snd_una, tp->iss)) {
				tcpstat.tcps_connects++;
				soisconnected(so);
				tp->t_state = TCPS_ESTABLISHED;
				/* Do window scaling on this connection? */
				if ((tp->t_flags & (TF_RCVD_SCALE|TF_REQ_SCALE)) ==
					(TF_RCVD_SCALE|TF_REQ_SCALE)) {
					tp->snd_scale = tp->requested_s_scale;
					tp->rcv_scale = tp->request_r_scale;
				}
				(void) tcp_reass(tp, NULL, NULL);
				/*
				 * if we didn't have to retransmit the SYN,
				 * use its rtt as our initial srtt & rtt var.
				 */
				if (tp->t_rtt)
					tcp_xmit_timer(tp, tp->t_rtt);
			} else
				tp->t_state = TCPS_SYN_RECEIVED;

trimthenstep6:
			/*
			 * Advance ti->ti_seq to correspond to first data byte.
			 * If data, trim to stay within window,
			 * dropping FIN if necessary.
			 */
			ti->ti_seq++;
			if (ti->ti_len > tp->rcv_wnd) {
				todrop = ti->ti_len - tp->rcv_wnd;
				m_adj(m, -todrop);
				ti->ti_len = tp->rcv_wnd;
				tiflags &= ~TH_FIN;
				tcpstat.tcps_rcvpackafterwin++;
				tcpstat.tcps_rcvbyteafterwin += todrop;
			}
			tp->snd_wl1 = ti->ti_seq - 1;
			tp->rcv_up = ti->ti_seq;
			goto step6;
	}

		/*
		 * States other than LISTEN or SYN_SENT.
		 * First check timestamp, if present.
		 * Then check that at least some bytes of segment are within 
		 * receive window.  If segment begins before rcv_nxt,
		 * drop leading data (and SYN); if nothing left, just ack.
		 * 
		 * RFC 1323 PAWS: If we have a timestamp reply on this segment
		 * and it's less than ts_recent, drop it.
		 * PAWS = Protection Against Wrapped Sequence Numbers
		 */
	if (ts_present && (tiflags & TH_RST) == 0 && tp->ts_recent &&
	    TSTMP_LT(ts_val, tp->ts_recent)) {

		/* Check to see if ts_recent is over 24 days old.  */
		if ((int)(tcp_now - tp->ts_recent_age) > TCP_PAWS_IDLE) {
			/*
			 * Invalidate ts_recent.  If this segment updates
			 * ts_recent, the age will be reset later and ts_recent
			 * will get a valid value.  If it does not, setting
			 * ts_recent to zero will at least satisfy the
			 * requirement that zero be placed in the timestamp
			 * echo reply when ts_recent isn't valid.  The
			 * age isn't reset until we get a valid ts_recent
			 * because we don't want out-of-order segments to be
			 * dropped when ts_recent is old.
			 */
			tp->ts_recent = 0;
		} else {
			tcpstat.tcps_rcvduppack++;
			tcpstat.tcps_rcvdupbyte += ti->ti_len;
			tcpstat.tcps_pawsdrop++;
			goto dropafterack;
		}
	}

	todrop = tp->rcv_nxt - ti->ti_seq;
	if (todrop > 0) {
		if (tiflags & TH_SYN) {
			tiflags &= ~TH_SYN;
			ti->ti_seq++;
			if (ti->ti_urp > 1) 
				ti->ti_urp--;
			else
				tiflags &= ~TH_URG;
			todrop--;
		}
		if ((todrop >= ti->ti_len) || (todrop == ti->ti_len &&
		    (tiflags & TH_FIN) == 0)) {
			/*
			 * Any valid FIN must be to the left of the
			 * window.  At this point, FIN must be a
			 * duplicate or out-of-sequence, so drop it.
			 */
			tiflags &= ~TH_FIN;
			/*
			 * Send ACK to resynchronize, and drop any data,
			 * but keep on processing for RST or ACK.
			 */
			tp->t_flags |= TF_ACKNOW;
			todrop = ti->ti_len;		    
			tcpstat.tcps_rcvduppack++;
			tcpstat.tcps_rcvdupbyte += todrop;
		} else {
			tcpstat.tcps_rcvpartduppack++;
			tcpstat.tcps_rcvpartdupbyte += todrop;
		}
		m_adj(m, todrop);
		ti->ti_seq += todrop;
		ti->ti_len -= todrop;
		if (ti->ti_urp > todrop)
			ti->ti_urp -= todrop;
		else {
			tiflags &= ~TH_URG;
			ti->ti_urp = 0;
		}
	}

	/*
	 * If new data are received on a connection after the
	 * user processes are gone, then RST the other end.
	 */
/* XXX - how do we check this!!! */
	if ((so->so_state & SS_NOFDREF) &&
	    tp->t_state > TCPS_CLOSE_WAIT && ti->ti_len) {
		tp = tcp_close(tp);
		tcpstat.tcps_rcvafterclose++;
		printf("tcp_input: dropwithreset: line %d\n", __LINE__);
		goto dropwithreset;
	}

	/*
	 * If segment ends after window, drop trailing data
	 * (and PUSH and FIN); if nothing left, just ACK.
	 */
	todrop = (ti->ti_seq + ti->ti_len) - (tp->rcv_nxt + tp->rcv_wnd);
	if (todrop > 0) {
		tcpstat.tcps_rcvpackafterwin++;
		if (todrop >= ti->ti_len) {
			tcpstat.tcps_rcvbyteafterwin += ti->ti_len;
			/*
			 * If a new connection request is received
			 * while in TIME_WAIT, drop the old connection
			 * and start over if the sequence numbers
			 * are above the previous ones.
			 */
			if (tiflags & TH_SYN &&
			    tp->t_state == TCPS_TIME_WAIT &&
			    SEQ_GT(ti->ti_seq, tp->rcv_nxt)) {
				iss = tp->rcv_nxt + TCP_ISSINCR;
				tp = tcp_close(tp);
				goto findpcb;
			}
			/*
			 * If window is closed can only take segments at
			 * window edge, and have to drop data and PUSH from
			 * incoming segments.  Continue processing, but
			 * remember to ack.  Otherwise, drop segment
			 * and ack.
			 */
			if (tp->rcv_wnd == 0 && ti->ti_seq == tp->rcv_nxt) {
				tp->t_flags |= TF_ACKNOW;
				tcpstat.tcps_rcvwinprobe++;
			} else
				goto dropafterack;
		} else
			tcpstat.tcps_rcvbyteafterwin += todrop;
		m_adj(m, -todrop);
		ti->ti_len -= todrop;
		tiflags &= ~(TH_PUSH|TH_FIN);
	}

	/*
	 * If last ACK falls within this segment's sequence numbers,
	 * record its timestamp.
	 * Fix from Braden, see Stevens p. 870
	 */
	if (ts_present && SEQ_LEQ(ti->ti_seq, tp->last_ack_sent) &&
	    SEQ_LT(tp->last_ack_sent, ti->ti_seq + ti->ti_len + ((tiflags & (TH_SYN | TH_FIN)) != 0))) {
		tp->ts_recent_age = tcp_now;
		tp->ts_recent = ts_val;
	}

	/*
	 * If the RST bit is set examine the state:
	 *    SYN_RECEIVED STATE:
	 *	If passive open, return to LISTEN state.
	 *	If active open, inform user that connection was refused.
	 *    ESTABLISHED, FIN_WAIT_1, FIN_WAIT2, CLOSE_WAIT STATES:
	 *	Inform user that connection was reset, and close tcb.
	 *    CLOSING, LAST_ACK, TIME_WAIT STATES
	 *	Close the tcb.
	 */
	if (tiflags & TH_RST) {
		switch (tp->t_state) {
			case TCPS_SYN_RECEIVED:
				so->so_error = ECONNREFUSED;
				goto close;

			case TCPS_ESTABLISHED:
			case TCPS_FIN_WAIT_1:
			case TCPS_FIN_WAIT_2:
			case TCPS_CLOSE_WAIT:
				so->so_error = ECONNRESET;
close:
				tp->t_state = TCPS_CLOSED;
				tcpstat.tcps_drops++;
				tp = tcp_close(tp);
				goto drop;

			case TCPS_CLOSING:
			case TCPS_LAST_ACK:
			case TCPS_TIME_WAIT:
				tp = tcp_close(tp);
				goto drop;
		}
	}

	/*
	 * If a SYN is in the window, then this is an
	 * error and we send an RST and drop the connection.
	 */
	if (tiflags & TH_SYN) {
		tp = tcp_drop(tp, ECONNRESET);
		printf("tcp_input: dropwithreset: line %d\n", __LINE__);
		goto dropwithreset;
	}

	/*
	 * If the ACK bit is off we drop the segment and return.
	 */
	if ((tiflags & TH_ACK) == 0) {
		printf("tcp_input: drop: no ack flag...\n");
		goto drop;
	}

	/*
	 * Ack processing.
	 */
	switch (tp->t_state) {

		/*
		 * In SYN_RECEIVED state, the ack ACKs our SYN, so enter
		 * ESTABLISHED state and continue processing.
		 * The ACK was checked above.
		 */
		case TCPS_SYN_RECEIVED:
			if (SEQ_GT(tp->snd_una, ti->ti_ack) ||
			    SEQ_GT(ti->ti_ack, tp->snd_max)) {
				printf("tcp_input: dropwithreset: line %d\n", __LINE__);
				goto dropwithreset;
			}
			tcpstat.tcps_connects++;
			soisconnected(so);
			tp->t_state = TCPS_ESTABLISHED;
			/* Do window scaling? */
			if ((tp->t_flags & (TF_RCVD_SCALE|TF_REQ_SCALE)) ==
				(TF_RCVD_SCALE|TF_REQ_SCALE)) {
				tp->snd_scale = tp->requested_s_scale;
				tp->rcv_scale = tp->request_r_scale;
			}
			(void) tcp_reass(tp, NULL, NULL);
			tp->snd_wl1 = ti->ti_seq - 1;
			/* fall into ... */

		/*
		 * In ESTABLISHED state: drop duplicate ACKs; ACK out of range
		 * ACKs.  If the ack is in the range
		 *	tp->snd_una < th->th_ack <= tp->snd_max
		 * then advance tp->snd_una to th->th_ack and drop
		 * data from the retransmission queue.  If this ACK reflects
		 * more up to date window information we update our window information.
		 */
		case TCPS_ESTABLISHED:
		case TCPS_FIN_WAIT_1:
		case TCPS_FIN_WAIT_2:
		case TCPS_CLOSE_WAIT:
		case TCPS_CLOSING:
		case TCPS_LAST_ACK:
		case TCPS_TIME_WAIT:
			if (SEQ_LEQ(ti->ti_ack, tp->snd_una)) {
				if (ti->ti_len == 0 && tiwin == tp->snd_wnd) {
					tcpstat.tcps_rcvdupack++;
					/*
					 * If we have outstanding data (other than
					 * a window probe), this is a completely
					 * duplicate ack (ie, window info didn't
					 * change), the ack is the biggest we've
					 * seen and we've seen exactly our rexmt
					 * threshhold of them, assume a packet
					 * has been dropped and retransmit it.
					 * Kludge snd_nxt & the congestion
					 * window so we send only this one
					 * packet.
					 *
					 * We know we're losing at the current
					 * window size so do congestion avoidance
					 * (set ssthresh to half the current window
					 * and pull our congestion window back to
					 * the new ssthresh).
					 *
					 * Dup acks mean that packets have left the
					 * network (they're now cached at the receiver) 
					 * so bump cwnd by the amount in the receiver
					 * to keep a constant cwnd packets in the
					 * network.
					 */
					if (tp->t_timer[TCPT_REXMT] == 0 || ti->ti_ack != tp->snd_una)
						tp->t_dupacks = 0;
					else if (++tp->t_dupacks == tcprexmtthresh) {
						tcp_seq onxt = tp->snd_nxt;
						uint32 win = min(tp->snd_wnd, tp->snd_cwnd) / 2 / tp->t_maxseg;

						if (win < 2)
							win = 2;
						tp->snd_ssthresh = win * tp->t_maxseg;
						tp->t_timer[TCPT_REXMT] = 0;
						tp->t_rtt = 0;
						tp->snd_nxt = ti->ti_ack;
						tp->snd_cwnd = tp->t_maxseg;
						(void) tcp_output(tp);
						tp->snd_cwnd = tp->snd_ssthresh + tp->t_maxseg * tp->t_dupacks;
						if (SEQ_GT(onxt, tp->snd_nxt))
							tp->snd_nxt = onxt;
						goto drop;
					} else if (tp->t_dupacks > tcprexmtthresh) {
						tp->snd_cwnd += tp->t_maxseg;
						(void) tcp_output(tp);
						goto drop;
					}
				} else 
					tp->t_dupacks = 0;
				break;
		}
		/*
		 * If the congestion window was inflated to account
		 * for the other side's cached packets, retract it.
		 */
		if (tp->t_dupacks >= tcprexmtthresh &&
		    tp->snd_cwnd > tp->snd_ssthresh)
			tp->snd_cwnd = tp->snd_ssthresh;
		tp->t_dupacks = 0;

		if (SEQ_GT(ti->ti_ack, tp->snd_max)) {
			tcpstat.tcps_rcvacktoomuch++;
			goto dropafterack;
		}
		acked = ti->ti_ack - tp->snd_una;
		tcpstat.tcps_rcvackpack++;
		tcpstat.tcps_rcvackbyte += acked;

		/*
		 * If we have a timestamp reply, update smoothed
		 * round trip time.  If no timestamp is present but
		 * transmit timer is running and timed sequence
		 * number was acked, update smoothed round trip time.
		 * Since we now have an rtt measurement, cancel the
		 * timer backoff (cf., Phil Karn's retransmit alg.).
		 * Recompute the initial retransmit timer.
		 */
		if (ts_present)
			tcp_xmit_timer(tp, tcp_now - ts_ecr + 1);
		else if (tp->t_rtt && SEQ_GT(ti->ti_ack, tp->t_rtseq))
			tcp_xmit_timer(tp,tp->t_rtt);

		/*
		 * If all outstanding data is acked, stop retransmit
		 * timer and remember to restart (more output or persist).
		 * If there is more data to be acked, restart retransmit
		 * timer, using current (possibly backed-off) value.
		 */
		if (ti->ti_ack == tp->snd_max) {
			tp->t_timer[TCPT_REXMT] = 0;
			needoutput = 1;
		} else if (tp->t_timer[TCPT_PERSIST] == 0)
			tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
		/*
		 * When new data is acked, open the congestion window.
		 * If the window gives us less than ssthresh packets
		 * in flight, open exponentially (maxseg per packet).
		 * Otherwise open linearly: maxseg per window
		 * (maxseg^2 / cwnd per packet).
		 */
		{
			uint cw = tp->snd_cwnd;
			uint incr = tp->t_maxseg;

			if (cw > tp->snd_ssthresh)
				incr = incr * incr / cw + incr / 8;
			tp->snd_cwnd = min(cw + incr, TCP_MAXWIN << tp->snd_scale);
		}
		if (acked > so->so_snd.sb_cc) {
			tp->snd_wnd -= so->so_snd.sb_cc;
			sbdrop(&so->so_snd, (int)so->so_snd.sb_cc);
			ourfinisacked = 1;
		} else {
			sbdrop(&so->so_snd, acked);
			tp->snd_wnd -= acked;
			ourfinisacked = 0;
		}
		if (so->so_snd.sb_flags & SB_NOTIFY)
			sowwakeup(so);
		tp->snd_una = ti->ti_ack;
		if (SEQ_LT(tp->snd_nxt, tp->snd_una))
			tp->snd_nxt = tp->snd_una;

		switch (tp->t_state) {

		/*
		 * In FIN_WAIT_1 STATE in addition to the processing
		 * for the ESTABLISHED state if our FIN is now acknowledged
		 * then enter FIN_WAIT_2.
		 */
			case TCPS_FIN_WAIT_1:
				if (ourfinisacked) {
					/*
					 * If we can't receive any more
					 * data, then closing user can proceed.
					 * Starting the timer is contrary to the
					 * specification, but if we don't get a FIN
					 * we'll hang forever.
					 */
					if (so->so_state & SS_CANTRCVMORE) {
						soisdisconnected(so);
						tp->t_timer[TCPT_2MSL] = tcp_maxidle;
					}
					tp->t_state = TCPS_FIN_WAIT_2;
				}
				break;

			/*
			 * In CLOSING STATE in addition to the processing for
			 * the ESTABLISHED state if the ACK acknowledges our FIN
			 * then enter the TIME-WAIT state, otherwise ignore
			 * the segment.
			 */
			case TCPS_CLOSING:
				if (ourfinisacked) {
					tp->t_state = TCPS_TIME_WAIT;
					tcp_canceltimers(tp);
					tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
					soisdisconnected(so);
				}
				break;

			/*
			 * In LAST_ACK, we may still be waiting for data to drain
			 * and/or to be acked, as well as for the ack of our FIN.
			 * If our FIN is now acknowledged, delete the TCB,
			 * enter the closed state and return.
			 */
			case TCPS_LAST_ACK:
				if (ourfinisacked) {
					tp = tcp_close(tp);
					goto drop;
				}
				break;

			/*
			 * In TIME_WAIT state the only thing that should arrive
			 * is a retransmission of the remote FIN.  Acknowledge
			 * it and restart the finack timer.
			 */
			case TCPS_TIME_WAIT:
				tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
				goto dropafterack;
		}
	}

step6:
	/*
	 * Update window information.
	 * Don't look at window if no ACK: TAC's send garbage on first SYN.
	 */
	if ((tiflags & TH_ACK) && 
	    (SEQ_LT(tp->snd_wl1, ti->ti_seq) || tp->snd_wl1 == ti->ti_seq) && 
	    (SEQ_LT(tp->snd_wl2, ti->ti_ack) ||
	    (tp->snd_wl2 == ti->ti_ack && tiwin > tp->snd_wnd))) {
		/* keep track of pure window updates */
		if (ti->ti_len == 0 &&
		    tp->snd_wl2 == ti->ti_ack && tiwin > tp->snd_wnd)
			tcpstat.tcps_rcvwinupd++;
		tp->snd_wnd = tiwin;
		tp->snd_wl1 = ti->ti_seq;
		tp->snd_wl2 = ti->ti_ack;
		if (tp->snd_wnd > tp->max_sndwnd)
			tp->max_sndwnd = tp->snd_wnd;
		needoutput = 1;
	}
	/*
	 * Process segments with URG.
	 */
	if ((tiflags & TH_URG) && ti->ti_urp &&
	    TCPS_HAVERCVDFIN(tp->t_state) == 0) {
		/*
		 * This is a kludge, but if we receive and accept
		 * random urgent pointers, we'll crash in
		 * soreceive.  It's hard to imagine someone
		 * actually wanting to send this much urgent data.
		 */
		if (ti->ti_urp + so->so_rcv.sb_cc > sb_max) {
			ti->ti_urp = 0;			/* XXX */
			tiflags &= ~TH_URG;		/* XXX */
			goto dodata;			/* XXX */
		}
		/*
		 * If this segment advances the known urgent pointer,
		 * then mark the data stream.  This should not happen
		 * in CLOSE_WAIT, CLOSING, LAST_ACK or TIME_WAIT STATES since
		 * a FIN has been received from the remote side. 
		 * In these states we ignore the URG.
		 *
		 * According to RFC961 (Assigned Protocols),
		 * the urgent pointer points to the last octet
		 * of urgent data.  We continue, however,
		 * to consider it to indicate the first octet
		 * of data past the urgent section as the original 
		 * spec states (in one of two places).
		 */
		if (SEQ_GT(ti->ti_seq + ti->ti_urp, tp->rcv_up)) {
			tp->rcv_up = ti->ti_seq + ti->ti_urp;
			so->so_oobmark = so->so_rcv.sb_cc +
			    (tp->rcv_up - tp->rcv_nxt) - 1;
			if (so->so_oobmark == 0)
				so->so_state |= SS_RCVATMARK;
			sohasoutofband(so);
			tp->t_oobflags &= ~(TCPOOB_HAVEDATA | TCPOOB_HADDATA);
		}
		/*
		 * Remove out of band data so doesn't get presented to user.
		 * This can happen independent of advancing the URG pointer,
		 * but if two URG's are pending at once, some out-of-band
		 * data may creep in... ick.
		 */
		if (ti->ti_urp <= ti->ti_len
#ifdef SO_OOBINLINE
		     && (so->so_options & SO_OOBINLINE) == 0
#endif
		     )
		        tcp_pulloutofband(so, ti, m);
	} else {
		/*
		 * If no out of band data is expected,
		 * pull receive urgent pointer along
		 * with the receive window.
		 */
		if (SEQ_GT(tp->rcv_nxt, tp->rcv_up))
			tp->rcv_up = tp->rcv_nxt;
	}
dodata:							/* XXX */

	/*
	 * Process the segment text, merging it into the TCP sequencing queue,
	 * and arranging for acknowledgment of receipt if necessary.
	 * This process logically involves adjusting tp->rcv_wnd as data
	 * is presented to the user (this happens in tcp_usrreq.c,
	 * case PRU_RCVD).  If a FIN has already been received on this
	 * connection then we just ignore the text.
	 */

	if ((ti->ti_len || (tiflags & TH_FIN)) &&
	    TCPS_HAVERCVDFIN(tp->t_state) == 0) {
		TCP_REASS(tp, ti, m, so, tiflags);
		len = so->so_rcv.sb_hiwat - (tp->rcv_adv - tp->rcv_nxt);
	} else {
		m_freem(m);
		tiflags &= ~TH_FIN;
	}
		
	/*
	 * If FIN is received ACK the FIN and let the user know
	 * that the connection is closing.  Ignore a FIN received before
	 * the connection is fully established.
	 */
	if ((tiflags & TH_FIN)) {
		if (TCPS_HAVERCVDFIN(tp->t_state) == 0) {
			socantrcvmore(so);
			tp->t_flags |= TF_ACKNOW;
			tp->rcv_nxt++;
		}
		switch (tp->t_state) {
			/*
			 * In ESTABLISHED STATE enter the CLOSE_WAIT state.
			 */
			case TCPS_SYN_RECEIVED:
			case TCPS_ESTABLISHED:
				tp->t_state = TCPS_CLOSE_WAIT;
				break;

			/*
			 * If still in FIN_WAIT_1 STATE FIN has not been acked so
			 * enter the CLOSING state.
			 */
			case TCPS_FIN_WAIT_1:
				tp->t_state = TCPS_CLOSING;
				break;

			/*
			 * In FIN_WAIT_2 state enter the TIME_WAIT state,
			 * starting the time-wait timer, turning off the other 
			 * standard timers.
			 */
			case TCPS_FIN_WAIT_2:
				tp->t_state = TCPS_TIME_WAIT;
				tcp_canceltimers(tp);
				tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
				soisdisconnected(so);
				break;

			/*
			 * In TIME_WAIT state restart the 2 MSL time_wait timer.
			 */
			case TCPS_TIME_WAIT:
				tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
				break;
		}
	}

	if (so->so_options & SO_DEBUG) {
		tcp_trace(TA_INPUT, ostate, tp, (caddr_t) &tcp_saveti,
		          0, tlen);
	}

	/*
	 * Return any desired output.
	 */
	if (needoutput || (tp->t_flags & TF_ACKNOW)) {
		(void) tcp_output(tp);
	}
	return;

dropafterack:
	/*
	 * Generate an ACK dropping incoming segment if it occupies
	 * sequence space, where the ACK reflects our state.
	 */
	if (tiflags & TH_RST)
		goto drop;
	m_freem(m);
	tp->t_flags |= TF_ACKNOW;
	(void) tcp_output(tp);
	return;

dropwithreset:
	/*
	 * Generate a RST, dropping incoming segment.
	 * Make ACK acceptable to originator of segment.
	 * Don't bother to respond if destination was broadcast/multicast.
	 */
	if ((tiflags & TH_RST) || m->m_flags & (M_BCAST|M_MCAST) ||
		IN_MULTICAST(ti->ti_dst.s_addr))
			goto drop;
	if (tiflags & TH_ACK) {
		tcp_respond(tp, ti, m, 0, ti->ti_ack, TH_RST);
	} else {
		if (tiflags & TH_SYN)
			ti->ti_len++;
		tcp_respond(tp, ti, m, ti->ti_seq + ti->ti_len, 0, TH_RST|TH_ACK);		
	}
	/* destroy temporarily created socket */
	if (dropsocket)
		(*so->so_proto->pr_userreq)(so, PRU_ABORT, NULL, NULL, NULL);
	return;

drop:
	/*
	 * Drop space held by incoming segment and return.
	 */
	if (tp && (tp->t_inpcb->inp_socket->so_options & SO_DEBUG)) {
		tcp_trace(TA_DROP, ostate, tp, (caddr_t) &tcp_saveti,
		          0, tlen);
	}

	m_freem(m);
	/* destroy temporarily created socket */

	if (dropsocket)
		(*so->so_proto->pr_userreq)(so, PRU_ABORT, NULL, NULL, NULL);
	return;
}

void tcp_dooptions(struct tcpcb *tp, u_char *cp, int cnt, struct tcpiphdr *ti,
                   int *ts_present, uint32 *ts_val, uint32 *ts_ecr)
{
	uint16 mss = 0;
	int opt, optlen;
	
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[0];
		if (opt == TCPOPT_EOL)
			break;
		if (opt == TCPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[1];
			if (optlen <= 0)
				break;
		}
		switch (opt) {
			case TCPOPT_MAXSEG:
				if (optlen != TCPOLEN_MAXSEG)
					continue;
				if (!(ti->ti_flags & TH_SYN))
					continue;
				memcpy((char *) &mss, (char *) cp + 2,sizeof(mss));
				ntohs(mss);
				break;
			case TCPOPT_WINDOW:
				if (optlen != TCPOLEN_WINDOW)
					continue;
				if (!(ti->ti_flags & TH_SYN))
					continue;
				tp->t_flags |= TF_RCVD_SCALE;
				tp->requested_s_scale = min(cp[2], TCP_MAX_WINSHIFT);
				break;
			case TCPOPT_TIMESTAMP:
				if (optlen != TCPOLEN_TIMESTAMP)
					continue;
				*ts_present = 1;
				memcpy((char *) ts_val, (char *)cp + 2, sizeof(*ts_val));
				ntohl(*ts_val);
				memcpy((char*)ts_ecr, (char *)cp + 6, sizeof(*ts_ecr));
				ntohl(*ts_ecr);

				/* 
				 * A timestamp received in a SYN makes
				 * it ok to send timestamp requests and replies.
				 */
				if (ti->ti_flags & TH_SYN) {
					tp->t_flags |= TF_RCVD_TSTMP;
					tp->ts_recent = *ts_val;
					tp->ts_recent_age = tcp_now;
				}
				break;
			default:
				continue;
		}
	}
}

/*
 * Pull out of band byte out of a segment so
 * it doesn't appear in the user's data queue.
 * It is still reflected in the segment length for
 * sequencing purposes.
 */
void tcp_pulloutofband(struct socket *so, struct tcpiphdr *ti, struct mbuf *m)
{
	int cnt = ti->ti_urp - 1;
	
	while (cnt >= 0) {
		if (m->m_len > cnt) {
			char *cp = mtod(m, caddr_t) + cnt;
			struct tcpcb *tp = sototcpcb(so);

			tp->t_iobc = *cp;
			tp->t_oobflags |= TCPOOB_HAVEDATA;
			memcpy(cp, cp+1, (unsigned)(m->m_len - cnt - 1));
			m->m_len--;
			return;
		}
		cnt -= m->m_len;
		m = m->m_next;
		if (m == NULL)
			break;
	}
}

/*
 * Collect new round-trip time estimate
 * and update averages and current timeout.
 */
void tcp_xmit_timer(struct tcpcb *tp, int16 rtt)
{
	int16 delta;
	int16 rttmin;

	tcpstat.tcps_rttupdated++;
	--rtt;
	if (tp->t_srtt != 0) {
		/*
		 * srtt is stored as fixed point with 3 bits after the
		 * binary point (i.e., scaled by 8).  The following magic
		 * is equivalent to the smoothing algorithm in rfc793 with
		 * an alpha of .875 (srtt = rtt/8 + srtt*7/8 in fixed
		 * point).  Adjust rtt to origin 0.
		 */
		delta = (rtt << 2) - (tp->t_srtt >> TCP_RTT_SHIFT);
		if ((tp->t_srtt += delta) <= 0)
			tp->t_srtt = 1;
		/*
		 * We accumulate a smoothed rtt variance (actually, a
		 * smoothed mean difference), then set the retransmit
		 * timer to smoothed rtt + 4 times the smoothed variance.
		 * rttvar is stored as fixed point with 2 bits after the
		 * binary point (scaled by 4).  The following is
		 * equivalent to rfc793 smoothing with an alpha of .75
		 * (rttvar = rttvar*3/4 + |delta| / 4).  This replaces
		 * rfc793's wired-in beta.
		 */
		if (delta < 0)
			delta = -delta;
		delta -= (tp->t_rttvar >> TCP_RTTVAR_SHIFT);
		if ((tp->t_rttvar += delta) <= 0)
			tp->t_rttvar = 1;
	} else {
		/* 
		 * No rtt measurement yet - use the unsmoothed rtt.
		 * Set the variance to half the rtt (so our first
		 * retransmit happens at 3*rtt).
		 */
		tp->t_srtt = rtt << (TCP_RTT_SHIFT + 2);
		tp->t_rttvar = rtt << (TCP_RTTVAR_SHIFT + 2 - 1);
	}
	tp->t_rtt = 0;
	tp->t_rxtshift = 0;

	/*
	 * the retransmit should happen at rtt + 4 * rttvar.
	 * Because of the way we do the smoothing, srtt and rttvar
	 * will each average +1/2 tick of bias.  When we compute
	 * the retransmit timer, we want 1/2 tick of rounding and
	 * 1 extra tick because of +-1/2 tick uncertainty in the
	 * firing of the timer.  The bias will give us exactly the
	 * 1.5 tick we need.  But, because the bias is
	 * statistical, we have to test that we don't drop below
	 * the minimum feasible timer (which is 2 ticks).
	 */
	if (tp->t_rttmin > rtt + 2)
		rttmin = tp->t_rttmin;
	else
		rttmin = rtt + 2;
	TCPT_RANGESET(tp->t_rxtcur, TCP_REXMTVAL(tp), rttmin, TCPTV_REXMTMAX);
	
	/*
	 * We received an ack for a packet that wasn't retransmitted;
	 * it is probably safe to discard any error indications we've
	 * received recently.  This isn't quite right, but close enough
	 * for now (a route might have failed after we sent a segment,
	 * and the return path might not be symmetrical).
	 */
	tp->t_softerror = 0;
}

/*
 * Set connection variables based on the effective MSS.
 * We are passed the TCPCB for the actual connection.  If we
 * are the server, we are called by the compressed state engine
 * when the 3-way handshake is complete.  If we are the client,
 * we are called when we receive the SYN,ACK from the server.
 *
 * NOTE: The t_maxseg value must be initialized in the TCPCB
 * before this routine is called!
 */
void tcp_mss_update(struct tcpcb *tp)
{
	int mss, rtt;
	u_long bufsize;
	struct rtentry *rt;
	struct socket *so;

	so = tp->t_inpcb->inp_socket;
	mss = tp->t_maxseg;

	rt = in_pcbrtentry(tp->t_inpcb);

	if (rt == NULL)
		return;

#ifdef RTV_MTU	/* if route characteristics exist ... */
	/*
	 * While we're here, check if there's an initial rtt
	 * or rttvar.  Convert from the route-table units
	 * to scaled multiples of the slow timeout timer.
	 */
	if (tp->t_srtt == 0 && (rtt = rt->rt_rmx.rmx_rtt)) {
		/*
		 * XXX the lock bit for MTU indicates that the value
		 * is also a minimum value; this is subject to time.
		 */
		if (rt->rt_rmx.rmx_locks & RTV_RTT)
			TCPT_RANGESET(tp->t_rttmin,
			    rtt / (RTM_RTTUNIT / PR_SLOWHZ),
			    TCPTV_MIN, TCPTV_REXMTMAX);
		tp->t_srtt = rtt / (RTM_RTTUNIT / (PR_SLOWHZ * TCP_RTT_SCALE));
		if (rt->rt_rmx.rmx_rttvar)
			tp->t_rttvar = rt->rt_rmx.rmx_rttvar /
			    (RTM_RTTUNIT / (PR_SLOWHZ * TCP_RTTVAR_SCALE));
		else
			/* default variation is +- 1 rtt */
			tp->t_rttvar =
			    tp->t_srtt * TCP_RTTVAR_SCALE / TCP_RTT_SCALE;
		TCPT_RANGESET(/*(long)*/tp->t_rxtcur,
		    ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1,
		    tp->t_rttmin, TCPTV_REXMTMAX);
	}
#endif

	/*
	 * If there's a pipesize, change the socket buffer
	 * to that size.  Make the socket buffers an integral
	 * number of mss units; if the mss is larger than
	 * the socket buffer, decrease the mss.
	 */
#ifdef RTV_SPIPE
	if ((bufsize = rt->rt_rmx.rmx_sendpipe) == 0)
#endif
		bufsize = so->so_snd.sb_hiwat;
	if (bufsize < mss) {
		mss = bufsize;
		/* Update t_maxseg and t_maxopd */
		tcp_mss(tp, mss);
	} else {
		bufsize = roundup(bufsize, mss);
		if (bufsize > sb_max)
			bufsize = sb_max;
		(void)sbreserve(&so->so_snd, bufsize);
	}

#ifdef RTV_RPIPE
	if ((bufsize = rt->rt_rmx.rmx_recvpipe) == 0)
#endif
		bufsize = so->so_rcv.sb_hiwat;
	if (bufsize > mss) {
		bufsize = roundup(bufsize, mss);
		if (bufsize > sb_max)
			bufsize = sb_max;
		(void)sbreserve(&so->so_rcv, bufsize);
#ifdef RTV_RPIPE
		if (rt->rt_rmx.rmx_recvpipe > 0) {
			tp->request_r_scale = 0;
			while (tp->request_r_scale < TCP_MAX_WINSHIFT &&
			       TCP_MAXWIN << tp->request_r_scale < 
			       so->so_rcv.sb_hiwat)
				tp->request_r_scale++;
		}
#endif
	}

#ifdef RTV_SSTHRESH
	if (rt->rt_rmx.rmx_ssthresh) {
		/*
		 * There's some sort of gateway or interface
		 * buffer limit on the path.  Use this to set
		 * the slow start threshhold, but set the
		 * threshold to no less than 2*mss.
		 */
		tp->snd_ssthresh = max(2 * mss, rt->rt_rmx.rmx_ssthresh);
	}
#endif /* RTV_MTU */
}
