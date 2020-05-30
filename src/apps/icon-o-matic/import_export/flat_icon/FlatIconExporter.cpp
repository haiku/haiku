/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "FlatIconExporter.h"

#include <new>
#include <stdio.h>

#include <Archivable.h>
#include <DataIO.h>
#include <Message.h>
#include <Node.h>

#include "AffineTransformer.h"
#include "ContourTransformer.h"
#include "FlatIconFormat.h"
#include "GradientTransformable.h"
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
FlatIconExporter::FlatIconExporter()
#if PRINT_STATISTICS
	: fStyleSectionSize(0)
	, fGradientSize(0)
	, fGradientTransformSize(0)
	, fPathSectionSize(0)
	, fShapeSectionSize(0)
	, fPointCount(0)
#endif // PRINT_STATISTICS
{
}

// destructor
FlatIconExporter::~FlatIconExporter()
{
#if PRINT_STATISTICS
	printf("Statistics\n"
		   "--style section size: %" B_PRId32 "\n"
		   "           gradients: %" B_PRId32 "\n"
		   " gradient transforms: %" B_PRId32 "\n"
		   "---path section size: %" B_PRId32 "\n"
		   "--shape section size: %" B_PRId32 "\n"
		   "---total/different points: %" B_PRId32 "/%" B_PRIdSSIZE "\n",
		   fStyleSectionSize,
		   fGradientSize,
		   fGradientTransformSize,
		   fPathSectionSize,
		   fShapeSectionSize,
		   fPointCount,
		   fUsedPoints.size());
#endif // PRINT_STATISTICS
}

// Export
status_t
FlatIconExporter::Export(const Icon* icon, BPositionIO* stream)
{
	LittleEndianBuffer buffer;

	// flatten icon
	status_t ret = _Export(buffer, icon);
	if (ret < B_OK)
		return ret;

	// write buffer to stream
	ssize_t written = stream->Write(buffer.Buffer(), buffer.SizeUsed());
	if (written != (ssize_t)buffer.SizeUsed()) {
		if (written < 0)
			return (status_t)written;
		return B_ERROR;
	}

	return B_OK;
}

// MIMEType
const char*
FlatIconExporter::MIMEType()
{
	return NULL;
}

// Export
status_t
FlatIconExporter::Export(const Icon* icon, BNode* node,
						 const char* attrName)
{
	LittleEndianBuffer buffer;

	// flatten icon
	status_t ret = _Export(buffer, icon);
	if (ret < B_OK) {
		printf("failed to export to buffer: %s\n", strerror(ret));
		return ret;
	}

#ifndef __HAIKU__
	// work arround a BFS bug, attributes with the same name but different
	// type fail to be written
	node->RemoveAttr(attrName);
#endif

	// write buffer to attribute
	ssize_t written = node->WriteAttr(attrName, B_VECTOR_ICON_TYPE, 0,
									  buffer.Buffer(), buffer.SizeUsed());
	if (written != (ssize_t)buffer.SizeUsed()) {
		if (written < 0) {
			printf("failed to write attribute: %s\n",
				strerror((status_t)written));
			return (status_t)written;
		}
		printf("failed to write attribute\n");
		return B_ERROR;
	}

	return B_OK;
}

// #pragma mark -

// _Export
status_t
FlatIconExporter::_Export(LittleEndianBuffer& buffer, const Icon* icon)
{
	if (!buffer.Write(FLAT_ICON_MAGIC))
		return B_NO_MEMORY;

#if PRINT_STATISTICS
	fGradientSize = 0;
	fGradientTransformSize = 0;
	fPointCount = 0;
#endif

	// styles
	StyleContainer* styles = icon->Styles();
	status_t ret = _WriteStyles(buffer, styles);
	if (ret < B_OK)
		return ret;

#if PRINT_STATISTICS
	fStyleSectionSize = buffer.SizeUsed();
#endif

	// paths
	PathContainer* paths = icon->Paths();
	ret = _WritePaths(buffer, paths);
	if (ret < B_OK)
		return ret;

#if PRINT_STATISTICS
	fPathSectionSize = buffer.SizeUsed() - fStyleSectionSize;
#endif

	// shapes
	ret = _WriteShapes(buffer, styles, paths, icon->Shapes());
	if (ret < B_OK)
		return ret;

#if PRINT_STATISTICS
	fShapeSectionSize = buffer.SizeUsed()
		- (fStyleSectionSize + fPathSectionSize);
#endif

	return B_OK;
}

