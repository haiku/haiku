/*
 * Copyright 1999  Erdi Chen
 */


#include "s3mmio.h"
#include "s3drv.h"
#include "DriverInterface.h"

#include <stdarg.h>
#include <stdio.h>


extern int __s3drv_get_svga_mode (
    SVGAMode * svgamode, PixelTiming * pixel_timing);

static S3DriverContext S3DC =
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

void s3drv_dpf (const char * format, ...)
{
#if DEBUG > 0
    va_list args;
    char buffer[4096];

    va_start (args, format);
    vsprintf (buffer, format, args);
    fputs (buffer, stderr);
    va_end (args);
#endif
}

S3DriverContext * s3drv_get_context ()
{
    return &S3DC;
}

int s3drv_init (byte_t * mb)
{
    if (!mb) return 0;
    S3DC.mmio_base = mb;
    S3DC.vga_base = mb + 0x8000;
    S3DC.device_id = read16 (S3DC.vga_base + 2);
    S3DC.viewport_x = 0;
    S3DC.viewport_y = 0;
    S3DC.display_start = DISPLAY_START;
    write32 (mb + 0x8510, 1);
    return 1;
}


static int __reg_unlock_count = 0;

void s3drv_lock_regs ()
{
    __reg_unlock_count --;
    if (__reg_unlock_count < 1)
    {
        __reg_unlock_count = 0;
        s3_lock_regs (S3DC.vga_base);
    }
}

void s3drv_unlock_regs ()
{
    s3_unlock_regs (S3DC.vga_base);
    __reg_unlock_count ++;
}

int s3drv_detect_vram_mb_size (byte_t * vram_start, int max_mb)
{
    int ram_size_mb;
    byte_t * vga_base = S3DC.vga_base;

    s3drv_unlock_regs ();

    if (vram_start)
    {
        byte_t cr58, cr66;
        byte_t * magic_spot;

        if (!vga_base) return 2;
        if (!vram_start) return 2;

        magic_spot = vram_start + 1024*1024;
        cr58 = read_cr (vga_base, 0x58);
        write_cr (vga_base, 0x58, 0x13);
        cr66 = read_cr (vga_base, 0x66);
        write_cr (vga_base, 0x66, cr66 | 1);
        for (ram_size_mb = 1; ram_size_mb < max_mb; ram_size_mb ++)
        {
            read32 (vga_base);
            write32 (magic_spot, 0xC55CC55C);
            if (read32 (magic_spot) != 0xC55CC55C) break;
            write32 (vram_start, 0);
            write32 (magic_spot, 0xFFFFFFFF);
            if (read32 (vram_start) == 0xFFFFFFFF) break;
            magic_spot += 1024*1024;
        }
        write_cr (vga_base, 0x58, cr58);
        write_cr (vga_base, 0x66, cr66);
    }
    else
    {
        byte_t cr36;
        word_t device;

        device = (read_cr (vga_base, 0x2d) << 8) | read_cr (vga_base, 0x2e);
        cr36 = read_cr (vga_base, 0x36);
        ram_size_mb = 2;
        if (isSavage4Family(device))
        {
            switch (cr36 >> 5)
            {
            case 0:
                ram_size_mb = 2;
                break;
            case 1:
                ram_size_mb = 4;
            case 2:
                ram_size_mb = 8;
                break;
            case 3:
                ram_size_mb = 12;
                break;
            case 4:
                ram_size_mb = 16;
                break;
            case 5:
                ram_size_mb = 32;
                break;
            case 6:
                ram_size_mb = 64;
                break;
            case 7:
                ram_size_mb = 128;
                break;
            case 8:
                ram_size_mb = 256;
                break;
            }
        }
        else
        {
            switch (cr36 >> 6)
            {
            case 0:
                ram_size_mb = 8;
                break;
            case 1:
            case 2:
                ram_size_mb = 4;
                break;
            case 3:
                ram_size_mb = 2;
                break;
            }
        }
    }

    s3drv_lock_regs ();

    return ram_size_mb;
}

