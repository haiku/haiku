/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2015, Augustin Cavalier <waddlesplash>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Effect from corTeX / Optimum.
 */
#ifndef NEBULA_DRAW_H
#define NEBULA_DRAW_H


void memshset(char* dstParam, int center_shade, int fixed_shade, int half_length);
void mblur(char* srcParam, int nbpixels);


#endif /* NEBULA_DRAW_H */
