/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef FLAT_ICON_IMPORTER_H
#define FLAT_ICON_IMPORTER_H

#include <SupportDefs.h>

class BMessage;
class BPositionIO;
class Icon;
class LittleEndianBuffer;
class PathContainer;
class ShapeContainer;
class StyleContainer;

class FlatIconImporter {
 public:
								FlatIconImporter();
	virtual						~FlatIconImporter();

	// Importer interface (Importer base not yet written)
	virtual	status_t			Import(Icon* icon,
									   BPositionIO* stream);

	// FlatIconImporter
			status_t			Import(Icon* icon,
									   uint8* buffer, size_t size);

 private:
			status_t			_ParseSections(LittleEndianBuffer& buffer,
											   Icon* icon);

			status_t			_ParseStyles(LittleEndianBuffer& buffer,
											 StyleContainer* styles);
			status_t			_ParsePaths(LittleEndianBuffer& buffer,
											PathContainer* paths);
			status_t			_ParseShapes(LittleEndianBuffer& buffer,
											 StyleContainer* styles,
											 PathContainer* paths,
											 ShapeContainer* shapes);
};

#endif // FLAT_ICON_IMPORTER_H
