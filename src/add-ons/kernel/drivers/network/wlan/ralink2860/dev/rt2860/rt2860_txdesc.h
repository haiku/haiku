
/*-
 * Copyright (c) 2009-2010 Alexander Egorenkov <egorenar@gmail.com>
 * Copyright (c) 2009 Damien Bergamini <damien.bergamini@free.fr>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _RT2860_TXDESC_H_
#define _RT2860_TXDESC_H_

#define RT2860_TXDESC_SDL1_BURST						(1 << 15)
#define RT2860_TXDESC_SDL1_LASTSEG						(1 << 14)

#define RT2860_TXDESC_SDL0_DDONE						(1 << 15)
#define RT2860_TXDESC_SDL0_LASTSEG						(1 << 14)

#define RT2860_TXDESC_FLAGS_SHIFT						0
#define RT2860_TXDESC_FLAGS_MASK						0xf9
#define RT2860_TXDESC_FLAGS_WI_VALID					(1 << 0)

#define RT2860_TXDESC_QSEL_SHIFT						1
#define RT2860_TXDESC_QSEL_MASK							0x3
#define RT2860_TXDESC_QSEL_MGMT							0
#define RT2860_TXDESC_QSEL_HCCA							1
#define RT2860_TXDESC_QSEL_EDCA							2

struct rt2860_txdesc
{
	uint32_t sdp0;
	uint16_t sdl1;
	uint16_t sdl0;
	uint32_t sdp1;
	uint8_t	reserved[3];
	uint8_t qsel_flags;
} __packed;

#endif /* #ifndef _RT2860_TXDESC_H_ */
