/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	2D registers
*/


#ifndef _2D_REGS_H
#define _2D_REGS_H

#define RADEON_DP_BRUSH_BKGD_CLR            0x1478
#define RADEON_DP_BRUSH_FRGD_CLR            0x147c
#define RADEON_DP_CNTL                      0x16c0
#       define RADEON_DST_X_LEFT_TO_RIGHT   (1 <<  0)
#       define RADEON_DST_Y_TOP_TO_BOTTOM   (1 <<  1)
#define RADEON_DP_CNTL_XDIR_YDIR_YMAJOR     0x16d0
#       define RADEON_DST_Y_MAJOR             (1 <<  2)
#       define RADEON_DST_Y_DIR_TOP_TO_BOTTOM (1 << 15)
#       define RADEON_DST_X_DIR_LEFT_TO_RIGHT (1 << 31)
#define RADEON_DP_DATATYPE                  0x16c4
#       define RADEON_HOST_BIG_ENDIAN_EN    (1 << 29)
#define RADEON_DP_GUI_MASTER_CNTL           0x146c
#       define RADEON_GMC_SRC_PITCH_OFFSET_CNTL (1    <<  0)
#       define RADEON_GMC_DST_PITCH_OFFSET_CNTL (1    <<  1)
#       define RADEON_GMC_SRC_CLIPPING          (1    <<  2)
#       define RADEON_GMC_DST_CLIPPING          (1    <<  3)
#       define RADEON_GMC_BRUSH_DATATYPE_MASK   (0x0f <<  4)
#       define RADEON_GMC_BRUSH_8X8_MONO_FG_BG  (0    <<  4)
#       define RADEON_GMC_BRUSH_8X8_MONO_FG_LA  (1    <<  4)
#       define RADEON_GMC_BRUSH_1X8_MONO_FG_BG  (4    <<  4)
#       define RADEON_GMC_BRUSH_1X8_MONO_FG_LA  (5    <<  4)
#       define RADEON_GMC_BRUSH_32x1_MONO_FG_BG (6    <<  4)
#       define RADEON_GMC_BRUSH_32x1_MONO_FG_LA (7    <<  4)
#       define RADEON_GMC_BRUSH_32x32_MONO_FG_BG (8    <<  4)
#       define RADEON_GMC_BRUSH_32x32_MONO_FG_LA (9    <<  4)
#       define RADEON_GMC_BRUSH_8x8_COLOR       (10   <<  4)
#       define RADEON_GMC_BRUSH_1X8_COLOR       (12   <<  4)
#       define RADEON_GMC_BRUSH_SOLID_COLOR     (13   <<  4)
#       define RADEON_GMC_BRUSH_NONE            (15   <<  4)
#       define RADEON_GMC_DST_8BPP_CI           (2    <<  8)
#       define RADEON_GMC_DST_15BPP             (3    <<  8)
#       define RADEON_GMC_DST_16BPP             (4    <<  8)
#       define RADEON_GMC_DST_24BPP             (5    <<  8)
#       define RADEON_GMC_DST_32BPP             (6    <<  8)
#       define RADEON_GMC_DST_8BPP_RGB          (7    <<  8)
#       define RADEON_GMC_DST_Y8                (8    <<  8)
#       define RADEON_GMC_DST_RGB8              (9    <<  8)
#       define RADEON_GMC_DST_VYUY              (11   <<  8)
#       define RADEON_GMC_DST_YVYU              (12   <<  8)
#       define RADEON_GMC_DST_AYUV444           (14   <<  8)
#       define RADEON_GMC_DST_ARGB4444          (15   <<  8)
#       define RADEON_GMC_DST_DATATYPE_MASK     (0x0f <<  8)
#       define RADEON_GMC_DST_DATATYPE_SHIFT    8
#       define RADEON_GMC_SRC_DATATYPE_MASK       (3    << 12)
#       define RADEON_GMC_SRC_DATATYPE_MONO_FG_BG (0    << 12)
#       define RADEON_GMC_SRC_DATATYPE_MONO_FG_LA (1    << 12)
#       define RADEON_GMC_SRC_DATATYPE_COLOR      (3    << 12)
#       define RADEON_GMC_BYTE_PIX_ORDER        (1    << 14)
#       define RADEON_GMC_BYTE_MSB_TO_LSB       (0    << 14)
#       define RADEON_GMC_BYTE_LSB_TO_MSB       (1    << 14)
#       define RADEON_GMC_CONVERSION_TEMP       (1    << 15)
#       define RADEON_GMC_CONVERSION_TEMP_6500  (0    << 15)
#       define RADEON_GMC_CONVERSION_TEMP_9300  (1    << 15)
#       define RADEON_GMC_ROP3_MASK             (0xff << 16)
#       define RADEON_DP_SRC_SOURCE_MASK        (7    << 24)
#       define RADEON_DP_SRC_SOURCE_MEMORY      (2    << 24)
#       define RADEON_DP_SRC_SOURCE_HOST_DATA   (3    << 24)
#       define RADEON_GMC_3D_FCN_EN             (1    << 27)
#       define RADEON_GMC_CLR_CMP_CNTL_DIS      (1    << 28)
#       define RADEON_GMC_AUX_CLIP_DIS          (1    << 29)
#       define RADEON_GMC_WR_MSK_DIS            (1    << 30)
#       define RADEON_GMC_LD_BRUSH_Y_X          (1    << 31)
#       define RADEON_ROP3_ZERO             0x00000000
#       define RADEON_ROP3_DSa              0x00880000
#       define RADEON_ROP3_SDna             0x00440000
#       define RADEON_ROP3_S                0x00cc0000
#       define RADEON_ROP3_DSna             0x00220000
#       define RADEON_ROP3_D                0x00aa0000
#       define RADEON_ROP3_DSx              0x00660000
#       define RADEON_ROP3_DSo              0x00ee0000
#       define RADEON_ROP3_DSon             0x00110000
#       define RADEON_ROP3_DSxn             0x00990000
#       define RADEON_ROP3_Dn               0x00550000
#       define RADEON_ROP3_SDno             0x00dd0000
#       define RADEON_ROP3_Sn               0x00330000
#       define RADEON_ROP3_DSno             0x00bb0000
#       define RADEON_ROP3_DSan             0x00770000
#       define RADEON_ROP3_ONE              0x00ff0000
#       define RADEON_ROP3_DPa              0x00a00000
#       define RADEON_ROP3_PDna             0x00500000
#       define RADEON_ROP3_P                0x00f00000
#       define RADEON_ROP3_DPna             0x000a0000
#       define RADEON_ROP3_D                0x00aa0000
#       define RADEON_ROP3_DPx              0x005a0000
#       define RADEON_ROP3_DPo              0x00fa0000
#       define RADEON_ROP3_DPon             0x00050000
#       define RADEON_ROP3_PDxn             0x00a50000
#       define RADEON_ROP3_PDno             0x00f50000
#       define RADEON_ROP3_Pn               0x000f0000
#       define RADEON_ROP3_DPno             0x00af0000
#       define RADEON_ROP3_DPan             0x005f0000

