/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#define NANOSVG_IMPLEMENTATION
#include "DocumentBuilder.h"

#include <new>
#include <stdio.h>

#include <Bitmap.h>

#include "AutoDeleter.h"
#include "GradientTransformable.h"
#include "Icon.h"
#include "PathContainer.h"
#include "Shape.h"
#include "ShapeContainer.h"
#include "StrokeTransformer.h"
#include "Style.h"
#include "StyleContainer.h"
#include "SVGImporter.h"
#include "VectorPath.h"

#include <agg_math_stroke.h>
#include <agg_trans_affine.h>

using std::nothrow;

// constructor
DocumentBuilder::DocumentBuilder(NSVGimage* source)
	: fWidth(0),
	  fHeight(0),
	  fViewBox(0.0, 0.0, -1.0, -1.0),
	  fTitle(""),
	  fSource(source)
{
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

// #pragma mark -

// GetIcon
status_t
DocumentBuilder::GetIcon(Icon* icon, SVGImporter* importer,
						 const char* fallbackName)
{
	double xMin = 0;
	double yMin = 0;
	double xMax = ceil(fSource->width);
	double yMax = ceil(fSource->height);

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

	float size = max_c(bounds.Width(), bounds.Height());
	double scale = 64.0 / size;
printf("scale: %f\n", scale);

	Transformable transform;
	transform.TranslateBy(BPoint(-bounds.left, -bounds.top));
	transform.ScaleBy(B_ORIGIN, scale, scale);

//	if (fTitle.CountChars() > 0)
//		icon->SetName(fTitle.String());
//	else
//		icon->SetName(fallbackName);

	for (NSVGshape* shape = fSource->shapes; shape != NULL; shape = shape->next) {
		if (shape->fill.type != NSVG_PAINT_NONE)
			_AddShape(shape, false, transform, icon);
		if (shape->stroke.type != NSVG_PAINT_NONE)
			_AddShape(shape, true, transform, icon);
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
				style->ReleaseReference();
				break;
			}
		}
	}

	return B_OK;
}


// AddVertexSource
status_t
AddPathsFromVertexSource(Icon* icon, Shape* shape, NSVGshape* svgShape)
{
//printf("AddPathsFromVertexSource(pathID = %ld)\n", index);

	for (NSVGpath* svgPath = svgShape->paths; svgPath != NULL;
		svgPath = svgPath->next) {
		VectorPath* path = new (nothrow) VectorPath();
		if (!path || !icon->Paths()->AddPath(path)) {
			delete path;
			return B_NO_MEMORY;
		}

		if (!shape->Paths()->AddPath(path))
			return B_NO_MEMORY;

		path->SetClosed(svgPath->closed);

		int pointCount = svgPath->npts;
		float* points = svgPath->pts;

		// First entry in the points list is always a "move" to the path
		// starting point
		if (!path->AddPoint(BPoint(points[0], points[1])))
			return B_NO_MEMORY;
		path->SetInOutConnected(path->CountPoints() - 1, false);

		pointCount--;
		points += 2;

		while (pointCount > 0) {
			BPoint vector1(points[0], points[1]);
			BPoint vector2(points[2], points[3]);
			BPoint endPoint(points[4], points[5]);

			if (!path->AddPoint(endPoint))
				return B_NO_MEMORY;

			int32 start = path->CountPoints() - 2;
			int32 end = path->CountPoints() - 1;

			path->SetInOutConnected(end, false);
			path->SetPointOut(start, vector1);
			path->SetPointIn(end, vector2);

			pointCount -= 3;
			points += 6;
		}
	}

	// FIXME handle closed vs open paths

	return B_OK;
}


