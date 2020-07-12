/*
 * Copyright 2017, Chế Vũ Gia Hy, cvghy116@gmail.com.
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2008-2014, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2005-2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * This file may be used under the terms of the MIT License.
 */
#ifndef _DEBUG_H_
#define _DEBUG_H_

// #define TRACE_XFS
#ifdef TRACE_XFS
#define TRACE(x...) dprintf("\n\33[34mxfs:\33[0m " x)
#define ASSERT(x) \
	{ if (!(x)) kernel_debugger("xfs: assert failed: " #x "\n"); }
#else
#define TRACE(x...)
#define ASSERT(x)
#endif
#define ERROR(x...) dprintf("\n\33[34mxfs:\33[0m " x)

#endif