// _WriteTransformable
static bool
_WriteTransformable(LittleEndianBuffer& buffer,
					const Transformable* transformable)
{
	int32 matrixSize = Transformable::matrix_size;
	double matrix[matrixSize];
	transformable->StoreTo(matrix);
	for (int32 i = 0; i < matrixSize; i++) {
//		if (!buffer.Write((float)matrix[i]))
//			return false;
		if (!write_float_24(buffer, (float)matrix[i]))
			return false;
	}
	return true;
}

// _WriteTranslation
static bool
_WriteTranslation(LittleEndianBuffer& buffer,
				  const Transformable* transformable)
{
	BPoint t(B_ORIGIN);
	transformable->Transform(&t);
	return write_coord(buffer, t.x) && write_coord(buffer, t.y);
}

// _WriteStyles
status_t
FlatIconExporter::_WriteStyles(LittleEndianBuffer& buffer,
							   StyleContainer* styles)
{
	if (styles->CountStyles() > 255)
		return B_RESULT_NOT_REPRESENTABLE;
	uint8 styleCount = min_c(255, styles->CountStyles());
	if (!buffer.Write(styleCount))
		return B_NO_MEMORY;

	for (int32 i = 0; i < styleCount; i++) {
		Style* style = styles->StyleAtFast(i);

		// style type
		uint8 styleType;

		const Gradient* gradient = style->Gradient();
		if (gradient) {
			styleType = STYLE_TYPE_GRADIENT;
		} else {
			rgb_color color = style->Color();
			if (color.red == color.green && color.red == color.blue) {
				// gray
				if (style->Color().alpha == 255)
					styleType = STYLE_TYPE_SOLID_GRAY_NO_ALPHA;
				else
					styleType = STYLE_TYPE_SOLID_GRAY;
			} else {
				// color
				if (style->Color().alpha == 255)
					styleType = STYLE_TYPE_SOLID_COLOR_NO_ALPHA;
				else
					styleType = STYLE_TYPE_SOLID_COLOR;
			}
		}

		if (!buffer.Write(styleType))
			return B_NO_MEMORY;

		if (styleType == STYLE_TYPE_SOLID_COLOR) {
			// solid color
			rgb_color color = style->Color();
			if (!buffer.Write(*(uint32*)&color))
				return B_NO_MEMORY;
		} else if (styleType == STYLE_TYPE_SOLID_COLOR_NO_ALPHA) {
			// solid color without alpha
			rgb_color color = style->Color();
			if (!buffer.Write(color.red)
				|| !buffer.Write(color.green)
				|| !buffer.Write(color.blue))
				return B_NO_MEMORY;
		} else if (styleType == STYLE_TYPE_SOLID_GRAY) {
			// solid gray
			rgb_color color = style->Color();
			if (!buffer.Write(color.red)
				|| !buffer.Write(color.alpha))
				return B_NO_MEMORY;
		} else if (styleType == STYLE_TYPE_SOLID_GRAY_NO_ALPHA) {
			// solid gray without alpha
			if (!buffer.Write(style->Color().red))
				return B_NO_MEMORY;
		} else if (styleType == STYLE_TYPE_GRADIENT) {
			// gradient
			if (!_WriteGradient(buffer, gradient))
				return B_NO_MEMORY;
		}
	}

	return B_OK;
}

// _AnalysePath
bool
FlatIconExporter::_AnalysePath(VectorPath* path, uint8 pointCount,
	int32& straightCount, int32& lineCount, int32& curveCount)
{
	straightCount = 0;
	lineCount = 0;
	curveCount = 0;

	BPoint lastPoint(B_ORIGIN);
	for (uint32 p = 0; p < pointCount; p++) {
		BPoint point;
		BPoint pointIn;
		BPoint pointOut;
		if (!path->GetPointsAt(p, point, pointIn, pointOut))
			return B_ERROR;
		if (point == pointIn && point == pointOut) {
			if (point.x == lastPoint.x || point.y == lastPoint.y)
				straightCount++;
			else
				lineCount++;
		} else
			curveCount++;
		lastPoint = point;

#if PRINT_STATISTICS
		fUsedPoints.insert(point);
		fUsedPoints.insert(pointIn);
		fUsedPoints.insert(pointOut);
		fPointCount += 3;
#endif
	}

	return true;
}


