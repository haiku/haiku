/* Intel PRO/1000 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 */
#include "if_compat.h"
#include "if_em.h"
#include "device.h"
#include "debug.h"
#include "mempool.h"

spinlock	mbuf_lock = 0;

struct mbuf *
m_gethdr(int how, int type)
{
	struct mbuf *m = mbuf_pool_get();
	if (!m)
		return m;
	memset(m, 0, sizeof(m));
	m->m_flags = M_PKTHDR;
	return m;
}

void 
m_clget(struct mbuf * m, int how)
{
//	TRACE("m_clget\n");

	m->m_ext.ext_buf = chunk_pool_get();
	m->m_data = m->m_ext.ext_buf;
	if (m->m_ext.ext_buf)
		m->m_flags |= M_EXT;
}

void
m_adj(struct mbuf *mp, int bytes)
{
	mp->m_data = (char *)mp->m_data + bytes;
}

void
m_freem(struct mbuf *mp)
{
	struct mbuf *m, *c;
//	TRACE("m_freem\n");

	for (m = mp; m; ) {
		if (m->m_flags & M_EXT)
			chunk_pool_put(m->m_ext.ext_buf);
		c = m;
		m = m->m_next;
		mbuf_pool_put(c);
	}
}

static void
ether_input(struct ifnet *ifp, struct mbuf *m)
{
/*
	uint8 *buf;
	int len;
	
	buf = mtod(m, uint8 *);
	len = m->m_len;
	TRACE("ether_input: received packet with %d bytes\n", len);
	TRACE("%02x %02x %02x %02x . %02x %02x %02x %02x . %02x %02x %02x %02x \n",
		buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], 
		buf[6], buf[7], buf[8], buf[8], buf[10], buf[11]);
	
	m_freem(m);
	return;
*/

	IF_APPEND(&ifp->if_rcv, m);
	release_sem_etc(ifp->if_rcv_sem, 1, B_DO_NOT_RESCHEDULE);
}

void 
ether_ifattach(struct ifnet *ifp, const uint8 *etheraddr)
{
	ipro1000_device *dev = ifp->if_softc->dev;

	TRACE("ether_ifattach\n");

	memcpy(dev->macaddr, etheraddr, 6);
	
	ifp->if_input = ether_input;

	ifp->if_rcv_sem = create_sem(0, "ifp->if_rcv_sem");
	set_sem_owner(ifp->if_rcv_sem, B_SYSTEM_TEAM);
	
	TRACE("calling if_init...\n");
	ifp->if_init(ifp->if_softc);
	TRACE("done calling if_init!\n");
}

void
ether_ifdetach(struct ifnet *ifp)
{
	TRACE("ether_ifdetach\n");

	delete_sem(ifp->if_rcv_sem);
}

struct mbuf *
if_dequeue(struct if_queue *queue)
{
	cpu_status s;
	struct mbuf *m;

//	TRACE("IF_DEQUEUE\n");
	
	s = disable_interrupts();
	acquire_spinlock(&mbuf_lock);
	
	m = (struct mbuf *) queue->ifq_head;
	if (m) {
		queue->ifq_head = m->m_nextq;
		if (!queue->ifq_head)
			queue->ifq_tail = 0;
		m->m_nextq = 0;
	}

	release_spinlock(&mbuf_lock);
	restore_interrupts(s);
	
	return m;
}

void
if_prepend(struct if_queue *queue, struct mbuf *mb)
{
	cpu_status s;
//	TRACE("IF_PREPEND\n");
	
	s = disable_interrupts();
	acquire_spinlock(&mbuf_lock);
	
	mb->m_nextq = (struct mbuf *) queue->ifq_head;
	queue->ifq_head = mb;
	if (!queue->ifq_tail)
		queue->ifq_tail = mb;

	release_spinlock(&mbuf_lock);
	restore_interrupts(s);
}

void
if_append(struct if_queue *queue, struct mbuf *mb)
{
	cpu_status s;
//	TRACE("IF_APPEND\n");
	
	s = disable_interrupts();
	acquire_spinlock(&mbuf_lock);
	
	mb->m_nextq = 0;
	if (!queue->ifq_tail) {
		queue->ifq_tail = mb;
		queue->ifq_head = mb;
	} else {
		queue->ifq_tail->m_nextq = mb;
		queue->ifq_tail = mb;
	}

	release_spinlock(&mbuf_lock);
	restore_interrupts(s);
}
