/* core_module.h
 * definitions needed by the core networking module
 */

#ifndef OBOS_CORE_MODULE_H
#define OBOS_CORE_MODULE_H

#include <mbuf.h>
#include <sys/protosw.h>
#include <sys/socketvar.h>
#include <netinet/in_pcb.h>
#include <net/if.h>
#include <net_timer.h>
#include <net_module.h>

struct core_module_info {
	module_info	module;

	int (*start)(void);
	int (*stop)(void);
	status_t (*control_net_module)(const char *name, uint32 op, void *data,
		size_t length);
	void (*add_domain)(struct domain *, int);
	void (*remove_domain)(int);
	void (*add_protocol)(struct protosw *, int);
	void (*remove_protocol)(struct protosw *pr);
	void (*add_protosw)(struct protosw *prt[], int layer);
	void (*start_rx_thread)(struct ifnet *dev);
	void (*start_tx_thread)(struct ifnet *dev);

	// functions for the 'max_xxx' values
	int (*get_max_hdr)(void);
	void (*set_max_linkhdr)(int maxLinkHdr);
	int (*get_max_linkhdr)(void);
	void (*set_max_protohdr)(int maxProtoHdr);
	int (*get_max_protohdr)(void);
	
	/* timer functions */
	net_timer_id (*net_add_timer)(net_timer_hook ,void *,
	                              bigtime_t);
	status_t (*net_remove_timer)(net_timer_id);

	/* ifq functions */
	struct ifq *(*start_ifq)(void);
	void (*stop_ifq)(struct ifq *);
		                           	
	/* pool functions */
	status_t (*pool_init)(pool_ctl **p, size_t sz);
	char *(*pool_get)(pool_ctl *p);
	void (*pool_put)(pool_ctl *p, void *ptr);
	void (*pool_destroy)(pool_ctl *p);

	/* socket functions - called "internally" */
	struct socket *(*sonewconn)(struct socket *, int);
	int  (*soreserve)(struct socket *, uint32, uint32);
	int  (*sockbuf_reserve)(struct sockbuf *, uint32);
	void (*sockbuf_append)(struct sockbuf *, struct mbuf *);
	int  (*sockbuf_appendaddr)(struct sockbuf *, struct sockaddr *, 
		 				struct mbuf *, struct mbuf *);
	void (*sockbuf_drop)(struct sockbuf *, int);
	void (*sockbuf_flush)(struct sockbuf *sb);
	void (*sowakeup)(struct socket *, struct sockbuf *);
	void (*socket_set_connected)(struct socket *so);
	void (*socket_set_connecting)(struct socket *so);
	void (*socket_set_disconnected)(struct socket *so);
	void (*socket_set_disconnecting)(struct socket *so);
	void (*socket_set_hasoutofband)(struct socket *so);
	void (*socket_set_cantrcvmore)(struct socket *so);
	void (*socket_set_cantsendmore)(struct socket *so);
	
	/* pcb options */
	int  (*in_pcballoc)(struct socket *, struct inpcb *);
	void (*in_pcbdetach)(struct inpcb *); 
	int  (*in_pcbbind)(struct inpcb *, struct mbuf *);
	int  (*in_pcbconnect)(struct inpcb *, struct mbuf *);
	int  (*in_pcbdisconnect)(struct inpcb *);
	struct inpcb * (*in_pcblookup)(struct inpcb *, 
	    struct in_addr, uint16, struct in_addr, uint16, int);
	int  (*in_control)(struct socket *, int, caddr_t,
	    struct ifnet *);
	void (*in_losing)(struct inpcb *);
	int  (*in_canforward)(struct in_addr);
	int  (*in_localaddr)(struct in_addr);
	struct rtentry *(*in_pcbrtentry)(struct inpcb *);
	void (*in_setsockaddr)(struct inpcb *, struct mbuf *);
	void (*in_setpeeraddr)(struct inpcb *, struct mbuf *);
	void (*in_pcbnotify)(struct inpcb *, struct sockaddr *,
                         uint16, struct in_addr, uint16, 
                         int, void (*)(struct inpcb *, int));
	int  (*inetctlerrmap)(int);
	uint16 (*in_cksum)(struct mbuf *m, int len, int off);

	
	/* mbuf routines... */
	struct mbuf * (*m_gethdr)(int);
	struct mbuf * (*m_get)(int);
	void (*m_cat)(struct mbuf *, struct mbuf *);
	void (*m_adj)(struct mbuf*, int);
	struct mbuf * (*m_prepend)(struct mbuf*, int);
	struct mbuf *(*m_pullup)(struct mbuf *, int);
	void (*m_copyback)(struct mbuf *, int, int, caddr_t);
	void (*m_copydata)(struct mbuf *, int, int, caddr_t);
	struct mbuf *(*m_copym)(struct mbuf *, int, int);
	struct mbuf * (*m_free)(struct mbuf *);
	void (*m_freem)(struct mbuf *);
	void (*m_reserve)(struct mbuf *, int);
	struct mbuf *(*m_devget)(char *, int, int, struct ifnet *,
			void (*copy)(const void *, void *, size_t));

