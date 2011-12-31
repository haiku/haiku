/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		JiSheng Zhang
 *		Jérôme DUVAL
 */
/*-
 * Copyright (c) 2003 Hidetoshi Shimokawa
 * Copyright (c) 1998-2002 Katsushi Kobayashi and Hidetoshi Shimokawa
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the acknowledgement as bellow:
 *
 *    This product includes software developed by K. Kobayashi and H. Shimokawa
 *
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 * $FreeBSD: src/sys/dev/firewire/fwdev.c,v 1.52 2007/06/06 14:31:36 simokawa Exp $
 *
 */

#include <sys/param.h>
#include <sys/types.h>

#include <Drivers.h>
#include <OS.h>
#include <KernelExport.h>
#include <malloc.h>
#include <module.h>
#include <stdio.h>
#include <ByteOrder.h>
#include <string.h>
#include <sys/ioccom.h>

#include "queue.h"
#include "firewire.h"
#include "iec13213.h"
#include "firewirereg.h"
#include "fwdma.h"
//#include "fwmem.h"
#include "iec68113.h"

#define TRACE(x...)
//#define TRACE(x...) dprintf(x)

#include "firewire_module.h"

#define	FWNODE_INVAL 0xffff

int32 api_version = B_CUR_DRIVER_API_VERSION;
struct fw_module_info *gFirewire;
const char fwname[] = "bus/fw/%ld";
#define FWNAMEMAX 9
static char ** devices;
uint32 devices_count = 0;


struct fw_drv1 {
	struct firewire_comm *fc;
	struct fw_xferq *ir;
	struct fw_xferq *it;
	struct fw_isobufreq bufreq;
	STAILQ_HEAD(, fw_bind) binds;
	STAILQ_HEAD(, fw_xfer) rq;
	sem_id rqSem;
};

static int
fwdev_allocbuf(struct fw_xferq *q,
	struct fw_bufspec *b)
{
	int i;

	if (q->flag & (FWXFERQ_RUNNING | FWXFERQ_EXTBUF))
		return(EBUSY);

	q->bulkxfer = (struct fw_bulkxfer *) malloc(
		sizeof(struct fw_bulkxfer) * b->nchunk);
	if (q->bulkxfer == NULL)
		return(ENOMEM);

	b->psize = roundup2(b->psize, sizeof(uint32_t));
	q->buf = gFirewire->fwdma_malloc_multiseg(sizeof(uint32_t), 
			b->psize, b->nchunk * b->npacket);
TRACE("fwdev_allocbuf %d\n", b->psize*b->nchunk*b->npacket);
	if (q->buf == NULL) {
		free(q->bulkxfer);
		q->bulkxfer = NULL;
		return(ENOMEM);
	}
	q->bnchunk = b->nchunk;
	q->bnpacket = b->npacket;
	q->psize = (b->psize + 3) & ~3;
	q->queued = 0;

	STAILQ_INIT(&q->stvalid);
	STAILQ_INIT(&q->stfree);
	STAILQ_INIT(&q->stdma);
	q->stproc = NULL;

	for (i = 0 ; i < q->bnchunk; i++) {
		q->bulkxfer[i].poffset = i * q->bnpacket;
		//q->bulkxfer[i].mbuf = NULL; //to be completed when find mbuf's counterpart
		STAILQ_INSERT_TAIL(&q->stfree, &q->bulkxfer[i], link);
	}

	q->flag &= ~FWXFERQ_MODEMASK;
	q->flag |= FWXFERQ_STREAM;
	q->flag |= FWXFERQ_EXTBUF;

	return (0);
}

static int
fwdev_freebuf(struct fw_xferq *q)
{
	if (q->flag & FWXFERQ_EXTBUF) {
		if (q->buf != NULL)
			gFirewire->fwdma_free_multiseg(q->buf);
		q->buf = NULL;
		free(q->bulkxfer);
		q->bulkxfer = NULL;
		q->flag &= ~FWXFERQ_EXTBUF;
		q->psize = 0;
		q->maxq = FWMAXQUEUE;
	}
	return (0);
}


