/*
 * Copyright (c) 1982, 1986, 1993
 *The Regents of the University of California.  All rights reserved.
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
 *
 *    This product includes software developed by the University of
 *    California, Berkeley and its contributors.
 *
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
 */   

#ifndef NETINET_TCP_VAR_H
#define NETINET_TCP_VAR_H

/*
 * Tcp control block, one per tcp connection
 */
struct tcpcb {
	struct tcpiphdr *seg_next;
	struct tcpiphdr *seg_prev;
	int16   t_state;                    /* state of this connection */
	int16   t_timer[TCPT_NTIMERS];      /* tcp timers */
	int16   t_rxtshift;                 /* log(2) of rexmt exp. backoff */
	int16   t_rxtcur;                   /* current retransmit value */
	int16   t_dupacks;                  /* consecutive dup acks recd */
	uint16  t_maxseg;                   /* maximum segment size */
	int8    t_force;                    /* 1 if forcing out a byte */
    uint16  t_flags;   

	struct  tcpiphdr *t_template;       /* skeletal packet for transmit */
	struct  inpcb *t_inpcb;             /* back pointer to internet pcb */
/*
 * The following fields are used as in the protocol specification.
 * See RFC783, Dec. 1981, page 21.
 */
/* send sequence variables */
	tcp_seq        snd_una;   /* send unacknowledged */
	tcp_seq        snd_nxt;   /* send next */
	tcp_seq        snd_up;    /* send urgent pointer */
	tcp_seq        snd_wl1;   /* window update seg seq number */
	tcp_seq        snd_wl2;   /* window update seg ack number */
	tcp_seq        iss;       /* initial send sequence number */
	unsigned long  snd_wnd;   /* send window */

/* receive sequence variables */
	unsigned long  rcv_wnd;   /* receive window */
	tcp_seq rcv_nxt;          /* receive next */
	tcp_seq rcv_up;           /* receive urgent pointer */
	tcp_seq irs;              /* initial receive sequence number */ 

/*
 * Additional variables for this implementation.
 */
/* receive variables */
	tcp_seq rcv_adv;          /* advertised window */
/* retransmit variables */
	tcp_seq snd_max;          /* highest sequence number sent;
                               * used to recognize retransmits
                               */
/* congestion control (for slow start, source quench, retransmit after loss) */
	unsigned long  snd_cwnd;     /* congestion-controlled window */
	unsigned long  snd_ssthresh; /* snd_cwnd size threshhold for
                                  * for slow start exponential to
                                  * linear switch
                                  */
	uint16 t_maxopd;             /* mss plus options */
	uint16 t_peermss;            /* peer's maximum segment size */

/*
 * transmit timing stuff.  See below for scale of srtt and rttvar.
 * "Variance" is actually smoothed difference.
 */
	int16   t_idle;             /* inactivity time */
	int16   t_rtt;              /* round trip time */
	tcp_seq t_rtseq;            /* sequence number being timed */
	uint16  t_srtt;             /* smoothed round-trip time */
	uint16  t_rttvar;           /* variance in round-trip time */
	uint16  t_rttmin;           /* minimum rtt allowed */
	uint32  max_sndwnd;         /* largest window peer has offered */

/* out-of-band data */
	int8    t_oobflags;         /* have some */
	int8    t_iobc;             /* input character */ 
	int16   t_softerror;        /* possible error, not yet reported... */
/* RFC 1323 variables */
	uint8   snd_scale;          /* window scaling for send window */
	uint8   rcv_scale;          /* window scaling for recv window */
	uint8   request_r_scale;    /* pending window scaling */
	uint8   requested_s_scale;
	uint32  ts_recent;          /* timestamp echo data */
	uint32  ts_recent_age;      /* when last updated */
	tcp_seq last_ack_sent;   
};

#define intotcpcb(ip)  ((struct tcpcb*)(ip)->inp_ppcb)
#define sototcpcb(so)  (intotcpcb(sotoinpcb(so)))

#define TF_ACKNOW       0x0001  /* send ACK immeadiately */
#define TF_DELACK       0x0002  /* send ACK, but try to delay it */
#define TF_NODELAY      0x0004  /* don't delay packets to coalesce */
#define TF_NOOPT        0x0008  /* don't use tcp options */
#define TF_SENTFIN      0x0010  /* have sent FIN */
#define TF_REQ_SCALE    0x0020  /* have/will request window scaling */
#define TF_RCVD_SCALE   0x0040  /* other side has requested scaling */
#define TF_REQ_TSTMP    0x0080  /* have/will request timestamps */
#define TF_RCVD_TSTMP   0x0100  /* a timestamp was received in SYN */
#define TF_SACK_PERMIT  0x0200  /* other side said I could SACK */
#define TF_SIGNATURE    0x0400  /* require TCP MD5 signature */

