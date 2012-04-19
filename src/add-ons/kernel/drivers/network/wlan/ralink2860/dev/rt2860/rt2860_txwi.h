
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

#ifndef _RT2860_TXWI_H_
#define _RT2860_TXWI_H_

#define RT2860_TXWI_FLAGS_SHIFT					0
#define RT2860_TXWI_FLAGS_MASK					0x1f
#define RT2860_TXWI_FLAGS_AMPDU					(1 << 4)
#define RT2860_TXWI_FLAGS_TS					(1 << 3)
#define RT2860_TXWI_FLAGS_CFACK					(1 << 2)
#define RT2860_TXWI_FLAGS_MIMOPS				(1 << 1)
#define RT2860_TXWI_FLAGS_FRAG					(1 << 0)

#define RT2860_TXWI_MPDU_DENSITY_SHIFT			5
#define RT2860_TXWI_MPDU_DENSITY_MASK			0x7

#define RT2860_TXWI_TXOP_SHIFT					0
#define RT2860_TXWI_TXOP_MASK					0x3
#define RT2860_TXWI_TXOP_HT						0
#define RT2860_TXWI_TXOP_PIFS					1
#define RT2860_TXWI_TXOP_SIFS					2
#define RT2860_TXWI_TXOP_BACKOFF				3

#define RT2860_TXWI_MCS_SHIFT					0
#define RT2860_TXWI_MCS_MASK					0x7f
#define RT2860_TXWI_MCS_SHOTPRE					(1 << 3)

#define RT2860_TXWI_BW_SHIFT					7
#define RT2860_TXWI_BW_MASK						0x1
#define RT2860_TXWI_BW_20						0
#define RT2860_TXWI_BW_40						1

#define RT2860_TXWI_SHORTGI_SHIFT				0
#define RT2860_TXWI_SHORTGI_MASK				0x1

#define RT2860_TXWI_STBC_SHIFT					1
#define RT2860_TXWI_STBC_MASK					0x3

#define RT2860_TXWI_IFS_SHIFT					3
#define RT2860_TXWI_IFS_MASK					0x1

#define RT2860_TXWI_PHYMODE_SHIFT				6
#define RT2860_TXWI_PHYMODE_MASK				0x3
#define RT2860_TXWI_PHYMODE_CCK					0
#define RT2860_TXWI_PHYMODE_OFDM				1
#define RT2860_TXWI_PHYMODE_HT_MIXED			2
#define RT2860_TXWI_PHYMODE_HT_GF				3

#define RT2860_TXWI_XFLAGS_SHIFT				0
#define RT2860_TXWI_XFLAGS_MASK					0x3
#define RT2860_TXWI_XFLAGS_NSEQ					(1 << 1)
#define RT2860_TXWI_XFLAGS_ACK					(1 << 0)

#define RT2860_TXWI_BAWIN_SIZE_SHIFT			2
#define RT2860_TXWI_BAWIN_SIZE_MASK				0x3f

#define RT2860_TXWI_MPDU_LEN_SHIFT				0
#define RT2860_TXWI_MPDU_LEN_MASK				0xfff

#define RT2860_TXWI_PID_SHIFT					12
#define RT2860_TXWI_PID_MASK					0xf

struct rt2860_txwi
{
	uint8_t mpdu_density_flags;
	uint8_t txop;
	uint8_t bw_mcs;
	uint8_t phymode_ifs_stbc_shortgi;
	uint8_t bawin_size_xflags;
	uint8_t wcid;
	uint16_t pid_mpdu_len;
	uint32_t iv;
	uint32_t eiv;
} __packed;

#endif /* #ifndef _RT2860_TXWI_H_ */