// write_path_no_curves
static bool
write_path_no_curves(LittleEndianBuffer& buffer, VectorPath* path,
					 uint8 pointCount)
{
//printf("write_path_no_curves()\n");
	for (uint32 p = 0; p < pointCount; p++) {
		BPoint point;
		if (!path->GetPointAt(p, point))
			return false;
		if (!write_coord(buffer, point.x) || !write_coord(buffer, point.y))
			return false;
	}
	return true;
}

// write_path_curves
static bool
write_path_curves(LittleEndianBuffer& buffer, VectorPath* path,
				  uint8 pointCount)
{
//printf("write_path_curves()\n");
	for (uint32 p = 0; p < pointCount; p++) {
		BPoint point;
		BPoint pointIn;
		BPoint pointOut;
		if (!path->GetPointsAt(p, point, pointIn, pointOut))
			return false;
		if (!write_coord(buffer, point.x)
			|| !write_coord(buffer, point.y))
			return false;
		if (!write_coord(buffer, pointIn.x)
			|| !write_coord(buffer, pointIn.y))
			return false;
		if (!write_coord(buffer, pointOut.x)
			|| !write_coord(buffer, pointOut.y))
			return false;
	}
	return true;
}

// write_path_with_commands
static bool
write_path_with_commands(LittleEndianBuffer& buffer, VectorPath* path,
						 uint8 pointCount)
{
	PathCommandQueue queue;
	return queue.Write(buffer, path, pointCount);
}


// _WritePaths
status_t
FlatIconExporter::_WritePaths(LittleEndianBuffer& buffer, PathContainer* paths)
{
	if (paths->CountPaths() > 255)
		return B_RESULT_NOT_REPRESENTABLE;
	uint8 pathCount = min_c(255, paths->CountPaths());
	if (!buffer.Write(pathCount))
		return B_NO_MEMORY;

	for (uint32 i = 0; i < pathCount; i++) {
		VectorPath* path = paths->PathAtFast(i);
		uint8 pathFlags = 0;
		if (path->IsClosed())
			pathFlags |= PATH_FLAG_CLOSED;

		if (path->CountPoints() > 255)
			return B_RESULT_NOT_REPRESENTABLE;
		uint8 pointCount = min_c(255, path->CountPoints());

		// see if writing segments with commands is more efficient
		// than writing all segments as curves with no commands
		int32 straightCount;
		int32 lineCount;
		int32 curveCount;
		if (!_AnalysePath(path, pointCount,
			 	straightCount, lineCount, curveCount))
			 return B_ERROR;

		int32 commandPathLength
			= pointCount + straightCount * 2 + lineCount * 4 + curveCount * 12;
		int32 plainPathLength
			= pointCount * 12;
//printf("segments: %d, command length: %ld, plain length: %ld\n",
//	pointCount, commandPathLength, plainPathLength);

		if (commandPathLength < plainPathLength) {
			if (curveCount == 0)
				pathFlags |= PATH_FLAG_NO_CURVES;
			else
				pathFlags |= PATH_FLAG_USES_COMMANDS;
		}

		if (!buffer.Write(pathFlags) || !buffer.Write(pointCount))
			return B_NO_MEMORY;

		if (pathFlags & PATH_FLAG_NO_CURVES) {
			if (!write_path_no_curves(buffer, path, pointCount))
				return B_ERROR;
		} else if (pathFlags & PATH_FLAG_USES_COMMANDS) {
			if (!write_path_with_commands(buffer, path, pointCount))
				return B_ERROR;
		} else {
			if (!write_path_curves(buffer, path, pointCount))
				return B_ERROR;
		}
	}

	return B_OK;
}

