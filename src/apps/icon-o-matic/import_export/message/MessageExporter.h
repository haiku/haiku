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
	class Icon;
	class PathContainer;
	class Shape;
	class Style;
	class StyleContainer;
	class Transformer;
	class VectorPath;
_END_ICON_NAMESPACE


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
										const PathContainer* globalPaths,
						 				const StyleContainer* globalStyles,
										BMessage* into) const;
};

#endif // MESSAGE_EXPORTER_H
