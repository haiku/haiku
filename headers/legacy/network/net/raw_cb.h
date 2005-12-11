/* raw_cb.h */

#ifndef NET_RAW_CB_H
#define NET_RAW_CB_H

struct rawcb {
	struct rawcb *rcb_next;
	struct rawcb *rcb_prev;
	struct socket *rcb_socket;
	struct sockaddr *rcb_faddr;
	struct sockaddr *rcb_laddr;
	struct sockproto rcb_proto;
};

#define sotorawcb(so)	((struct rawcb*)(so)->so_pcb)

#endif /* NET_RAW_CB_H */