int s3drv_is_text_mode ()
{
    return !(read_gr (S3DC.vga_base, 0x06) & 1);
}

int s3drv_save_font (byte_t * vram, byte_t * font_buf, int buf_size)
{
    int i;
    byte_t * font;
    byte_t * plane2;
    if (!font_buf) return 0;
    font = font_buf;
    plane2 = vram + 2;
    *font = *plane2;
    for (i = 0; i < buf_size; i ++) *(font++) = *(plane2+=4);
	return 1;
}

int s3drv_restore_font (byte_t * vram, byte_t * font_buf, int buf_size)
{
    int i;
    byte_t * font;
    byte_t * plane2;
    if (!font_buf) return 0;
    plane2 = vram + 2;
    font = font_buf;
    *plane2 = *font;
    for (i = 0; i < buf_size; i ++) *(plane2+=4) = *(font++);
	return 1;
}

int s3drv_get_mode (PixelTiming * timing, int bpp,
                    byte_t * mode_buf, int buf_size)
{
    SVGAMode * s3mode;
    if (!timing || !mode_buf) return 0;
    if (buf_size < sizeof (SVGAMode)) return - sizeof (SVGAMode);

    s3mode = (SVGAMode*) mode_buf;

    __s3drv_get_svga_mode(s3mode, timing);
    s3mode->pixel_size = bpp;
    s3mode->pixel_width = timing->x.disp;
    s3mode->pixel_height = timing->y.disp;
    s3mode->seq_regs[0] = 0x01;
    s3mode->seq_regs[1] = 0x0F;
    s3mode->seq_regs[2] = 0x00;
    s3mode->seq_regs[3] = 0x0E;
    s3mode->attr_regs[0x10] = 0x01;
    s3mode->attr_regs[0x11] = 0x00;
    s3mode->attr_regs[0x12] = 0x0F;
    s3mode->attr_regs[0x13] = 0x00;
    s3mode->graph_regs[0] = 0x00;
    s3mode->graph_regs[1] = 0x00;
    s3mode->graph_regs[2] = 0x00;
    s3mode->graph_regs[3] = 0x00;
    s3mode->graph_regs[4] = 0x00;
    s3mode->graph_regs[5] = 0x00;
    s3mode->graph_regs[6] = 0x05;
    s3mode->graph_regs[7] = 0x0F;
    s3mode->graph_regs[8] = 0xFF;
    s3mode->misc_ctrl = s3mode->misc_output & 0xc0;
    s3mode->cr31 = 0x0d;
    /* Savage3D and Savage/MX need double horizontal timing for 16bpp */
    s3mode->cr43 = (!isSavage4Family(S3DC.device_id) &&
                     (((bpp + 7)/8) == 2) ) ? 0x80 : 0;
    s3mode->cr50 = 0xC1 | ((((bpp+7)>>3) - 1) << 4);
    s3mode->cr51 =
        (((s3mode->pixel_width * s3mode->pixel_size + 31) >> 6) & 0x300) >> 4;
    s3mode->cr58 = 0x13;
    s3mode->cr66 = 0x89;
    
    switch (bpp)
    {
    case 8:
	s3mode->cr67 = 0x00;
	break;
    case 15:
	s3mode->cr67 = 0x20;
	break;
    case 16:
	s3mode->cr67 = 0x40;
	break;
    case 32:
	s3mode->cr67 = 0xd0;
	break;
    default:
	s3mode->cr67 = 0x00;
    }
    s3mode->cr69 = 0x80;
    return 1;
}

int s3drv_display_is_off ()
{
    return read_sr (S3DC.vga_base, 0x01) & 0x20;
}

void s3drv_turn_display_off ()
{
    write_sr (S3DC.vga_base, 0x01, read_sr (S3DC.vga_base, 0x01) | 0x20);
}

void s3drv_turn_display_on ()
{
    byte_t sr1 = read_sr (S3DC.vga_base, 0x01) & ~0x20;
    s3drv_wait_for_vsync ();
    write_sr (S3DC.vga_base, 0x01, sr1);
}

