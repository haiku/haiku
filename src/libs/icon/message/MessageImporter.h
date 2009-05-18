/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef MESSAGE_IMPORTER_H
#define MESSAGE_IMPORTER_H


#ifdef ICON_O_MATIC
#	include "Importer.h"
#else
#	include <SupportDefs.h>
#endif

class BMessage;
class BPositionIO;

namespace BPrivate {
namespace Icon {

class Icon;
class PathContainer;
class ShapeContainer;
class StyleContainer;


#ifdef ICON_O_MATIC
class MessageImporter : public Importer {
#else
class MessageImporter {
#endif
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

}	// namespace Icon
}	// namespace BPrivate

#endif // MESSAGE_IMPORTER_H
