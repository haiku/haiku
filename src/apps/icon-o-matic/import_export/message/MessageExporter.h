/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef MESSAGE_EXPORTER_H
#define MESSAGE_EXPORTER_H


#include "Exporter.h"

class BMessage;
class BPositionIO;

_BEGIN_ICON_NAMESPACE
	template <class Type> class Container;
	class Icon;
	class Shape;
	class Style;
	class Transformer;
	class VectorPath;
_END_ICON_NAMESPACE

/** Exporter for the native Icon-O-Matic save format. */
class MessageExporter : public Exporter {
 public:
								MessageExporter();
	virtual						~MessageExporter();

	virtual	status_t			Export(const Icon* icon,
									   BPositionIO* stream);

	virtual	const char*			MIMEType();

 private:
			status_t			_Export(const VectorPath* path,
										BMessage* into) const;
			status_t			_Export(const Style* style,
										BMessage* into) const;
			status_t			_Export(const Shape* shape,
										const Container<VectorPath>* globalPaths,
										const Container<Style>* globalStyles,
										BMessage* into) const;
};

#endif // MESSAGE_EXPORTER_H
