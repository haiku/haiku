/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 graphics driver for BeOS 5.
 * Copyright (c) 2003-2004, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/

#include "bits.h"


/*****************************************************************************/
/*
 * Set bits in a byte pointed by addr; mask must contain 0s at the bits
 * positions to be set and must contain 1s at all other bits; val must
 * contain the values of bits to be set.
 */
__inline void set8(volatile char *addr, char mask, char val)
{
    if (mask == 0)
        *addr = val;
    else
        *addr = (*addr & mask) | (val & ~mask);
}
/*****************************************************************************/
__inline void set16(volatile short *addr, short mask, short val)
{
    if (mask == 0)
        *addr = val;
    else
        *addr = (*addr & mask) | (val & ~mask);
}
/*****************************************************************************/
__inline void set32(volatile int *addr, int mask, int val)
{
    if (mask == 0)
        *addr = val;
    else
        *addr = (*addr & mask) | (val & ~mask);
}
/*****************************************************************************/
__inline void ioSet8(short port, char mask, char val)
{
char current;
    if (mask == 0) {
        __asm__ __volatile__ (
            "movb %0, %%al\n\t"
            "movw %1, %%dx\n\t"
            "outb %%al, %%dx"
            : /* no output */
            : "r"(val), "r"(port)
            : "%eax", "%edx"
        );
    }
    else {
        __asm__ __volatile__ (
            "movw %1, %%dx;"
            "inb %%dx, %%al;"
            "movb %%al, %0"
            : "=r"(current)
            : "r"(port)
            : "%eax", "%edx"
        );
        current = (current & mask) | (val & ~mask);
        __asm__ __volatile__ (
            "movb %0, %%al;"
            "movw %1, %%dx;"
            "outb %%al, %%dx"
            : /* no output */
            : "r"(current), "r"(port)
            : "%eax", "%edx"
        );
    }
}
/*****************************************************************************/
__inline char ioGet8(short port)
{
char current;
    __asm__ __volatile__ (
        "movw %1, %%dx;"
        "inb %%dx, %%al;"
        "movb %%al, %0"
        : "=r"(current)
        : "r"(port)
        : "%eax", "%edx"
    );
    return current;
}
/*****************************************************************************/
