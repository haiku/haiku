/*
 * Copyright 1999  Erdi Chen
 */

#ifndef __DATA_TYPES_H__
#define __DATA_TYPES_H__


#define TEXT_MODE_SIZE 256*1024
#define TEXT_FONT_SIZE 32*1024
#define DISPLAY_START  1024*0

#define CRTC_INDEX  0x3d4
#define CRTC_DATA   0x3d5
#define SEQ_INDEX   0x3c4
#define SEQ_DATA    0x3c5

typedef unsigned char  byte_t;
typedef unsigned short word_t;
typedef unsigned long  dword_t;

extern byte_t vga_mode_numbers[];

typedef struct
{
    byte_t    col;
    byte_t    row;
    byte_t    height;
    byte_t    buf_size0;
    byte_t    buf_size1;
    byte_t    seq_regs[0x04];
    byte_t    misc_output;
    byte_t    crtc_regs[0x19];
    byte_t    attr_regs[0x14];
    byte_t    graph_regs[0x09];
} VGAMode;

typedef struct
{
    byte_t    col;
    byte_t    row;
    byte_t    height;
    byte_t    buf_size0;
    byte_t    buf_size1;
    byte_t    seq_regs[0x04];
    byte_t    misc_output;
    byte_t    crtc_regs[0x19];
    byte_t    attr_regs[0x14];
    byte_t    graph_regs[0x09];
    byte_t    refresh_rate;
    word_t    pixel_size;
    word_t    pixel_width;
    word_t    pixel_height;
    byte_t    sr12;
    byte_t    sr13;
    byte_t    sr29;
    byte_t    misc_ctrl;
    byte_t    cr43;
    byte_t    cr50;
    byte_t    cr51;
    byte_t    cr5d;
    byte_t    cr5e;
    byte_t    cr5f;
    byte_t    cr66;
    byte_t    cr67;
    byte_t    cr31;
    byte_t    cr58;
    byte_t    cr69;
} SVGAMode;

typedef struct
{
    byte_t    s3mode;
    byte_t    mode_tab0;
    byte_t    mode_tab1;
    byte_t    mode_attr;
    byte_t    cr66;
    byte_t    cr67;
    byte_t    seq4;
    byte_t    patch0;
    byte_t    patch1;
} SVGAModeInfo;

typedef struct
{
    byte_t    crtc0;
    byte_t    crtc1;
    byte_t    rate_attr;
    byte_t    rate_freq;
    byte_t    sr13;
    byte_t    sr12;
    byte_t    sr29;
    byte_t    misc_ctrl;
    byte_t    cr50;
    byte_t    cr5d;
    byte_t    cr5e;
} SVGAPatchTable;


#endif /* __DATA_TYPES_H__ */