static status_t
fw_open (const char *name, uint32 flags, void **cookie)
{
	int32 count = -1;
	int32 i;
	struct fw_drv1 *d;
	struct firewire_softc *sc = NULL;
	
	*cookie = NULL;
	for (i = 0; i < devices_count; i++) {
		if (strcmp(name, devices[i]) == 0) {
			count = i;
			break;
		}
	}
	
	if (count < 0) {
		return B_BAD_VALUE;
	}
	
	if (gFirewire->get_handle(count, &sc) != B_OK) {
		return ENODEV;
	}
	
	if (sc == NULL)
		return (ENXIO);

	FW_GLOCK(sc->fc);
	if (*cookie != NULL) {
		FW_GUNLOCK(sc->fc);
		return (EBUSY);
	}
	/* set dummy value for allocation */
	*cookie = (void *)-1;
	FW_GUNLOCK(sc->fc);

	*cookie = malloc(sizeof(struct fw_drv1));
	if (*cookie == NULL)
		return (ENOMEM);
	memset(*cookie, 0, sizeof(struct fw_drv1));

	d = (struct fw_drv1 *)(*cookie);
	d->fc = sc->fc;
	STAILQ_INIT(&d->binds);
	STAILQ_INIT(&d->rq);
	d->rqSem = create_sem(0, "fw_raw rq");
	if (d->rqSem < B_OK) {
		dprintf("fwraw: failed creating rq semaphore\n");
		free(*cookie);
		return B_ERROR;
	}

	return B_OK;	
}


static status_t 
fw_close (void *cookie)
{
	return B_OK;
}


static status_t
fw_free(void *cookie)
{
	struct firewire_comm *fc;
	struct fw_drv1 *d;
	struct fw_xfer *xfer;
	struct fw_bind *fwb;

	TRACE("fw_free\n");
	d = (struct fw_drv1 *)cookie;
	delete_sem(d->rqSem);
	fc = d->fc;

	/* remove binding */
	for (fwb = STAILQ_FIRST(&d->binds); fwb != NULL;
			fwb = STAILQ_FIRST(&d->binds)) {
		gFirewire->fw_bindremove(fc, fwb);
		STAILQ_REMOVE_HEAD(&d->binds, chlist);
		gFirewire->fw_xferlist_remove(&fwb->xferlist);
		free(fwb);
	}
	if (d->ir != NULL) {
		struct fw_xferq *ir = d->ir;

		if ((ir->flag & FWXFERQ_OPEN) == 0)
			return (EINVAL);
		if (ir->flag & FWXFERQ_RUNNING) {
			ir->flag &= ~FWXFERQ_RUNNING;
			fc->irx_disable(fc, ir->dmach);
		}
		/* free extbuf */
		fwdev_freebuf(ir);
		/* drain receiving buffer */
		for (xfer = STAILQ_FIRST(&ir->q);
			xfer != NULL; xfer = STAILQ_FIRST(&ir->q)) {
			ir->queued --;
			STAILQ_REMOVE_HEAD(&ir->q, link);

			xfer->resp = 0;
			gFirewire->fw_xfer_done(xfer);
		}
		ir->flag &= ~(FWXFERQ_OPEN |
			FWXFERQ_MODEMASK | FWXFERQ_CHTAGMASK);
		d->ir = NULL;

	}
	if (d->it != NULL) {
		struct fw_xferq *it = d->it;

		if ((it->flag & FWXFERQ_OPEN) == 0)
			return (EINVAL);
		if (it->flag & FWXFERQ_RUNNING) {
			it->flag &= ~FWXFERQ_RUNNING;
			fc->itx_disable(fc, it->dmach);
		}
		/* free extbuf */
		fwdev_freebuf(it);
		it->flag &= ~(FWXFERQ_OPEN | FWXFERQ_MODEMASK | FWXFERQ_CHTAGMASK);
		d->it = NULL;
	}
	free(d);
	d = NULL;
	
	return B_OK;
}


