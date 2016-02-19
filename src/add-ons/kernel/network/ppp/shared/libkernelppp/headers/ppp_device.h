#ifndef __PPP_DEV_H
#define __PPP_DEV_H
#include <net_device.h>
#include <net_stack.h>

#include <KPPPInterface.h>

#include <lock.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>

struct ppp_device : net_device, DoublyLinkedListLinkImpl<ppp_device> {
	KPPPInterface *         KPPP_Interface;
	uint32                  frame_size;
	net_fifo                ppp_fifo;
};

#endif
