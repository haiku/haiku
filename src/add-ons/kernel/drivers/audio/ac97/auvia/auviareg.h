/*
 * Auvia BeOS Driver for Via VT82xx Southbridge audio
 *
 * Copyright (c) 2003, Jerome Duval (jerome.duval@free.fr)
 
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tyler C. Sarna
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
 
#ifndef _DEV_PCI_AUVIAREG_H_
#define _DEV_PCI_AUVIAREG_H_

// VT8233 specific registers contain a '8233_'

#define AUVIA_PCICONF_JUNK	0x40
#define		AUVIA_PCICONF_ENABLES	 0x00ff0000	/* reg 42 mask */
#define		AUVIA_PCICONF_ACLINKENAB 0x00008000	/* ac link enab */
#define		AUVIA_PCICONF_ACNOTRST	 0x00004000	/* ~(ac reset) */
#define		AUVIA_PCICONF_ACSYNC	 0x00002000	/* ac sync */
#define		AUVIA_PCICONF_ACVSR	 0x00000800	/* var. samp. rate */
#define		AUVIA_PCICONF_ACSGD	 0x00000400	/* SGD enab */
#define		AUVIA_PCICONF_ACFM	 0x00000200	/* FM enab */
#define		AUVIA_PCICONF_ACSB	 0x00000100	/* SB enab */

#define	AUVIA_PLAY_BASE			0x00
#define	AUVIA_RECORD_BASE		0x10
#define	AUVIA_8233_RECORD_BASE		0x60

/* *_RP_* are offsets from AUVIA_PLAY_BASE or AUVIA_RECORD_BASE or AUVIA_8233_RECORD_BASE*/
#define	AUVIA_RP_STAT			0x00
#define		AUVIA_RPSTAT_INTR		0x03
#define	AUVIA_RP_CONTROL		0x01
#define		AUVIA_RPCTRL_START		0x80
#define		AUVIA_RPCTRL_TERMINATE		0x40
#define		AUVIA_RPCTRL_AUTOSTART		0x20
/* The following are 8233 specific */
#define		AUVIA_RPCTRL_STOP		0x04
#define		AUVIA_RPCTRL_EOL		0x02
#define		AUVIA_RPCTRL_FLAG		0x01
#define	AUVIA_RP_MODE			0x02		/* 82c686 specific */
#define		AUVIA_RPMODE_INTR_FLAG		0x01
#define		AUVIA_RPMODE_INTR_EOL		0x02
#define		AUVIA_RPMODE_STEREO		0x10
#define		AUVIA_RPMODE_16BIT		0x20
#define		AUVIA_RPMODE_AUTOSTART		0x80
#define	AUVIA_RP_DMAOPS_BASE		0x04

#define	AUVIA_8233_RP_DXS_LVOL		0x02
#define	AUVIA_8233_RP_DXS_RVOL		0x03
#define	AUVIA_8233_RP_RATEFMT		0x08
#define		AUVIA_8233_RATEFMT_48K		0xfffff
#define		AUVIA_8233_RATEFMT_STEREO		0x00100000
#define		AUVIA_8233_RATEFMT_16BIT		0x00200000

#define	VIA_RP_DMAOPS_COUNT		0x0c

#define AUVIA_8233_MP_BASE			0x40
	/* STAT, CONTROL, DMAOPS_BASE, DMAOPS_COUNT are valid */
#define AUVIA_8233_OFF_MP_FORMAT		0x02
#define		AUVIA_8233_MP_FORMAT_8BIT		0x00
#define		AUVIA_8233_MP_FORMAT_16BIT		0x80
#define		AUVIA_8233_MP_FORMAT_CHANNEL_MASK	0x70 /* 1, 2, 4, 6 */
#define AUVIA_8233_OFF_MP_SCRATCH		0x03
#define AUVIA_8233_OFF_MP_STOP		0x08

#define AUVIA_CODEC_CTL			0x80
#define AUVIA_CODEC_READ		0x00800000
#define AUVIA_CODEC_BUSY		0x01000000
#define AUVIA_CODEC_PRIVALID	0x02000000
#define AUVIA_CODEC_INDEX(x)	((x)<<16)

#define AUVIA_SGD_SHADOW		0x84
#define AUVIA_SGD_STAT_FLAG_MASK	0x00000007
#define AUVIA_SGD_STAT_EOL_MASK		0x00000070
#define AUVIA_SGD_STAT_STOP_MASK	0x00000700
#define AUVIA_SGD_STAT_ACTIVE_MASK	0x00007000
#define AUVIA_SGD_STAT_PLAYBACK		0x00000001
#define AUVIA_SGD_STAT_RECORD		0x00000002
#define AUVIA_SGD_STAT_FM			0x00000004
#define AUVIA_SGD_STAT_ALL			0x00000007

#define AUVIA_8233_SGD_STAT_SDX0_MASK	0x00000008
#define AUVIA_8233_SGD_STAT_SDX1_MASK	0x00000080
#define AUVIA_8233_SGD_STAT_SDX2_MASK	0x00000800
#define AUVIA_8233_SGD_STAT_SDX3_MASK	0x00008000
#define AUVIA_8233_SGD_STAT_MP_MASK		0x00080000
#define AUVIA_8233_SGD_STAT_REC0_MASK	0x08000000
#define AUVIA_8233_SGD_STAT_REC1_MASK	0x80000000
#define AUVIA_8233_SGD_STAT_FLAG		0x00000001
#define AUVIA_8233_SGD_STAT_EOL			0x00000002
#define AUVIA_8233_SGD_STAT_STOP		0x00000004
#define AUVIA_8233_SGD_STAT_ACTIVE		0x00000008
#define AUVIA_8233_SGD_STAT_FLAG_EOL	0x00000003

#define AUVIA_DMAOP_EOL		0x80000000
#define AUVIA_DMAOP_FLAG	0x40000000
#define AUVIA_DMAOP_STOP	0x20000000
#define AUVIA_DMAOP_COUNT(x)	((x)&0x00FFFFFF)

#endif /* _DEV_PCI_AUVIAREG_H_ */
