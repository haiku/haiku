/*
 * Copyright 2008-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SEARCH_H_
#define _SEARCH_H_


#include <sys/types.h>


typedef enum {
	FIND,
	ENTER
} ACTION;

typedef struct entry {
	char *keyr;
	void *data;
} ENTRY;

typedef enum {
	preorder,
	postorder,
	endorder,
	leaf
} VISIT;


#ifdef __cplusplus
extern "C" {
#endif

extern int hcreate(size_t elementCount);
extern void hdestroy(void);
extern ENTRY *hsearch(ENTRY iteam, ACTION action);
extern void insque(void *element, void *insertAfter);
extern void *lfind(const void *key, const void *base, size_t *_elementCount,
	size_t width, int (*compareFunction)(const void *, const void *));
extern void  *lsearch(const void *key, void *base, size_t *_elementCount,
	size_t width, int (*compareFunction)(const void *, const void *));
extern void remque(void *element);
extern void *tdelete(const void *key, void **_root,
	int (*compare)(const void *, const void *));
extern void *tfind(const void *key, void *const *root,
	int (*compare)(const void *, const void *));
extern void *tsearch(const void *key, void **_root,
	int (*compare)(const void *, const void *));
extern void twalk(const void *root,
	void (*action)(const void *, VISIT, int ));

#ifdef __cplusplus
}
#endif

#endif	/* _SEARCH_H_ */
