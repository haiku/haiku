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

#include "IconBuild.h"


class BMessage;
class BPositionIO;


_BEGIN_ICON_NAMESPACE


template <class Type> class Container;
class Icon;
class PathContainer;
class Shape;
class Style;
class VectorPath;


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
											 Container<VectorPath>* paths) const;
			status_t			_ImportStyles(const BMessage* archive,
											  Container<Style>* styles) const;
			status_t			_ImportShapes(const BMessage* archive,
											  Container<VectorPath>* paths,
											  Container<Style>* styles,
											  Container<Shape>* shapes) const;
};


_END_ICON_NAMESPACE


#endif // MESSAGE_IMPORTER_H
