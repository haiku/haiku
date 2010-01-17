/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _SNET_BUFFER_H_
#define _SNET_BUFFER_H_

#include <util/list.h>

/* 
 * This is a simple data structure to hold network buffers.
 * It drops many functionality that the Haiku net_buffer provides.
 *
 * - Inspired by linux sk_buff/bsd mbuf (put/pull)
 * - Contiguoussafe (no push operation)
 *
 * So snet_buffers are ONLY meant to be used when:
 *  1) You know exactily the maximun/final size of the frame 
 *		before allocating it, and you will never exceed it.
 *  2) You are not supposed to prepend data, only append.
 *
 */

/* Configuration parameters */
#define SNB_BUFFER_ATTACHED
//#define SNB_PERFORMS_OVERFLOW_CHECKS
//#define SNB_PERFORMS_POINTER_CHECKS

struct snet_buffer;

typedef struct snet_buffer snet_buffer;

/* Creates a snb_buffer allocating size space for its full content */
snet_buffer* snb_create(uint16 size);
/* Free the snb_buffer*/
void snb_free(snet_buffer* snb);
/* Free the snb_buffer*/
void* snb_get(snet_buffer* snb);
/* Size of the snb_buffer*/
uint16 snb_size(snet_buffer* snb);

/* Cookie of the snb_buffer*/
void* snb_cookie(snet_buffer* snb);
/* Get Cookie of the snb_buffer*/
void snb_set_cookie(snet_buffer* snb, void* cookie);

/* Place the memory given by data to the "tail" of the snb */
void snb_put(snet_buffer* snb, void* data, uint16 size);
/* Returns a header chunk of size data */
void* snb_pull(snet_buffer* snb, uint16 size);
/* Discards all data put or pulled from the buffer */
void snb_reset(snet_buffer* snb);

/* Return true if we canot "put" more data in the buffer */
bool snb_completed(snet_buffer* snb);
/* Return true if we cannot pull more more data from the buffer */
bool snb_finished(snet_buffer* snb);
/* Return the amount of data we can still put in the buffer */
uint16 snb_remaining_to_put(snet_buffer* snb);
/* Return the amount of data we can still pull in the buffer */
uint16 snb_remaining_to_pull(snet_buffer* snb);

/* These to functions are provided to avoid memory fragmentation 
 * allocating and freeing many snb_buffers and its possible overhead. 
 * Thypical scenario would be
 * that you create a snb_buffer to send data, once you send you free it,
 * and need another one to hold the response. The idea would be once you send 
 * that buffer, to snb_park the buffer, and whenever you need to allocate 
 * another one snb_fetch it. That funcion will reuse most appropiated
 * previous used one snb_buff by its memory use.
 */
void			snb_park(struct list* l, snet_buffer* snb);
snet_buffer*	snb_fetch(struct list* l, uint16 size);
uint16			snb_packets(struct list* l);

void			snb_dump(snet_buffer* snb);

#endif
