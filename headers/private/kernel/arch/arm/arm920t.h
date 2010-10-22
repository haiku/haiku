/*
 * Copyright (c) 2008 Travis Geiselbrecht
 * Copyright (c) 2009 François Revol, revol@free.fr
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __PLATFORM_ARM920T_H
#define __PLATFORM_ARM920T_H

#define SDRAM_BASE 0x30000000

#define VECT_BASE 0x40200000
#define VECT_SIZE 0x10000

#define DEVICE_BASE 0x48000000
#define DEVICE_SIZE 0x2000000

/* framebuffer */
#define FB_BASE 0x88000000
#define FB_SIZE 0x200000

/* UART */
#define UART0_BASE    0x50000000
#define UART1_BASE    0x50004000
#define UART2_BASE    0x50008000

#endif /* __PLATFORM_ARM920T_H */