static int
fw_read_async(struct fw_drv1 *d, off_t position, void *buf, size_t *num_bytes)
{
	int err = 0;
	struct fw_xfer *xfer;
	struct fw_bind *fwb;
	struct fw_pkt *fp;
	struct tcode_info *tinfo;
	cpu_status s;
	
	int len, pbytes = (int)*num_bytes;//pbytes:to be processed bytes
	*num_bytes = 0;

	FW_GLOCK(d->fc);
	while ((xfer = STAILQ_FIRST(&d->rq)) == NULL && err == B_OK) {
		//err = msleep(&d->rq, FW_GMTX(d->fc), FWPRI, "fwra", 0);
		FW_GUNLOCK(d->fc);
		err = acquire_sem_etc(d->rqSem, 1, B_CAN_INTERRUPT | B_TIMEOUT,
			B_INFINITE_TIMEOUT);//B_INFINITE_TIMEOUT means no timeout?
		FW_GLOCK(d->fc);
	}

//	if (err != 0) {
	if (err != B_OK) {

		FW_GUNLOCK(d->fc);
		return (err);
	}

	s = splfw();
	STAILQ_REMOVE_HEAD(&d->rq, link);
	FW_GUNLOCK(xfer->fc);
	splx(s);
	fp = &xfer->recv.hdr;
#if 0 /* for GASP ?? */
	if (fc->irx_post != NULL)
		fc->irx_post(fc, fp->mode.ld);
#endif
	tinfo = &xfer->fc->tcode[fp->mode.hdr.tcode];
//	err = uiomove((void *)fp, tinfo->hdr_len, uio);
	len = tinfo->hdr_len;
	len = MIN(len, pbytes);
	err = user_memcpy(buf, fp, len);
	pbytes -= len;
	*num_bytes += len;
	buf = (void *)((caddr_t)buf + len);

//	if (err)
	if (err < B_OK) {
		goto out;
	}
	
//	err = uiomove((void *)xfer->recv.payload, xfer->recv.pay_len, uio);
	len = xfer->recv.pay_len;
	len = MIN(len, pbytes);	
	err = user_memcpy(buf, (void *)xfer->recv.payload, len);
	pbytes -= len;
	*num_bytes += len;//get have been processed bytes num
	buf = (void *)((caddr_t)buf + len);

out:
	/* recycle this xfer */
	fwb = (struct fw_bind *)xfer->sc;
	gFirewire->fw_xfer_unload(xfer);
	xfer->recv.pay_len = B_PAGE_SIZE;
	FW_GLOCK(xfer->fc);
	STAILQ_INSERT_TAIL(&fwb->xferlist, xfer, link);
	FW_GUNLOCK(xfer->fc);
	return (err);
}


/*
 * read request.
 */