// _WriteTransformer
static bool
_WriteTransformer(LittleEndianBuffer& buffer, Transformer* t)
{
	if (AffineTransformer* affine
		= dynamic_cast<AffineTransformer*>(t)) {
		// affine
		if (!buffer.Write((uint8)TRANSFORMER_TYPE_AFFINE))
			return false;
		double matrix[6];
		affine->store_to(matrix);
		for (int32 i = 0; i < 6; i++) {
			if (!write_float_24(buffer, (float)matrix[i]))
				return false;
		}

	} else if (ContourTransformer* contour
		= dynamic_cast<ContourTransformer*>(t)) {
		// contour
		if (!buffer.Write((uint8)TRANSFORMER_TYPE_CONTOUR))
			return false;
		uint8 width = (uint8)((int8)contour->width() + 128);
		uint8 lineJoin = (uint8)contour->line_join();
		uint8 miterLimit = (uint8)contour->miter_limit();
		if (!buffer.Write(width)
			|| !buffer.Write(lineJoin)
			|| !buffer.Write(miterLimit))
			return false;

	} else if (dynamic_cast<PerspectiveTransformer*>(t)) {
		// perspective
		if (!buffer.Write((uint8)TRANSFORMER_TYPE_PERSPECTIVE))
			return false;
		// TODO: ... (upgrade AGG for storage support of trans_perspective)

	} else if (StrokeTransformer* stroke
		= dynamic_cast<StrokeTransformer*>(t)) {
		// stroke
		if (!buffer.Write((uint8)TRANSFORMER_TYPE_STROKE))
			return false;
		uint8 width = (uint8)((int8)stroke->width() + 128);
		uint8 lineOptions = (uint8)stroke->line_join();
		lineOptions |= ((uint8)stroke->line_cap()) << 4;
		uint8 miterLimit = (uint8)stroke->miter_limit();

		if (!buffer.Write(width)
			|| !buffer.Write(lineOptions)
			|| !buffer.Write(miterLimit))
			return false;
	}

	return true;
}

// _WritePathSourceShape
static bool
_WritePathSourceShape(LittleEndianBuffer& buffer, Shape* shape,
					  StyleContainer* styles, PathContainer* paths)
{
	// find out which style this shape uses
	Style* style = shape->Style();
	if (!style)
		return false;

	int32 styleIndex = styles->IndexOf(style);
	if (styleIndex < 0 || styleIndex > 255)
		return false;

	if (shape->Paths()->CountPaths() > 255)
		return B_RESULT_NOT_REPRESENTABLE;
	uint8 pathCount = min_c(255, shape->Paths()->CountPaths());

	// write shape type and style index
	if (!buffer.Write((uint8)SHAPE_TYPE_PATH_SOURCE)
		|| !buffer.Write((uint8)styleIndex)
		|| !buffer.Write(pathCount))
		return false;

	// find out which paths this shape uses
	for (uint32 i = 0; i < pathCount; i++) {
		VectorPath* path = shape->Paths()->PathAtFast(i);
		int32 pathIndex = paths->IndexOf(path);
		if (pathIndex < 0 || pathIndex > 255)
			return false;

		if (!buffer.Write((uint8)pathIndex))
			return false;
	}

	if (shape->CountTransformers() > 255)
		return B_RESULT_NOT_REPRESENTABLE;
	uint8 transformerCount = min_c(255, shape->CountTransformers());

	// shape flags
	uint8 shapeFlags = 0;
	if (!shape->IsIdentity()) {
		if (shape->IsTranslationOnly())
			shapeFlags |= SHAPE_FLAG_TRANSLATION;
		else
			shapeFlags |= SHAPE_FLAG_TRANSFORM;
	}
	if (shape->Hinting())
		shapeFlags |= SHAPE_FLAG_HINTING;
	if (shape->MinVisibilityScale() != 0.0
		|| shape->MaxVisibilityScale() != 4.0)
		shapeFlags |= SHAPE_FLAG_LOD_SCALE;
	if (transformerCount > 0)
		shapeFlags |= SHAPE_FLAG_HAS_TRANSFORMERS;

	if (!buffer.Write((uint8)shapeFlags))
		return false;

	if (shapeFlags & SHAPE_FLAG_TRANSFORM) {
		// transformation
		if (!_WriteTransformable(buffer, shape))
			return false;
	} else if (shapeFlags & SHAPE_FLAG_TRANSLATION) {
		// translation
		if (!_WriteTranslation(buffer, shape))
			return false;
	}

	if (shapeFlags & SHAPE_FLAG_LOD_SCALE) {
		// min max visibility scale
		if (!buffer.Write(
				(uint8)(shape->MinVisibilityScale() * 63.75 + 0.5))
			|| !buffer.Write(
				(uint8)(shape->MaxVisibilityScale() * 63.75 + 0.5))) {
			return false;
		}
	}

	if (shapeFlags & SHAPE_FLAG_HAS_TRANSFORMERS) {
		// transformers
		if (!buffer.Write(transformerCount))
			return false;

		for (uint32 i = 0; i < transformerCount; i++) {
			Transformer* transformer = shape->TransformerAtFast(i);
			if (!_WriteTransformer(buffer, transformer))
				return false;
		}
	}

	return true;
}

