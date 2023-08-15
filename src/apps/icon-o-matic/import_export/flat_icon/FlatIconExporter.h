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
	template <class Type> class Container;
	class Gradient;
	class LittleEndianBuffer;
	class Shape;
	class Style;
	class VectorPath;
_END_ICON_NAMESPACE


#define HVIF_EXPORTER_ERRORS_BASE		(B_ERRORS_END + 1)
#define E_TOO_MANY_PATHS				(HVIF_EXPORTER_ERRORS_BASE + 0)
#define E_PATH_TOO_MANY_POINTS			(HVIF_EXPORTER_ERRORS_BASE + 1)
#define E_TOO_MANY_SHAPES				(HVIF_EXPORTER_ERRORS_BASE + 2)
#define E_SHAPE_TOO_MANY_PATHS			(HVIF_EXPORTER_ERRORS_BASE + 3)
#define E_SHAPE_TOO_MANY_TRANSFORMERS	(HVIF_EXPORTER_ERRORS_BASE + 4)
#define E_TOO_MANY_STYLES				(HVIF_EXPORTER_ERRORS_BASE + 5)


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

/*! Export to HVIF file or to an existing file's attribute. */
class FlatIconExporter : public Exporter {
 public:
								FlatIconExporter();
	virtual						~FlatIconExporter();

	// Exporter interface
	virtual	status_t			Export(const Icon* icon,
									   BPositionIO* stream);
	virtual const char*			ErrorCodeToString(status_t code);
	virtual	const char*			MIMEType() { return NULL; }

	// FlatIconExporter
	/*! Export to file attribute */
	status_t					Export(const Icon* icon, BNode* node,
									   const char* attrName);

 private:
			status_t			_Export(LittleEndianBuffer& buffer,
										const Icon* icon);

			status_t			_WriteStyles(LittleEndianBuffer& buffer,
											 const Container<Style>* styles);
			status_t			_WritePaths(LittleEndianBuffer& buffer,
											const Container<VectorPath>* paths);
			status_t			_WriteShapes(LittleEndianBuffer& buffer,
											 const Container<Style>* styles,
											 const Container<VectorPath>* paths,
											 const Container<Shape>* shapes);

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