static status_t 
fw_read (void *cookie, off_t position, void *buf, size_t *num_bytes)
{
	struct fw_drv1 *d;
	struct fw_xferq *ir;
	struct firewire_comm *fc;
	int err = 0, slept = 0;
	struct fw_pkt *fp;
	cpu_status s;
	
	int len, pbytes = (int)*num_bytes;
	*num_bytes = 0;

	d = (struct fw_drv1 *)cookie;
	fc = d->fc;
	ir = d->ir;

	if (ir == NULL)
		return (fw_read_async(d, position, buf, num_bytes));

	if (ir->buf == NULL)
		return (EIO);

	FW_GLOCK(fc);
readloop:
	if (ir->stproc == NULL) {
		/* iso bulkxfer */
		ir->stproc = STAILQ_FIRST(&ir->stvalid);
		if (ir->stproc != NULL) {
			s = splfw();
			STAILQ_REMOVE_HEAD(&ir->stvalid, link);
			splx(s);
			ir->queued = 0;
		}
	}
	if (ir->stproc == NULL) {
		/* no data avaliable */
		if (slept == 0) {
			slept = 1;
			ir->flag |= FWXFERQ_WAKEUP;
			//err = msleep(ir, FW_GMTX(fc), FWPRI, "fw_read", hz);
			FW_GUNLOCK(fc);
			err = acquire_sem_etc(ir->Sem, 1, B_CAN_INTERRUPT | B_TIMEOUT, 1000000);
			FW_GLOCK(fc);

			ir->flag &= ~FWXFERQ_WAKEUP;
			if (err == B_OK)
				goto readloop;
		} else if (slept == 1)
			err = EIO;
		FW_GUNLOCK(fc);
		return err;
	} else if (ir->stproc != NULL) {
		/* iso bulkxfer */
		FW_GUNLOCK(fc);
		fp = (struct fw_pkt *)fwdma_v_addr(ir->buf, 
				ir->stproc->poffset + ir->queued);
		if (fc->irx_post != NULL)
			fc->irx_post(fc, fp->mode.ld);
		if (fp->mode.stream.len == 0) {
			err = EIO;
			return err;
		}
	//	err = uiomove((caddr_t)fp,
	//		fp->mode.stream.len + sizeof(uint32_t), uio);
		len = fp->mode.stream.len + sizeof(uint32_t);
		len = MIN(len, pbytes);
		err = user_memcpy(buf, (void *)fp, len);
		pbytes -= len;
		*num_bytes += len;
		buf = (void *)((caddr_t)buf + len);

		ir->queued ++;
		if (ir->queued >= ir->bnpacket) {
			s = splfw();
			STAILQ_INSERT_TAIL(&ir->stfree, ir->stproc, link);
			splx(s);
			fc->irx_enable(fc, ir->dmach);
			ir->stproc = NULL;
		}
		if (pbytes >= ir->psize) {
			slept = -1;
			FW_GLOCK(fc);
			goto readloop;
		}
	}
	return err;
}


static int
fw_write_async(struct fw_drv1 *d, off_t position, const void *buf,
	size_t *num_bytes)
{
	struct fw_xfer *xfer;
	struct fw_pkt pkt;
	struct tcode_info *tinfo;
	int err;
	
	int len, pbytes = (int)*num_bytes;
	*num_bytes = 0;

	bzero(&pkt, sizeof(struct fw_pkt));
//	if ((err = uiomove((caddr_t)&pkt, sizeof(uint32_t), uio)))
	len = sizeof(uint32_t);
	len = MIN(len, pbytes);
	err = user_memcpy(&pkt, buf, len);
	pbytes -= len;
	*num_bytes += len;
	buf = (void *)((caddr_t)buf + len);
	if (err < B_OK)
		return (err);

	tinfo = &d->fc->tcode[pkt.mode.hdr.tcode];
//	if ((err = uiomove((caddr_t)&pkt + sizeof(uint32_t),
//	    tinfo->hdr_len - sizeof(uint32_t), uio)))
	len = tinfo->hdr_len - sizeof(uint32_t);
	len = MIN(len, pbytes);
	err = user_memcpy((void*)((caddr_t)&pkt + sizeof(uint32_t)), 
			buf, len);
	pbytes -= len;
	*num_bytes += len;
	buf = (void *)((caddr_t)buf + len);

	if (err < B_OK)
		return (err);

	if ((xfer = gFirewire->fw_xfer_alloc_buf(pbytes,
	    B_PAGE_SIZE/*XXX*/)) == NULL)
		return (ENOMEM);

	bcopy(&pkt, &xfer->send.hdr, sizeof(struct fw_pkt));
//	xfer->send.pay_len = uio->uio_resid;
	xfer->send.pay_len = pbytes;

/*	if (uio->uio_resid > 0) {
		if ((err = uiomove((caddr_t)&xfer->send.payload[0],
		    uio->uio_resid, uio)));// here the FreeBSD's stack author may be careless?
			goto out;
	}*/
	if (pbytes > 0) {
		len = pbytes;
		err = user_memcpy((void*)&xfer->send.payload[0], 
				buf, len);
		pbytes -= len;
		*num_bytes += len;
		buf = (void *)((caddr_t)buf + len);
		if (err < B_OK)
			goto out;
	}

	xfer->fc = d->fc;
	xfer->sc = NULL;
	xfer->hand = gFirewire->fw_xferwake;
	xfer->send.spd = 2 /* XXX */;

	if ((err = gFirewire->fw_asyreq(xfer->fc, -1, xfer)))
		goto out;

	if ((err = gFirewire->fw_xferwait(xfer)))
		goto out;

	if (xfer->resp != 0) {
		err = xfer->resp;
		goto out;
	}

	if (xfer->flag & FWXF_RCVD) {
		FW_GLOCK(xfer->fc);
		STAILQ_INSERT_TAIL(&d->rq, xfer, link);
		FW_GUNLOCK(xfer->fc);
		return B_OK;
	}

out:
	gFirewire->fw_xfer_free(xfer);
	return (err);
}


