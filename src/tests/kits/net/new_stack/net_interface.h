/* net_interface.h
 * definitions of interface networking module API
 */

#ifndef OBOS_NET_INTERFACE_H
#define OBOS_NET_INTERFACE_H

#include <drivers/module.h>

#ifdef __cplusplus
extern "C" {
#endif

struct net_buffer;

typedef struct if_data { 
        /* generic interface information */
		// ifmedia_t	ifi_media;			/* media, see if_media.h */
        uint8  ifi_type;               /* ethernet, tokenring, etc */  
        uint8  ifi_addrlen;            /* media address length */ 
        uint8  ifi_hdrlen;             /* media header length */ 
        uint32  ifi_mtu;                /* maximum transmission unit */
										/* this does not count framing; */
										/* e.g. ethernet would be 1500 */ 
        uint32  ifi_metric;             /* routing metric (external only) */ 
        /* volatile statistics */ 
        uint32  ifi_ipackets;           /* packets received on interface */ 
        uint32  ifi_ierrors;            /* input errors on interface */ 
        uint32  ifi_opackets;           /* packets sent on interface */ 
        uint32  ifi_oerrors;            /* output errors on interface */ 
        uint32  ifi_collisions;         /* collisions on csma interfaces */ 
        uint32  ifi_ibytes;             /* total number of octets received */ 
        uint32  ifi_obytes;             /* total number of octets sent */ 
        uint32  ifi_imcasts;            /* packets received via multicast */ 
        uint32  ifi_omcasts;            /* packets sent via multicast */ 
        uint32  ifi_iqdrops;            /* dropped on input, this interface */ 
        uint32  ifi_noproto;            /* destined for unsupported protocol */ 
        bigtime_t ifi_lastchange; /* time of last administrative change */ 
} ifdata_t; 

#define        if_mtu          if_data.ifi_mtu 
#define        if_type         if_data.ifi_type 
#define		   if_media        if_data.ifi_media 
#define        if_addrlen      if_data.ifi_addrlen 
#define        if_hdrlen       if_data.ifi_hdrlen 
#define        if_metric       if_data.ifi_metric 
#define        if_baudrate     if_data.ifi_baudrate 
#define        if_ipackets     if_data.ifi_ipackets 
#define        if_ierrors      if_data.ifi_ierrors 
#define        if_opackets     if_data.ifi_opackets 
#define        if_oerrors      if_data.ifi_oerrors 
#define        if_collisions   if_data.ifi_collisions 
#define        if_ibytes       if_data.ifi_ibytes 
#define        if_obytes       if_data.ifi_obytes 
#define        if_imcasts      if_data.ifi_imcasts 
#define        if_omcasts      if_data.ifi_omcasts 
#define        if_noproto      if_data.ifi_noproto 
#define        if_lastchange   if_data.ifi_lastchange 



#define        IFF_UP          0x1             /* interface is up */ 
#define        IFF_BROADCAST   0x2             /* broadcast address valid */ 
#define        IFF_DEBUG       0x4             /* turn on debugging */ 
#define        IFF_LOOPBACK    0x8             /* is a loopback net */ 
#define        IFF_POINTOPOINT 0x10            /* interface is point-to-point link */
#define        IFF_PTP         IFF_POINTOPOINT
#define		   IFF_NOARP       0x40				/* don't arp */
#define        IFF_AUTOUP      0x80            /* for autodial ppp - anytime the interface is downed, it will immediately be re-upped by the datalink */
#define        IFF_PROMISC     0x100           /* receive all packets */ 
#define        IFF_ALLMULTI    0x200           /* receive all multicast packets */ 
#define        IFF_SIMPLEX     0x800           /* can't hear own transmissions */ 
#define        IFF_MULTICAST   0x8000          /* supports multicast */ 

/* flags set internally only: */ 
#define	IFF_CANTCHANGE (IFF_BROADCAST|IFF_LOOPBACK|IFF_POINTOPOINT| \
		IFF_NOARP|IFF_SIMPLEX|IFF_MULTICAST|IFF_ALLMULTI)



typedef struct ifnet {
	struct ifnet 						*if_next;
	char 								*if_name;
	uint32 								if_flags;
	struct net_interface_module_info 	*module;	
	struct if_data 						if_data;
	volatile thread_id					if_reader_thread;
} ifnet_t;

typedef struct net_interface_hwaddr {
	uint32	len;
	uint8	hwaddr[256];
} net_interface_hwaddr;

struct net_interface_module_info {
	module_info	info;

	status_t (*init)(void * params);
	status_t (*uninit)(ifnet_t *ifnet);
	
	status_t (*up)(ifnet_t *ifnet);
	status_t (*down)(ifnet_t *ifnet);

	status_t (*send_buffer)(ifnet_t *ifnet, struct net_buffer *buffer);
	status_t (*receive_buffer)(ifnet_t *ifnet, struct net_buffer **buffer);
	
	status_t (*control)(ifnet_t *ifnet, int opcode, void *arg);
	
	status_t (*get_hardware_address)(ifnet_t *ifnet, net_interface_hwaddr *hwaddr);
	status_t (*get_multicast_addresses)(ifnet_t *ifnet, net_interface_hwaddr **hwaddr, int nb_addresses);
	status_t (*set_multicast_addresses)(ifnet_t *ifnet, net_interface_hwaddr *hwaddr, int nb_addresses);
	
	status_t (*set_mtu)(ifnet_t *ifnet, uint32 mtu);
	status_t (*set_promiscuous)(ifnet_t *ifnet, bool enable);
	status_t (*set_media)(ifnet_t *ifnet, uint32 media);
};

#define NET_INTERFACE_MODULE_ROOT	"network/interfaces/"

#ifdef __cplusplus
}
#endif

#endif /* OBOS_NET_INTERFACE_H */
