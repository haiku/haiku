/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	CRTC registers
*/

#ifndef _CRTC_REGS_H
#define _CRTC_REGS_H

#define RADEON_CRTC_CRNT_FRAME              0x0214
#define RADEON_CRTC_DEBUG                   0x021c
#define RADEON_CRTC_GEN_CNTL                0x0050
#       define RADEON_CRTC_DBL_SCAN_EN      (1 <<  0)
#       define RADEON_CRTC_INTERLACE_EN     (1 <<  1)
#       define RADEON_CRTC_CSYNC_EN         (1 <<  4)
#       define RADEON_CRTC_PIX_WIDTH_SHIFT  8
#       define RADEON_CRTC_PIX_WIDTH_MASK   (15 << 8)
#       define RADEON_CRTC_ICON_EN          (1 << 15)
#       define RADEON_CRTC_CUR_EN           (1 << 16)
#       define RADEON_CRTC_CUR_MODE_MASK    (7 << 20)
#       define RADEON_CRTC_EXT_DISP_EN      (1 << 24)
#       define RADEON_CRTC_EN               (1 << 25)
#       define RADEON_CRTC_DISP_REQ_EN_B    (1 << 26)
#define RADEON_CRTC_EXT_CNTL                0x0054
#       define RADEON_CRTC_VGA_XOVERSCAN    (1 <<  0)
#       define RADEON_VGA_ATI_LINEAR        (1 <<  3)
#       define RADEON_XCRT_CNT_EN           (1 <<  6)
#       define RADEON_CRTC_HSYNC_DIS        (1 <<  8)
#       define RADEON_CRTC_VSYNC_DIS        (1 <<  9)
#       define RADEON_CRTC_DISPLAY_DIS      (1 << 10)
#       define RADEON_CRTC_SYNC_TRISTAT     (1 << 11)
#       define RADEON_CRTC_HSYNC_TRISTAT    (1 << 12)
#       define RADEON_CRTC_VSYNC_TRISTAT    (1 << 13)
#       define RADEON_CRTC_CRT_ON           (1 << 15)
#define RADEON_CRTC2_GEN_CNTL                0x03f8
#       define RADEON_CRTC2_DBL_SCAN_EN      (1 <<  0)
#       define RADEON_CRTC2_INTERLACE_EN     (1 <<  1)
#       define RADEON_CRTC2_SYNC_TRISTAT     (1 <<  4)
#       define RADEON_CRTC2_HSYNC_TRISTAT    (1 <<  5)
#       define RADEON_CRTC2_VSYNC_TRISTAT    (1 <<  6)
#       define RADEON_CRTC2_CRT2_ON          (1 <<  7)
#       define RADEON_CRTC2_PIX_WIDTH_SHIFT  8
#       define RADEON_CRTC2_PIX_WIDTH_MASK   (0xf << 8)
#       define RADEON_CRTC2_ICON_EN          (1 << 15)
#       define RADEON_CRTC2_CUR_EN           (1 << 16)
#       define RADEON_CRTC2_CUR_MODE_MASK    (7 << 20)
#       define RADEON_CRTC2_DISP_DIS         (1 << 23)
#       define RADEON_CRTC2_EN               (1 << 25)
#       define RADEON_CRTC2_DISP_REQ_EN_B    (1 << 26)
#       define RADEON_CRTC2_HSYNC_DIS        (1 << 28)
#       define RADEON_CRTC2_VSYNC_DIS        (1 << 29)
#define RADEON_CRTC_GUI_TRIG_VLINE          0x0218
#define RADEON_CRTC_MORE_CNTL					0x27C
#		define RADEON_CRTC_HORZ_BLANK_MODE_SEL	(1 << 0)
#		define RADEON_CRTC_VERT_BLANK_MODE_SEL	(1 << 1)
#		define RADEON_CRTC_AUTO_HORZ_CENTER_EN	(1 << 2)
#		define RADEON_CRTC_AUTO_VERT_CENTER_EN	(1 << 3)
#		define RADEON_CRTC_H_CUTOFF_ACTIVE_EN	(1 << 4)
#		define RADEON_CRTC_V_CUTOFF_ACTIVE_EN	(1 << 5)
#define RADEON_CRTC_H_SYNC_STRT_WID         0x0204
#       define RADEON_CRTC_H_SYNC_STRT_PIX        (0x07  <<  0)
#       define RADEON_CRTC_H_SYNC_STRT_CHAR       (0x3ff <<  3)
#       define RADEON_CRTC_H_SYNC_STRT_CHAR_SHIFT 3
#       define RADEON_CRTC_H_SYNC_WID             (0x3f  << 16)
#       define RADEON_CRTC_H_SYNC_WID_SHIFT       16
#       define RADEON_CRTC_H_SYNC_POL             (1     << 23)
#define RADEON_CRTC2_H_SYNC_STRT_WID         0x0304
#       define RADEON_CRTC2_H_SYNC_STRT_PIX        (0x07  <<  0)
#       define RADEON_CRTC2_H_SYNC_STRT_CHAR       (0x3ff <<  3)
#       define RADEON_CRTC2_H_SYNC_STRT_CHAR_SHIFT 3
#       define RADEON_CRTC2_H_SYNC_WID             (0x3f  << 16)
#       define RADEON_CRTC2_H_SYNC_WID_SHIFT       16
#       define RADEON_CRTC2_H_SYNC_POL             (1     << 23)
#define RADEON_CRTC_H_TOTAL_DISP            0x0200
#       define RADEON_CRTC_H_TOTAL          (0x03ff << 0)
#       define RADEON_CRTC_H_TOTAL_SHIFT    0
#       define RADEON_CRTC_H_DISP           (0x01ff << 16)
#       define RADEON_CRTC_H_DISP_SHIFT     16
#define RADEON_CRTC2_H_TOTAL_DISP           0x0300
#       define RADEON_CRTC2_H_TOTAL          (0x03ff << 0)
#       define RADEON_CRTC2_H_TOTAL_SHIFT    0
#       define RADEON_CRTC2_H_DISP           (0x01ff << 16)
#       define RADEON_CRTC2_H_DISP_SHIFT     16
#define RADEON_CRTC_OFFSET                  0x0224
#define RADEON_CRTC2_OFFSET                 0x0324
#define RADEON_CRTC_OFFSET_CNTL             0x0228
#	define RADEON_CRTC_TILE_EN          (1 << 15)
#define RADEON_CRTC2_OFFSET_CNTL            0x0328
#	define RADEON_CRTC2_TILE_EN         (1 << 15)
#define RADEON_CRTC_PITCH                   0x022c
#define RADEON_CRTC2_PITCH                  0x032c
#define RADEON_CRTC_STATUS                  0x005c
#       define RADEON_CRTC_VBLANK_SAVE      (1 <<  1)
#define RADEON_CRTC_V_SYNC_STRT_WID         0x020c
#       define RADEON_CRTC_V_SYNC_STRT       (0x7ff <<  0)
#       define RADEON_CRTC_V_SYNC_STRT_SHIFT 0
#       define RADEON_CRTC_V_SYNC_WID        (0x1f  << 16)
#       define RADEON_CRTC_V_SYNC_WID_SHIFT  16
#       define RADEON_CRTC_V_SYNC_POL        (1     << 23)
#define RADEON_CRTC2_V_SYNC_STRT_WID         0x030c
#       define RADEON_CRTC2_V_SYNC_STRT       (0x7ff <<  0)
#       define RADEON_CRTC2_V_SYNC_STRT_SHIFT 0
#       define RADEON_CRTC2_V_SYNC_WID        (0x1f  << 16)
#       define RADEON_CRTC2_V_SYNC_WID_SHIFT  16
#       define RADEON_CRTC2_V_SYNC_POL        (1     << 23)
#define RADEON_CRTC_V_TOTAL_DISP            0x0208
#       define RADEON_CRTC_V_TOTAL          (0x07ff << 0)
#       define RADEON_CRTC_V_TOTAL_SHIFT    0
#       define RADEON_CRTC_V_DISP           (0x07ff << 16)
#       define RADEON_CRTC_V_DISP_SHIFT     16
#define RADEON_CRTC2_V_TOTAL_DISP            0x0308
#       define RADEON_CRTC2_V_TOTAL          (0x07ff << 0)
#       define RADEON_CRTC2_V_TOTAL_SHIFT    0
#       define RADEON_CRTC2_V_DISP           (0x07ff << 16)
#       define RADEON_CRTC2_V_DISP_SHIFT     16
#define RADEON_CRTC_VLINE_CRNT_VLINE        0x0210
#       define RADEON_CRTC_CRNT_VLINE_MASK  (0x7ff << 16)
#define RADEON_CRTC2_CRNT_FRAME             0x0314
#define RADEON_CRTC2_DEBUG                  0x031c
#define RADEON_CRTC2_GUI_TRIG_VLINE         0x0318
#define RADEON_CRTC2_STATUS                 0x03fc
#define RADEON_CRTC2_VLINE_CRNT_VLINE       0x0310

#define RADEON_CUR_CLR0                     0x026c
#define RADEON_CUR_CLR1                     0x0270
#define RADEON_CUR_HORZ_VERT_OFF            0x0268
#define RADEON_CUR_HORZ_VERT_POSN           0x0264
#define RADEON_CUR_OFFSET                   0x0260
#       define RADEON_CUR_LOCK              (1 << 31)
#define RADEON_CUR2_CLR0                    0x036c
#define RADEON_CUR2_CLR1                    0x0370
#define RADEON_CUR2_HORZ_VERT_OFF           0x0368
#define RADEON_CUR2_HORZ_VERT_POSN          0x0364
#define RADEON_CUR2_OFFSET                  0x0360
#       define RADEON_CUR2_LOCK             (1 << 31)

#endif
