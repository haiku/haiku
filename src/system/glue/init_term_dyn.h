/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef INIT_TERM_DYN_H
#define INIT_TERM_DYN_H


#include <image.h>


void _init_before(image_id id);
void _init_after(image_id id);
void _term_before(image_id id);
void _term_after(image_id id);

#endif	/* INIT_TERM_DYN_H */