int s3drv_set_logical_width (int width)
{
    if (width >= S3DC.width)
    {
        int stride;
	width += 15;
	width &= ~15;
	stride = (width * S3DC.bpp + 7) >> 3;
        S3DC.logical_width = width;
	write32 (S3DC.mmio_base + 0x81c8, stride); /* set stride */
	if (S3DC.global_bd)
	{
	    S3DC.global_bd = (S3DC.global_bd & 0xffffe00f) | stride;
	    write32 (S3DC.mmio_base + 0x816c, S3DC.global_bd);
	    write32 (S3DC.mmio_base + 0x816c, S3DC.global_bd);
	}
    }

    return S3DC.logical_width;
}

int s3drv_adjust_viewport (int x, int y)
{
    int  viewport_start;

    if (x < 0 || y < 0) return 0;
    S3DC.viewport_x = x;
    S3DC.viewport_y = y;
    viewport_start = ((y * S3DC.logical_width + x) * S3DC.bpp + 7) / 8;
    viewport_start += S3DC.display_start;
    if (S3DC.bpp == 32)
    {
        S3DC.cursor_adjustment = (viewport_start & 15) / 4;
        viewport_start &= ~15;
    }
    write32 (S3DC.mmio_base + 0x81c0, viewport_start);
    return 1;
}

int s3drv_set_display_start (int offset)
{
    if (offset > -1)
    {
        S3DC.display_start = offset + DISPLAY_START;
	write32 (S3DC.mmio_base + 0x8168, S3DC.display_start);
        s3drv_adjust_viewport (S3DC.viewport_x, S3DC.viewport_y);
    }
    return offset;
}

void s3drv_wait_for_vsync ()
{
    int i;
    if (!s3drv_display_is_off ())
        for (i = 0; i < 1000; i ++)
            if ( (read8 (S3DC.vga_base + 0x3da) & 9) == 9) break;
}

void s3drv_wait_for_idle ()
{
    int i;
    if (isSavage4Family(S3DC.device_id)) for (i = 0; i < 10000000; i ++)
    {
        if ( (read32 (S3DC.mmio_base + 0x48c00) & 0x5e01ffff) == 0x5e000000)
            break;
    }
    else for (i = 0; i < 10000000; i ++)
    {
        if ( (read32 (S3DC.mmio_base + 0x48c00) & 0x005fffff) == 0x005e0000)
            break;
    }
}

int s3drv_set_cursor_color (dword_t fg_color, dword_t bg_color)
{
    read_cr (S3DC.vga_base, 0x45);
    if (S3DC.bpp != 8)
    {
        write_cr (S3DC.vga_base, 0x4a, fg_color >> 16);
        write_cr (S3DC.vga_base, 0x4a, fg_color >> 8);
    }
    write_cr (S3DC.vga_base, 0x4a, fg_color);

    read_cr (S3DC.vga_base, 0x45);
    if (S3DC.bpp != 8)
    {
        write_cr (S3DC.vga_base, 0x4b, bg_color >> 16);
        write_cr (S3DC.vga_base, 0x4b, bg_color >> 8);
    }
    write_cr (S3DC.vga_base, 0x4b, bg_color);

    return 1;
}

/* Assume width and height are byte-aligned. */

