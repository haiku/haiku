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

#include "DocumentBuilder.h"

#include <new>
#include <stdio.h>

#include <Bitmap.h>

#include <agg_bounding_rect.h>

#include "AutoDeleter.h"
#include "GradientTransformable.h"
#include "Icon.h"
#include "PathContainer.h"
#include "Shape.h"
#include "ShapeContainer.h"
#include "StrokeTransformer.h"
#include "Style.h"
#include "StyleContainer.h"
#include "SVGGradients.h"
#include "SVGImporter.h"
#include "VectorPath.h"

using std::nothrow;

namespace agg {
namespace svg {

// constructor
DocumentBuilder::DocumentBuilder()
	: fGradients(20),
	  fCurrentGradient(NULL),
	  fWidth(0),
	  fHeight(0),
	  fViewBox(0.0, 0.0, -1.0, -1.0),
	  fTitle("")
{
}


// remove_all
void
DocumentBuilder::remove_all()
{
	fPathStorage.remove_all();
	fAttributesStorage.remove_all();
	fAttributesStack.remove_all();
	fTransform.reset();
}

// begin_path
void
DocumentBuilder::begin_path()
{
	push_attr();
	unsigned idx = fPathStorage.start_new_path();
	fAttributesStorage.add(path_attributes(cur_attr(), idx));
}

// end_path
void
DocumentBuilder::end_path()
{
	if (fAttributesStorage.size() == 0) {
		throw exception("end_path: The path was not begun");
	}
	path_attributes attr = cur_attr();
	unsigned idx = fAttributesStorage[fAttributesStorage.size() - 1].index;
	attr.index = idx;
	fAttributesStorage[fAttributesStorage.size() - 1] = attr;
	pop_attr();
}

// move_to
void
DocumentBuilder::move_to(double x, double y, bool rel)		  // M, m
{
	if (rel)
		fPathStorage.move_rel(x, y);
	else
		fPathStorage.move_to(x, y);
}

// line_to
void
DocumentBuilder::line_to(double x,  double y, bool rel)		 // L, l
{
	if (rel)
		fPathStorage.line_rel(x, y);
	else
		fPathStorage.line_to(x, y);
}

// hline_to
void
DocumentBuilder::hline_to(double x, bool rel)				   // H, h
{
	if (rel)
		fPathStorage.hline_rel(x);
	else
		fPathStorage.hline_to(x);
}

// vline_to
void
DocumentBuilder::vline_to(double y, bool rel)				   // V, v
{
	if (rel)
		fPathStorage.vline_rel(y);
	else
		fPathStorage.vline_to(y);
}

// curve3
void
DocumentBuilder::curve3(double x1, double y1,				   // Q, q
						double x,  double y, bool rel)
{
	if (rel)
		fPathStorage.curve3_rel(x1, y1, x, y);
	else
		fPathStorage.curve3(x1, y1, x, y);
}

// curve3
void
DocumentBuilder::curve3(double x, double y, bool rel)		   // T, t
{
	if (rel)
		fPathStorage.curve3_rel(x, y);
	else
		fPathStorage.curve3(x, y);
}

// curve4
void
DocumentBuilder::curve4(double x1, double y1,				   // C, c
						double x2, double y2,
						double x,  double y, bool rel)
{
	if (rel) {
		fPathStorage.curve4_rel(x1, y1, x2, y2, x, y);
	} else {
		fPathStorage.curve4(x1, y1, x2, y2, x, y);
	}
}

// curve4
void
DocumentBuilder::curve4(double x2, double y2,				   // S, s
						double x,  double y, bool rel)
{
	if (rel) {
		fPathStorage.curve4_rel(x2, y2, x, y);
	} else {
		fPathStorage.curve4(x2, y2, x, y);
	}
}

// elliptical_arc
void
DocumentBuilder::elliptical_arc(double rx, double ry, double angle,
							    bool large_arc_flag, bool sweep_flag,
							    double x, double y, bool rel)
{
	angle = angle / 180.0 * pi;
	if (rel) {
		fPathStorage.arc_rel(rx, ry, angle, large_arc_flag, sweep_flag, x, y);
	} else {
		fPathStorage.arc_to(rx, ry, angle, large_arc_flag, sweep_flag, x, y);
	}
}

// close_subpath
void
DocumentBuilder::close_subpath()
{
	fPathStorage.end_poly(path_flags_close);
}

// SetTitle
void
DocumentBuilder::SetTitle(const char* title)
{
	fTitle = title;
}

// SetDimensions
void
DocumentBuilder::SetDimensions(uint32 width, uint32 height, BRect viewBox)
{
	fWidth = width;
	fHeight = height;
	fViewBox = viewBox;
}

// cur_attr
path_attributes&
DocumentBuilder::cur_attr()
{
	if (fAttributesStack.size() == 0) {
		throw exception("cur_attr: Attribute stack is empty");
	}
	return fAttributesStack[fAttributesStack.size() - 1];
}

// push_attr
void
DocumentBuilder::push_attr()
{
//printf("DocumentBuilder::push_attr() (size: %d)\n", fAttributesStack.size());
	fAttributesStack.add(fAttributesStack.size() ? fAttributesStack[fAttributesStack.size() - 1]
												 : path_attributes());
}

// pop_attr
void
DocumentBuilder::pop_attr()
{
//printf("DocumentBuilder::pop_attr() (size: %d)\n", fAttributesStack.size());
	if (fAttributesStack.size() == 0) {
		throw exception("pop_attr: Attribute stack is empty");
	}
	fAttributesStack.remove_last();
}

// fill
void
DocumentBuilder::fill(const rgba8& f)
{
	path_attributes& attr = cur_attr();
	attr.fill_color = f;
	attr.fill_flag = true;
}

// stroke
void
DocumentBuilder::stroke(const rgba8& s)
{
	path_attributes& attr = cur_attr();
	attr.stroke_color = s;
	attr.stroke_flag = true;
}

// even_odd
void
DocumentBuilder::even_odd(bool flag)
{
	cur_attr().even_odd_flag = flag;
}

// stroke_width
void
DocumentBuilder::stroke_width(double w)
{
	path_attributes& attr = cur_attr();
	attr.stroke_width = w;
	attr.stroke_flag = true;
}

// fill_none
void
DocumentBuilder::fill_none()
{
	cur_attr().fill_flag = false;
}

// fill_url
void
DocumentBuilder::fill_url(const char* url)
{
	sprintf(cur_attr().fill_url, "%s", url);
}

// stroke_none
void
DocumentBuilder::stroke_none()
{
	cur_attr().stroke_flag = false;
}

// stroke_url
void
DocumentBuilder::stroke_url(const char* url)
{
	sprintf(cur_attr().stroke_url, "%s", url);
}

// opacity
void
DocumentBuilder::opacity(double op)
{
	cur_attr().opacity *= op;
//printf("opacity: %.1f\n", cur_attr().opacity);
}

// fill_opacity
void
DocumentBuilder::fill_opacity(double op)
{
	cur_attr().fill_color.opacity(op);
//	cur_attr().opacity *= op;
}

// stroke_opacity
void
DocumentBuilder::stroke_opacity(double op)
{
	cur_attr().stroke_color.opacity(op);
//	cur_attr().opacity *= op;
}

// line_join
void
DocumentBuilder::line_join(line_join_e join)
{
	cur_attr().line_join = join;
}

// line_cap
void
DocumentBuilder::line_cap(line_cap_e cap)
{
	cur_attr().line_cap = cap;
}

// miter_limit
void
DocumentBuilder::miter_limit(double ml)
{
	cur_attr().miter_limit = ml;
}

// transform
trans_affine&
DocumentBuilder::transform()
{
	return cur_attr().transform;
}

// parse_path
void
DocumentBuilder::parse_path(PathTokenizer& tok)
{
	char lastCmd = 0;
	while(tok.next()) {
		double arg[10];
		char cmd = tok.last_command();
		unsigned i;
		switch(cmd) {
			case 'M': case 'm':
				arg[0] = tok.last_number();
				arg[1] = tok.next(cmd);
				if (lastCmd != cmd)
					move_to(arg[0], arg[1], cmd == 'm');
				else
					line_to(arg[0], arg[1], lastCmd == 'm');
				break;

			case 'L': case 'l':
				arg[0] = tok.last_number();
				arg[1] = tok.next(cmd);
				line_to(arg[0], arg[1], cmd == 'l');
				break;

			case 'V': case 'v':
				vline_to(tok.last_number(), cmd == 'v');
				break;

			case 'H': case 'h':
				hline_to(tok.last_number(), cmd == 'h');
				break;

			case 'Q': case 'q':
				arg[0] = tok.last_number();
				for(i = 1; i < 4; i++) {
					arg[i] = tok.next(cmd);
				}
				curve3(arg[0], arg[1], arg[2], arg[3], cmd == 'q');
				break;

			case 'T': case 't':
				arg[0] = tok.last_number();
				arg[1] = tok.next(cmd);
				curve3(arg[0], arg[1], cmd == 't');
				break;

			case 'C': case 'c':
				arg[0] = tok.last_number();
				for(i = 1; i < 6; i++) {
					arg[i] = tok.next(cmd);
				}
				curve4(arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], cmd == 'c');
				break;

			case 'S': case 's':
				arg[0] = tok.last_number();
				for(i = 1; i < 4; i++) {
					arg[i] = tok.next(cmd);
				}
				curve4(arg[0], arg[1], arg[2], arg[3], cmd == 's');
				break;

			case 'A': case 'a': {
				arg[0] = tok.last_number();
				for(i = 1; i < 3; i++) {
					arg[i] = tok.next(cmd);
				}
				bool large_arc_flag = (bool)tok.next(cmd);
				bool sweep_flag = (bool)tok.next(cmd);
				for(i = 3; i < 5; i++) {
					arg[i] = tok.next(cmd);
				}
				elliptical_arc(arg[0], arg[1], arg[2],
							   large_arc_flag, sweep_flag,
							   arg[3], arg[4], cmd == 'a');
				break;
			}

			case 'Z': case 'z':
				close_subpath();
				break;

			default:
			{
				char buf[100];
				sprintf(buf, "parse_path: Invalid path command '%c'", cmd);
				throw exception(buf);
			}
		}
		lastCmd = cmd;
	}
}

// #pragma mark -

// GetIcon
status_t
DocumentBuilder::GetIcon(Icon* icon, SVGImporter* importer,
						 const char* fallbackName)
{
	double xMin;
	double yMin;
	double xMax;
	double yMax;

	int32 pathCount = fAttributesStorage.size();

	agg::conv_transform<agg::path_storage> transformedPaths(
		fPathStorage, fTransform);
	agg::bounding_rect(transformedPaths, *this, 0, pathCount,
					   &xMin, &yMin, &xMax, &yMax);

	xMin = floor(xMin);
	yMin = floor(yMin);
	xMax = ceil(xMax);
	yMax = ceil(yMax);

	BRect bounds;
	if (fViewBox.IsValid()) {
		bounds = fViewBox;
printf("view box: ");
bounds.PrintToStream();
	} else {
		bounds.Set(0.0, 0.0, (int32)fWidth - 1, (int32)fHeight - 1);
printf("width/height: ");
bounds.PrintToStream();
	}

	BRect boundingBox(xMin, yMin, xMax, yMax);

	if (!bounds.IsValid() || !boundingBox.Intersects(bounds)) {
		bounds = boundingBox;
printf("using bounding box: ");
bounds.PrintToStream();
	}

	float size = min_c(bounds.Width() + 1.0, bounds.Height() + 1.0);
	double scale = 64.0 / size;
printf("scale: %f\n", scale);

	Transformable transform;
	transform.TranslateBy(BPoint(-bounds.left, -bounds.top));
	transform.ScaleBy(B_ORIGIN, scale, scale);

//	if (fTitle.CountChars() > 0)
//		icon->SetName(fTitle.String());
//	else
//		icon->SetName(fallbackName);

	for (int32 i = 0; i < pathCount; i++) {

		path_attributes& attributes = fAttributesStorage[i];

		if (attributes.fill_flag)
			_AddShape(attributes, false, transform, icon);

		if (attributes.stroke_flag)
			_AddShape(attributes, true, transform, icon);
	}

	// clean up styles and paths (remove duplicates)
	int32 count = icon->Shapes()->CountShapes();
	for (int32 i = 1; i < count; i++) {
		Shape* shape = icon->Shapes()->ShapeAtFast(i);
		Style* style = shape->Style();
		if (!style)
			continue;
		int32 styleIndex = icon->Styles()->IndexOf(style);
		for (int32 j = 0; j < styleIndex; j++) {
			Style* earlierStyle = icon->Styles()->StyleAtFast(j);
			if (*style == *earlierStyle) {
				shape->SetStyle(earlierStyle);
				icon->Styles()->RemoveStyle(style);
				style->Release();
				break;
			}
		}
	}

	return B_OK;
}

// StartGradient
void
DocumentBuilder::StartGradient(bool radial)
{
	if (fCurrentGradient) {
		fprintf(stderr, "DocumentBuilder::StartGradient() - ERROR: "
						"previous gradient (%s) not finished!\n",
						fCurrentGradient->ID());
	}

	if (radial)
		fCurrentGradient = new SVGRadialGradient();
	else
		fCurrentGradient = new SVGLinearGradient();

	_AddGradient(fCurrentGradient);
}

// EndGradient
void
DocumentBuilder::EndGradient()
{
	if (fCurrentGradient) {
//		fCurrentGradient->PrintToStream();
	} else {
		fprintf(stderr, "DocumentBuilder::EndGradient() - "
				"ERROR: no gradient started!\n");
	}
	fCurrentGradient = NULL;
}

// #pragma mark -

// _AddGradient
void
DocumentBuilder::_AddGradient(SVGGradient* gradient)
{
	if (gradient) {
		fGradients.AddItem((void*)gradient);
	}
}

// _GradientAt
SVGGradient*
DocumentBuilder::_GradientAt(int32 index) const
{
	return (SVGGradient*)fGradients.ItemAt(index);
}

// _FindGradient
SVGGradient*
DocumentBuilder::_FindGradient(const char* name) const
{
	for (int32 i = 0; SVGGradient* g = _GradientAt(i); i++) {
		if (strcmp(g->ID(), name) == 0)
			return g;
	}
	return NULL;
}


// AddVertexSource
template<class VertexSource>
status_t
AddPathsFromVertexSource(Icon* icon, Shape* shape,
						 VertexSource& source, int32 index)
{
//printf("AddPathsFromVertexSource(pathID = %ld)\n", index);

	// start with the first path
	VectorPath* path = new (nothrow) VectorPath();
	if (!path || !icon->Paths()->AddPath(path)) {
		delete path;
		return B_NO_MEMORY;
	}

	if (!shape->Paths()->AddPath(path))
		return B_NO_MEMORY;

	source.rewind(index);
	double x1, y1;
	unsigned cmd = source.vertex(&x1, &y1);
	bool keepGoing = true;
	int32 subPath = 0;
	while (keepGoing) {
		if (agg::is_next_poly(cmd)) {
//printf("next polygon\n");
			if (agg::is_end_poly(cmd)) {
//printf("  end polygon\n");
				path->SetClosed(true);
				subPath++;
			} else {
//printf("  not end polygon\n");
			}

 			if (agg::is_stop(cmd)) {
//printf("  stop = true\n");
 				keepGoing = false;
			} else {
				if (subPath > 0) {
//printf("  new subpath\n");
					path->CleanUp();
					if (path->CountPoints() == 0) {
//printf("  path with no points!\n");
						icon->Paths()->RemovePath(path);
						shape->Paths()->RemovePath(path);
						path->Release();
					}
					path = new (nothrow) VectorPath();
					if (!path || !icon->Paths()->AddPath(path)) {
						delete path;
						return B_NO_MEMORY;
					}
					if (!shape->Paths()->AddPath(path))
						return B_NO_MEMORY;
				}
			}
		}
		switch (cmd) {
			case agg::path_cmd_move_to:
//printf("move to (%.2f, %.2f) (subPath: %ld)\n", x1, y1, subPath);
				if (path->CountPoints() > 0) {
					// cannot MoveTo on a path that has already points!
					path->CleanUp();
					path = new (nothrow) VectorPath();
					if (!path || !icon->Paths()->AddPath(path)) {
						delete path;
						return B_NO_MEMORY;
					}
					if (!shape->Paths()->AddPath(path))
						return B_NO_MEMORY;
				}
				if (!path->AddPoint(BPoint(x1, y1)))
					return B_NO_MEMORY;
				path->SetInOutConnected(path->CountPoints() - 1, false);
				break;

			case agg::path_cmd_line_to:
//printf("line to (%.2f, %.2f) (subPath: %ld)\n", x1, y1, subPath);
				if (!path->AddPoint(BPoint(x1, y1)))
					return B_NO_MEMORY;
				path->SetInOutConnected(path->CountPoints() - 1, false);
				break;

			case agg::path_cmd_curve3: {
				double x2, y2;
				cmd = source.vertex(&x2, &y2);
//printf("curve3 (%.2f, %.2f)\n", x1, y1);
//printf("	   (%.2f, %.2f)\n", x2, y2);

				// convert to curve4 for easier editing
				int32 start = path->CountPoints() - 1;
				BPoint from;
				path->GetPointAt(start, from);

				double cx2 = (1.0/3.0) * from.x + (2.0/3.0) * x1;
				double cy2 = (1.0/3.0) * from.y + (2.0/3.0) * y1;
				double cx3 = (2.0/3.0) * x1 + (1.0/3.0) * x2;
				double cy3 = (2.0/3.0) * y1 + (1.0/3.0) * y2;

				path->SetPointOut(start, BPoint(cx2, cy2));

				if (!path->AddPoint(BPoint(x2, y2)))
					return B_NO_MEMORY;

				int32 end = path->CountPoints() - 1;
				path->SetInOutConnected(end, false);
				path->SetPointIn(end, BPoint(cx3, cy3));
				break;
			}

			case agg::path_cmd_curve4: {
				double x2, y2;
				double x3, y3;
				cmd = source.vertex(&x2, &y2);
				cmd = source.vertex(&x3, &y3);

				if (!path->AddPoint(BPoint(x3, y3)))
					return B_NO_MEMORY;

				int32 start = path->CountPoints() - 2;
				int32 end = path->CountPoints() - 1;

//printf("curve4 [%ld] (%.2f, %.2f) -> [%ld] (%.2f, %.2f) -> (%.2f, %.2f)\n", start, x1, y1, end, x2, y2, x3, y3);

				path->SetInOutConnected(end, false);
				path->SetPointOut(start, BPoint(x1, y1));
				path->SetPointIn(end, BPoint(x2, y2));
				break;
			}
			default:
//printf("unkown command\n");
				break;
		}
		cmd = source.vertex(&x1, &y1);
	}
//path->PrintToStream();
	path->CleanUp();
	if (path->CountPoints() == 0) {
//printf("path with no points!\n");
		icon->Paths()->RemovePath(path);
		shape->Paths()->RemovePath(path);
		path->Release();
	}

	return B_OK;
}


// _AddShape
status_t
DocumentBuilder::_AddShape(path_attributes& attributes, bool outline,
						   const Transformable& transform, Icon* icon)
{
	Shape* shape = new (nothrow) Shape(NULL);
	if (!shape || !icon->Shapes()->AddShape(shape)) {
		delete shape;
		return B_NO_MEMORY;
	}

	if (AddPathsFromVertexSource(icon, shape, fPathStorage, attributes.index) < B_OK)
		printf("failed to convert from vertex source\n");

	shape->multiply(attributes.transform);
	shape->Multiply(transform);

	StrokeTransformer* stroke = NULL;
	if (outline) {
		stroke = new (nothrow) StrokeTransformer(shape->VertexSource());

		if (stroke) {
			stroke->width(attributes.stroke_width);
			stroke->line_cap(attributes.line_cap);
			stroke->line_join(attributes.line_join);
		}

		if (!shape->AddTransformer(stroke)) {
			delete stroke;
			stroke = NULL;
		}
	} else {
//		if (attributes.even_odd_flag)
//			shape->SetFillingRule(FILL_MODE_EVEN_ODD);
//		else
//			shape->SetFillingRule(FILL_MODE_NON_ZERO);
	}


	Gradient* gradient = NULL;
	const char* url = outline ? attributes.stroke_url : attributes.fill_url;
	if (url[0] != 0) {
		if (SVGGradient* g = _FindGradient(url)) {
			gradient = g->GetGradient(shape->Bounds());
		}
	}

	ObjectDeleter<Gradient> gradientDeleter(gradient);

	rgb_color color;

	BGradient::ColorStop* step;
	if (gradient && (step = gradient->ColorAt(0))) {
		color.red		= step->color.red;
		color.green		= step->color.green;
		color.blue		= step->color.blue;
	} else {
		if (outline) {
			color.red	= attributes.stroke_color.r;
			color.green	= attributes.stroke_color.g;
			color.blue	= attributes.stroke_color.b;
			color.alpha = (uint8)(attributes.stroke_color.a * attributes.opacity);
		} else {
			color.red	= attributes.fill_color.r;
			color.green	= attributes.fill_color.g;
			color.blue	= attributes.fill_color.b;
			color.alpha = (uint8)(attributes.fill_color.a * attributes.opacity);
		}
	}

	Style* style = new (nothrow) Style(color);
	if (!style || !icon->Styles()->AddStyle(style)) {
		delete style;
		return B_NO_MEMORY;
	}

	// NOTE: quick hack to freeze all transformations (only works because
	// paths and styles are not used by multiple shapes!!)
//	if (modifiers() & B_COMMAND_KEY) {
		int32 pathCount = shape->Paths()->CountPaths();
		for (int32 i = 0; i < pathCount; i++) {
			VectorPath* path = shape->Paths()->PathAtFast(i);
			path->ApplyTransform(*shape);
		}
		if (gradient)
			gradient->Multiply(*shape);

		if (stroke)
			stroke->width(stroke->width() * shape->scale());

		shape->Reset();
//	}

	if (gradient)
		style->SetGradient(gradient);

	shape->SetStyle(style);

	return B_OK;
}

} // namespace svg
} // namespace agg

