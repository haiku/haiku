/*
 * Copyright 1999  Erdi Chen
 */


#include "datatype.h"
#include "s3mmio.h"
#include "s3accel.h"
#include "s3drv.h"


#define	MIX_MASK			0x000f

#define	MIX_NOT_DST			0x0000
#define	MIX_0				0x0001
#define	MIX_1				0x0002
#define	MIX_DST				0x0003
#define	MIX_NOT_SRC			0x0004
#define	MIX_XOR				0x0005
#define	MIX_XNOR			0x0006
#define	MIX_SRC				0x0007
#define	MIX_NAND			0x0008
#define	MIX_NOT_SRC_OR_DST		0x0009
#define	MIX_SRC_OR_NOT_DST		0x000a
#define	MIX_OR				0x000b
#define	MIX_AND				0x000c
#define	MIX_SRC_AND_NOT_DST		0x000d
#define	MIX_NOT_SRC_AND_DST		0x000e
#define	MIX_NOR				0x000f


byte_t s3alu[16] =
{
    MIX_0,
    MIX_AND,
    MIX_SRC_AND_NOT_DST,
    MIX_SRC,
    MIX_NOT_SRC_AND_DST,
    MIX_DST,
    MIX_XOR,
    MIX_OR,
    MIX_NOR,
    MIX_XNOR,
    MIX_NOT_DST,
    MIX_SRC_OR_NOT_DST,
    MIX_NOT_SRC,
    MIX_NOT_SRC_OR_DST,
    MIX_NAND,
    MIX_1
};


byte_t * __vga_base = 0;
byte_t * __mmio_base = 0;
dword_t  s3accelCmd = 0;


typedef struct
{
    S3DriverContext * s3dc;
    byte_t * mmio_base;
    byte_t * vga_base;
    byte_t * bci_base;
    byte_t * fb_base;
    int    fb_size;
    word_t device_id;
    dword_t global_bd;
    dword_t primary_bd;
    dword_t secondary_bd;
    int    bpp;
    int    logical_width;
    int    logical_height;
} S3AccelContext;

static S3AccelContext S3AC = { 0 };


int s3accel_init (S3DriverContext * s3dc)
{
    byte_t sr1;
    int stride;
    int display_start;

    if (!s3dc) return 0;
    S3AC.s3dc = s3dc;
    S3AC.mmio_base = __mmio_base = S3AC.s3dc->mmio_base;
    S3AC.vga_base = __vga_base = S3AC.s3dc->vga_base;
    S3AC.fb_base = S3AC.s3dc->fb_base;
    S3AC.fb_size = S3AC.s3dc->fb_size;
    S3AC.bpp = S3AC.s3dc->bpp;
    S3AC.logical_width = S3AC.s3dc->logical_width;

    s3drv_unlock_regs ();
    write32 (S3AC.mmio_base + 0x850C, 0x11);
    write32 (S3AC.mmio_base + 0x850C, 0x11);
    S3AC.device_id = read16 (__vga_base + 2);
    sr1 = read_sr (__vga_base, 1);
    write_sr (__vga_base, 1, sr1 | 0x20);

    UpdateGEReg (ALT_CURXY,   0);
    UpdateGEReg (ALT_STEP,    0);
    UpdateGEReg (ERR_TERM,    0);
    UpdateGEReg (SHORT_STROKE,0);
    UpdateGEReg (BKGD_COLOR,  0x00000000);
    UpdateGEReg (FRGD_COLOR,  0xffffffff);
    UpdateGEReg (WRT_MASK,    0xffffffff);
    UpdateGEReg (RD_MASK,     0xffffffff);
    UpdateGEReg (COLOR_CMP,   0x00000000);
    UpdateGEReg (ALT_MIX,     0x00270007);
    UpdateGEReg (SCISSORS_LT, 0x20001000);
    UpdateGEReg (SCISSORS_RB, 0x4fff3fff);
    UpdateGEReg (PIX_CNTL,    0xd000a000);
    UpdateGEReg (MULT_MISC,   0xf000e200);
    UpdateGEReg (ALT_PCNT,    0);

    display_start = S3AC.s3dc->display_start;
    s3drv_dpf ("display_start = %d\n", display_start);
    write32 (__mmio_base + 0x81c0, display_start);
    stride = (S3AC.logical_width * S3AC.bpp + 7) >> 3;
    write32 (__mmio_base + 0x81c8, stride); /* set stride */
    write32 (__mmio_base + 0x8168, display_start);
    S3AC.global_bd = (S3AC.logical_width & 0x1ff0)|(S3AC.bpp<<16)|0x10000001;
    /* Need to write GBD twice after an engine reset */
    write32 (__mmio_base + 0x816c, S3AC.global_bd);
    write32 (__mmio_base + 0x816c, S3AC.global_bd);
    S3AC.s3dc->global_bd = S3AC.global_bd;
    write_sr (__vga_base, 1, sr1);
    s3drv_lock_regs ();
    return 1;
}

