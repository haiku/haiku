/* Intel PRO/1000 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies, and that both the
 * copyright notice and this permission notice appear in supporting documentation.
 *
 * Marcus Overhagen makes no representations about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 *
 * MARCUS OVERHAGEN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL MARCUS
 * OVERHAGEN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "if_compat.h"
#include "if_em.h"
#include "device.h"
#include "debug.h"
#include "mempool.h"

#undef malloc
#undef free

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

	INIT_DEBUGOUT("ether_ifattach");

	TAILQ_INIT(&ifp->if_multiaddrs);

	memcpy(dev->macaddr, etheraddr, 6);
	
	ifp->if_input = ether_input;

	ifp->if_rcv_sem = create_sem(0, "ifp->if_rcv_sem");
	set_sem_owner(ifp->if_rcv_sem, B_SYSTEM_TEAM);
	
	INIT_DEBUGOUT("calling if_init...");
	ifp->if_init(ifp->if_softc);
	INIT_DEBUGOUT("done calling if_init!");
}


static struct ifmultiaddr *
ether_find_multi(struct ifnet *ifp, const struct sockaddr *_address)
{
	const struct sockaddr_dl *address = (const struct sockaddr_dl *)_address;
	struct ifmultiaddr *ifma;

	TAILQ_FOREACH (ifma, &ifp->if_multiaddrs, ifma_link) {
		if (memcmp(LLADDR(address),
				LLADDR((struct sockaddr_dl *)ifma->ifma_addr),
				ETHER_ADDR_LEN) == 0)
			return ifma;
	}

	return NULL;
}


int
ether_add_multi(struct ifnet *ifp, const struct sockaddr *address)
{
	struct ifmultiaddr *addr = ether_find_multi(ifp, address);

	if (addr != NULL) {
		addr->ifma_refcount++;
		return 0;
	}

	addr = (struct ifmultiaddr *)malloc(sizeof(struct ifmultiaddr));
	if (addr == NULL)
		return ENOBUFS;

	memcpy(&addr->ifma_addr_storage, address, sizeof(struct sockaddr_dl));
	addr->ifma_addr = (struct sockaddr *)&addr->ifma_addr_storage;

	addr->ifma_refcount = 1;

	TAILQ_INSERT_HEAD(&ifp->if_multiaddrs, addr, ifma_link);

	return ifp->if_ioctl(ifp, SIOCADDMULTI, NULL);
}


static void
ether_delete_multi(struct ifnet *ifp, struct ifmultiaddr *ifma)
{
	TAILQ_REMOVE(&ifp->if_multiaddrs, ifma, ifma_link);
	free(ifma);
}


int
ether_rem_multi(struct ifnet *ifp, const struct sockaddr *address)
{
	struct ifmultiaddr *addr = ether_find_multi(ifp, address);
	if (addr == NULL)
		return EADDRNOTAVAIL;

	addr->ifma_refcount--;
	if (addr->ifma_refcount == 0) {
		ether_delete_multi(ifp, addr);
		return ifp->if_ioctl(ifp, SIOCDELMULTI, NULL);
	}

	return 0;
}


void
ether_ifdetach(struct ifnet *ifp)
{
	struct ifmultiaddr *ifma, *next;

	INIT_DEBUGOUT("ether_ifdetach");

	delete_sem(ifp->if_rcv_sem);

	TAILQ_FOREACH_SAFE(ifma, &ifp->if_multiaddrs, ifma_link, next)
		ether_delete_multi(ifp, ifma);
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
