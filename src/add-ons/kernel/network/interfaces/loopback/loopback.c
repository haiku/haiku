/* loopback.c - loopback device 
 */

#include <stdio.h>
#include <malloc.h>
#include <kernel/OS.h>

#include "sys/socket.h"
#include "protocols.h"
#include "netinet/in.h"
#include "netinet/ip.h"
#include "sys/socketvar.h"
#include "sys/protosw.h"
#include "sys/domain.h"
#include "sys/sockio.h"

#include "net_malloc.h"
#include "core_module.h"
#include "net_module.h"
#include "core_funcs.h"

#include <KernelExport.h>
#define printf dprintf

status_t std_ops(int32 op, ...);

static struct core_module_info *core = NULL;

static struct protosw *proto[IPPROTO_MAX];
static struct ifnet *me = NULL;

static int loopback_output(struct ifnet *ifp, struct mbuf *m, struct sockaddr *sa,
			struct rtentry *rt)
{
	/* turn it straight back... */
	/* This is lame as we should be detecting the protocol, but it gets
	 * us working.
	 * XXX - fix me!
	 */
	struct ip *ip = mtod(m, struct ip *);

	ip->ip_dst = ip->ip_src;
	ip->ip_src.s_addr = INADDR_LOOPBACK;

	IFQ_ENQUEUE(ifp->rxq, m);
	return 0;
}

static void loopback_input(struct mbuf *buf)
{
	if (!buf)
		return;

	buf->m_pkthdr.rcvif = me;
	
	if (proto[IPPROTO_IP] && proto[IPPROTO_IP]->pr_input) {
		proto[IPPROTO_IP]->pr_input(buf, 0);
		return;
	} else
		printf("No input tourtine found for IP\n");

	m_freem(buf);
	return;
}

static int loopback_dev_stop(struct ifnet *dev)
{
	if (!dev || dev->if_type != IFT_LOOP)
		return EINVAL;

	dev->if_flags &= ~IFF_UP;

	if (dev->rx_thread > 0)
		kill_thread(dev->rx_thread);
	if (dev->tx_thread > 0)
		kill_thread(dev->tx_thread);

	dev->if_flags &= ~IFF_RUNNING;
	
	return 0;
}

static int loopback_ioctl(struct ifnet *ifp, ulong cmd, caddr_t data)
{
	int error = 0;

	switch(cmd) {
		case SIOCSIFADDR:
			ifp->if_flags |= (IFF_UP | IFF_RUNNING);
			if (ifp->rx_thread < 0)
				start_rx_thread(ifp);
			if (ifp->tx_thread < 0)
				start_tx_thread(ifp);
			break;
		default:
			error = EINVAL;
	}
	return error;
}
	
static int loopback_init(void)
{
	me = (struct ifnet*)malloc(sizeof(struct ifnet));
	if (!me)
		return -1;
		
	memset(me, 0, sizeof(*me));

	memset(proto, 0, sizeof(struct protosw *) * IPPROTO_MAX);

	me->devid = -1;
	me->name = "loop";
	me->if_unit = 0;
	me->if_type = IFT_LOOP;
	me->rx_thread = -1;
	me->tx_thread = -1;
	me->if_addrlen = 0;
	me->if_hdrlen = 0;
	me->if_flags = IFF_LOOPBACK | IFF_MULTICAST;
	me->if_mtu = 16384; /* can be as large as we want */
	me->input = &loopback_input;
	me->output = &loopback_output;
	me->stop = &loopback_dev_stop;
	me->ioctl = &loopback_ioctl;
	
	add_protosw(proto, NET_LAYER1);
	if_attach(me);

	return 0;
}

static int loopback_module_init(void *cpp)
{
	if (cpp)
		core = cpp;

	loopback_init();
	
	return 0;
}

struct kernel_net_module_info device_info = {
	{
		"network/interfaces/loopback",
		0,
		std_ops
	},
	loopback_module_init,
	NULL,
	NULL
};

// #pragma mark -

_EXPORT status_t std_ops(int32 op, ...) 
{
	switch(op) {
		case B_MODULE_INIT:
			get_module(NET_CORE_MODULE_NAME, (module_info **)&core);
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
	(module_info*) &device_info,
	NULL
};

