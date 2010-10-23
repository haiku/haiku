
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

#ifndef _RT2860_RXDESC_H_
#define _RT2860_RXDESC_H_

#define RT2860_RXDESC_SDL0_DDONE						(1 << 15)

#define RT2860_RXDESC_FLAGS_LAST_AMSDU					(1 << 19)
#define RT2860_RXDESC_FLAGS_CIPHER_ALG					(1 << 18)
#define RT2860_RXDESC_FLAGS_PLCP_RSSIL					(1 << 17)
#define RT2860_RXDESC_FLAGS_DECRYPTED					(1 << 16)
#define RT2860_RXDESC_FLAGS_AMPDU						(1 << 15)
#define RT2860_RXDESC_FLAGS_L2PAD						(1 << 14)
#define RT2860_RXDESC_FLAGS_RSSI						(1 << 13)
#define RT2860_RXDESC_FLAGS_HTC							(1 << 12)
#define RT2860_RXDESC_FLAGS_AMSDU						(1 << 11)
#define RT2860_RXDESC_FLAGS_CRC_ERR						(1 << 8)
#define RT2860_RXDESC_FLAGS_MYBSS						(1 << 7)
#define RT2860_RXDESC_FLAGS_BCAST						(1 << 6)
#define RT2860_RXDESC_FLAGS_MCAST						(1 << 5)
#define RT2860_RXDESC_FLAGS_U2M							(1 << 4)
#define RT2860_RXDESC_FLAGS_FRAG						(1 << 3)
#define RT2860_RXDESC_FLAGS_NULL_DATA					(1 << 2)
#define RT2860_RXDESC_FLAGS_DATA						(1 << 1)
#define RT2860_RXDESC_FLAGS_BA							(1 << 0)

#define RT2860_RXDESC_FLAGS_CIPHER_ERR_SHIFT			9
#define RT2860_RXDESC_FLAGS_CIPHER_ERR_MASK				0x3
#define RT2860_RXDESC_FLAGS_CIPHER_ERR_NONE				0
#define RT2860_RXDESC_FLAGS_CIPHER_ERR_ICV				1
#define RT2860_RXDESC_FLAGS_CIPHER_ERR_MIC				2
#define RT2860_RXDESC_FLAGS_CIPHER_ERR_INVALID_KEY		3

#define RT2860_RXDESC_FLAGS_PLCP_SIGNAL_SHIFT			20
#define RT2860_RXDESC_FLAGS_PLCP_SIGNAL_MASK			0xfff

struct rt2860_rxdesc
{
	uint32_t sdp0;
	uint16_t sdl1;
	uint16_t sdl0;
	uint32_t sdp1;
	uint32_t flags;
} __packed;

#endif /* #ifndef _RT2860_RXDESC_H_ */
