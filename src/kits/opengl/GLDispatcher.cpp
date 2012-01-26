/*
 * Copyright 2000-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Paul <brian.e.paul@gmail.com>
 */


extern "C" {
#include "glapi.h"
#if __GNUC__ > 2
// New Mesa
#include "glapi_priv.h"
#include "glapitable.h"
#endif


/*
 * NOTE: this file portion implements C-based dispatch of the OpenGL entrypoints
 * (glAccum, glBegin, etc).
 * This code IS NOT USED if we're compiling on an x86 system and using
 * the glapi_x86.S assembly code.
 */
#if !(defined(USE_X86_ASM) || defined(USE_SPARC_ASM))

#define KEYWORD1 PUBLIC
#define KEYWORD2
#define NAME(func) gl##func

#define DISPATCH(func, args, msg)					\
	const struct _glapi_table *dispatch;					\
	dispatch = _glapi_Dispatch ? _glapi_Dispatch : _glapi_get_dispatch();\
	(dispatch->func) args

#define RETURN_DISPATCH(func, args, msg) 				\
	const struct _glapi_table *dispatch;					\
	dispatch = _glapi_Dispatch ? _glapi_Dispatch : _glapi_get_dispatch();\
	return (dispatch->func) args

#endif
}


/* NOTE: this file portion implement a thin OpenGL entrypoints dispatching
	C++ wrapper class
 */

#include "GLDispatcher.h"

BGLDispatcher::BGLDispatcher()
{
}


BGLDispatcher::~BGLDispatcher()
{
}


status_t
BGLDispatcher::CheckTable(const struct _glapi_table *table)
{
	_glapi_check_table(table ? table : _glapi_get_dispatch());
	return B_OK;
}


status_t
BGLDispatcher::SetTable(struct _glapi_table *table)
{
	_glapi_set_dispatch(table);
	return B_OK;
}
