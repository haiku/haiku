/* ===-- crtend.c - Provide .eh_frame --------------------------------------===
 *
 *      	       The LLVM Compiler Infrastructure
 *
 * This file is dual licensed under the MIT and the University of Illinois Open
 * Source Licenses. See LICENSE.TXT for details.
 *
 * ===----------------------------------------------------------------------===
 */

#include <inttypes.h>

const int32_t __EH_FRAME_END__[]
    __attribute__((section(".eh_frame"), used)) = { 0 };

#ifndef CRT_HAS_INITFINI_ARRAY
typedef void (*fp)(void);
const fp __CTOR_END__[]
    __attribute__((section(".ctors"), visibility("hidden"), used)) = { 0 };
const fp __DTOR_END__[]
    __attribute__((section(".dtors"), visibility("hidden"), used)) = { 0 };
#endif
