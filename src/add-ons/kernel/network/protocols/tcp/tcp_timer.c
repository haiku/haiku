/* tcp_timer.c */

#ifndef _KERNEL_
#include <stdio.h>
#endif

#include "sys/protosw.h"
#include "netinet/in_pcb.h"
#include "netinet/tcp.h"
#include "netinet/tcp_timer.h"
#include "netinet/tcp_var.h"
#include "netinet/tcp_seq.h"
#include "netinet/tcp_fsm.h"

#include "core_module.h"
#include "core_funcs.h"

#ifdef _KERNEL_
#include <KernelExport.h>
#endif

extern struct core_module_info *core;

int	tcp_backoff[TCP_MAXRXTSHIFT + 1] =
    { 1, 2, 4, 8, 16, 32, 64, 64, 64, 64, 64, 64, 64 };

int tcp_totbackoff = 511;	/* sum of tcp_backoff[] */

int tcp_keepidle = TCPTV_KEEP_IDLE;
int tcp_keepintvl = TCPTV_KEEPINTVL;
int tcp_maxpersistidle = TCPTV_KEEP_IDLE;   /* max idle time in persist */
int tcp_maxidle;   

void tcp_canceltimers(struct tcpcb *tp)
{
	int i;
	
	for (i=0; i < TCPT_NTIMERS; i++)
		tp->t_timer[i] = 0;
}

struct tcpcb * tcp_timers(struct tcpcb *tp, int timer)
{
	int rexmt;
	
	switch (timer) {
		/* TCPT_2MSL is used for
		 * FIN_WAIT2 timer
		 * TIME_WAIT
		 */
		case TCPT_2MSL:
			if (tp->t_state != TCPS_TIME_WAIT &&
			    tp->t_idle <= tcp_maxidle)
				tp->t_timer[TCPT_2MSL] = tcp_keepintvl;
			else
				tp = tcp_close(tp);
			break;
		/* TCPT_PERSIST is used to wait for being told it can send
		 * data. The window ahs been set to 0, so no data can be
		 * sent, but there's data waiting to be sent. when the timer
		 * expires we'll force a byte to be sent (despite the window
		 * being 0) and reset the time...
		 */
		case TCPT_PERSIST:
			tcpstat.tcps_persisttimeo++;
			tcp_setpersist(tp);
			tp->t_force = 1;
			tcp_output(tp);
			tp->t_force = 0;
			break;
		/* TCPT_KEEP is used for
		 * send data
		 * drop connection if too long idle
		 */
		case TCPT_KEEP:
			tcpstat.tcps_keeptimeo++;
			if (tp->t_state < TCPS_ESTABLISHED)
				goto dropit;
			if (tp->t_inpcb->inp_socket->so_options & SO_KEEPALIVE &&
			    tp->t_state <= TCPS_CLOSE_WAIT) {
				if (tp->t_idle >= tcp_keepidle + tcp_maxidle)
					goto dropit;
				tcpstat.tcps_keepprobe++;
				tcp_respond(tp, tp->t_template, NULL, tp->rcv_nxt, 
				            tp->snd_una - 1, 0);
				tp->t_timer[TCPT_KEEP] = tcp_keepintvl;
			} else
				tp->t_timer[TCPT_KEEP] = tcp_keepidle;
			break;
dropit:
			tcpstat.tcps_keepdrops++;
			tp = tcp_drop(tp, ETIMEDOUT);
			break;
		/* TCPT_REXMT is the transmission timer */
		case TCPT_REXMT:
			if (++tp->t_rxtshift > TCP_MAXRXTSHIFT) {
				tp->t_rxtshift = TCP_MAXRXTSHIFT;
				tcpstat.tcps_timeoutdrop++;
				tp = tcp_drop(tp, tp->t_softerror ? tp->t_softerror : ETIMEDOUT);
				break;
			}
			tcpstat.tcps_rexmttimeo++;
			rexmt = TCP_REXMTVAL(tp) * tcp_backoff[tp->t_rxtshift];
			TCPT_RANGESET(tp->t_rxtcur, rexmt, tp->t_rttmin, TCPTV_REXMTMAX);
			tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
			if (tp->t_rxtshift > TCP_MAXRXTSHIFT / 4) {
				in_losing(tp->t_inpcb);
				tp->t_rttvar += (tp->t_srtt >> TCP_RTT_SHIFT);
				tp->t_srtt = 0;
			}
			tp->snd_nxt = tp->snd_una;
			tp->t_rtt = 0;
			{
				uint win = min(tp->snd_wnd, tp->snd_cwnd) / 2/ tp->t_maxseg;
				if (win < 2)
					win = 2;
				tp->snd_cwnd = tp->t_maxseg;
				tp->snd_ssthresh = win * tp->t_maxseg;
				tp->t_dupacks = 0;
			}
			tcp_output(tp);
			break;
	}
	return (tp);
}

void tcp_slowtimer(void *data)
{
	struct inpcb *ip, *ipnxt;
	struct tcpcb *tp;
	int i;

	tcp_maxidle = TCPTV_KEEPCNT * tcp_keepintvl;
	
	ip = tcb.inp_next;
	if (!ip)
		return;
	for (; ip != &tcb; ip = ipnxt) {
		ipnxt = ip->inp_next;
		tp = intotcpcb(ip);
		if (!tp)
			continue;
		for (i=0;i < TCPT_NTIMERS;i++) {
			if (tp->t_timer[i] && --tp->t_timer[i] == 0) {
				tcp_userreq(tp->t_inpcb->inp_socket, PRU_SLOWTIMO, NULL,
			                (struct mbuf *)i, NULL);
				if (ipnxt->inp_prev != ip)
					goto tpgone;
			}
		}
		tp->t_idle++;
		if (tp->t_rtt)
			tp->t_rtt++;
tpgone:
	; /* mwcc wants a ; here, so it gets one */
	}
	tcp_iss += TCP_ISSINCR / PR_SLOWHZ;
	tcp_now++;
		
	return;
}

void tcp_fasttimer(void *data)
{
	struct inpcb *inp;
	struct tcpcb *tp;

	inp = tcb.inp_next;
	if (inp) {
		for (; inp != &tcb; inp = inp->inp_next) {
			if ((tp = (struct tcpcb*)inp->inp_ppcb) &&
		    	(tp->t_flags & TF_DELACK)) {
				tp->t_flags &= ~TF_DELACK;
				tp->t_flags |= TF_ACKNOW;
				tcpstat.tcps_delack++;
				tcp_output(tp);
			}
		}
	}

	return;
}
