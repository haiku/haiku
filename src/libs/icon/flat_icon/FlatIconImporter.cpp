/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "FlatIconImporter.h"

#include <new>
#include <stdio.h>

#include <Archivable.h>
#include <DataIO.h>
#include <Message.h>

#include "AffineTransformer.h"
#include "AutoDeleter.h"
#include "ContourTransformer.h"
#include "FlatIconFormat.h"
#include "Gradient.h"
#include "Icon.h"
#include "LittleEndianBuffer.h"
#include "PathCommandQueue.h"
#include "PathContainer.h"
#include "PerspectiveTransformer.h"
#include "Shape.h"
#include "StrokeTransformer.h"
#include "Style.h"
#include "StyleContainer.h"
#include "VectorPath.h"

using std::nothrow;

// constructor
FlatIconImporter::FlatIconImporter()
{
}

// destructor
FlatIconImporter::~FlatIconImporter()
{
}

// Import
status_t
FlatIconImporter::Import(Icon* icon, BPositionIO* stream)
{
	// seek around in the stream to figure out the size
	off_t size = stream->Seek(0, SEEK_END);
	if (stream->Seek(0, SEEK_SET) != 0)
		return B_ERROR;

	// we chicken out on anything larger than 256k
	if (size <= 0 || size > 256 * 1024)
		return B_BAD_VALUE;

	// read the entire stream into a buffer
	LittleEndianBuffer buffer(size);
	if (!buffer.Buffer())
		return B_NO_MEMORY;

	if (stream->Read(buffer.Buffer(), size) != size)
		return B_ERROR;

	status_t ret = _ParseSections(buffer, icon);

	return ret;
}

// Import
status_t
FlatIconImporter::Import(Icon* icon, uint8* _buffer, size_t size)
{
	if (!_buffer)
		return B_BAD_VALUE;

	// attach LittleEndianBuffer to buffer
	LittleEndianBuffer buffer(_buffer, size);

	return _ParseSections(buffer, icon);
}

// #pragma mark -

// _ParseSections
status_t
FlatIconImporter::_ParseSections(LittleEndianBuffer& buffer, Icon* icon)
{
	// test if this is an icon at all
	uint32 magic;
	if (!buffer.Read(magic) || magic != FLAT_ICON_MAGIC)
		return B_ERROR;

	// styles
	StyleContainer* styles = icon->Styles();
	status_t ret = _ParseStyles(buffer, styles);
	if (ret < B_OK) {
		printf("FlatIconImporter::_ParseSections() - "
			   "error parsing styles: %s\n", strerror(ret));
		return ret;
	}

	// paths
	PathContainer* paths = icon->Paths();
	ret = _ParsePaths(buffer, paths);
	if (ret < B_OK) {
		printf("FlatIconImporter::_ParseSections() - "
			   "error parsing paths: %s\n", strerror(ret));
		return ret;
	}

	// shapes
	ret = _ParseShapes(buffer, styles, paths, icon->Shapes());
	if (ret < B_OK) {
		printf("FlatIconImporter::_ParseSections() - "
			   "error parsing shapes: %s\n", strerror(ret));
		return ret;
	}

	return B_OK;
}

// _ReadTransformable
static bool
_ReadTransformable(LittleEndianBuffer& buffer, Transformable* transformable)
{
	int32 matrixSize = Transformable::matrix_size;
	double matrix[matrixSize];
	for (int32 i = 0; i < matrixSize; i++) {
		float value;
		if (!read_float_24(buffer, value))
			return false;
		matrix[i] = value;
	}
	transformable->LoadFrom(matrix);
	return true;
}

// _ReadTranslation
static bool
_ReadTranslation(LittleEndianBuffer& buffer, Transformable* transformable)
{
	BPoint t;
	if (read_coord(buffer, t.x) && read_coord(buffer, t.y)) {
		transformable->TranslateBy(t);
		return true;
	}

	return false;
}

