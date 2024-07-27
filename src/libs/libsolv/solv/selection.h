/*
 * Copyright (c) 2012, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * selection.h
 * 
 */

#ifndef LIBSOLV_SELECTION_H
#define LIBSOLV_SELECTION_H

#include "pool.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SELECTION_NAME			(1 << 0)
#define SELECTION_PROVIDES		(1 << 1)
#define SELECTION_FILELIST		(1 << 2)
#define SELECTION_CANON			(1 << 3)
#define SELECTION_DOTARCH		(1 << 4)
#define SELECTION_REL			(1 << 5)

#define SELECTION_INSTALLED_ONLY	(1 << 8)
#define SELECTION_GLOB			(1 << 9)
#define SELECTION_FLAT			(1 << 10)
#define SELECTION_NOCASE		(1 << 11)
#define SELECTION_SOURCE_ONLY		(1 << 12)
#define SELECTION_WITH_SOURCE		(1 << 13)

extern int  selection_make(Pool *pool, Queue *selection, const char *name, int flags);
extern void selection_filter(Pool *pool, Queue *sel1, Queue *sel2);
extern void selection_add(Pool *pool, Queue *sel1, Queue *sel2);
extern void selection_solvables(Pool *pool, Queue *selection, Queue *pkgs);

#ifdef __cplusplus
}
#endif

#endif
