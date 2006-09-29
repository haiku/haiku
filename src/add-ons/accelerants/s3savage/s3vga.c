/*
 * Copyright 1999  Erdi Chen
 */


#include "datatype.h"
#include "s3drv.h"

#define BASE_FREQ 14.318

static int pll_limits[] =
{
    1, 511,          /* min M, max M */
    1, 127,          /* min N, max N */
    0, 4,            /* min R, max R */
    317500, 635000   /* min VCO, max VCO */
};

typedef struct
{
    word_t h_display_end;
    word_t h_sync_start;
    word_t h_sync_end;
    word_t h_total;
    word_t h_polarity;
    word_t v_display_end;
    word_t v_sync_start;
    word_t v_sync_end;
    word_t v_total;
    word_t v_polarity;
    word_t h_blank_start;
    word_t h_blank_end;
    word_t v_blank_start;
    word_t v_blank_end;
} CharTiming;


static int vga_convert_timing(int pixel_per_char,
                              CharTiming * char_timing,
                              PixelTiming * pixel_timing)
{
    int  i, blank_offset;
    char_timing->h_total = pixel_timing->x.total / pixel_per_char;
    char_timing->h_sync_start = pixel_timing->x.sync_start / pixel_per_char;
    char_timing->h_sync_end = pixel_timing->x.sync_end / pixel_per_char;
    char_timing->h_display_end = pixel_timing->x.disp / pixel_per_char;
#if 0
    char_timing->h_blank_start = char_timing->h_display_end;
    char_timing->h_blank_end = char_timing->h_sync_end + 1;
#else
    blank_offset = (pixel_timing->x.disp * 3 + 100) / 200;
    if (blank_offset < 2) blank_offset = 2;
    char_timing->h_blank_start = char_timing->h_display_end + blank_offset;
    char_timing->h_blank_end = char_timing->h_total - blank_offset;
    i = char_timing->h_blank_end - char_timing->h_blank_start - 127;
    if (i > 0)
    {
        char_timing->h_blank_start += (i + 1) / 2;
        char_timing->h_blank_end -= i / 2;
    }
    char_timing->h_blank_end ++;
#endif
    char_timing->v_total = pixel_timing->y.total;
    char_timing->v_sync_start = pixel_timing->y.sync_start;
    char_timing->v_sync_end = pixel_timing->y.sync_end;
    char_timing->v_display_end = pixel_timing->y.disp;
    char_timing->v_blank_start = char_timing->v_display_end + 7;
    char_timing->v_blank_end = (char_timing->v_total + 7) - 0x10;

    char_timing->h_polarity = (pixel_timing->x.polarity ? 1 : 0) << 6;
    char_timing->v_polarity = (pixel_timing->y.polarity ? 1 : 0) << 7;

    return 1;
}

static int  vga_get_mode (VGAMode * vgamode,
                          CharTiming * char_timing,
                          PixelTiming * pixel_timing)
{
    word_t line_compare;
    word_t double_scan;
    word_t row_width;
    word_t mem_addr_size;
    word_t pixel_per_addr;

    if (!vgamode || !pixel_timing) return 0;

    double_scan = 0;
    line_compare = 0x3ff;
    mem_addr_size = 4;
    pixel_per_addr = 1;
    row_width = 2 * pixel_timing->x.disp / (pixel_per_addr * mem_addr_size);

    char_timing->h_total -= 5;
    char_timing->h_display_end --;
    char_timing->v_total -= 2;
    char_timing->v_display_end --;
    char_timing->v_sync_start --;
    char_timing->v_sync_end --;

    vgamode->misc_output = 3 |
        (!char_timing->h_polarity) << 6 |
        (!char_timing->v_polarity) << 7;

    vgamode->crtc_regs[0x00] = char_timing->h_total & 0xff;
    vgamode->crtc_regs[0x01] = char_timing->h_display_end & 0xff;
    vgamode->crtc_regs[0x02] = char_timing->h_blank_start & 0xff;
    vgamode->crtc_regs[0x03] = 0x80 | (char_timing->h_blank_end & 0x1f);
    vgamode->crtc_regs[0x04] = char_timing->h_sync_start & 0xff;
    vgamode->crtc_regs[0x05] = ((char_timing->h_blank_end & 0x20) << 2) |
        (char_timing->h_sync_end & 0x1f);
    vgamode->crtc_regs[0x06] = char_timing->v_total & 0xff;
    vgamode->crtc_regs[0x07] = ((char_timing->v_total & 0x100) >> 8) |
        ((char_timing->v_display_end & 0x100) >> 7) |
        ((char_timing->v_sync_start & 0x100) >> 6) |
        ((char_timing->v_blank_start & 0x100) >> 5) |
        ((line_compare & 0x100) >> 4) |
        ((char_timing->v_total & 0x200) >> 4) |
        ((char_timing->v_display_end & 0x200) >> 3) |
        ((char_timing->v_sync_start & 0x200) >> 2);
    vgamode->crtc_regs[0x08] = 0;
    vgamode->crtc_regs[0x09] = (double_scan & 0x80) |
        ((char_timing->v_blank_start & 0x200) >> 4) |
        ((line_compare & 0x200) >> 3);
    vgamode->crtc_regs[0x2a] = 0;
    vgamode->crtc_regs[0x0b] = 0;
    vgamode->crtc_regs[0x0c] = 0;
    vgamode->crtc_regs[0x0d] = 0;
    vgamode->crtc_regs[0x0e] = 0xff;
    vgamode->crtc_regs[0x0f] = 0;
    vgamode->crtc_regs[0x10] = char_timing->v_sync_start & 0xff;
    vgamode->crtc_regs[0x11] = (char_timing->v_sync_end & 0x0f) | 0x80;
    vgamode->crtc_regs[0x12] = char_timing->v_display_end & 0xff;
    vgamode->crtc_regs[0x13] = row_width & 0xff;
    vgamode->crtc_regs[0x14] = ((mem_addr_size > 3) ? 0x60 : 0);
    vgamode->crtc_regs[0x15] = char_timing->v_blank_start & 0xff;
    vgamode->crtc_regs[0x16] = char_timing->v_blank_end & 0xff;
    vgamode->crtc_regs[0x17] = ((mem_addr_size == 1) ? 0x40 : 0x08) | 0xa3;
    vgamode->crtc_regs[0x18] = line_compare & 0xff;

    return 1;
}

