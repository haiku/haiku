/* core_funcs.h
 * convenience macros to for calling core functions in the kernel
 */

#ifndef OBOS_CORE_FUNCS_H
#define OBOS_CORE_FUNCS_H

#include "core_module.h"

#	define add_protosw         core->add_protosw
#   define add_domain          core->add_domain
#   define remove_domain       core->remove_domain
#   define add_protocol        core->add_protocol
#   define remove_protocol     core->remove_protocol
#	define start_rx_thread	   core->start_rx_thread
#	define start_tx_thread	   core->start_tx_thread

#   define net_add_timer       core->net_add_timer
#   define net_remove_timer    core->net_remove_timer

#   define start_ifq           core->start_ifq
#   define stop_ifq            core->stop_ifq
 
#	define pool_init           core->pool_init
#	define pool_get            core->pool_get
#	define pool_put            core->pool_put

#	define m_get               core->m_get
#	define m_gethdr            core->m_gethdr
#	define m_free              core->m_free
#	define m_freem             core->m_freem
#   define m_cat               core->m_cat
#	define m_adj               core->m_adj
#	define m_prepend           core->m_prepend
#	define m_pullup            core->m_pullup
#	define m_copydata          core->m_copydata
#	define m_copyback          core->m_copyback
#	define m_copym             core->m_copym
#   define m_reserve           core->m_reserve
#   define m_devget            core->m_devget

#	define if_attach           core->if_attach
#   define if_detach           core->if_detach

#	define in_pcballoc         core->in_pcballoc
#	define in_pcbconnect       core->in_pcbconnect
#	define in_pcbdisconnect    core->in_pcbdisconnect
#	define in_pcbbind          core->in_pcbbind
#	define in_pcblookup        core->in_pcblookup
#	define in_pcbdetach        core->in_pcbdetach
#	define in_pcbrtentry       core->in_pcbrtentry
#   define in_canforward       core->in_canforward
#	define in_localaddr        core->in_localaddr
#	define in_losing           core->in_losing
#	define in_broadcast        core->in_broadcast
#	define in_control          core->in_control
#	define in_setsockaddr      core->in_setsockaddr
#	define in_setpeeraddr      core->in_setpeeraddr
#   define in_pcbnotify        core->in_pcbnotify
#   define inetctlerrmap       core->inetctlerrmap

#	define ifa_ifwithdstaddr   core->ifa_ifwithdstaddr
#	define ifa_ifwithaddr      core->ifa_ifwithaddr
#	define ifa_ifwithnet       core->ifa_ifwithnet
#	define ifa_ifwithroute     core->ifa_ifwithroute
#	define ifaof_ifpforaddr    core->ifaof_ifpforaddr
#	define ifafree             core->ifafree

#	define sbappend            core->sbappend
#	define sbappendaddr        core->sbappendaddr
#	define sbdrop              core->sbdrop
#	define sbflush             core->sbflush
#	define sbreserve           core->sbreserve

#	define soreserve           core->soreserve
#	define sowakeup            core->sowakeup
#	define sonewconn           core->sonewconn
#	define soisconnected       core->soisconnected
#	define soisconnecting      core->soisconnecting
#	define soisdisconnected    core->soisdisconnected
#	define soisdisconnecting   core->soisdisconnecting
#	define sohasoutofband      core->sohasoutofband
#	define socantsendmore      core->socantsendmore
#	define socantrcvmore       core->socantrcvmore

#	define rtfree              core->rtfree
#	define rtalloc             core->rtalloc
#	define rtalloc1            core->rtalloc1
#	define rtrequest           core->rtrequest

#	define rt_setgate          core->rt_setgate
#	define get_rt_tables       core->get_rt_tables

#	define rn_addmask          core->rn_addmask
#	define rn_head_search      core->rn_head_search

#	define get_interfaces      core->get_interfaces
#	define get_primary_addr    core->get_primary_addr

#   define initsocket          core->initsocket
#   define socreate            core->socreate
#   define soclose             core->soclose

#   define sobind              core->sobind
#   define soconnect           core->soconnect
#   define solisten            core->solisten
#   define soaccept            core->soaccept

#   define soo_ioctl           core->soo_ioctl
#   define net_sysctl          core->net_sysctl

#   define readit              core->readit
#   define writeit             core->writeit
 
#endif	/* OBOS_CORE_FUNCS_H */
