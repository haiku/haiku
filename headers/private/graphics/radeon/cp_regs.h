/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	Command Processor registers
*/

#ifndef _CP_REGS_H
#define _CP_REGS_H


#define RADEON_SCRATCH_REG0		0x15e0
#define RADEON_SCRATCH_REG1		0x15e4
#define RADEON_SCRATCH_REG2		0x15e8
#define RADEON_SCRATCH_REG3		0x15ec
#define RADEON_SCRATCH_REG4		0x15f0
#define RADEON_SCRATCH_REG5		0x15f4
#define RADEON_SCRATCH_UMSK		0x0770
#define RADEON_SCRATCH_ADDR		0x0774

/* Registers for CP and Microcode Engine */
#define RADEON_CP_ME_RAM_ADDR               0x07d4
#define RADEON_CP_ME_RAM_RADDR              0x07d8
#define RADEON_CP_ME_RAM_DATAH              0x07dc
#define RADEON_CP_ME_RAM_DATAL              0x07e0

#define RADEON_CP_RB_BASE                   0x0700
#define RADEON_CP_RB_CNTL                   0x0704
#define RADEON_CP_RB_RPTR_ADDR              0x070c
#define RADEON_CP_RB_RPTR                   0x0710
#define RADEON_CP_RB_WPTR                   0x0714

#define RADEON_CP_IB_BASE                   0x0738
#define RADEON_CP_IB_BUFSZ                  0x073c

#define RADEON_CP_CSQ_CNTL                  0x0740
#       define RADEON_CSQ_CNT_PRIMARY_MASK  (0xff << 0)
#       define RADEON_CSQ_PRIDIS_INDDIS     (0    << 28)
#       define RADEON_CSQ_PRIPIO_INDDIS     (1    << 28)
#       define RADEON_CSQ_PRIBM_INDDIS      (2    << 28)
#       define RADEON_CSQ_PRIPIO_INDBM      (3    << 28)
#       define RADEON_CSQ_PRIBM_INDBM       (4    << 28)
#       define RADEON_CSQ_PRIPIO_INDPIO     (15   << 28)
#define RADEON_CP_STAT						0x07c0
#define RADEON_CP_CSQ_STAT                  0x07f8
#       define RADEON_CSQ_RPTR_PRIMARY_MASK  (0xff <<  0)
#       define RADEON_CSQ_WPTR_PRIMARY_MASK  (0xff <<  8)
#       define RADEON_CSQ_RPTR_INDIRECT_MASK (0xff << 16)
#       define RADEON_CSQ_WPTR_INDIRECT_MASK (0xff << 24)
#define RADEON_CP_CSQ_ADDR                  0x07f0
#define RADEON_CP_CSQ_DATA                  0x07f4
#define RADEON_CP_CSQ_APER_PRIMARY          0x1000
#define RADEON_CP_CSQ_APER_INDIRECT         0x1300

#define RADEON_CP_RB_WPTR_DELAY             0x0718
#       define RADEON_PRE_WRITE_TIMER_SHIFT      0
#       define RADEON_PRE_WRITE_LIMIT_SHIFT     23

/* CP packet types */
#define RADEON_CP_PACKET0                           0x00000000
#define RADEON_CP_PACKET1                           0x40000000
#define RADEON_CP_PACKET2                           0x80000000
#define RADEON_CP_PACKET3                           0xC0000000
#       define RADEON_CP_PACKET_MASK                0xC0000000
#       define RADEON_CP_PACKET_COUNT_MASK          0x3fff0000
#       define RADEON_CP_PACKET_MAX_DWORDS          (1 << 12)
#       define RADEON_CP_PACKET0_REG_MASK           0x000007ff
#       define RADEON_CP_PACKET1_REG0_MASK          0x000007ff
#       define RADEON_CP_PACKET1_REG1_MASK          0x003ff800

#define RADEON_CP_PACKET0_ONE_REG_WR                0x00008000

#define RADEON_CP_PACKET3_NOP                       0xC0001000
#define RADEON_CP_PACKET3_NEXT_CHAR                 0xC0001900
#define RADEON_CP_PACKET3_PLY_NEXTSCAN              0xC0001D00
#define RADEON_CP_PACKET3_SET_SCISSORS              0xC0001E00
#define RADEON_CP_PACKET3_3D_RNDR_GEN_INDX_PRIM     0xC0002300
#define RADEON_CP_PACKET3_LOAD_MICROCODE            0xC0002400
#define RADEON_CP_PACKET3_3D_RNDR_GEN_PRIM          0xC0002500
#define RADEON_CP_PACKET3_WAIT_FOR_IDLE             0xC0002600
#define RADEON_CP_PACKET3_3D_DRAW_VBUF              0xC0002800
#define RADEON_CP_PACKET3_3D_DRAW_IMMD              0xC0002900
#define RADEON_CP_PACKET3_3D_DRAW_INDX              0xC0002A00
#define RADEON_CP_PACKET3_LOAD_PALETTE              0xC0002C00
#define RADEON_CP_PACKET3_3D_LOAD_VBPNTR            0xC0002F00
#define RADEON_CP_PACKET3_3D_CLEAR_ZMASK            0xC0003200
#define RADEON_CP_PACKET3_CNTL_PAINT                0xC0009100
#define RADEON_CP_PACKET3_CNTL_BITBLT               0xC0009200
#define RADEON_CP_PACKET3_CNTL_SMALLTEXT            0xC0009300
#define RADEON_CP_PACKET3_CNTL_HOSTDATA_BLT         0xC0009400
#define RADEON_CP_PACKET3_CNTL_POLYLINE             0xC0009500
#define RADEON_CP_PACKET3_CNTL_POLYSCANLINES        0xC0009800
#define RADEON_CP_PACKET3_CNTL_PAINT_MULTI          0xC0009A00
#define RADEON_CP_PACKET3_CNTL_BITBLT_MULTI         0xC0009B00
#define RADEON_CP_PACKET3_CNTL_TRANS_BITBLT         0xC0009C00


#define RADEON_ISYNC_CNTL		0x1724
#	define RADEON_ISYNC_ANY2D_IDLE3D	(1 << 0)
#	define RADEON_ISYNC_ANY3D_IDLE2D	(1 << 1)
#	define RADEON_ISYNC_TRIG2D_IDLE3D	(1 << 2)
#	define RADEON_ISYNC_TRIG3D_IDLE2D	(1 << 3)
#	define RADEON_ISYNC_WAIT_IDLEGUI	(1 << 4)
#	define RADEON_ISYNC_CPSCRATCH_IDLEGUI	(1 << 5)


#define CP_PACKET0( reg, n )						\
	(RADEON_CP_PACKET0 | (((n) - 1) << 16) | ((reg) >> 2))


#endif
