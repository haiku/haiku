/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_CBUF_H
#define _KERNEL_CBUF_H


#include <OS.h>


typedef struct cbuf cbuf;


#ifdef __cplusplus
extern "C" {
#endif

status_t cbuf_init(void);
cbuf *cbuf_get_chain(size_t len);
cbuf *cbuf_get_chain_noblock(size_t len);
void cbuf_free_chain_noblock(cbuf *buf);
void cbuf_free_chain(cbuf *buf);

size_t cbuf_get_length(cbuf *buf);
void *cbuf_get_ptr(cbuf *buf, size_t offset);
bool cbuf_is_contig_region(cbuf *buf, size_t start, size_t end);

status_t cbuf_memcpy_to_chain(cbuf *chain, size_t offset, const void *_src, size_t len);
status_t cbuf_memcpy_from_chain(void *dest, cbuf *chain, size_t offset, size_t len);

status_t cbuf_user_memcpy_to_chain(cbuf *chain, size_t offset, const void *_src, size_t len);
status_t cbuf_user_memcpy_from_chain(void *dest, cbuf *chain, size_t offset, size_t len);

uint16 cbuf_ones_cksum16(cbuf *chain, size_t offset, size_t len);

cbuf *cbuf_merge_chains(cbuf *chain1, cbuf *chain2);
cbuf *cbuf_duplicate_chain(cbuf *chain, size_t offset, size_t len);

status_t cbuf_truncate_head(cbuf *chain, size_t trunc_bytes);
status_t cbuf_truncate_tail(cbuf *chain, size_t trunc_bytes);

void cbuf_test(void);
	// ToDo: to be removed...

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_CBUF_H */
