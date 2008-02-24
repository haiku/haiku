/*
 * Copyright 2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SEARCH_H_
#define _SEARCH_H_

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


extern int hcreate(size_t);
extern void hdestroy(void);
extern ENTRY *hsearch(ENTRY, ACTION);
extern void insque(void *, void *);
extern void *lfind(const void *, const void *, size_t *,
	size_t, int (*)(const void *, const void *));
extern void  *lsearch(const void *, void *, size_t *,
	size_t, int (*)(const void *, const void *));
extern void remque(void *);
extern void *tdelete(const void *restrict, void **restrict,
	int(*)(const void *, const void *));
extern void *tfind(const void *, void *const *,
	int(*)(const void *, const void *));
extern void *tsearch(const void *, void **,
	int(*)(const void *, const void *));
extern void twalk(const void *,
	void (*)(const void *, VISIT, int ));

#endif /* _SEARCH_H_ */

