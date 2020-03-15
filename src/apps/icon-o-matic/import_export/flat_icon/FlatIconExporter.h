/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef FLAT_ICON_EXPORTER_H
#define FLAT_ICON_EXPORTER_H


#include "Exporter.h"


class BMessage;
class BNode;
class BPositionIO;

_BEGIN_ICON_NAMESPACE
	class Gradient;
	class LittleEndianBuffer;
	class PathContainer;
	class ShapeContainer;
	class StyleContainer;
	class VectorPath;
_END_ICON_NAMESPACE


#define PRINT_STATISTICS 0

#if PRINT_STATISTICS
# include <Point.h>
# include <hash_set>

#if __GNUC__ >= 4
  using __gnu_cxx::hash_set;
#endif

class PointHash {
 public:
	int operator()(const BPoint& point) const
	{
		return (int)point.x * 17 + (int)point.y;
	}
};

typedef hash_set<BPoint, PointHash> PointSet;
#endif

class FlatIconExporter : public Exporter {
 public:
								FlatIconExporter();
	virtual						~FlatIconExporter();

	// Exporter interface
	virtual	status_t			Export(const Icon* icon,
									   BPositionIO* stream);

	virtual	const char*			MIMEType();

	// FlatIconExporter
	status_t					Export(const Icon* icon, BNode* node,
									   const char* attrName);

 private:
			status_t			_Export(LittleEndianBuffer& buffer,
										const Icon* icon);

			status_t			_WriteStyles(LittleEndianBuffer& buffer,
											 StyleContainer* styles);
			status_t			_WritePaths(LittleEndianBuffer& buffer,
											PathContainer* paths);
			status_t			_WriteShapes(LittleEndianBuffer& buffer,
											 StyleContainer* styles,
											 PathContainer* paths,
											 ShapeContainer* shapes);

			bool				_WriteGradient(LittleEndianBuffer& buffer,
											   const Gradient* gradient);

			bool				_AnalysePath(VectorPath* path,
											 uint8 pointCount,
											 int32& straightCount,
											 int32& lineCount,
											 int32& curveCount);

#if PRINT_STATISTICS
			int32				fStyleSectionSize;
			int32				fGradientSize;
			int32				fGradientTransformSize;
			int32				fPathSectionSize;
			int32				fShapeSectionSize;

			PointSet			fUsedPoints;
			int32				fPointCount;
#endif // PRINT_STATISTICS
};

#endif // FLAT_ICON_EXPORTER_H