// _WriteShapes
status_t
FlatIconExporter::_WriteShapes(LittleEndianBuffer& buffer,
							   StyleContainer* styles,
							   PathContainer* paths,
							   ShapeContainer* shapes)
{
	if (shapes->CountShapes() > 255)
		return B_RESULT_NOT_REPRESENTABLE;
	uint8 shapeCount = min_c(255, shapes->CountShapes());
	if (!buffer.Write(shapeCount))
		return B_NO_MEMORY;

	for (uint32 i = 0; i < shapeCount; i++) {
		Shape* shape = shapes->ShapeAtFast(i);
		if (!_WritePathSourceShape(buffer, shape, styles, paths))
			return B_ERROR;
	}

	return B_OK;
}

// _WriteGradient
bool
FlatIconExporter::_WriteGradient(LittleEndianBuffer& buffer,
								 const Gradient* gradient)
{
#if PRINT_STATISTICS
	size_t currentSize = buffer.SizeUsed();
#endif

	uint8 gradientType = (uint8)gradient->Type();
	uint8 gradientFlags = 0;
	uint8 gradientStopCount = (uint8)gradient->CountColors();

	// figure out flags
	if (!gradient->IsIdentity())
		gradientFlags |= GRADIENT_FLAG_TRANSFORM;

	bool alpha = false;
	bool gray = true;
	for (int32 i = 0; i < gradientStopCount; i++) {
		BGradient::ColorStop* step = gradient->ColorAtFast(i);
		if (step->color.alpha < 255)
			alpha = true;
		if (step->color.red != step->color.green
			|| step->color.red != step->color.blue)
			gray = false;
	}
	if (!alpha)
		gradientFlags |= GRADIENT_FLAG_NO_ALPHA;
	if (gray)
		gradientFlags |= GRADIENT_FLAG_GRAYS;

	if (!buffer.Write(gradientType)
		|| !buffer.Write(gradientFlags)
		|| !buffer.Write(gradientStopCount))
		return false;

	if (gradientFlags & GRADIENT_FLAG_TRANSFORM) {
		if (!_WriteTransformable(buffer, gradient))
			return false;
#if PRINT_STATISTICS
	fGradientTransformSize += Transformable::matrix_size * sizeof(float);
#endif
	}

	for (int32 i = 0; i < gradientStopCount; i++) {
		BGradient::ColorStop* step = gradient->ColorAtFast(i);
		uint8 stopOffset = (uint8)(step->offset * 255.0);
		uint32 color = (uint32&)step->color;
		if (!buffer.Write(stopOffset))
			return false;
		if (alpha) {
			if (gray) {
				if (!buffer.Write(step->color.red)
					|| !buffer.Write(step->color.alpha))
					return false;
			} else {
				if (!buffer.Write(color))
					return false;
			}
		} else {
			if (gray) {
				if (!buffer.Write(step->color.red))
					return false;
			} else {
				if (!buffer.Write(step->color.red)
					|| !buffer.Write(step->color.green)
					|| !buffer.Write(step->color.blue))
					return false;
			}
		}
	}

#if PRINT_STATISTICS
	fGradientSize += buffer.SizeUsed() - currentSize;
#endif

	return true;
}



