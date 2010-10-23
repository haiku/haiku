
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

#ifndef _RT2860_RXWI_H_
#define _RT2860_RXWI_H_

#define RT2860_RXWI_KEYIDX_SHIFT			0
#define RT2860_RXWI_KEYIDX_MASK				0x3

#define RT2860_RXWI_BSSIDX_SHIFT			2
#define RT2860_RXWI_BSSIDX_MASK				0x7

#define RT2860_RXWI_UDF_SHIFT				5
#define RT2860_RXWI_UDF_MASK				0x7

#define RT2860_RXWI_SIZE_SHIFT				0
#define RT2860_RXWI_SIZE_MASK				0xfff

#define RT2860_RXWI_TID_SHIFT				12
#define RT2860_RXWI_TID_MASK				0xf

#define RT2860_RXWI_FRAG_SHIFT				0
#define RT2860_RXWI_FRAG_MASK				0xf

#define RT2860_RXWI_SEQ_SHIFT				4
#define RT2860_RXWI_SEQ_MASK				0xfff

#define RT2860_RXWI_MCS_SHIFT				0
#define RT2860_RXWI_MCS_MASK				0x7f
#define RT2860_RXWI_MCS_SHOTPRE				(1 << 3)

#define RT2860_RXWI_BW_SHIFT				7
#define RT2860_RXWI_BW_MASK					0x1
#define RT2860_RXWI_BW_20					0
#define RT2860_RXWI_BW_40					1

#define RT2860_RXWI_SHORTGI_SHIFT			0
#define RT2860_RXWI_SHORTGI_MASK			0x1

#define RT2860_RXWI_STBC_SHIFT				1
#define RT2860_RXWI_STBC_MASK				0x3

#define RT2860_RXWI_PHYMODE_SHIFT			6
#define RT2860_RXWI_PHYMODE_MASK			0x3
#define RT2860_RXWI_PHYMODE_CCK				0
#define RT2860_RXWI_PHYMODE_OFDM			1
#define RT2860_RXWI_PHYMODE_HT_MIXED		2
#define RT2860_RXWI_PHYMODE_HT_GF			3

struct rt2860_rxwi
{
	uint8_t wcid;
	uint8_t udf_bssidx_keyidx;
	uint16_t tid_size;
	uint16_t seq_frag;
	uint8_t bw_mcs;
	uint8_t phymode_stbc_shortgi;
	uint8_t rssi[3];
	uint8_t reserved1;
	uint8_t snr[2];
	uint16_t reserved2;
} __packed;

#endif /* #ifndef _RT2860_RXWI_H_ */