int s3drv_load_cursor (
    byte_t * lfb, int offset, int width, int height,
    byte_t * and_mask, byte_t * xor_mask)
{
    int i, x, y, bit_shift;
    byte_t and_buf[512];
    byte_t xor_buf[512];
    byte_t * and_ptr = (byte_t*)and_mask;
    byte_t * xor_ptr = (byte_t*)xor_mask;

    if (!lfb || !and_mask || !xor_mask) return 0;

    bit_shift = width & 7;
    width /= 8;

    if (width != 64 || height != 64)
    {
        and_ptr = (byte_t*)and_buf;
        xor_ptr = (byte_t*)xor_buf;

        for (y = 0; y < height; y ++)
        {
            for (x = 0; x < width; x ++)
            {
                *and_ptr ++ = *and_mask ++;
                *xor_ptr ++ = *xor_mask ++;
            }

            if (bit_shift)
            {
                x ++;
                *and_ptr ++ = 0xFF ^ ((*and_mask ++) & (~(0xFF >> bit_shift)));
                *xor_ptr ++ = (*xor_mask ++) & (~(0xFF >> bit_shift));
            }
    
            for ( ; x < 8; x ++)
            {
                *and_ptr ++ = 0xFF;
                *xor_ptr ++ = 0x00;
            }
        }
    
        for ( ; y < 64; y ++)
        {
            *(dword_t*)and_ptr = ~0; and_ptr += 4;
            *(dword_t*)and_ptr = ~0; and_ptr += 4;
            *(dword_t*)xor_ptr = 0; xor_ptr += 4;
            *(dword_t*)xor_ptr = 0; xor_ptr += 4;
        }

        and_ptr = (byte_t*)and_buf;
        xor_ptr = (byte_t*)xor_buf;
    }
    
    lfb += offset * 1024;
    write_cr (S3DC.vga_base, 0x4d, offset);
    write_cr (S3DC.vga_base, 0x4c, offset >> 8);

    for (i = 0; i < 256; i ++)
    {
#if 0 
        dword_t tmp = (*(word_t*)and_ptr << 16) | *(word_t*)xor_ptr;
        *(dword_t*)lfb = tmp; lfb += 4;
#else
        write16 (lfb, *(word_t*)and_ptr);
        lfb += 2;
        write16 (lfb, *(word_t*)xor_ptr);
        lfb += 2;
#endif
        and_ptr += 2; xor_ptr += 2;
    }

    return 1;
}

void s3drv_move_cursor (int x, int y)
{
    int xoff = 0, yoff = 0;

    x += S3DC.cursor_adjustment;
    if (x < 0)
    {
        xoff = (x < -63) ? 63 : -x;
        x = 0;
    }
    if (y < 0)
    {
        yoff = (y < -63) ? 63 : -y;
        y = 0;
    }

    write_cr (S3DC.vga_base, 0x46, (x & 0xff00) >> 8);
    write_cr (S3DC.vga_base, 0x47, (x & 0xff));
    write_cr (S3DC.vga_base, 0x49, (y & 0xff));
    write_cr (S3DC.vga_base, 0x4e, (xoff));
    write_cr (S3DC.vga_base, 0x4f, (yoff));
    write_cr (S3DC.vga_base, 0x48, (y & 0xff00) >> 8);
}

void s3drv_show_cursor ()
{
    write_cr (S3DC.vga_base, 0x45, read_cr (S3DC.vga_base, 0x45) | 1);
}

void s3drv_hide_cursor ()
{
    write_cr (S3DC.vga_base, 0x45, read_cr (S3DC.vga_base, 0x45) & ~1);
}

void s3drv_set_palette (int index, int red, int green, int blue)
{
    byte_t * vga_base = S3DC.vga_base;
    write8 (vga_base + 0x3c8, index);
    write8 (vga_base + 0x3c9, red);
    write8 (vga_base + 0x3c9, green);
    write8 (vga_base + 0x3c9, blue);
}

void s3drv_get_palette (int index, int *red, int *green, int *blue)
{
    byte_t * vga_base = S3DC.vga_base;
    write8 (vga_base + 0x3c7, index);
    *red = read8 (vga_base + 0x3c9);
    *green = read8 (vga_base + 0x3c9);
    *blue = read8 (vga_base + 0x3c9);
}