int s3accel_wait_for_idle ()
{
    s3drv_wait_for_idle ();
    UpdateGEReg (WRT_MASK,    0xffffffff);
    UpdateGEReg (RD_MASK,     0xffffffff);
    UpdateGEReg (ALT_MIX,     0x00270007);
    UpdateGEReg (SCISSORS_LT, 0x20001000);
    UpdateGEReg (SCISSORS_RB, 0x4fff3fff);
    UpdateGEReg (PIX_CNTL,    0xd000a000);

    return 1;
}

int s3accel_wait_for_fifo (int count)
{
    int i;
    s3drv_wait_for_idle ();
    i = 0;
    return i;
}

__inline__ void s3accel_set_fg_color (dword_t color)
{
    s3accel_wait_for_fifo (1);
    UpdateGEReg (FRGD_COLOR, color);
}

__inline__ void s3accel_set_bg_color (dword_t color)
{
    s3accel_wait_for_fifo (1);
    UpdateGEReg (BKGD_COLOR, color);
}

void s3accel_set_clip_rect (int x1, int y1, int x2, int y2)
{
    s3accel_wait_for_fifo (2);
    UpdateGEReg (SCISSORS_LT, (y1 << 16) | (x1 & 0xffff));
    UpdateGEReg (SCISSORS_RB, (y2 << 16) | (x2 & 0xffff));
}

#if S3ACCEL_NEED_FUNCTIONS

void s3accel_setup_fill_rect (int color, int rop, dword_t planemask)
{
    s3accel_wait_for_fifo (2);
    UpdateGEReg   (WRT_MASK, planemask);
    UpdateGEReg16 (FRGD_MIX, 0x0020 | s3alu[rop]);
    s3drv_dpf ("FRGD_MIX is 0x%4X\n", 0x0020 | s3alu[rop]);
    UpdateGEReg   (FRGD_COLOR, color);
    s3accelCmd = 0x40b1;
}

void s3accel_repeat_fill_rect (int x, int y, int w, int h)
{
    w--; h--;
    s3accel_wait_for_fifo (2);
    UpdateGEReg (ALT_CURXY, (x<<16)|y);
    UpdateGEReg (ALT_PCNT, (w<<16)|h); 
    UpdateGEReg (CMD, 0x000040b1); 
}

void s3accel_fill_rect (int x, int y, int w, int h)
{
    w--; h--;
    UpdateGEReg16 (FRGD_MIX, 0x0027);
    write32 (__mmio_base + ALT_CURXY, (x<<16)|y);
    UpdateGEReg   (ALT_PCNT, (w<<16)|h);
    UpdateGEReg (CMD,  0x000040b1); 
}

__inline__ void s3accel_setup_copy_rect (
    int xdir, int ydir, int rop, dword_t planemask,
    int transparency_color)
{
    s3accelCmd = 0xc011;
    if (xdir == 1) s3accelCmd |= 0x20;
    if (ydir == 1) s3accelCmd |= 0x80;
    s3accel_wait_for_fifo (2);
    UpdateGEReg (WRT_MASK, planemask);
    if (transparency_color != -1)
    {
        UpdateGEReg (COLOR_CMP, transparency_color);
        UpdateGEReg16 (PIX_CNTL, 0xa100);
    }
    UpdateGEReg16 (FRGD_MIX, 0x0060 | s3alu[rop]);
}

