/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	Register Backbone Manager registers
*/

#ifndef _RBBM_REGS_H
#define _RBBM_REGS_H

#define RADEON_GEN_INT_CNTL                 0x0040
#		define RADEON_CRTC_VBLANK_MASK		(1 <<  0)
#		define RADEON_CRTC2_VBLANK_MASK		(1 <<  9)
#		define RADEON_GUIDMA_MASK			(1 <<  30)
#		define RADEON_VIDDMA_MASK			(1 <<  31)
#define RADEON_GEN_INT_STATUS               0x0044
#		define RADEON_CRTC_VBLANK_STAT		(1 <<  0)
#		define RADEON_CRTC_VBLANK_STAT_AK	(1 <<  0)
#		define RADEON_CAP0_INT_ACTIVE		(1 <<  8)
#		define RADEON_CRTC2_VBLANK_STAT		(1 <<  9)
#		define RADEON_CRTC2_VBLANK_STAT_AK	(1 <<  9)
#		define RADEON_GUIDMA_STAT			(1 <<  30)
#		define RADEON_GUIDMA_AK				(1 <<  30)
#		define RADEON_VIDDMA_STAT			(1 <<  31)
#		define RADEON_VIDDMA_AK				(1 <<  31)

#define	RADEON_CAP_INT_CNTL					0x0908
#define RADEON_CAP_INT_STATUS				0x090c

#define RADEON_RBBM_SOFT_RESET              0x00f0
#       define RADEON_SOFT_RESET_CP           (1 <<  0)
#       define RADEON_SOFT_RESET_HI           (1 <<  1)
#       define RADEON_SOFT_RESET_SE           (1 <<  2)
#       define RADEON_SOFT_RESET_RE           (1 <<  3)
#       define RADEON_SOFT_RESET_PP           (1 <<  4)
#       define RADEON_SOFT_RESET_E2           (1 <<  5)
#       define RADEON_SOFT_RESET_RB           (1 <<  6)
#       define RADEON_SOFT_RESET_HDP          (1 <<  7)
#       define RADEON_SOFT_RESET_MC           (1 <<  8)
#       define RADEON_SOFT_RESET_AIC          (1 <<  9)

#define RADEON_CRC_CMDFIFO_ADDR             0x0740
#define RADEON_CRC_CMDFIFO_DOUT             0x0744

#define RADEON_RBBM_STATUS                  0x0e40
#       define RADEON_RBBM_FIFOCNT_MASK     0x007f
#       define RADEON_RBBM_ACTIVE           (1 << 31)

#define RADEON_WAIT_UNTIL                   0x1720
#	define RADEON_WAIT_CRTC_PFLIP       (1 << 0)
#	define RADEON_WAIT_2D_IDLECLEAN     (1 << 16)
#	define RADEON_WAIT_3D_IDLECLEAN     (1 << 17)
#	define RADEON_WAIT_HOST_IDLECLEAN   (1 << 18)

// these regs are only described for R200+
// but they are used in the original Radeon SDK already
#define RADEON_RB2D_DSTCACHE_CTLSTAT        0x342c
#       define RADEON_RB2D_DC_FLUSH         (3 << 0)
#       define RADEON_RB2D_DC_FREE          (3 << 2)
#       define RADEON_RB2D_DC_FLUSH_ALL     0xf
#       define RADEON_RB2D_DC_BUSY          (1 << 31)
#define RADEON_RB2D_DSTCACHE_MODE           0x3428


#endif