int s3drv_save_mode (byte_t * mode_buf, int buf_size)
{
    int i;
    byte_t * vga_base = S3DC.vga_base;
    SVGAMode * mode = (SVGAMode*)(mode_buf);

    if (!mode_buf || buf_size < sizeof (SVGAMode))
    {
        return - sizeof (SVGAMode);
    }

    s3drv_unlock_regs ();
    for (i = 0; i < 4; i ++) mode->seq_regs[i] = read_sr (vga_base, i+1);
    mode->misc_output = read8 (vga_base + 0x3cc);
    for (i = 0; i < 0x19; i ++) mode->crtc_regs[i] = read_cr (vga_base, i);
    for (i = 0; i < 0x14; i ++) mode->attr_regs[i] = read_ar (vga_base, i);
    for (i = 0; i < 0x09; i ++) mode->graph_regs[i] = read_gr (vga_base, i);
    mode->refresh_rate = 0xff;
    mode->sr12 = read_sr (vga_base, 0x12);
    mode->sr13 = read_sr (vga_base, 0x13);
    mode->sr29 = read_sr (vga_base, 0x29);
    mode->misc_ctrl = mode->misc_output & 0xc0;
    mode->cr43 = read_cr (vga_base, 0x43);
    mode->cr50 = read_cr (vga_base, 0x50);
    mode->cr51 = read_cr (vga_base, 0x51);
    mode->cr5d = read_cr (vga_base, 0x5d);
    mode->cr5e = read_cr (vga_base, 0x5e);
    mode->cr5f = read_cr (vga_base, 0x5f);
    mode->cr66 = read_cr (vga_base, 0x66);
    mode->cr67 = read_cr (vga_base, 0x67);
    mode->cr31 = read_cr (vga_base, 0x31);
    mode->cr58 = read_cr (vga_base, 0x58);
    mode->cr69 = read_cr (vga_base, 0x69);
    mode->pixel_width =
        ( ( ((mode->cr5f & 0x0c) << 7) |
            ((mode->cr5d & 0x02) << 7) |
            ( mode->crtc_regs[1] ) ) + 1 ) * 8;
    mode->pixel_height = 1 +
        ( ((mode->cr5e & 0x02) << 9) |
          ((mode->crtc_regs[0x07] & 0x40) << 3) |
          ((mode->crtc_regs[0x07] & 0x02) << 7) |
          ( mode->crtc_regs[0x12] ) );
        
    switch ( (mode->cr67 & 0xf0) >> 4)
    {
    case 0:
    case 1:
        mode->pixel_size = (read_cr (vga_base, 0x3a) & 0x10) ? 8 : 4;
        break;
    case 2:
    case 3:
        mode->pixel_size = 15;
        break;
    case 4:
    case 5:
        mode->pixel_size = 16;
        break;
    case 0xd:
        mode->pixel_size = 32;
        break;
    default:
        mode->pixel_size = 8;
    }
    s3drv_lock_regs ();

    return sizeof (SVGAMode);
}

static word_t crtc_reg_defaults[] =
{
    0x4838, 0xa539, 0x0531, 0x4032,  0x0833, 0x0034, 0x0035, 0x053a,
    0x103c, 0x0040, 0x0042, 0x0043,  0x0045, 0x0050, 0x0051, 0x0053,
    0x0055, 0x0058, 0x005d, 0x005e,  0x005f, 0x005b, 0x0066, 0x0067,
    0x0069, 0x006a, 0xc071, 0x0090,  
    0x0087, 0x3070, 0xc071, 0x0772,  0x1f73, 0x1f74, 0x1f75, 0x0f76,
    0x1f77, 0x0178, 0x0179, 0x1f7a,  0x1f7b, 0x171c, 0x177d, 0x177e,
    0x0060,
    0x0000
};

static word_t seq_reg_defaults[] =
{
    0x0608, 0x0014, 0x4018, 0x0029,  0x601c,
    0x0000
};

