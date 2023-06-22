/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef FLAT_ICON_IMPORTER_H
#define FLAT_ICON_IMPORTER_H


#ifdef ICON_O_MATIC
#	include "Importer.h"
#else
#	include <SupportDefs.h>
#endif

#include "Container.h"
#include "IconBuild.h"


class BMessage;
class BPositionIO;


_BEGIN_ICON_NAMESPACE


class Icon;
class LittleEndianBuffer;
class PathContainer;
class Shape;
class Style;
class VectorPath;

#ifdef ICON_O_MATIC
class FlatIconImporter : public Importer {
#else
class FlatIconImporter {
#endif
 public:
								FlatIconImporter();
	virtual						~FlatIconImporter();

	// FlatIconImporter
			status_t			Import(Icon* icon,
									   BPositionIO* stream);

			status_t			Import(Icon* icon,
									   uint8* buffer, size_t size);

 private:
			status_t			_ParseSections(LittleEndianBuffer& buffer,
											   Icon* icon);

			status_t			_ParseStyles(LittleEndianBuffer& buffer,
											 Container<Style>* styles);
			status_t			_ParsePaths(LittleEndianBuffer& buffer,
											Container<VectorPath>* paths);
			Shape*				_ReadPathSourceShape(
									LittleEndianBuffer& buffer,
									Container<Style>* styles,
									Container<VectorPath>* paths);
			status_t			_ParseShapes(LittleEndianBuffer& buffer,
											 Container<Style>* styles,
											 Container<VectorPath>* paths,
											 Container<Shape>* shapes);
};


_END_ICON_NAMESPACE


#endif	// FLAT_ICON_IMPORTER_H
