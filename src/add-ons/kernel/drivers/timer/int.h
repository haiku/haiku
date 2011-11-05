/*
 * Copyright 2003-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef __INT_H
#define __INT_H


status_t reserve_io_interrupt_vectors(long count, long startVector);
status_t allocate_io_interrupt_vectors(long count, long *startVector);
void free_io_interrupt_vectors(long count, long startVector);

#endif /* __INT_H */