static status_t
fw_write (void *cookie, off_t position, const void *buf, size_t *num_bytes)
{
	int err = 0;
	int slept = 0;
	struct fw_drv1 *d;
	struct fw_pkt *fp;
	struct firewire_comm *fc;
	struct fw_xferq *it;
	cpu_status s;

	int len, pbytes = (int)*num_bytes;
	*num_bytes = 0;

	d = (struct fw_drv1 *)cookie;
	fc = d->fc;
	it = d->it;

	if (it == NULL)
		return (fw_write_async(d, position, buf, num_bytes));

	if (it->buf == NULL)
		return (EIO);

	FW_GLOCK(fc);
isoloop:
	if (it->stproc == NULL) {
		it->stproc = STAILQ_FIRST(&it->stfree);
		if (it->stproc != NULL) {
			s = splfw();
			STAILQ_REMOVE_HEAD(&it->stfree, link);
			splx(s);
			it->queued = 0;
		} else if (slept == 0) {
			slept = 1;
#if 0	/* XXX to avoid lock recursion */
			err = fc->itx_enable(fc, it->dmach);
			if (err)
				goto out;
#endif
//			err = msleep(it, FW_GMTX(fc), FWPRI, "fw_write", hz);
			FW_GUNLOCK(fc);
			err = acquire_sem_etc(it->Sem, 1, B_CAN_INTERRUPT | B_TIMEOUT, 1000000);
			FW_GLOCK(fc);

//			if (err)
			if (err < B_OK)
				goto out;
			goto isoloop;
		} else {
			err = EIO;
			goto out;
		}
	}
	FW_GUNLOCK(fc);
	fp = (struct fw_pkt *)fwdma_v_addr(it->buf,
			it->stproc->poffset + it->queued);
//	err = uiomove((caddr_t)fp, sizeof(struct fw_isohdr), uio);
	len = sizeof(struct fw_isohdr);
	len = MIN(len, pbytes);
	err = user_memcpy(fp, buf, len);
	pbytes -= len;
	*num_bytes += len;
	buf = (void *)((caddr_t)buf + len);

//	err = uiomove((caddr_t)fp->mode.stream.payload,
//				fp->mode.stream.len, uio);
	len = fp->mode.stream.len;
	len = MIN(len, pbytes);
	err = user_memcpy(fp->mode.stream.payload, buf, len);
	pbytes -= len;
	*num_bytes += len;
	buf = (void *)((caddr_t)buf + len);

	it->queued ++;
	if (it->queued >= it->bnpacket) {
		s = splfw();
		STAILQ_INSERT_TAIL(&it->stvalid, it->stproc, link);
		splx(s);
		it->stproc = NULL;
		err = fc->itx_enable(fc, it->dmach);
	}
//	if (uio->uio_resid >= sizeof(struct fw_isohdr)) {
	if (pbytes >= sizeof(struct fw_isohdr)) {
		slept = 0;
		FW_GLOCK(fc);
		goto isoloop;
	}
	return err;

out:
	FW_GUNLOCK(fc);
	return err;
}


static void
fw_hand(struct fw_xfer *xfer)
{
	struct fw_bind *fwb;
	struct fw_drv1 *d;

	fwb = (struct fw_bind *)xfer->sc;
	d = (struct fw_drv1 *)fwb->sc;
	FW_GLOCK(xfer->fc);
	STAILQ_INSERT_TAIL(&d->rq, xfer, link);
	FW_GUNLOCK(xfer->fc);
//	wakeup(&d->rq);
	release_sem_etc(d->rqSem, 1, B_DO_NOT_RESCHEDULE);
}


