/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef MESSAGE_IMPORTER_H
#define MESSAGE_IMPORTER_H

#include "Importer.h"

class BMessage;
class BPositionIO;
class Icon;
class PathContainer;
class ShapeContainer;
class StyleContainer;

class MessageImporter : public Importer {
 public:
								MessageImporter();
	virtual						~MessageImporter();

			status_t			Import(Icon* icon,
									   BPositionIO* stream);

 private:
			status_t			_ImportPaths(const BMessage* archive,
											 PathContainer* paths) const;
			status_t			_ImportStyles(const BMessage* archive,
											  StyleContainer* styles) const;
			status_t			_ImportShapes(const BMessage* archive,
											  PathContainer* paths,
											  StyleContainer* styles,
											  ShapeContainer* shapes) const;
};

#endif // MESSAGE_IMPORTER_H