int s3drv_restore_mode (byte_t * mode_buf, int buf_size, int restore_vga)
{
    int i;
    byte_t cr3a;
    byte_t * vga_base = S3DC.vga_base;
    SVGAMode * mode = (SVGAMode*)(mode_buf);
    if (!mode || buf_size < sizeof (SVGAMode))
    {
        return - sizeof (SVGAMode);
    }

    if (mode->pixel_size) S3DC.bpp = mode->pixel_size;
    if (mode->pixel_width) S3DC.width = mode->pixel_width;
    if (mode->pixel_height) S3DC.height = mode->pixel_height;
    if (!S3DC.logical_width) S3DC.logical_width = mode->pixel_width;
    S3DC.display_start = 0;

    s3drv_unlock_regs ();
    write_sr (vga_base, 0x01, read_sr (vga_base, 0x01) | 0x20);
    for (i = 0; crtc_reg_defaults[i] != 0; i ++)
        write16 (vga_base + CRTC_INDEX, crtc_reg_defaults[i]);
    for (i = 0; seq_reg_defaults[i] != 0; i ++)
        write16 (vga_base + SEQ_INDEX, seq_reg_defaults[i]);
    if (restore_vga > 0)
    {
        write_cr (vga_base, 0x11, read_cr (vga_base, 0x11) & 0x7f);
        for (i = 1; i < 4; i ++)
            write_sr (vga_base, i+1, mode->seq_regs[i]);
        for (i = 0; i < 0x0c; i ++)
            write_cr (vga_base, i, mode->crtc_regs[i]);
        for (i = 0x10; i < 0x19; i ++)
            write_cr (vga_base, i, mode->crtc_regs[i]);
        for (i = 0; i < 0x14; i ++)
            write_ar (vga_base, i, mode->attr_regs[i]);
        for (i = 0; i < 0x09; i ++)
            write_gr (vga_base, i, mode->graph_regs[i]);
    }

    write8 (vga_base + 0x3c2, 0x23);
    write_sr (vga_base, 0x15, (mode->misc_ctrl & 0x10) | 2);
    write8 (vga_base + 0x3c2,
            ((mode->misc_output & 0x3f) |
             (mode->misc_ctrl & 0xc0)));
    if ((mode->misc_output & 0x0c) == 0x0c)
    {
        write_sr (vga_base, 0x12, mode->sr12);
	write_sr (vga_base, 0x13, mode->sr13);
	write_sr (vga_base, 0x29, mode->sr29);
    }

    if (mode->refresh_rate != 0)
    {
        write_cr (vga_base, 0x43, mode->cr43);
	write_cr (vga_base, 0x50, mode->cr50);
	write_cr (vga_base, 0x51, mode->cr51);
	write_cr (vga_base, 0x5d, mode->cr5d);
	write_cr (vga_base, 0x5e, mode->cr5e);
	write_cr (vga_base, 0x5e, mode->cr5f);
	write_cr (vga_base, 0x66, mode->cr66);
	write_cr (vga_base, 0x67, mode->cr67);
	write_cr (vga_base, 0x31, mode->cr31);
	write_cr (vga_base, 0x58, mode->cr58);
	write_cr (vga_base, 0x69, mode->cr69);
    }

    /* Turn off hardware graphics cursor */
    
    write_cr (vga_base, 0x45, read_cr (vga_base, 0x45) & ~1);
    cr3a = read_cr (vga_base, 0x3a);
    
    /* Set attribute controller for enhanced mode */
    if (mode->pixel_size < 8) cr3a &= ~0x10;
    else cr3a |= 0x10;
    write_cr (vga_base, 0x3a, cr3a);

    /* RAMDAC mask */
    write8 (vga_base + 0x3c6, 0xff);

    write_sr (vga_base, 0x01, mode->seq_regs[0]);
    s3drv_lock_regs ();

    if (mode->cr66 & 1)
    {
        write32 (S3DC.mmio_base + 0x81c0, S3DC.display_start);
        write32 (S3DC.mmio_base + 0x81c8,   /* set stride */
                 (S3DC.logical_width * S3DC.bpp + 7) >> 3);
        write32 (S3DC.mmio_base + 0x8168, S3DC.display_start);
        /* Need to write GBD twice after an engine reset */
        if (S3DC.global_bd)
        {
            write32 (S3DC.mmio_base + 0x816c, S3DC.global_bd);
            write32 (S3DC.mmio_base + 0x816c, S3DC.global_bd);
        }
    }

    return 1;
}