#define TCPOOB_HAVEDATA 0x01    /* we have TCP OOB data */
#define TCPOOB_HADDATA  0x02    /* we've had some TCP OOB data */

/*
 * The smoothed round-trip time and estimated variance
 * are stored as fixed point numbers scaled by the values below.
 * For convenience, these scales are also used in smoothing the average
 * (smoothed = (1/scale)sample + ((scale-1)/scale)smoothed).
 * With these scales, srtt has 3 bits to the right of the binary point,
 * and thus an "ALPHA" of 0.875.  rttvar has 2 bits to the right of the
 * binary point, and is smoothed with an ALPHA of 0.75.
 */
#define	TCP_RTT_SCALE		8	/* multiplier for srtt; 3 bits frac. */
#define	TCP_RTT_SHIFT		3	/* shift for srtt; 3 bits frac. */
#define	TCP_RTTVAR_SCALE	4	/* multiplier for rttvar; 2 bits */
#define	TCP_RTTVAR_SHIFT	2	/* multiplier for rttvar; 2 bits */

#define TCP_REXMTVAL(tp) \
        ((((tp)->t_srtt >> TCP_RTT_SHIFT) + (tp)->t_rttvar) >> 2) 
/*
 * TCP statistics.
 * Many of these should be kept per connection,
 * but that's inconvenient at the moment.
 */
struct  tcpstat {
	uint32 tcps_connattempt;     /* connections initiated */
	uint32 tcps_accepts;         /* connections accepted */
	uint32 tcps_connects;        /* connections established */
	uint32 tcps_drops;           /* connections dropped */
	uint32 tcps_conndrops;       /* embryonic connections dropped */
	uint32 tcps_closed;          /* conn. closed (includes drops) */
	uint32 tcps_segstimed;       /* segs where we tried to get rtt */
	uint32 tcps_rttupdated;      /* times we succeeded */
	uint32 tcps_delack;          /* delayed acks sent */
	uint32 tcps_timeoutdrop;     /* conn. dropped in rxmt timeout */
	uint32 tcps_rexmttimeo;      /* retransmit timeouts */
	uint32 tcps_persisttimeo;    /* persist timeouts */
	uint32 tcps_persistdrop;     /* connections dropped in persist */
	uint32 tcps_keeptimeo;       /* keepalive timeouts */
	uint32 tcps_keepprobe;       /* keepalive probes sent */
	uint32 tcps_keepdrops;       /* connections dropped in keepalive */
	
	uint32 tcps_sndtotal;        /* total packets sent */
	uint32 tcps_sndpack;         /* data packets sent */
	uint64 tcps_sndbyte;         /* data bytes sent */
	uint32 tcps_sndrexmitpack;   /* data packets retransmitted */
	uint64 tcps_sndrexmitbyte;   /* data bytes retransmitted */
	uint64 tcps_sndrexmitfast;   /* Fast retransmits */
	uint32 tcps_sndacks;         /* ack-only packets sent */
	uint32 tcps_sndprobe;        /* window probes sent */
	uint32 tcps_sndurg;          /* packets sent with URG only */
	uint32 tcps_sndwinup;        /* window update-only packets sent */
	uint32 tcps_sndctrl;         /* control (SYN|FIN|RST) packets sent */ 