__inline__ void s3accel_repeat_copy_rect (
    int src_x, int src_y, int dest_x, int dest_y, int w, int h)
{
    w--; h--;
    if (!(s3accelCmd & 0x20))
    {
        src_x += w; dest_x += w;
    }
    if (!(s3accelCmd & 0x80))
    {
        src_y += h; dest_y += h;
    }

    s3accel_wait_for_fifo (4);
    UpdateGEReg   (ALT_CURXY, (src_x<<16)|src_y); 
    UpdateGEReg   (ALT_STEP, (dest_x<<16)|dest_y);
    UpdateGEReg   (ALT_PCNT, (w<<16)|h);
    UpdateGEReg (CMD, s3accelCmd);
}

void s3accel_copy_rect (int is_patterned, int src_x, int src_y,
                        int dest_x, int dest_y, int w, int h)
{
    int cmd = 0xc0b1;

    w--; h--;
    if (is_patterned)
       cmd |= 0x4000;
    else
    {
        if ((src_x < dest_x) )//&& (src_x + w) > dest_x)
        {
            src_x += w; dest_x += w;
            cmd ^= 32;
        }
        if ((src_y < dest_y) )//&& (src_y + h) > dest_y)
        {
            src_y += h; dest_y += h;
            cmd ^= 128;
        }
    }

    UpdateGEReg16 (FRGD_MIX, 0x0067);
    UpdateGEReg   (ALT_CURXY, (src_x<<16)|src_y);
    UpdateGEReg   (ALT_STEP, (dest_x<<16)|dest_y);
    UpdateGEReg   (ALT_PCNT, (w<<16)|h);
    UpdateGEReg (CMD, cmd);  
}

void s3accel_draw_line (int x1, int y1, int x2, int y2, int no_endpoint)
{
    int cmd = 0x20b1;
    int min, max, err_term, *tmp=&max;

    /*
      max = max(abs(x2-x1),abs(y2-y1));
      min = min(abs(x2-x1),abs(y2-y1));
    */ 
    
    max = x2 - x1;
    min = y2 - y1;

    if (max < 0)
    {
        max = -max;
        cmd ^= 32;
    }
    
    if (min < 0)
    {
        min = -min;
        cmd ^= 128;
    }
    
    if (max < min)
    {
        max = min;
        min = &tmp;
        cmd |= 0x40;
    }
		
    err_term = 2 * min - max;
    if (!(cmd & 0x20)) err_term --;
    if (no_endpoint) cmd |= 4;
    
    s3accel_wait_for_idle ();
    UpdateGEReg16 (FRGD_MIX, 0x0027);
    UpdateGEReg   (ALT_CURXY, (x1<<16)|y1);
    /* Databook says MAJ_AXIS_PCNT = max - 1,
       but MAJ_AXIS_PCNT = max seems to produce correct result. */
    UpdateGEReg16 (MAJ_AXIS_PCNT, max);
    UpdateGEReg   (ALT_STEP, ((2*(min-max))<<16)|(2*min));
    UpdateGEReg   (ERR_TERM, err_term);
    UpdateGEReg (CMD, cmd);
}


void s3accel_repeat_draw_line (
    int x1, int y1, int x2, int y2, dword_t bias)
{
    s3accel_draw_line (x1, y1, x2, y2, bias & 0x100);
}


void s3accel_write_image (int x, int y, int w, int h)
{
    int  count;

    count = (((w * S3AC.bpp) + 31) >> 5) * h;
    w--; h--;
    s3accel_wait_for_idle ();
    UpdateGEReg16 (FRGD_MIX, 0x0047);
    UpdateGEReg   (ALT_CURXY, (x<<16)|y); /* destination */
    UpdateGEReg   (ALT_PCNT, (w<<16)|h);  /* width/height */
    UpdateGEReg (CMD, 0x55b1);

    while (count --) { write32 (__mmio_base, ~0); }
}
#endif /* S3ACCEL_NEED_FUNCTIONS */
