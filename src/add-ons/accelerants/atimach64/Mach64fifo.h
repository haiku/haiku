/* $XConsortium: mach64fifo.h /main/1 1996/10/27 18:06:29 kaleb $ */
/*
 * Copyright 1996 by Kevin E. Martin, Chapel Hill, North Carolina.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Kevin
 * E. Martin not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Kevin E. Martin makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * KEVIN E. MARTIN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.  */

/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/mach64/mach64fifo.h,v 3.1 1996/12/27 06:55:52 dawes Exp $ */

extern int mach64FIFOdepthDefault(
#if NeedFunctionPrototypes
    int cdepth,
    int clock,
    int width
#endif
);

extern int mach64FIFOdepthVTA3(
#if NeedFunctionPrototypes
    int cdepth,
    int clock,
    int width
#endif
);

extern int mach64FIFOdepthVTA4(
#if NeedFunctionPrototypes
    int cdepth,
    int clock,
    int width
#endif
);

extern int mach64FIFOdepthGT(
#if NeedFunctionPrototypes
    int cdepth,
    int clock,
    int width
#endif
);

extern int mach64FIFOdepthCT(
#if NeedFunctionPrototypes
    int cdepth,
    int clock,
    int width
#endif
);

extern int mach64FIFOdepthCTD(
#if NeedFunctionPrototypes
    int cdepth,
    int clock,
    int width
#endif
);
