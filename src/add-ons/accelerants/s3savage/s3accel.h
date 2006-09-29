/*
 * Copyright 1999  Erdi Chen
 */


#ifndef __S3ACCEL_H__
#define __S3ACCEL_H__


#include "s3drv.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    ALT_CURXY    = 0x8100,
    CUR_Y        = 0x8100,
    CUR_X        = 0x8102,
    ALT_STEP     = 0x8108,
    DESTY_AXSTP  = 0x8108,
    DESTX_DIASTP = 0x810a,
    ERR_TERM     = 0x8110,
    CMD          = 0x8118,
    SHORT_STROKE = 0x811c,
    BKGD_COLOR   = 0x8120,
    FRGD_COLOR   = 0x8124,
    WRT_MASK     = 0x8128,
    RD_MASK      = 0x812c,
    COLOR_CMP    = 0x8130,
    ALT_MIX      = 0x8134,
    BKGD_MIX     = 0x8134,
    FRGD_MIX     = 0x8136,
    SCISSORS_LT  = 0x8138,
    SCISSORS_T   = 0x8138,
    SCISSORS_L   = 0x813a,
    SCISSORS_RB  = 0x813c,
    SCISSORS_B   = 0x813c,
    SCISSORS_R   = 0x813e,
    PIX_CNTL     = 0x8140,
    MULT_MISC2   = 0x8142,
    MULT_MISC    = 0x8144,
    READ_SEL     = 0x8144,
    ALT_PCNT     = 0x8148,
    MIN_AXIS_PCNT = 0x8148,
    MAJ_AXIS_PCNT = 0x814a
} S3GERegister;

extern byte_t s3alu[16];
extern byte_t * __vga_base;
extern byte_t * __mmio_base;


#define UpdateGEReg(reg, val)\
    write32 (__mmio_base + reg, val);\

#define UpdateGEReg16(reg, val)\
    write16 (__mmio_base + reg, val);\


/*
 * Function prototypes.
 */
extern int s3accel_init (S3DriverContext * s3dc);
extern int s3accel_wait_for_idle ();
extern int s3accel_wait_for_fifo (int count);
extern void s3accel_set_pattern ();
extern void s3accel_set_fg_color (dword_t fg_color);
extern void s3accel_set_bg_color (dword_t bg_color);
extern void s3accel_set_clip_rect (int x1, int y1, int x2, int y2);

extern void s3accel_setup_copy_rect (
    int xdir, int ydir, int rop,
    dword_t planemask, int transparency_color);
extern void s3accel_repeat_copy_rect (
    int x1, int y1, int x2, int y2, int w, int h);
extern void s3accel_copy_rect (int is_patterned, int src_x, int src_y,
			       int dest_x, int dest_y, int w, int h);

extern void s3accel_setup_fill_rect (int color, int rop, dword_t planemask);
extern void s3accel_repeat_fill_rect (int x, int y, int w, int h);
extern void s3accel_fill_rect (int x, int y, int w, int h);

extern void s3accel_fill_polygon ();
extern void s3accel_setup_fill_polygon ();
extern void s3accel_repeat_fill_polygon (int count, short * xw);

extern void s3accel_draw_line (int x1, int y1, int x2, int y2,
                               int no_endpoint);
extern void s3accel_draw_polyline (int count, short * xy);
extern void s3accel_draw_segments (int count, short * xy);
extern void s3accel_setup_draw_line ();
extern void s3accel_repeat_draw_line (int x1, int y1, int x2, int y2,
                                      dword_t bias);

extern void s3accel_draw_scanline ();
extern void s3accel_setup_draw_scanline ();
extern void s3accel_repeat_draw_scanline (int x, int width);
extern void s3accel_draw_multi_scanlines (int count, int starty, short * xw);

extern void s3accel_write_image (int x, int y, int w, int h);
extern void s3accel_write_bitmap ();


#ifdef __cplusplus
}
#endif


#endif /* __S3ACCEL_H__ */