/* --------------------------------------------------------------
 * 1, 511,          min M, max M
 * 1, 127,          min N, max N
 * 0, 4,            min R, max R
 * 317500, 635000   min VCO, max VCO
 * --------------------------------------------------------------*/
void __s3drv_encode_clock (long freq, int * pll_limits,
                           int * mdiv, int * ndiv, int * pll_r)
{
    double ffreq, ffreq_min, ffreq_max;
    double div, diff, best_diff;
    int m;
    int n, r;
    int best_n = 16 + 2;
    int best_r = 2;
    int best_m = 125 + 2;

    int min_m, max_m;
    int min_n, max_n;
    int min_r, max_r;
    long freq_min, freq_max;

    min_m = pll_limits[0]; max_m = pll_limits[1];
    min_n = pll_limits[2]; max_n = pll_limits[3];
    min_r = pll_limits[4]; max_r = pll_limits[5];
    freq_min = pll_limits[6]; freq_max = pll_limits[7];
    
    ffreq     = freq     / 1000.0 / BASE_FREQ;
    ffreq_min = freq_min / 1000.0 / BASE_FREQ;
    ffreq_max = freq_max / 1000.0 / BASE_FREQ;
    
    if (ffreq < ffreq_min / (1<<max_r))
    {
        ffreq = ffreq_min / (1<<max_r);
    }

    if (ffreq > ffreq_max / (1<<min_r))
    {
        ffreq = ffreq_max / (1 << min_r);
    }

    best_diff = ffreq;

    for (r = min_r; r <= max_r; r ++)
    {
        for (n = min_n + 2; n <= max_n + 2; n ++)
        {
            m = (int)(ffreq * n * (1 << r) + 0.5);
            if (m < min_m + 2 || m > max_m + 2)
                continue;
            div = (double)(m) / (double)(n);

            if ((div >= ffreq_min) &&
                (div <= ffreq_max))
            {
                diff = ffreq - div / (1 << r);
                if (diff < 0.0) 
                    diff = -diff;
                if (diff < best_diff)
                {
                    best_diff = diff;
                    best_m    = m;
                    best_n    = n;
                    best_r    = r;
                }
            }
        }
    }

    *ndiv = best_n - 2;
    *pll_r = best_r;
    *mdiv = best_m - 2;
}

static void __s3drv_convert_clock (SVGAMode * svgamode, dword_t dot_clock_khz)
{
    int  m, n, r;

    __s3drv_encode_clock (dot_clock_khz, & pll_limits[0], &m, &n, &r);
    svgamode->sr12 = (n & 0x3f) | ((r & 0x3) << 6);
    svgamode->sr13 = (m & 0xff);
    svgamode->sr29 = (r & 0x04) | ((m & 0x100) >> 5) | ((n & 0x40) >> 2);
}

int  __s3drv_get_svga_mode(SVGAMode * svgamode,
                           PixelTiming * pixel_timing)
{
    int blank_width, sync_width;
    dword_t dot_clock;
    CharTiming char_timing;

    if (!svgamode || !pixel_timing) return 0;

    vga_convert_timing (8, & char_timing, pixel_timing);
    vga_get_mode ((VGAMode*) svgamode, & char_timing, pixel_timing);
    svgamode->misc_output |= 0x0c;
    dot_clock = pixel_timing->dot_clock_khz;
    if (!dot_clock)
    {
        svgamode->refresh_rate = 60;
        dot_clock = pixel_timing->x.total * pixel_timing->y.total * 60 / 1000;
    }
    else
        svgamode->refresh_rate = dot_clock * 1000 /
            (pixel_timing->x.total * pixel_timing->y.total);
    __s3drv_convert_clock (svgamode, dot_clock);

    blank_width = char_timing.h_blank_end - char_timing.h_blank_start;
    sync_width = char_timing.h_sync_end - char_timing.h_sync_start;
    svgamode->cr5d |= (char_timing.h_total & 0x100) >> 8 |
        (char_timing.h_display_end & 0x100) >> 7 |
        (char_timing.h_blank_start & 0x100) >> 6 |
        (char_timing.h_sync_start & 0x100) >> 4;
    if (blank_width > 32) svgamode->cr5d |= 0x20;
    if (sync_width > 64) svgamode->cr5d |= 0x04;

    svgamode->cr5e = (char_timing.v_total & 0x400) >> 10 |
        (char_timing.v_display_end & 0x400) >> 9 |
        (char_timing.v_blank_start & 0x400) >> 8 |
        (char_timing.v_sync_start & 0x400) >> 6;

    svgamode->cr5f |= (char_timing.h_total & 0x600) >> 9 |
        (char_timing.h_display_end & 0x600) >> 7 |
        (char_timing.h_blank_start & 0x600) >> 5 |
        (char_timing.h_sync_start & 0x600) >> 3;

    return 1;
}
