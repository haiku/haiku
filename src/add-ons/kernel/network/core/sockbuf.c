/* socket "server" */

#include <stdio.h>
#include <kernel/OS.h>

#ifdef _KERNEL_MODE
#include <KernelExport.h>
#endif

#include "core_private.h"
#include <sys/socket.h>
#include <net_socket.h>
#include <pools.h>
#include <netinet/in_pcb.h>
#include <net_misc.h>
#include <protocols.h>

uint32 sb_max = SB_MAX; /* hard value, recompile needed to alter :( */

/*
 * Allot mbufs to a sockbuf.
 * Attempt to scale mbmax so that mbcnt doesn't become limiting
 * if buffering efficiency is near the normal case.
 */
int sockbuf_reserve(struct sockbuf *sb, uint32 cc)
{
	uint64 dd = (uint64)cc;
	uint64 ee = (sb_max * MCLBYTES) / ((MSIZE) + (MCLBYTES));

        if (cc == 0) 
		return 0;
	if (dd > ee)
		return 0;

        sb->sb_hiwat = cc;
        sb->sb_mbmax = min((cc * 2), sb_max);
        if (sb->sb_lowat > sb->sb_hiwat)
                sb->sb_lowat = sb->sb_hiwat;
        return (1);
}

void sockbuf_drop(struct sockbuf *sb, int len)
{
	struct mbuf *m, *mn;
	struct mbuf *next;

	next = (m = sb->sb_mb) ? m->m_nextpkt : NULL;
	while (len > 0) {
		if (m == NULL) {
			if (next == NULL)
				return;

			m = next;
			next = m->m_nextpkt;
			continue;
		}
		if (m->m_len > len) {
			m->m_len -= len;
			m->m_data += len;
			sb->sb_cc -= len;
			break;
		}
		len -= m->m_len;
		sbfree(sb, m);
		MFREE(m, mn);
		m = mn;
	}
	while (m && m->m_len == 0) {
		sbfree(sb, m);
		MFREE(m, mn);
		m = mn;
	}
	if (m) {
		sb->sb_mb = m;
		m->m_nextpkt = next;
	} else
		sb->sb_mb = next;
}

/*
 * Free all mbufs in a sockbuf.
 * Check that all resources are reclaimed.
 */
void sockbuf_flush(struct sockbuf *sb)
{
	if (sb->sb_flags & SB_LOCK) {
		return;
	}
	while (sb->sb_mbcnt)
		sockbuf_drop(sb, (int)sb->sb_cc);
	if (sb->sb_cc || sb->sb_mb)
		return;
}

/*
 * Free mbufs held by a socket, and reserved mbuf space.
 */
void sockbuf_release(struct sockbuf *sb)
{
        sockbuf_flush(sb);
        sb->sb_hiwat = sb->sb_mbmax = 0;
}

void sockbuf_append(struct sockbuf *sb, struct mbuf *m)
{
        struct mbuf *n;

        if (!m)
                return;
        if ((n = sb->sb_mb) != NULL) {
                while (n->m_nextpkt)
                        n = n->m_nextpkt;
                do {
                        if (n->m_flags & M_EOR) {
                                sockbuf_appendrecord(sb, m); /* XXXXXX!!!! */
                                return;
                        }
                } while (n->m_next && (n = n->m_next));
        }
        sockbuf_compress(sb, m, n);
}

void sockbuf_appendrecord(struct sockbuf *sb, struct mbuf *m0)
{
        struct mbuf *m;

        if (!m0)
                return;
        if ((m = sb->sb_mb) != NULL)
                while (m->m_nextpkt)
                        m = m->m_nextpkt;
        /*
         * Put the first mbuf on the queue.
         * Note this permits zero length records.
         */
        sballoc(sb, m0);
        if (m)
                m->m_nextpkt = m0;
        else
                sb->sb_mb = m0;
        m = m0->m_next;
        m0->m_next = 0;
        if (m && (m0->m_flags & M_EOR)) {
                m0->m_flags &= ~M_EOR;
                m->m_flags |= M_EOR;
        }
        sockbuf_compress(sb, m, m0);
}

int sockbuf_appendaddr(struct sockbuf *sb, struct sockaddr *asa, 
		 struct mbuf *m0, struct mbuf *control)
{
	struct mbuf *m, *n;
	int space = asa->sa_len;

	if (m0 && (m0->m_flags & M_PKTHDR) == 0)
		return(-1);

