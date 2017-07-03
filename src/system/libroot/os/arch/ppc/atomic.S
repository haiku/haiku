/*
 * Copyright 2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 */

#include <asm_defs.h>

.text

#ifdef __powerpc__
#warning IMPLEMENT GCC 64-bit ATOMICS ON POWERPC 32-bit!

/* These are to fill in 64-bit atomic calls emitted by
 * by GCC when 64-bit atomics are unavailable.
 * (aka, on 32-bit PowerPC
 */

FUNCTION(__atomic_fetch_add_8):
	sync
	blr
FUNCTION_END(__atomic_fetch_add_8)

FUNCTION(__atomic_store_8):
	sync
	blr
FUNCTION_END(__atomic_store_8)

FUNCTION(__atomic_load_8):
	sync
	blr
FUNCTION_END(__atomic_load_8)

#endif /* __powerpc__ */
