/* some misc functions... */

#include <stdio.h>
#include <strings.h>

#ifdef _KERNEL_
#include <KernelExport.h>
#endif

#include "net/if.h"

struct ifq *start_ifq(void)
{
	struct ifq *nifq = (struct ifq*)malloc(sizeof(*nifq));

	if (!nifq)
		return NULL;

	memset(nifq, 0, sizeof(*nifq));
	
	nifq->lock = create_sem(1, "ifq_lock");
	nifq->pop = create_sem(0, "ifq_pop");
#ifdef _KERNEL_
	set_sem_owner(nifq->lock, B_SYSTEM_TEAM);
	set_sem_owner(nifq->pop, B_SYSTEM_TEAM);
#endif

	if (nifq->lock < B_OK || nifq->pop < B_OK)
		return NULL;

	nifq->len = 0;
	nifq->maxlen = 50;
	nifq->head = nifq->tail = NULL;
	return nifq;
}

void stop_ifq(struct ifq *q)
{
	struct mbuf *m = NULL;
	
	acquire_sem_etc(q->lock, 1, B_CAN_INTERRUPT, 0);
	while (q->head) {
		m = q->head;
		q->head = m->m_nextpkt;
		m->m_nextpkt = NULL;
		m_freem(m);
	}
	q->len = 0;
	delete_sem(q->pop);
	release_sem_etc(q->lock, 1, B_CAN_INTERRUPT);
	delete_sem(q->lock);
	free(q);
}
