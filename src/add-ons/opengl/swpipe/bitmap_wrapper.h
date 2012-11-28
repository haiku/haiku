/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Based on gallium gdi winsys
 *
 * Authors:
 * 	Artur Wyszynski <harakash@gmail.com>
 */
#ifndef __BBITMAP_WRAPPER_H__
#define __BBITMAP_WRAPPER_H__


#include <interface/GraphicsDefs.h>
#include <support/SupportDefs.h>


typedef void Bitmap;

#ifdef __cplusplus
extern "C" {
#endif


Bitmap* create_bitmap(int32 width, int32 height, color_space colorSpace);
void copy_bitmap_bits(const Bitmap* bitmap, void* data, int32 length);

void get_bitmap_size(const Bitmap* bitmap, int32* width, int32* height);
color_space get_bitmap_color_space(const Bitmap* bitmap);
int32 get_bitmap_bytes_per_row(const Bitmap* bitmap);
int32 get_bitmap_bits_length(const Bitmap* bitmap);

void delete_bitmap(Bitmap* bitmap);
void dump_bitmap(const Bitmap* bitmap);


#ifdef __cplusplus
}
#endif


#endif /* __BBITMAP_WRAPPER_H__ */
