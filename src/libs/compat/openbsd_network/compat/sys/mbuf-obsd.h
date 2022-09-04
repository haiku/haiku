/*	$OpenBSD: uipc_mbuf.c,v 1.283 2022/02/22 01:15:01 guenther Exp $	*/
/*	$NetBSD: uipc_mbuf.c,v 1.15.4.1 1996/06/13 17:11:44 cgd Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1988, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)uipc_mbuf.c	8.2 (Berkeley) 1/4/94
 */

/*
 *	@(#)COPYRIGHT	1.1 (NRL) 17 January 1995
 *
 * NRL grants permission for redistribution and use in source and binary
 * forms, with or without modification, of the software and documentation
 * created at NRL provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgements:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 *	This product includes software developed at the Information
 *	Technology Division, US Naval Research Laboratory.
 * 4. Neither the name of the NRL nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THE SOFTWARE PROVIDED BY NRL IS PROVIDED BY NRL AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL NRL OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the US Naval
 * Research Laboratory (NRL).
 */

struct mbuf_list {
	struct mbuf		*ml_head;
	struct mbuf		*ml_tail;
	u_int			ml_len;
};

struct mbuf_queue {
	struct mutex		mq_mtx;
	struct mbuf_list	mq_list;
	u_int			mq_maxlen;
	u_int			mq_drops;
};


static struct mbuf *
m_dup_pkt(struct mbuf *m0, unsigned int adj, int wait)
{
	struct mbuf *m;
	int len;

	KASSERT(m0->m_flags & M_PKTHDR);

	len = m0->m_pkthdr.len + adj;
	if (len > MAXMCLBYTES) /* XXX */
		return (NULL);

	m = m_get(wait, m0->m_type);
	if (m == NULL)
		return (NULL);

	if (m_dup_pkthdr(m, m0, wait) != 0)
		goto fail;

	if (len > MHLEN) {
		MCLGETL(m, wait, len);
		if (!ISSET(m->m_flags, M_EXT))
			goto fail;
	}

	m->m_len = m->m_pkthdr.len = len;
	m_adj(m, adj);
	m_copydata(m0, 0, m0->m_pkthdr.len, mtod(m, caddr_t));

	return (m);

fail:
	m_freem(m);
	return (NULL);
}


/*
 * Compute the amount of space available after the end of data in an mbuf.
 * Read-only clusters never have space available.
 */
static int
m_trailingspace(struct mbuf *m)
{
	if (M_READONLY(m))
		return 0;
	KASSERT(M_DATABUF(m) + M_SIZE(m) >= (m->m_data + m->m_len));
	return M_DATABUF(m) + M_SIZE(m) - (m->m_data + m->m_len);
}


/*
 * mbuf lists
 */

#define MBUF_LIST_INITIALIZER() { NULL, NULL, 0 }

#define	ml_len(_ml)		((_ml)->ml_len)
#define	ml_empty(_ml)		((_ml)->ml_len == 0)

#define MBUF_LIST_FIRST(_ml)	((_ml)->ml_head)
#define MBUF_LIST_NEXT(_m)	((_m)->m_nextpkt)

#define MBUF_LIST_FOREACH(_ml, _m)					\
	for ((_m) = MBUF_LIST_FIRST(_ml);				\
		(_m) != NULL;						\
		(_m) = MBUF_LIST_NEXT(_m))

static void
ml_init(struct mbuf_list *ml)
{
	ml->ml_head = ml->ml_tail = NULL;
	ml->ml_len = 0;
}

static void
ml_enqueue(struct mbuf_list *ml, struct mbuf *m)
{
	if (ml->ml_tail == NULL)
		ml->ml_head = ml->ml_tail = m;
	else {
		ml->ml_tail->m_nextpkt = m;
		ml->ml_tail = m;
	}

	m->m_nextpkt = NULL;
	ml->ml_len++;
}

static void
ml_enlist(struct mbuf_list *mla, struct mbuf_list *mlb)
{
	if (!ml_empty(mlb)) {
		if (ml_empty(mla))
			mla->ml_head = mlb->ml_head;
		else
			mla->ml_tail->m_nextpkt = mlb->ml_head;
		mla->ml_tail = mlb->ml_tail;
		mla->ml_len += mlb->ml_len;

		ml_init(mlb);
	}
}

static struct mbuf *
ml_dequeue(struct mbuf_list *ml)
{
	struct mbuf *m;

	m = ml->ml_head;
	if (m != NULL) {
		ml->ml_head = m->m_nextpkt;
		if (ml->ml_head == NULL)
			ml->ml_tail = NULL;

		m->m_nextpkt = NULL;
		ml->ml_len--;
	}

	return (m);
}