// _ReadColorStyle
static Style*
_ReadColorStyle(LittleEndianBuffer& buffer, bool alpha)
{
	rgb_color color;
	if (alpha) {
		if (!buffer.Read((uint32&)color))
			return NULL;
	} else {
		color.alpha = 255;
		if (!buffer.Read(color.red)
			|| !buffer.Read(color.green)
			|| !buffer.Read(color.blue))
			return NULL;
	}
	return new (nothrow) Style(color);
}

// _ReadGradientStyle
static Style*
_ReadGradientStyle(LittleEndianBuffer& buffer)
{
	Style* style = new (nothrow) Style();
	if (!style)
		return NULL;

	ObjectDeleter<Style> styleDeleter(style);

	uint8 gradientType;
	uint8 gradientFlags;
	uint8 gradientStopCount;
	if (!buffer.Read(gradientType)
		|| !buffer.Read(gradientFlags)
		|| !buffer.Read(gradientStopCount)) {
		return NULL;
	}

	Gradient gradient(true);
		// empty gradient

	gradient.SetType((gradient_type)gradientType);
	// TODO: support more stuff with flags
	// ("inherits transformation" and so on)
	if (gradientFlags & GRADIENT_FLAG_TRANSFORM) {
		if (!_ReadTransformable(buffer, &gradient))
			return NULL;
	}

	bool alpha = !(gradientFlags & GRADIENT_FLAG_NO_ALPHA);

	for (int32 i = 0; i < gradientStopCount; i++) {
		uint8 stopOffset;
		rgb_color color;

		if (!buffer.Read(stopOffset))
			return NULL;
		
		if (alpha) {
			if (!buffer.Read((uint32&)color))
				return NULL;
		} else {
			color.alpha = 255;
			if (!buffer.Read(color.red)
				|| !buffer.Read(color.green)
				|| !buffer.Read(color.blue)) {
				return NULL;
			}
		}

		gradient.AddColor(color, stopOffset / 255.0);
	}

	style->SetGradient(&gradient);

	styleDeleter.Detach();
	return style;
}