/*
 * ioctl support.
 */
static status_t
fw_ioctl (void *cookie, uint32 cmd, void *data, size_t length)
{
	struct firewire_comm *fc;
	struct fw_drv1 *d;
	int i, len, err = 0;
	struct fw_device *fwdev;
	struct fw_bind *fwb;
	struct fw_xferq *ir, *it;
	struct fw_xfer *xfer;
	struct fw_pkt *fp;
	struct fw_devinfo *devinfo;
	void *ptr;

	struct fw_devlstreq *fwdevlst = (struct fw_devlstreq *)data;
	struct fw_asyreq *asyreq = (struct fw_asyreq *)data;
	struct fw_isochreq *ichreq = (struct fw_isochreq *)data;
	struct fw_isobufreq *ibufreq = (struct fw_isobufreq *)data;
	struct fw_asybindreq *bindreq = (struct fw_asybindreq *)data;
	struct fw_crom_buf *crom_buf = (struct fw_crom_buf *)data;

	if (!data)
		return(EINVAL);

	d = (struct fw_drv1 *)cookie;
	fc = d->fc;
	ir = d->ir;
	it = d->it;

	switch (cmd) {
	case FW_STSTREAM:
		if (it == NULL) {
			i = gFirewire->fw_open_isodma(fc, /* tx */1);
			if (i < 0) {
				err = EBUSY;
				break;
			}
			it = fc->it[i];
			err = fwdev_allocbuf(it, &d->bufreq.tx);
			if (err) {
				it->flag &= ~FWXFERQ_OPEN;
				break;
			}
		}
		it->flag &= ~0xff;
		it->flag |= (0x3f & ichreq->ch);
		it->flag |= ((0x3 & ichreq->tag) << 6);
		d->it = it;
		break;
	case FW_GTSTREAM:
		if (it != NULL) {
			ichreq->ch = it->flag & 0x3f;
			ichreq->tag = it->flag >> 2 & 0x3;
		} else
			err = EINVAL;
		break;
	case FW_SRSTREAM:
		if (ir == NULL) {
			i = gFirewire->fw_open_isodma(fc, /* tx */0);
			if (i < 0) {
				err = EBUSY;
				break;
			}
			ir = fc->ir[i];
			err = fwdev_allocbuf(ir, &d->bufreq.rx);
			if (err) {
				ir->flag &= ~FWXFERQ_OPEN;
				break;
			}
		}
		ir->flag &= ~0xff;
		ir->flag |= (0x3f & ichreq->ch);
		ir->flag |= ((0x3 & ichreq->tag) << 6);
		d->ir = ir;
		err = fc->irx_enable(fc, ir->dmach);
		break;
	case FW_GRSTREAM:
		if (d->ir != NULL) {
			ichreq->ch = ir->flag & 0x3f;
			ichreq->tag = ir->flag >> 2 & 0x3;
		} else
			err = EINVAL;
		break;
	case FW_SSTBUF:
		bcopy(ibufreq, &d->bufreq, sizeof(d->bufreq));
		break;
	case FW_GSTBUF:
		bzero(&ibufreq->rx, sizeof(ibufreq->rx));
		if (ir != NULL) {
			ibufreq->rx.nchunk = ir->bnchunk;
			ibufreq->rx.npacket = ir->bnpacket;
			ibufreq->rx.psize = ir->psize;
		}
		bzero(&ibufreq->tx, sizeof(ibufreq->tx));
		if (it != NULL) {
			ibufreq->tx.nchunk = it->bnchunk;
			ibufreq->tx.npacket = it->bnpacket;
			ibufreq->tx.psize = it->psize;
		}
		break;
	case FW_ASYREQ:
	{
		struct tcode_info *tinfo;
		int pay_len = 0;

		fp = &asyreq->pkt;
		tinfo = &fc->tcode[fp->mode.hdr.tcode];

		if ((tinfo->flag & FWTI_BLOCK_ASY) != 0)
			pay_len = MAX(0, asyreq->req.len - tinfo->hdr_len);

		xfer = gFirewire->fw_xfer_alloc_buf(pay_len, B_PAGE_SIZE/*XXX*/);
		if (xfer == NULL)
			return (ENOMEM);

		switch (asyreq->req.type) {
		case FWASREQNODE:
			break;
		case FWASREQEUI:
			fwdev = gFirewire->fw_noderesolve_eui64(fc,
						&asyreq->req.dst.eui);
			if (fwdev == NULL) {
				dprintf("FWASREQEUI cannot find node\n");
				err = EINVAL;
				goto out;
			}
			fp->mode.hdr.dst = FWLOCALBUS | fwdev->dst;
			break;
		case FWASRESTL:
			/* XXX what's this? */
			break;
		case FWASREQSTREAM:
			/* nothing to do */
			break;
		}

		bcopy(fp, (void *)&xfer->send.hdr, tinfo->hdr_len);
		if (pay_len > 0)
			bcopy((char *)fp + tinfo->hdr_len,
			    (void *)xfer->send.payload, pay_len);
		xfer->send.spd = asyreq->req.sped;
		xfer->hand = gFirewire->fw_xferwake;

		if ((err = gFirewire->fw_asyreq(fc, -1, xfer)) != 0)
			goto out;
		if ((err = gFirewire->fw_xferwait(xfer)) != 0)
			goto out;
		if (xfer->resp != 0) {
			err = EIO;
			goto out;
		}
		if ((tinfo->flag & FWTI_TLABEL) == 0)
			goto out;

		/* copy response */
		tinfo = &fc->tcode[xfer->recv.hdr.mode.hdr.tcode];
		if (xfer->recv.hdr.mode.hdr.tcode == FWTCODE_RRESB ||
		    xfer->recv.hdr.mode.hdr.tcode == FWTCODE_LRES) {
			pay_len = xfer->recv.pay_len;
			if (asyreq->req.len >= xfer->recv.pay_len + tinfo->hdr_len) {
				asyreq->req.len = xfer->recv.pay_len +
					tinfo->hdr_len;
			} else {
				err = EINVAL;
				pay_len = 0;
			}
		} else {
			pay_len = 0;
		}
		bcopy(&xfer->recv.hdr, fp, tinfo->hdr_len);
		bcopy(xfer->recv.payload, (char *)fp + tinfo->hdr_len, pay_len);
out:
		gFirewire->fw_xfer_free_buf(xfer);
		break;
	}
	case FW_IBUSRST:
		fc->ibr(fc);
		break;
	case FW_CBINDADDR:
		fwb = gFirewire->fw_bindlookup(fc,
				bindreq->start.hi, bindreq->start.lo);
		if (fwb == NULL) {
			err = EINVAL;
			break;
		}
		gFirewire->fw_bindremove(fc, fwb);
		STAILQ_REMOVE(&d->binds, fwb, fw_bind, chlist);
		gFirewire->fw_xferlist_remove(&fwb->xferlist);
		free(fwb);
		break;
	case FW_SBINDADDR:
		if (bindreq->len <= 0) {
			err = EINVAL;
			break;
		}
		if (bindreq->start.hi > 0xffff) {
			err = EINVAL;
			break;
		}
		fwb = (struct fw_bind *)malloc(sizeof (struct fw_bind));
		if (fwb == NULL) {
			err = ENOMEM;
			break;
		}
		fwb->start = ((u_int64_t)bindreq->start.hi << 32) |
		    bindreq->start.lo;
		fwb->end = fwb->start + bindreq->len;
		fwb->sc = (void *)d;
		STAILQ_INIT(&fwb->xferlist);
		err = gFirewire->fw_bindadd(fc, fwb);
		if (err == 0) {
			gFirewire->fw_xferlist_add(&fwb->xferlist,
			    /* XXX */
			    B_PAGE_SIZE, B_PAGE_SIZE, 5,
			    fc, (void *)fwb, fw_hand);
			STAILQ_INSERT_TAIL(&d->binds, fwb, chlist);
		}
		break;
	case FW_GDEVLST:
		i = len = 1;
		/* myself */
		devinfo = &fwdevlst->dev[0];
		devinfo->dst = fc->nodeid;
		devinfo->status = 0;	/* XXX */
		devinfo->eui.hi = fc->eui.hi;
		devinfo->eui.lo = fc->eui.lo;
		STAILQ_FOREACH(fwdev, &fc->devices, link) {
			if (len < FW_MAX_DEVLST) {
				devinfo = &fwdevlst->dev[len++];
				devinfo->dst = fwdev->dst;
				devinfo->status = 
					(fwdev->status == FWDEVINVAL) ? 0 : 1;
				devinfo->eui.hi = fwdev->eui.hi;
				devinfo->eui.lo = fwdev->eui.lo;
			}
			i++;
		}
		fwdevlst->n = i;
		fwdevlst->info_len = len;
		break;
	case FW_GTPMAP:
		bcopy(fc->topology_map, data,
				(fc->topology_map->crc_len + 1) * 4);
		break;
	case FW_GCROM:
		STAILQ_FOREACH(fwdev, &fc->devices, link)
			if (FW_EUI64_EQUAL(fwdev->eui, crom_buf->eui))
				break;
		if (fwdev == NULL) {
			if (!FW_EUI64_EQUAL(fc->eui, crom_buf->eui)) {
				err = FWNODE_INVAL;
				break;
			}
			/* myself */
			ptr = malloc(CROMSIZE);
			len = CROMSIZE;
			for (i = 0; i < CROMSIZE / 4; i++)
				((uint32_t *)ptr)[i]
					= B_BENDIAN_TO_HOST_INT32(fc->config_rom[i]);
		} else {
			/* found */
			ptr = (void *)&fwdev->csrrom[0];
			if (fwdev->rommax < CSRROMOFF)
				len = 0;
			else
				len = fwdev->rommax - CSRROMOFF + 4;
		}
		if (crom_buf->len < len)
			len = crom_buf->len;
		else
			crom_buf->len = len;
//		err = copyout(ptr, crom_buf->ptr, len);//to be completed
		err = user_memcpy(crom_buf->ptr, ptr, len);
		if (fwdev == NULL)
			/* myself */
			free(ptr);
		break;
	default:
		fc->ioctl (fc, cmd, data, length);
		break;
	}
	return err;
}


