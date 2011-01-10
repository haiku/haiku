/*-
 * Copyright (c) 2002-2003
 * 	Hidetoshi Shimokawa. All rights reserved.
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
 *    must display the following acknowledgement:
 *
 *	This product includes software developed by Hidetoshi Shimokawa.
 *
 * 4. Neither the name of the author nor the names of its contributors
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
 */

#include <sys/param.h>
#include <sys/types.h>
#include <endian.h>

#include <sys/ioccom.h>
#include "queue.h"
#include "fwglue.h"
#include "firewire.h"
#include "iec13213.h"
#include "firewirereg.h"
#include "fwmem.h"


static int fwmem_debug=0;

static struct fw_xfer *
fwmem_xfer_req(
	struct fw_device *fwdev,
	caddr_t sc,
	int spd,
	int slen,
	int rlen,
	void *hand)
{
	struct fw_xfer *xfer;

	xfer = fw_xfer_alloc();
	if (xfer == NULL)
		return NULL;

	xfer->fc = fwdev->fc;
	xfer->send.hdr.mode.hdr.dst = FWLOCALBUS | fwdev->dst;
	if (spd < 0)
		xfer->send.spd = fwdev->speed;
	else
		xfer->send.spd = min_c(spd, fwdev->speed);
	xfer->hand = (void (*)(fw_xfer*))hand;
	xfer->sc = sc;
	xfer->send.pay_len = slen;
	xfer->recv.pay_len = rlen;

	return xfer;
}

struct fw_xfer *
fwmem_read_quad(
	struct fw_device *fwdev,
	caddr_t	sc,
	uint8_t spd,
	uint16_t dst_hi,
	uint32_t dst_lo,
	void *data,
	void (*hand)(struct fw_xfer *))
{
	struct fw_xfer *xfer;
	struct fw_pkt *fp;

	xfer = fwmem_xfer_req(fwdev, (char *)sc, spd, 0, 4, (void*)hand);
	if (xfer == NULL) {
		return NULL;
	}

	fp = &xfer->send.hdr;
	fp->mode.rreqq.tcode = FWTCODE_RREQQ;
	fp->mode.rreqq.dest_hi = dst_hi;
	fp->mode.rreqq.dest_lo = dst_lo;

	xfer->send.payload = NULL;
	xfer->recv.payload = (uint32_t *)data;

	if (fwmem_debug)
		printf("fwmem_read_quad: %d %04x:%08x\n", fwdev->dst,
				dst_hi, dst_lo);

	if (fw_asyreq(xfer->fc, -1, xfer) == 0)
		return xfer;

	fw_xfer_free(xfer);
	return NULL;
}

struct fw_xfer *
fwmem_write_quad(
	struct fw_device *fwdev,
	caddr_t	sc,
	uint8_t spd,
	uint16_t dst_hi,
	uint32_t dst_lo,
	void *data,
	void (*hand)(struct fw_xfer *))
{
	struct fw_xfer *xfer;
	struct fw_pkt *fp;

	xfer = fwmem_xfer_req(fwdev, sc, spd, 0, 0, (void*)hand);
	if (xfer == NULL)
		return NULL;

	fp = &xfer->send.hdr;
	fp->mode.wreqq.tcode = FWTCODE_WREQQ;
	fp->mode.wreqq.dest_hi = dst_hi;
	fp->mode.wreqq.dest_lo = dst_lo;
	fp->mode.wreqq.data = *(uint32_t *)data;

	xfer->send.payload = xfer->recv.payload = NULL;

	if (fwmem_debug)
		printf("fwmem_write_quad: %d %04x:%08x %08x\n", fwdev->dst,
			dst_hi, dst_lo, *(uint32_t *)data);

	if (fw_asyreq(xfer->fc, -1, xfer) == 0)
		return xfer;

	fw_xfer_free(xfer);
	return NULL;
}

struct fw_xfer *
fwmem_read_block(
	struct fw_device *fwdev,
	caddr_t	sc,
	uint8_t spd,
	uint16_t dst_hi,
	uint32_t dst_lo,
	int len,
	void *data,
	void (*hand)(struct fw_xfer *))
{
	struct fw_xfer *xfer;
	struct fw_pkt *fp;

	xfer = fwmem_xfer_req(fwdev, sc, spd, 0, roundup2(len, 4), (void*)hand);
	if (xfer == NULL)
		return NULL;

	fp = &xfer->send.hdr;
	fp->mode.rreqb.tcode = FWTCODE_RREQB;
	fp->mode.rreqb.dest_hi = dst_hi;
	fp->mode.rreqb.dest_lo = dst_lo;
	fp->mode.rreqb.len = len;
	fp->mode.rreqb.extcode = 0;

	xfer->send.payload = NULL;
	xfer->recv.payload = (uint32_t*)data;

	if (fwmem_debug)
		printf("fwmem_read_block: %d %04x:%08x %d\n", fwdev->dst,
				dst_hi, dst_lo, len);
	if (fw_asyreq(xfer->fc, -1, xfer) == 0)
		return xfer;

	fw_xfer_free(xfer);
	return NULL;
}

struct fw_xfer *
fwmem_write_block(
	struct fw_device *fwdev,
	caddr_t	sc,
	uint8_t spd,
	uint16_t dst_hi,
	uint32_t dst_lo,
	int len,
	void *data,
	void (*hand)(struct fw_xfer *))
{
	struct fw_xfer *xfer;
	struct fw_pkt *fp;

	xfer = fwmem_xfer_req(fwdev, sc, spd, len, 0, (void*)hand);
	if (xfer == NULL)
		return NULL;

	fp = &xfer->send.hdr;
	fp->mode.wreqb.tcode = FWTCODE_WREQB;
	fp->mode.wreqb.dest_hi = dst_hi;
	fp->mode.wreqb.dest_lo = dst_lo;
	fp->mode.wreqb.len = len;
	fp->mode.wreqb.extcode = 0;

	xfer->send.payload = (uint32_t*)data;
	xfer->recv.payload = NULL;

	if (fwmem_debug)
		printf("fwmem_write_block: %d %04x:%08x %d\n", fwdev->dst,
				dst_hi, dst_lo, len);
	if (fw_asyreq(xfer->fc, -1, xfer) == 0)
		return xfer;

	fw_xfer_free(xfer);
	return NULL;
}
