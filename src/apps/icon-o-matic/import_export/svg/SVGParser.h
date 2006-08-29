/*
 * Copyright 2006, Haiku. All rights reserved.
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

#ifndef SVG_PARSER_H
#define SVG_PARSER_H

#include "PathTokenizer.h"
#include "DocumentBuilder.h"

namespace agg {
namespace svg {

class Parser {
	enum { buf_size = BUFSIZ };
 public:

								Parser(DocumentBuilder& builder);
	virtual						~Parser();

			void				parse(const char* pathToFile);
			const char*			title() const
									{ return fTitle; }

			bool				TagsIgnored() const
									{ return fTagsIgnored; }

 private:
	// XML event handlers
	static	void				start_element(void* data, const char* el,
											  const char** attr);
	static	void				end_element(void* data, const char* el);
	static	void				content(void* data, const char* s, int len);

			void				parse_svg(const char** attr);
			void				parse_attr(const char** attr);
			void				parse_path(const char** attr);
			void				parse_poly(const char** attr, bool close_flag);
			void				parse_circle(const char** attr);
			void				parse_ellipse(const char** attr);
			void				parse_rect(const char** attr);
			void				parse_line(const char** attr);
			void				parse_style(const char* str);
			trans_affine		parse_transform(const char* str);

			void				parse_gradient(const char** attr, bool radial);
			void				parse_gradient_stop(const char** attr);
		
			unsigned			parse_matrix(const char* str, trans_affine& transform);
			unsigned			parse_translate(const char* str, trans_affine& transform);
			unsigned			parse_rotate(const char* str, trans_affine& transform);
			unsigned			parse_scale(const char* str, trans_affine& transform);
			unsigned			parse_skew_x(const char* str, trans_affine& transform);
			unsigned			parse_skew_y(const char* str, trans_affine& transform);
			
			bool				parse_attr(const char* name,
										   const char* value);
			bool				parse_name_value(const char* nv_start,
												 const char* nv_end);
			void				copy_name(const char* start, const char* end);
			void				copy_value(const char* start, const char* end);
	
private:
			DocumentBuilder&	fBuilder;
			PathTokenizer		fPathTokenizer;
			char*				fBuffer;
			char*				fTitle;
			unsigned			fTitleLength;

			bool				fTitleFlag;
			bool				fPathFlag;

			char*				fAttrName;
			char*				fAttrValue;
			unsigned			fAttrNameLength;
			unsigned			fAttrValueLength;

			rgba8				fGradientStopColor;

			bool				fTagsIgnored;
};

} // namespace svg
} // namespace agg

#endif // SVG_PARSER_H