	uint32 tcps_rcvtotal;        /* total packets received */
	uint32 tcps_rcvpack;         /* packets received in sequence */
	uint64 tcps_rcvbyte;         /* bytes received in sequence */
	uint32 tcps_rcvbadsum;       /* packets received with ccksum errs */
	uint32 tcps_rcvbadoff;       /* packets received with bad offset */
	uint32 tcps_rcvmemdrop;      /* packets dropped for lack of memory */
	uint32 tcps_rcvnosec;        /* packets dropped for lack of ipsec */
	uint32 tcps_rcvshort;        /* packets received too short */
	uint32 tcps_rcvduppack;      /* duplicate-only packets received */
	uint64 tcps_rcvdupbyte;      /* duplicate-only bytes received */
	uint32 tcps_rcvpartduppack;  /* packets with some duplicate data */
	uint64 tcps_rcvpartdupbyte;  /* dup. bytes in part-dup. packets */
	uint32 tcps_rcvoopack;       /* out-of-order packets received */
	uint64 tcps_rcvoobyte;       /* out-of-order bytes received */
	uint32 tcps_rcvpackafterwin; /* packets with data after window */
	uint64 tcps_rcvbyteafterwin; /* bytes rcvd after window */
	uint32 tcps_rcvafterclose;   /* packets rcvd after "close" */
	uint32 tcps_rcvwinprobe;     /* rcvd window probe packets */
	uint32 tcps_rcvdupack;       /* rcvd duplicate acks */
	uint32 tcps_rcvacktoomuch;   /* rcvd acks for unsent data */
	uint32 tcps_rcvackpack;      /* rcvd ack packets */
	uint64 tcps_rcvackbyte;      /* bytes acked by rcvd acks */
	uint32 tcps_rcvwinupd;       /* rcvd window update packets */
	uint32 tcps_pawsdrop;        /* segments dropped due to PAWS */
	uint32 tcps_predack;         /* times hdr predict ok for acks */
	uint32 tcps_preddat;         /* times hdr predict ok for data pkts */

	uint32 tcps_pcbhashmiss;     /* input packets missing pcb hash */
	uint32 tcps_noport;          /* no socket on port */
	uint32 tcps_badsyn;          /* SYN packet with src==dst rcv'ed */

	uint32 tcps_rcvbadsig;       /* rcvd bad/missing TCP signatures */
	uint64 tcps_rcvgoodsig;      /* rcvd good TCP signatures */
	uint32 tcps_inhwcsum;        /* input hardware-checksummed packets */ 
	uint32 tcps_outhwcsum;       /* output hardware-checksummed packets */
};   

/*
 * Names for TCP sysctl objects.
 */

#define TCPCTL_RFC1323          1 /* enable/disable RFC1323 timestamps/scaling */
#define TCPCTL_KEEPINITTIME     2 /* TCPT_KEEP value */
#define TCPCTL_KEEPIDLE         3 /* allow tcp_keepidle to be changed */
#define TCPCTL_KEEPINTVL        4 /* allow tcp_keepintvl to be changed */
#define TCPCTL_SLOWHZ           5 /* return kernel idea of PR_SLOWHZ */
#define TCPCTL_BADDYNAMIC       6 /* return bad dynamic port bitmap */
#define TCPCTL_RECVSPACE        7 /* receive buffer space */
#define TCPCTL_SENDSPACE        8 /* send buffer space */
#define TCPCTL_IDENT            9 /* get connection owner */
#define TCPCTL_SACK            10 /* selective acknowledgement, rfc 2018 */
#define TCPCTL_MSSDFLT         11 /* Default maximum segment size */
#define TCPCTL_RSTPPSLIMIT     12 /* RST pps limit */
#define TCPCTL_MAXID           13  

#ifdef _NETWORK_STACK
struct inpcb tcb;
struct tcpstat tcpstat;
int tcp_mssdflt;
int tcp_do_rfc1323;
unsigned long tcp_now;

void   tcp_input(struct mbuf *, int);
int    tcp_output(struct tcpcb*);
int    tcp_mss(struct tcpcb *, uint);
void   tcp_mss_update(struct tcpcb *);
void   tcp_quench(struct inpcb *, int);
int    tcp_userreq(struct socket *, int, struct mbuf *, struct mbuf *, 
                struct mbuf *);
struct tcpcb * tcp_timers(struct tcpcb *, int);
struct tcpcb *tcp_close(struct tcpcb *);
void   tcp_setpersist(struct tcpcb *);
struct tcpcb *tcp_drop(struct tcpcb *, int);
void   tcp_respond(struct tcpcb *, struct tcpiphdr *, struct mbuf *,
                 tcp_seq, tcp_seq, int);
void   tcp_xmit_timer(struct tcpcb *, int16);
void   tcp_dooptions(struct tcpcb *, u_char *, int, struct tcpiphdr *,
                   int *, uint32 *, uint32 *);
struct tcpiphdr *tcp_template(struct tcpcb *);
void   tcp_pulloutofband(struct socket *, struct tcpiphdr *, struct mbuf *);

void   tcp_canceltimers(struct tcpcb *);
void   tcp_trace(int16, int16, struct tcpcb *, void *, int, int);

void   tcp_slowtimer(void *data);
void   tcp_fasttimer(void *data);



#endif

#endif /* NETINET_TCP_VAR_H */
