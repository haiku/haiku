/*
 * Copyright 2020, Kacper Kasper <kacperkasper@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Copyright 2002-2005 Maxim Shemanarev (http://www.antigrain.com)
 */

#ifndef AGG_COMP_OP_ADAPTER_H
#define AGG_COMP_OP_ADAPTER_H

#include <stdio.h>

#include <agg_pixfmt_rgba.h>

#include "PatternHandler.h"


template<typename CompOp, typename RenBuf>
struct AggCompOpAdapter {
	typedef typename CompOp::color_type color_type;
	typedef typename CompOp::color_type::value_type value_type;

	static void
	blend_pixel(int x, int y,
				const color_type& c,
				uint8 cover, RenBuf* buffer,
				const PatternHandler* pattern)
	{
		value_type* p = buffer->row_ptr(y) + x * sizeof(color_type);
		rgb_color color = pattern->ColorAt(x, y);
		CompOp::blend_pix(p,
			color.red, color.green, color.blue, color.alpha, cover);
	}

	static void
	blend_hline(int x, int y,
				unsigned len,
				const color_type& c,
				uint8 cover, RenBuf* buffer,
				const PatternHandler* pattern)
	{
		value_type* p = buffer->row_ptr(y) + x * sizeof(color_type);
		do {
			rgb_color color = pattern->ColorAt(x, y);
			CompOp::blend_pix(p,
				color.red, color.green, color.blue, color.alpha, cover);
			x++;
			p += sizeof(color_type) / sizeof(value_type);
		} while(--len);
	}

	static void
	blend_solid_hspan(int x, int y,
					  unsigned len,
					  const color_type& c,
					  const uint8* covers, RenBuf* buffer,
					  const PatternHandler* pattern)
	{
		value_type* p = buffer->row_ptr(y) + x * sizeof(color_type);
		do {
			rgb_color color = pattern->ColorAt(x, y);
			CompOp::blend_pix(p,
				color.red, color.green, color.blue, color.alpha, *covers);
			covers++;
			p += sizeof(color_type) / sizeof(value_type);
			x++;
		} while(--len);
	}

	static void
	blend_solid_hspan_subpix(int x, int y,
							 unsigned len,
							 const color_type& c,
							 const uint8* covers, RenBuf* buffer,
							 const PatternHandler* pattern)
	{
		fprintf(stderr,
			"B_ALPHA_COMPOSITE_* subpixel drawing not implemented\n");
	}

	static void
	blend_solid_vspan(int x, int y,
					  unsigned len,
					  const color_type& c,
					  const uint8* covers, RenBuf* buffer,
					  const PatternHandler* pattern)
	{
		value_type* p = buffer->row_ptr(y) + x * sizeof(color_type);
		do {
			rgb_color color = pattern->ColorAt(x, y);
			CompOp::blend_pix(p,
				color.red, color.green, color.blue, color.alpha, *covers);
			covers++;
			p += buffer->stride();
			y++;
		} while(--len);
	}

	static void
	blend_color_hspan(int x, int y,
					  unsigned len,
					  const color_type* colors,
					  const uint8* covers,
					  uint8 cover, RenBuf* buffer,
					  const PatternHandler* pattern)
	{
		value_type* p = buffer->row_ptr(y) + x * sizeof(color_type);
		do {
			CompOp::blend_pix(p,
				colors->r, colors->g, colors->b, colors->a,
				covers ? *covers++ : cover);
			p += sizeof(color_type) / sizeof(value_type);
			++colors;
		} while(--len);
	}
};


#endif // AGG_COMP_OP_ADAPTER_H
