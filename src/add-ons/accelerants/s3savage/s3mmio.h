/*
 * Copyright 1999  Erdi Chen
 */


#ifndef __S3MMIO_H__
#define __S3MMIO_H__

#include "datatype.h"

#ifdef __STRICT_ANSI__

#undef  __inline__
#define __inline__
#define read8(addr) (*(volatile byte_t*) (addr))
#define read16(addr) (*(volatile word_t*) (addr))
#define read32(addr) (*(volatile dword_t*) (addr))
#define write8(addr, val) ((*(volatile byte_t*) (addr)) = (byte_t) (val))
#define write16(addr, val) ((*(volatile word_t*) (addr)) = (word_t) (val))
#define write32(addr, val) ((*(volatile dword_t*) (addr)) = (dword_t) (val))

#else

static __inline__ byte_t read8 (byte_t * addr)
{
    return *(volatile byte_t*) addr;
}

static __inline__ word_t read16 (byte_t * addr)
{
    return *(volatile word_t*) addr;
}

static __inline__ dword_t read32 (byte_t *addr)
{
    return *(volatile dword_t*) addr;
}

static __inline__ void write8 (byte_t * addr, byte_t val)
{
    *(volatile byte_t*) addr = val;
}

static __inline__ void write16 (byte_t * addr, word_t val)
{
    *(volatile word_t*) addr = val;
}

static __inline__ void write32 (byte_t * addr, dword_t val)
{
    *(volatile dword_t*) addr = val;
}

#endif /* __STRICT_ANSI__ */


static __inline__ byte_t read_sr (byte_t * vga_base, byte_t reg)
{
    write8 (vga_base + SEQ_INDEX, reg);
    return read8 (vga_base + SEQ_DATA);
}

static __inline__ void write_sr (byte_t * vga_base, byte_t reg, byte_t val)
{
    write16 (vga_base + SEQ_INDEX, (val << 8) | reg);
}

static __inline__ byte_t read_cr (byte_t * vga_base, byte_t reg)
{
    write8 (vga_base + CRTC_INDEX, reg);
    return read8 (vga_base + CRTC_DATA);
}

static __inline__ void write_cr (byte_t * vga_base, byte_t reg, byte_t val)
{
    write16 (vga_base + CRTC_INDEX, (val << 8) | reg);
}

static __inline__ byte_t read_gr (byte_t * vga_base, byte_t reg)
{
    write8 (vga_base + 0x3ce, reg);
    return read8 (vga_base + 0x3cf);
}

static __inline__ void write_gr (byte_t * vga_base, byte_t reg, byte_t val)
{
    write16 (vga_base + 0x3ce, (val << 8) | reg);
}

static __inline__ byte_t read_ar (byte_t * vga_base, byte_t reg)
{
    byte_t value, index;
    read8 (vga_base + 0x3da);
    index = read8 (vga_base + 0x3c0);
    write8 (vga_base + 0x3c0, reg);
    value = read8 (vga_base + 0x3c1);
    read8 (vga_base + 0x3da);
    write8 (vga_base + 0x3c0, index);
    return value;
}

static __inline__ void write_ar (byte_t * vga_base, byte_t reg, byte_t val)
{
    byte_t index;
    read8 (vga_base + 0x3da);
    index = read8 (vga_base + 0x3c0);
    write8 (vga_base + 0x3c0, reg);
    write8 (vga_base + 0x3c0, val);
    write8 (vga_base + 0x3c0, index);
}

static __inline__ void s3_lock_regs (byte_t * vga_base)
{
    write_cr (vga_base, 0x11, read_cr (vga_base, 0x11) | 0x80);
    write_cr (vga_base, 0x33, (0x50 | read_cr (vga_base, 0x33)) & ~2);
    write_cr (vga_base, 0x38, 0x00);
    write_cr (vga_base, 0x39, 0x00);
    write_sr (vga_base, 0x08, 0x00);
}

static __inline__ void s3_unlock_regs (byte_t * vga_base)
{
    write_sr (vga_base, 0x08, 0x06);
    write_cr (vga_base, 0x38, 0x48);
    write_cr (vga_base, 0x39, 0xa0);
    write_cr (vga_base, 0x40, 0x01);
    write_cr (vga_base, 0x11, read_cr (vga_base, 0x11) & 0x7f);
    write_cr (vga_base, 0x33, (~0x50 & read_cr (vga_base, 0x33)) | 2);
    write_cr (vga_base, 0x35, 0x00);
}


#endif /* __S3MMIO_H__ */
