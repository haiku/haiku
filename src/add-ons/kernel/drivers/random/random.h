/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, korli@berlios.de
 */
#ifndef _RANDOM_H
#define _RANDOM_H


typedef struct random_module_info {
	status_t (*init)();
	void (*uninit)();
	status_t (*read)(void *_buffer, size_t *_numBytes);
	status_t (*write)(const void *_buffer, size_t *_numBytes);
} random_module_info;


#endif /* _RANDOM_H */
