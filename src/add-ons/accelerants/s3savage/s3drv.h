/*
 * Copyright 1999  Erdi Chen
 */


#ifndef __S3DRV_H__
#define __S3DRV_H__


#include "datatype.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    word_t disp;
    word_t sync_start;
    word_t sync_end;
    word_t total;
    word_t polarity;
} CrtcTiming;

typedef struct
{
    dword_t dot_clock_khz;
    CrtcTiming x;
    CrtcTiming y;
} PixelTiming;

typedef struct
{
    byte_t *  mmio_base;
    byte_t *  vga_base;
    byte_t *  rom_base;
    byte_t *  fb_base;
    word_t    device_id;
    int  fb_size;
    int  vram_size;
    dword_t bpp;
    dword_t width;
    dword_t height;
    dword_t logical_width;
    dword_t display_start;
    dword_t global_bd;
    word_t viewport_x;
    word_t viewport_y;
    int    cursor_adjustment;
} S3DriverContext;


extern void s3drv_dpf (const char * format, ...);
extern S3DriverContext * s3drv_get_context ();
extern int s3drv_init (byte_t * mmio_base);
extern void s3drv_lock_regs ();
extern void s3drv_unlock_regs ();
extern int s3drv_detect_vram_mb_size (
    byte_t * vram_start, int max_mb);

extern int s3drv_is_text_mode ();
extern int s3drv_save_font (
    byte_t * vram, byte_t * font_buf, int buf_size);
extern int s3drv_restore_font (
    byte_t * vram, byte_t * font_buf, int buf_size);
extern int s3drv_get_mode (
    PixelTiming * timing, int bpp,
    byte_t * mode_buf, int buf_size);
#if 0
extern int s3drv_get_gtf_mode (
    int width, int height, int depth,
    int rrate, byte_t * mode_buf,
    int buf_size);
#endif
extern int s3drv_get_bios_mode (
    int vesamode, int refresh_rate,
    byte_t * mode_buf, int buf_size,
    byte_t * rom);
extern int s3drv_save_mode (
    byte_t * mode_buf, int buf_size);
extern int s3drv_restore_mode (
    byte_t * mode_buf, int buf_size,
    int restore_vga);

extern int s3drv_display_is_off ();
extern void s3drv_turn_display_off ();
extern void s3drv_turn_display_on ();

extern int s3drv_set_logical_width (int width);
extern int s3drv_adjust_viewport (int x, int y);
extern int s3drv_set_display_start (int offset);

extern void s3drv_wait_for_vsync ();
extern void s3drv_wait_for_idle ();

extern int s3drv_set_cursor_color (
    dword_t foreground, dword_t background);
extern int s3drv_load_cursor (
    byte_t * lfb, int offset, int width, int height,
    byte_t * image, byte_t * mask);
extern void s3drv_move_cursor (int x, int y);
extern void s3drv_show_cursor ();
extern void s3drv_hide_cursor ();

extern void s3drv_set_palette (
    int index, int red,
    int green, int blue);
extern void s3drv_get_palette (
    int index, int *red,
    int *green, int *blue);

#ifdef __cplusplus
}
#endif


#endif /* __S3DRV_H__ */