static struct mbuf *
ml_dechain(struct mbuf_list *ml)
{
	struct mbuf *m0;

	m0 = ml->ml_head;

	ml_init(ml);

	return (m0);
}

static unsigned int
ml_purge(struct mbuf_list *ml)
{
	struct mbuf *m, *n;
	unsigned int len;

	for (m = ml->ml_head; m != NULL; m = n) {
		n = m->m_nextpkt;
		m_freem(m);
	}

	len = ml->ml_len;
	ml_init(ml);

	return (len);
}

static unsigned int
ml_hdatalen(struct mbuf_list *ml)
{
	struct mbuf *m;

	m = ml->ml_head;
	if (m == NULL)
		return (0);

	KASSERT(ISSET(m->m_flags, M_PKTHDR));
	return (m->m_pkthdr.len);
}


/*
 * mbuf queues
 */

#define MBUF_QUEUE_INITIALIZER(_maxlen, _ipl) \
	{ MUTEX_INITIALIZER(_ipl), MBUF_LIST_INITIALIZER(), (_maxlen), 0 }

#define	mq_len(_mq)		ml_len(&(_mq)->mq_list)
#define	mq_empty(_mq)		ml_empty(&(_mq)->mq_list)
#define	mq_full(_mq)		(mq_len((_mq)) >= (_mq)->mq_maxlen)
#define	mq_drops(_mq)		((_mq)->mq_drops)
#define	mq_set_maxlen(_mq, _l)	((_mq)->mq_maxlen = (_l))

static void
mq_init(struct mbuf_queue *mq, u_int maxlen, int ipl)
{
	mtx_init(&mq->mq_mtx, ipl);
	ml_init(&mq->mq_list);
	mq->mq_maxlen = maxlen;
}

static int
mq_push(struct mbuf_queue *mq, struct mbuf *m)
{
	struct mbuf *dropped = NULL;

	mtx_enter(&mq->mq_mtx);
	if (mq_len(mq) >= mq->mq_maxlen) {
		mq->mq_drops++;
		dropped = ml_dequeue(&mq->mq_list);
	}
	ml_enqueue(&mq->mq_list, m);
	mtx_leave(&mq->mq_mtx);

	if (dropped)
		m_freem(dropped);

	return (dropped != NULL);
}

static int
mq_enqueue(struct mbuf_queue *mq, struct mbuf *m)
{
	int dropped = 0;

	mtx_enter(&mq->mq_mtx);
	if (mq_len(mq) < mq->mq_maxlen)
		ml_enqueue(&mq->mq_list, m);
	else {
		mq->mq_drops++;
		dropped = 1;
	}
	mtx_leave(&mq->mq_mtx);

	if (dropped)
		m_freem(m);

	return (dropped);
}

static struct mbuf *
mq_dequeue(struct mbuf_queue *mq)
{
	struct mbuf *m;

	mtx_enter(&mq->mq_mtx);
	m = ml_dequeue(&mq->mq_list);
	mtx_leave(&mq->mq_mtx);

	return (m);
}

static int
mq_enlist(struct mbuf_queue *mq, struct mbuf_list *ml)
{
	struct mbuf *m;
	int dropped = 0;

	mtx_enter(&mq->mq_mtx);
	if (mq_len(mq) < mq->mq_maxlen)
		ml_enlist(&mq->mq_list, ml);
	else {
		dropped = ml_len(ml);
		mq->mq_drops += dropped;
	}
	mtx_leave(&mq->mq_mtx);

	if (dropped) {
		while ((m = ml_dequeue(ml)) != NULL)
			m_freem(m);
	}

	return (dropped);
}

static void
mq_delist(struct mbuf_queue *mq, struct mbuf_list *ml)
{
	mtx_enter(&mq->mq_mtx);
	*ml = mq->mq_list;
	ml_init(&mq->mq_list);
	mtx_leave(&mq->mq_mtx);
}

static struct mbuf *
mq_dechain(struct mbuf_queue *mq)
{
	struct mbuf *m0;

	mtx_enter(&mq->mq_mtx);
	m0 = ml_dechain(&mq->mq_list);
	mtx_leave(&mq->mq_mtx);

	return (m0);
}

static unsigned int
mq_purge(struct mbuf_queue *mq)
{
	struct mbuf_list ml;

	mq_delist(mq, &ml);

	return (ml_purge(&ml));
}

static unsigned int
mq_hdatalen(struct mbuf_queue *mq)
{
	unsigned int hdatalen;

	mtx_enter(&mq->mq_mtx);
	hdatalen = ml_hdatalen(&mq->mq_list);
	mtx_leave(&mq->mq_mtx);

	return (hdatalen);
}