	if (m0)
		space += m0->m_pkthdr.len;
	for (n = control; n; n = n->m_next) {
		space += n->m_len;
		if (n->m_next == 0)     /* keep pointer to last control buf */
			break;
	}
	if (space > sbspace(sb))
		return (0);
	if (asa->sa_len > MLEN)
		return (0);
	MGET(m, MT_SONAME);
	if (m == NULL)
		return (0);
	m->m_len = asa->sa_len;
	memcpy(mtod(m, caddr_t), (caddr_t)asa, asa->sa_len);
	if (n)
		n->m_next = m0;         /* concatenate data to control */
	else
		control = m0;
	m->m_next = control;
	for (n = m; n; n = n->m_next)
		sballoc(sb, n);
	if ((n = sb->sb_mb) != NULL) {
		while (n->m_nextpkt)
			n = n->m_nextpkt;
		n->m_nextpkt = m;
	} else
		sb->sb_mb = m;

	return (1);
}

void sockbuf_compress(struct sockbuf *sb, struct mbuf *m, struct mbuf *n)
{
        int eor = 0;
        struct mbuf *o;

        while (m) {
                eor |= m->m_flags & M_EOR;
                if (m->m_len == 0 &&
                    (eor == 0 ||
                    (((o = m->m_next) || (o = n)) &&
                    o->m_type == m->m_type))) {
                        m = m_free(m);
                        continue;
                }
                if (n && (n->m_flags & (M_EXT | M_EOR)) == 0 &&
                    (n->m_data + n->m_len + m->m_len) < &n->m_dat[MLEN] &&
                    n->m_type == m->m_type) {
                        memcpy(mtod(n, caddr_t) + n->m_len, mtod(m, caddr_t),
                            (unsigned)m->m_len);
                        n->m_len += m->m_len;
                        sb->sb_cc += m->m_len;
                        m = m_free(m);
                        continue;
                }
                if (n)
                        n->m_next = m;
                else
                        sb->sb_mb = m;
                sballoc(sb, m);
                n = m;
                m->m_flags &= ~M_EOR;
                m = m->m_next;
                n->m_next = 0;
        }
        if (eor) {
                if (n)
                        n->m_flags |= eor;
                else
                        printf("semi-panic: sockbuf_compress\n");
        }
}

void sockbuf_droprecord(struct sockbuf *sb)
{
        struct mbuf *m, *mn;

        m = sb->sb_mb;
        if (m) {
                sb->sb_mb = m->m_nextpkt;
                do {
                        sbfree(sb, m);
                        MFREE(m, mn);
                } while ((m = mn) != NULL);
        }
}

int sockbuf_wait(struct sockbuf *sb)
{
	if (sb->sb_cc > 0)
		return 0;
	sb->sb_flags |= SB_WAIT;
	return nsleep(sb->sb_pop, "sockbuf_wait", sb->sb_timeo);
}

void sockbuf_insertoob(struct sockbuf *sb, struct mbuf *m0)
{
	struct mbuf *m;
	struct mbuf **mp;

	if (m0 == NULL)
		return;
	for (mp = &sb->sb_mb; (m = *mp) != NULL; mp = &((*mp)->m_nextpkt)) {
again:
		switch (m->m_type) {
			case MT_OOBDATA:
				continue;  /* WANT next train */
			case MT_CONTROL:
				if ((m = m->m_next) != NULL)
					goto again; /* inspect THIS 
					             * train further */
		}
		break;
	}
	/*
	 * Put the first mbuf on the queue.
	 * Note this permits zero length records.
	 */
	sballoc(sb, m0);
	m0->m_nextpkt = *mp;
	*mp = m0;
	m = m0->m_next;
	m0->m_next = 0;
	if (m && (m0->m_flags & M_EOR)) {
		m0->m_flags &= ~M_EOR;
		m->m_flags |= M_EOR;
	}
	sockbuf_compress(sb, m, m0);
}

/*
 * Lock a sockbuf already known to be locked;
 * return any error returned from sleep (EINTR).
 */
int sockbuf_lock(struct sockbuf *sb) // XXX never called at all
{
	int error;

	while (sb->sb_flags & SB_LOCK) {
		sb->sb_flags |= SB_WANT;
		error = nsleep(sb->sb_sleep, "sockbuf_lock", 0);
		if (error)
			return (error);
	}
	sb->sb_flags |= SB_LOCK;
	return (0);
}