	/* module control routines... */
	void (*add_device)(struct ifnet *);
	struct ifnet *(*get_interfaces)(void);
	int (*in_broadcast)(struct in_addr, struct ifnet *);
		
	/* routing */
	void (*rtalloc)(struct route *ro);
	struct rtentry *(*rtalloc1)(struct sockaddr *, int);
	void (*rtfree)(struct rtentry *);
	int	(*rtrequest)(int, struct sockaddr *,
                     struct sockaddr *, struct sockaddr *, int,
                     struct rtentry **);
	struct radix_node *(*rn_addmask)(void *, int, int);
	struct radix_node *(*rn_head_search)(void *argv_v);
	struct radix_node_head **(*get_rt_tables)(void);
	int	(*rt_setgate)(struct rtentry *, struct sockaddr *,
	                  struct sockaddr *);

	/* ifnet functions */
	struct ifaddr *(*ifa_ifwithdstaddr)(struct sockaddr *addr);
	struct ifaddr *(*ifa_ifwithnet)(struct sockaddr *addr);
	void (*if_attach)(struct ifnet *);
	void (*if_detach)(struct ifnet *);
	struct ifaddr *(*ifa_ifwithaddr)(struct sockaddr *);
	struct ifaddr *(*ifa_ifwithroute)(int, struct sockaddr *,
                                       struct sockaddr *);
	struct ifaddr *(*ifaof_ifpforaddr)(struct sockaddr *, struct ifnet *);
	void (*ifafree)(struct ifaddr *);

	struct in_ifaddr *(*get_primary_addr)(void);	

	int (*net_sysctl)(int *, uint, void *, size_t *, void *, size_t);

	/* socket functions - used by socket driver */
	int (*socket_init)			(struct socket **nso);
	int (*socket_create)		(struct socket *so, int dom, int type, int proto);
	int (*socket_close)			(struct socket *so);
	int (*socket_bind)			(struct socket *so, caddr_t, int);
	int (*socket_listen)		(struct socket *so, int backlog);
	int (*socket_connect)		(struct socket *so, caddr_t, int);
	int (*socket_accept)		(struct socket *so, struct socket **nso, void *, int *);
	int (*socket_recv)			(struct socket *so, struct msghdr *, caddr_t namelenp, int *retsize);
	int (*socket_send)			(struct socket *so, struct msghdr *, int flags, int *retsize);
	int (*socket_ioctl)			(struct socket *so, int, caddr_t);
	int (*socket_writev)		(struct socket *so, struct iovec *, int flags);
	int (*socket_readv)			(struct socket *so, struct iovec *, int *flags);
	int (*socket_setsockopt)	(struct socket *so, int, int, const void *, size_t);
	int (*socket_getsockopt)	(struct socket *so, int, int, void *, size_t *);
	int (*socket_getpeername)	(struct socket *so, struct sockaddr *, int *);
	int (*socket_getsockname)	(struct socket *so, struct sockaddr *, int *);
	int (*socket_set_event_callback)(struct socket *so, socket_event_callback, void *, int);
	int (*socket_shutdown)		(struct socket *so, int how);
};

#define NET_CORE_MODULE_NAME	"network/core/v1"

#endif /* OBOS_CORE_MODULE_H */
