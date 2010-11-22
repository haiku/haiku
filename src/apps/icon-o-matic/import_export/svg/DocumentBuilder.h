/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.2
// Copyright (C) 2002-2004 Maxim Shemanarev (http://www.antigrain.com)
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
//----------------------------------------------------------------------------
// Contact: mcseem@antigrain.com
//		  mcseemagg@yahoo.com
//		  http://www.antigrain.com
//----------------------------------------------------------------------------

#ifndef DOCUMENT_BUILD_H
#define DOCUMENT_BUILD_H

#include <stdio.h>

#include <List.h>
#include <Rect.h>
#include <String.h>

#include <agg_array.h>
#include <agg_color_rgba.h>
#include <agg_conv_transform.h>
#include <agg_conv_stroke.h>
#include <agg_conv_contour.h>
#include <agg_conv_curve.h>
#include <agg_path_storage.h>
#include <agg_rasterizer_scanline_aa.h>

#include "IconBuild.h"
#include "PathTokenizer.h"


class SVGImporter;

_BEGIN_ICON_NAMESPACE
	class Icon;
	class Transformable;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE

namespace agg {
namespace svg {

class SVGGradient;

// Basic path attributes
struct path_attributes {

	unsigned		index;
	rgba8			fill_color;
	rgba8			stroke_color;
	double			opacity;
	bool			fill_flag;
	bool			stroke_flag;
	bool			even_odd_flag;
	line_join_e		line_join;
	line_cap_e		line_cap;
	double			miter_limit;
	double			stroke_width;
	trans_affine	transform;

	char			stroke_url[64];
	char			fill_url[64];

	// Empty constructor
	path_attributes() :
		index			(0),
		fill_color		(rgba(0,0,0)),
		stroke_color	(rgba(0,0,0)),
		opacity			(1.0),
		fill_flag		(true),
		stroke_flag		(false),
		even_odd_flag	(false),
		line_join		(miter_join),
		line_cap		(butt_cap),
		miter_limit		(4.0),
		stroke_width	(1.0),
		transform		()
	{
		stroke_url[0] = 0;
		fill_url[0] = 0;
	}

	// Copy constructor
	path_attributes(const path_attributes& attr) :
		index			(attr.index),
		fill_color		(attr.fill_color),
		stroke_color	(attr.stroke_color),
		opacity			(attr.opacity),
		fill_flag		(attr.fill_flag),
		stroke_flag		(attr.stroke_flag),
		even_odd_flag	(attr.even_odd_flag),
		line_join		(attr.line_join),
		line_cap		(attr.line_cap),
		miter_limit		(attr.miter_limit),
		stroke_width	(attr.stroke_width),
		transform		(attr.transform)
	{
		sprintf(stroke_url, "%s", attr.stroke_url);
		sprintf(fill_url, "%s", attr.fill_url);
	}

	// Copy constructor with new index value
	path_attributes(const path_attributes& attr, unsigned idx) :
		index			(idx),
		fill_color		(attr.fill_color),
		stroke_color	(attr.stroke_color),
		fill_flag		(attr.fill_flag),
		stroke_flag		(attr.stroke_flag),
		even_odd_flag	(attr.even_odd_flag),
		line_join		(attr.line_join),
		line_cap		(attr.line_cap),
		miter_limit		(attr.miter_limit),
		stroke_width	(attr.stroke_width),
		transform		(attr.transform)
	{
		sprintf(stroke_url, "%s", attr.stroke_url);
		sprintf(fill_url, "%s", attr.fill_url);
	}
};

class DocumentBuilder {
 public:

	typedef pod_bvector<path_attributes>		attr_storage;

								DocumentBuilder();

			void				remove_all();

	// Use these functions as follows:
	// begin_path() when the XML tag <path> comes ("start_element" handler)
	// parse_path() on "d=" tag attribute
	// end_path() when parsing of the entire tag is done.
			void				begin_path();
			void				parse_path(PathTokenizer& tok);
			void				end_path();

	// The following functions are essentially a "reflection" of
	// the respective SVG path commands.
			void				move_to(double x, double y, bool rel = false);	// M, m
			void				line_to(double x,  double y, bool rel = false);	// L, l
			void				hline_to(double x, bool rel = false);			// H, h
			void				vline_to(double y, bool rel = false);			// V, v
			void				curve3(double x1, double y1,					// Q, q
									   double x,  double y, bool rel = false);
			void				curve3(double x, double y, bool rel = false);	// T, t
			void				curve4(double x1, double y1,					// C, c
									   double x2, double y2,
									   double x,  double y, bool rel = false);
			void				curve4(double x2, double y2,					// S, s
									   double x,  double y, bool rel = false);
			void				elliptical_arc(double rx, double ry,
											   double angle,
											   bool large_arc_flag,
											   bool sweep_flag,
											   double x, double y,
											   bool rel = false);				// A, a
			void				close_subpath();								// Z, z

/*			template<class VertexSource>
			void				add_path(VertexSource& vs,
										 unsigned path_id = 0,
										 bool solid_path = true)
								{
									fPathStorage.add_path(vs, path_id, solid_path);
								}*/

			void				SetTitle(const char* title);
			void				SetDimensions(uint32 width, uint32 height, BRect viewBox);


			// Call these functions on <g> tag (start_element, end_element respectively)
			void				push_attr();
			void				pop_attr();

			// Attribute setting functions.
			void				fill(const rgba8& f);
			void				stroke(const rgba8& s);
			void				even_odd(bool flag);
			void				stroke_width(double w);
			void				fill_none();
			void				fill_url(const char* url);
			void				stroke_none();
			void				stroke_url(const char* url);
			void				opacity(double op);
			void				fill_opacity(double op);
			void				stroke_opacity(double op);
			void				line_join(line_join_e join);
			void				line_cap(line_cap_e cap);
			void				miter_limit(double ml);
			trans_affine&		transform();

/*			// Make all polygons CCW-oriented
			void				arrange_orientations()
			{
				fPathStorage.arrange_orientations_all_paths(path_flags_ccw);
			}*/

			unsigned			operator [](unsigned idx)
	        {
	            fTransform = fAttributesStorage[idx].transform;
	            return fAttributesStorage[idx].index;
	        }

			status_t			GetIcon(Icon* icon,
										SVGImporter* importer,
										const char* fallbackName);

			void				StartGradient(bool radial = false);
			void				EndGradient();
			SVGGradient*		CurrentGradient() const
									{ return fCurrentGradient; }

 private:
			void				_AddGradient(SVGGradient* gradient);
			SVGGradient*		_GradientAt(int32 index) const;
			SVGGradient*		_FindGradient(const char* name) const;
			status_t			_AddShape(path_attributes& attributes,
										  bool outline,
										  const Transformable& transform,
										  Icon* icon);

			path_attributes&	cur_attr();

			path_storage		fPathStorage;
			attr_storage		fAttributesStorage;
			attr_storage		fAttributesStack;

			trans_affine		fTransform;

			BList				fGradients;
			SVGGradient*		fCurrentGradient;

			uint32				fWidth;
			uint32				fHeight;
			BRect				fViewBox;
			BString				fTitle;
};

} // namespace svg
} // namespace agg

#endif // DOCUMENT_BUILD_H