status_t
init_hardware()
{
	return B_OK;
}


const char **
publish_devices(void)
{
	return (const char **)devices;
}


static device_hooks hooks = {
	&fw_open,
	&fw_close,
	&fw_free,
	&fw_ioctl,
	&fw_read,
	&fw_write,
	NULL,
	NULL,
	NULL,
	NULL
};

device_hooks *
find_device(const char *name)
{
	return &hooks;
}


status_t
init_driver()
{
	status_t err;
	struct firewire_softc *sc = NULL;
	uint32 i;
	
#if DEBUG && !defined(__HAIKU__)
	load_driver_symbols("fw_raw");
#endif

	if ((err = get_module(FIREWIRE_MODULE_NAME, (module_info **)&gFirewire)) != B_OK) {
		dprintf("fw_raw: couldn't load "FIREWIRE_MODULE_NAME"\n");
		return err;
	}

	devices_count = 0;
	while (gFirewire->get_handle(devices_count, &sc) == B_OK) {
		devices_count++;
	}

	if (devices_count <= 0) {
		put_module(FIREWIRE_MODULE_NAME);
		return ENODEV;
	}
	
	devices = malloc(sizeof(char *) * (devices_count + 1));
	if (!devices) {
		put_module(FIREWIRE_MODULE_NAME);
		return ENOMEM;
	}
	for (i = 0; i < devices_count; i++) {
		devices[i] = strdup(fwname);
		snprintf(devices[i], FWNAMEMAX, fwname, i);
	}
	devices[devices_count] = NULL;
	
	TRACE("fw_raw init_driver returns OK\n");
	return B_OK;
}


void
uninit_driver()
{
	int32 i = 0;
	for (i = 0; i < devices_count; i++) {
		free(devices[i]);
	}
	free(devices);
	devices = NULL;

	put_module(FIREWIRE_MODULE_NAME);
}
