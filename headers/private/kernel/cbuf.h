/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_CBUF_H
#define _KERNEL_CBUF_H

#include <kernel.h>

#define CBUF_LEN 2048

#define CBUF_FLAG_CHAIN_HEAD 1
#define CBUF_FLAG_CHAIN_TAIL 2

typedef struct cbuf {
	struct cbuf *next;
	size_t len;
	size_t total_len;
	void *data;
	int flags;
	char dat[CBUF_LEN - sizeof(struct cbuf *) - 2*sizeof(int) - sizeof(void *) - sizeof(int)];
} cbuf;

int cbuf_init(void);
cbuf *cbuf_get_chain(size_t len);
cbuf *cbuf_get_chain_noblock(size_t len);
void cbuf_free_chain_noblock(cbuf *buf);
void cbuf_free_chain(cbuf *buf);

size_t cbuf_get_len(cbuf *buf);
void *cbuf_get_ptr(cbuf *buf, size_t offset);
int cbuf_is_contig_region(cbuf *buf, size_t start, size_t end);

int cbuf_memcpy_to_chain(cbuf *chain, size_t offset, const void *_src, size_t len);
int cbuf_memcpy_from_chain(void *dest, cbuf *chain, size_t offset, size_t len);

int cbuf_user_memcpy_to_chain(cbuf *chain, size_t offset, const void *_src, size_t len);
int cbuf_user_memcpy_from_chain(void *dest, cbuf *chain, size_t offset, size_t len);

uint16 cbuf_ones_cksum16(cbuf *chain, size_t offset, size_t len);

cbuf *cbuf_merge_chains(cbuf *chain1, cbuf *chain2);
cbuf *cbuf_duplicate_chain(cbuf *chain, size_t offset, size_t len);

int cbuf_truncate_head(cbuf *chain, size_t trunc_bytes);
int cbuf_truncate_tail(cbuf *chain, size_t trunc_bytes);

void cbuf_test(void);

#endif