// _AddShape
status_t
DocumentBuilder::_AddShape(NSVGshape* svgShape, bool outline,
						   const Transformable& transform, Icon* icon)
{
	Shape* shape = new (nothrow) Shape(NULL);
	if (!shape || !icon->Shapes()->AddShape(shape)) {
		delete shape;
		return B_NO_MEMORY;
	}

	if (AddPathsFromVertexSource(icon, shape, svgShape) < B_OK)
		printf("failed to convert from vertex source\n");

	shape->SetName(svgShape->id);
	shape->Multiply(transform);

	StrokeTransformer* stroke = NULL;
	NSVGpaint* paint = NULL;
	if (outline) {
		stroke = new (nothrow) StrokeTransformer(shape->VertexSource());
		paint = &svgShape->stroke;

		if (stroke) {
			stroke->width(svgShape->strokeWidth);
			switch(svgShape->strokeLineCap) {
				case NSVG_CAP_BUTT:
					stroke->line_cap(agg::butt_cap);
					break;
				case NSVG_CAP_ROUND:
					stroke->line_cap(agg::round_cap);
					break;
				case NSVG_CAP_SQUARE:
					stroke->line_cap(agg::square_cap);
					break;
			}

			switch(svgShape->strokeLineJoin) {
				case NSVG_JOIN_MITER:
					stroke->line_join(agg::miter_join);
					break;
				case NSVG_JOIN_ROUND:
					stroke->line_join(agg::round_join);
					break;
				case NSVG_JOIN_BEVEL:
					stroke->line_join(agg::bevel_join);
					break;
			}
		}

		if (!shape->AddTransformer(stroke)) {
			delete stroke;
			stroke = NULL;
		}
	} else {
		paint = &svgShape->fill;
#if 0 // FIXME filling rule are missing from Shape class
		if (svgShape->fillRule == NSVG_FILLRULE_EVENODD)
			shape->SetFillingRule(FILL_MODE_EVEN_ODD);
		else
			shape->SetFillingRule(FILL_MODE_NON_ZERO);
#endif
	}

	Gradient gradient(true);
	rgb_color color;
	switch(paint->type) {
		case NSVG_PAINT_COLOR:
			color.red = paint->color & 0xFF;
			color.green = (paint->color >> 8) & 0xFF;
			color.blue = (paint->color >> 16) & 0xFF;
			color.alpha = (paint->color >> 24) & 0xFF;
			break;
		case NSVG_PAINT_LINEAR_GRADIENT:
		{
			gradient.SetType(GRADIENT_LINEAR);
			// The base gradient axis in Icon-O-Matic is a horizontal line from
			// (-64, 0) to (64, 0). The base gradient axis used by nanosvg is
			// a vertical line from (0, 0) to (0, 1). This initial transform
			// converts from one space to the other.
			agg::trans_affine baseTransform(0, 1.0/128.0, -1.0/128.0, 0,
				-0.5, 0.5);
			gradient.multiply(baseTransform);
			break;
		}
		case NSVG_PAINT_RADIAL_GRADIENT:
		{
			gradient.SetType(GRADIENT_CIRCULAR);
			agg::trans_affine baseTransform(0, 1.0/64.0, -1.0/64.0, 0,
				0, 0);
			gradient.multiply(baseTransform);
			break;
		}
	}

	if (paint->type != NSVG_PAINT_COLOR) {
		gradient.SetInterpolation(INTERPOLATION_LINEAR);
		agg::trans_affine gradientTransform(
			paint->gradient->xform[0], paint->gradient->xform[1],
			paint->gradient->xform[2], paint->gradient->xform[3],
			paint->gradient->xform[4], paint->gradient->xform[5]
		);

		// The transform from nanosvg converts a screen space coordinate into
		// a gradient offset. It is the inverse of what we need at this stage,
		// so we just invert it and multiply our base transform with it.
		gradient.multiply_inv(gradientTransform);

		// Finally, scale the gradient according to the global scaling to fit
		// the 64x64 box we work in.
		gradient.Multiply(*shape);

		for (int i = 0; i < paint->gradient->nstops; i++) {
			rgb_color stopColor;
			stopColor.red = paint->gradient->stops[i].color & 0xFF;
			stopColor.green = (paint->gradient->stops[i].color >> 8) & 0xFF;
			stopColor.blue = (paint->gradient->stops[i].color >> 16) & 0xFF;
			stopColor.alpha = (paint->gradient->stops[i].color >> 24) & 0xFF;
			gradient.AddColor(stopColor, paint->gradient->stops[i].offset);
		}

		BGradient::ColorStop* step = gradient.ColorAt(0);
		if (step) {
			color.red		= step->color.red;
			color.green		= step->color.green;
			color.blue		= step->color.blue;
			color.alpha		= step->color.alpha;
		}
	}

	color.alpha = (uint8)(color.alpha * svgShape->opacity);

	Style* style = new (nothrow) Style(color);
	if (!style || !icon->Styles()->AddStyle(style)) {
		delete style;
		return B_NO_MEMORY;
	}

	// NOTE: quick hack to freeze all transformations (only works because
	// paths and styles are not used by multiple shapes!!)
	int32 pathCount = shape->Paths()->CountPaths();
	for (int32 i = 0; i < pathCount; i++) {
		VectorPath* path = shape->Paths()->PathAtFast(i);
		path->ApplyTransform(*shape);
	}

	if (stroke)
		stroke->width(stroke->width() * shape->scale());

	if (paint->type != NSVG_PAINT_COLOR)
		style->SetGradient(&gradient);

	shape->Reset();

	shape->SetStyle(style);

	return B_OK;
}