#define RADEON_DP_GUI_MASTER_CNTL_C         0x1c84
#define RADEON_DP_MIX                       0x16c8
#define RADEON_DP_SRC_BKGD_CLR              0x15dc
#define RADEON_DP_SRC_FRGD_CLR              0x15d8
#define RADEON_DP_WRITE_MASK                0x16cc

#define RADEON_BRUSH_SCALE                  0x1470
#define RADEON_BRUSH_Y_X                    0x1474
#define RADEON_BRUSH_DATA0                  0x1480
#define RADEON_BRUSH_DATA1                  0x1484
#define RADEON_BRUSH_DATA2                  0x1488
#define RADEON_BRUSH_DATA3                  0x148c
#define RADEON_BRUSH_DATA4                  0x1490
#define RADEON_BRUSH_DATA5                  0x1494
#define RADEON_BRUSH_DATA6                  0x1498
#define RADEON_BRUSH_DATA7                  0x149c
#define RADEON_BRUSH_DATA8                  0x14a0
#define RADEON_BRUSH_DATA9                  0x14a4
#define RADEON_BRUSH_DATA10                 0x14a8
#define RADEON_BRUSH_DATA11                 0x14ac
#define RADEON_BRUSH_DATA12                 0x14b0
#define RADEON_BRUSH_DATA13                 0x14b4
#define RADEON_BRUSH_DATA14                 0x14b8
#define RADEON_BRUSH_DATA15                 0x14bc
#define RADEON_BRUSH_DATA16                 0x14c0
#define RADEON_BRUSH_DATA17                 0x14c4
#define RADEON_BRUSH_DATA18                 0x14c8
#define RADEON_BRUSH_DATA19                 0x14cc
#define RADEON_BRUSH_DATA20                 0x14d0
#define RADEON_BRUSH_DATA21                 0x14d4
#define RADEON_BRUSH_DATA22                 0x14d8
#define RADEON_BRUSH_DATA23                 0x14dc
#define RADEON_BRUSH_DATA24                 0x14e0
#define RADEON_BRUSH_DATA25                 0x14e4
#define RADEON_BRUSH_DATA26                 0x14e8
#define RADEON_BRUSH_DATA27                 0x14ec
#define RADEON_BRUSH_DATA28                 0x14f0
#define RADEON_BRUSH_DATA29                 0x14f4
#define RADEON_BRUSH_DATA30                 0x14f8
#define RADEON_BRUSH_DATA31                 0x14fc
#define RADEON_BRUSH_DATA32                 0x1500
#define RADEON_BRUSH_DATA33                 0x1504
#define RADEON_BRUSH_DATA34                 0x1508
#define RADEON_BRUSH_DATA35                 0x150c
#define RADEON_BRUSH_DATA36                 0x1510
#define RADEON_BRUSH_DATA37                 0x1514
#define RADEON_BRUSH_DATA38                 0x1518
#define RADEON_BRUSH_DATA39                 0x151c
#define RADEON_BRUSH_DATA40                 0x1520
#define RADEON_BRUSH_DATA41                 0x1524
#define RADEON_BRUSH_DATA42                 0x1528
#define RADEON_BRUSH_DATA43                 0x152c
#define RADEON_BRUSH_DATA44                 0x1530
#define RADEON_BRUSH_DATA45                 0x1534
#define RADEON_BRUSH_DATA46                 0x1538
#define RADEON_BRUSH_DATA47                 0x153c
#define RADEON_BRUSH_DATA48                 0x1540
#define RADEON_BRUSH_DATA49                 0x1544
#define RADEON_BRUSH_DATA50                 0x1548
#define RADEON_BRUSH_DATA51                 0x154c
#define RADEON_BRUSH_DATA52                 0x1550
#define RADEON_BRUSH_DATA53                 0x1554
#define RADEON_BRUSH_DATA54                 0x1558
#define RADEON_BRUSH_DATA55                 0x155c
#define RADEON_BRUSH_DATA56                 0x1560
#define RADEON_BRUSH_DATA57                 0x1564
#define RADEON_BRUSH_DATA58                 0x1568
#define RADEON_BRUSH_DATA59                 0x156c
#define RADEON_BRUSH_DATA60                 0x1570
#define RADEON_BRUSH_DATA61                 0x1574
#define RADEON_BRUSH_DATA62                 0x1578
#define RADEON_BRUSH_DATA63                 0x157c

#define RADEON_DEFAULT_SC_BOTTOM_RIGHT      0x16e8
#       define RADEON_DEFAULT_SC_RIGHT_MAX  (0x1fff <<  0)
#       define RADEON_DEFAULT_SC_BOTTOM_MAX (0x1fff << 16)

#define RADEON_DST_LINE_START               0x1600
#define RADEON_DST_LINE_END                 0x1604
#define RADEON_DST_LINE_PATCOUNT            0x1608
#		define RADEON_BRES_CNTL_SHIFT		8
#define RADEON_DEFAULT_OFFSET               0x16e0
#define RADEON_DEFAULT_PITCH                0x16e4

#define RADEON_SRC_PITCH_OFFSET             0x1428
#define RADEON_DST_PITCH_OFFSET             0x142c


#endif