// _ParseStyles
status_t
FlatIconImporter::_ParseStyles(LittleEndianBuffer& buffer,
							   StyleContainer* styles)
{
	uint8 styleCount;
	if (!buffer.Read(styleCount))
		return B_ERROR;

	for (int32 i = 0; i < styleCount; i++) {
		uint8 styleType;
		if (!buffer.Read(styleType))
			return B_ERROR;
		Style* style = NULL;
		if (styleType == TAG_STYLE_SOLID_COLOR) {
			// solid color
			style = _ReadColorStyle(buffer, true);
			if (!style)
				return B_NO_MEMORY;
		} else if (styleType == TAG_STYLE_SOLID_COLOR_NO_ALPHA) {
			// solid color without alpha
			style = _ReadColorStyle(buffer, false);
			if (!style)
				return B_NO_MEMORY;
		} else if (styleType == TAG_STYLE_GRADIENT) {
			// gradient
			style = _ReadGradientStyle(buffer);
			if (!style)
				return B_NO_MEMORY;
		} else {
			// unkown style type, skip tag
			uint16 tagLength;
			if (!buffer.Read(tagLength))
				return B_ERROR;
			buffer.Skip(tagLength);
			continue;
		}
		// add style if we were able to read one
		if (style && !styles->AddStyle(style)) {
			delete style;
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}

// read_path_no_curves
static bool
read_path_no_curves(LittleEndianBuffer& buffer, VectorPath* path,
					uint8 pointCount)
{
	for (uint32 p = 0; p < pointCount; p++) {
		BPoint point;
		if (!read_coord(buffer, point.x)
			|| !read_coord(buffer, point.y))
			return false;

		if (!path->AddPoint(point))
			return false;
	}
	return true;
}

// read_path_curves
static bool
read_path_curves(LittleEndianBuffer& buffer, VectorPath* path,
				 uint8 pointCount)
{
	for (uint32 p = 0; p < pointCount; p++) {
		BPoint point;
		if (!read_coord(buffer, point.x)
			|| !read_coord(buffer, point.y))
			return false;

		BPoint pointIn;
		if (!read_coord(buffer, pointIn.x)
			|| !read_coord(buffer, pointIn.y))
			return false;

		BPoint pointOut;
		if (!read_coord(buffer, pointOut.x)
			|| !read_coord(buffer, pointOut.y))
			return false;

		if (!path->AddPoint(point, pointIn, pointOut, false))
			return false;
	}
	return true;
}

// read_path_with_commands
static bool
read_path_with_commands(LittleEndianBuffer& buffer, VectorPath* path,
						uint8 pointCount)
{
	PathCommandQueue queue;
	return queue.Read(buffer, path, pointCount);
}


// _ParsePaths
status_t
FlatIconImporter::_ParsePaths(LittleEndianBuffer& buffer,
							  PathContainer* paths)
{
	uint8 pathCount;
	if (!buffer.Read(pathCount))
		return B_ERROR;

	for (int32 i = 0; i < pathCount; i++) {
		uint8 pathFlags;
		uint8 pointCount;
		if (!buffer.Read(pathFlags) || !buffer.Read(pointCount))
			return B_ERROR;

		VectorPath* path = new (nothrow) VectorPath();
		if (!path)
			return B_NO_MEMORY;

		// chose path reading strategy depending on path flags
		bool error = false;
		if (pathFlags & PATH_FLAGS_NO_CURVES) {
			if (!read_path_no_curves(buffer, path, pointCount))
				error = true;
		} else if (pathFlags & PATH_FLAGS_USES_COMMANDS) {
			if (!read_path_with_commands(buffer, path, pointCount))
				error = true;
		} else {
			if (!read_path_curves(buffer, path, pointCount))
				error = true;
		}

		if (error) {
			delete path;
			return B_ERROR;
		}
		// post process path to clean it up
		path->CleanUp();
		if (pathFlags & PATH_FLAGS_CLOSED)
			path->SetClosed(true);
		// add path to container
		if (!paths->AddPath(path)) {
			delete path;
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}

// _ReadTransformer
static Transformer*
_ReadTransformer(LittleEndianBuffer& buffer, VertexSource& source)
{
	uint8 transformerType;
	if (!buffer.Read(transformerType))
		return NULL;

	switch (transformerType) {
		case TAG_TRANSFORMER_AFFINE: {
			AffineTransformer* affine
				= new (nothrow) AffineTransformer(source);
			if (!affine)
				return NULL;
			double matrix[6];
			for (int32 i = 0; i < 6; i++) {
				float value;
				if (!buffer.Read(value)) {
					delete affine;
					return NULL;
				}
				matrix[i] = value;
			}
			affine->load_from(matrix);
			return affine;
		}
		case TAG_TRANSFORMER_CONTOUR: {
			ContourTransformer* contour
				= new (nothrow) ContourTransformer(source);
			uint8 width;
			uint8 lineJoin;
			uint8 miterLimit;
			if (!contour
				|| !buffer.Read(width)
				|| !buffer.Read(lineJoin)
				|| !buffer.Read(miterLimit)) {
				delete contour;
				return NULL;
			}
			contour->width(width - 128.0);
			contour->line_join((agg::line_join_e)lineJoin);
			contour->miter_limit(miterLimit);
			return contour;
		}
		case TAG_TRANSFORMER_PERSPECTIVE: {
			PerspectiveTransformer* perspective
				= new (nothrow) PerspectiveTransformer(source);
			// TODO: upgrade AGG to be able to support storage of
			// trans_perspective
			return perspective;
		}
		case TAG_TRANSFORMER_STROKE: {
			StrokeTransformer* stroke
				= new (nothrow) StrokeTransformer(source);
			uint8 width;
			uint8 lineJoin;
			uint8 lineCap;
			uint8 miterLimit;
//			uint8 shorten;
			if (!stroke
				|| !buffer.Read(width)
				|| !buffer.Read(lineJoin)
				|| !buffer.Read(lineCap)
				|| !buffer.Read(miterLimit)) {
				delete stroke;
				return NULL;
			}
			stroke->width(width - 128.0);
			stroke->line_join((agg::line_join_e)lineJoin);
			stroke->line_cap((agg::line_cap_e)lineCap);
			stroke->miter_limit(miterLimit);
			return stroke;
		}
		default: {
			// unkown transformer, skip tag
			uint16 tagLength;
			if (!buffer.Read(tagLength))
				return NULL;
			buffer.Skip(tagLength);
			return NULL;
		}
	}
}

// _ReadPathSourceShape
static Shape*
_ReadPathSourceShape(LittleEndianBuffer& buffer,
					 StyleContainer* styles, PathContainer* paths)
{
	// find out which style this shape uses
	uint8 styleIndex;
	uint8 pathCount;
	if (!buffer.Read(styleIndex) || !buffer.Read(pathCount))
		return NULL;

	Style* style = styles->StyleAt(styleIndex);
	if (!style) {
		printf("_ReadPathSourceShape() - "
			   "shape references non-existing style %d\n", styleIndex);
		return NULL;
	}

	// create the shape
	Shape* shape = new (nothrow) Shape(style);
	ObjectDeleter<Shape> shapeDeleter(shape);

	if (!shape || shape->InitCheck() < B_OK)
		return NULL;

	// find out which paths this shape uses
	for (uint32 i = 0; i < pathCount; i++) {
		uint8 pathIndex;
		if (!buffer.Read(pathIndex))
			return NULL;

		VectorPath* path = paths->PathAt(pathIndex);
		if (!path) {
			printf("_ReadPathSourceShape() - "
				   "shape references non-existing path %d\n", pathIndex);
			continue;
		}
		shape->Paths()->AddPath(path);
	}

	// shape flags
	uint8 shapeFlags;
	if (!buffer.Read(shapeFlags))
		return NULL;

	shape->SetHinting(shapeFlags & SHAPE_FLAG_HINTING);

	if (shapeFlags & SHAPE_FLAG_TRANSFORM) {
		// transformation
		if (!_ReadTransformable(buffer, shape))
			return NULL;
	} else if (shapeFlags & SHAPE_FLAG_TRANSLATION) {
		// translation
		if (!_ReadTranslation(buffer, shape))
			return NULL;
	}

	if (shapeFlags & SHAPE_FLAG_LOD_SCALE) {
		// min max visibility scale
		uint8 minScale;
		uint8 maxScale;
		if (!buffer.Read(minScale) || !buffer.Read(maxScale))
			return NULL;
		shape->SetMinVisibilityScale((float)minScale);
		shape->SetMaxVisibilityScale((float)maxScale);
	}

	// transformers
	if (shapeFlags & SHAPE_FLAG_HAS_TRANSFORMERS) {
		uint8 transformerCount;
		if (!buffer.Read(transformerCount))
			return NULL;
		for (uint32 i = 0; i < transformerCount; i++) {
			Transformer* transformer
				= _ReadTransformer(buffer, shape->VertexSource());
			if (transformer && !shape->AddTransformer(transformer)) {
				delete transformer;
				return NULL;
			}
		}
	}

	shapeDeleter.Detach();
	return shape;
}

// _ParseShapes
status_t
FlatIconImporter::_ParseShapes(LittleEndianBuffer& buffer,
							   StyleContainer* styles,
							   PathContainer* paths,
							   ShapeContainer* shapes)
{
	uint8 shapeCount;
	if (!buffer.Read(shapeCount))
		return B_ERROR;

	for (uint32 i = 0; i < shapeCount; i++) {
		uint8 shapeType;
		if (!buffer.Read(shapeType))
			return B_ERROR;
		Shape* shape = NULL;
		if (shapeType == TAG_SHAPE_PATH_SOURCE) {
			// path source shape
			shape = _ReadPathSourceShape(buffer, styles, paths);
			if (!shape)
				return B_NO_MEMORY;
		} else {
			// unkown shape type, skip tag
			uint16 tagLength;
			if (!buffer.Read(tagLength))
				return B_ERROR;
			buffer.Skip(tagLength);
			continue;
		}
		// add shape if we were able to read one
		if (shape && !shapes->AddShape(shape)) {
			delete shape;
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}




