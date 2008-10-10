/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "MessageExporter.h"

#include <ByteOrder.h>
#include <DataIO.h>
#include <Message.h>
#include <TypeConstants.h>

#include "Defines.h"
#include "Icon.h"
#include "PathContainer.h"
#include "Shape.h"
#include "Style.h"
#include "StyleContainer.h"
#include "Transformer.h"
#include "VectorPath.h"

// constructor
MessageExporter::MessageExporter()
{
}

// destructor
MessageExporter::~MessageExporter()
{
}

// Export
status_t
MessageExporter::Export(const Icon* icon, BPositionIO* stream)
{
	status_t ret = B_OK;
	BMessage archive;

	PathContainer* paths = icon->Paths();
	StyleContainer* styles = icon->Styles();

	// paths
	if (ret == B_OK) {
		BMessage allPaths;
		int32 count = paths->CountPaths();
		for (int32 i = 0; i < count; i++) {
			VectorPath* path = paths->PathAtFast(i);
			BMessage pathArchive;
			ret = _Export(path, &pathArchive);
			if (ret < B_OK)
				break;
			ret = allPaths.AddMessage("path", &pathArchive);
			if (ret < B_OK)
				break;
		}
	
		if (ret == B_OK)
			ret = archive.AddMessage("paths", &allPaths);
	}

	// styles
	if (ret == B_OK) {
		BMessage allStyles;
		int32 count = styles->CountStyles();
		for (int32 i = 0; i < count; i++) {
			Style* style = styles->StyleAtFast(i);
			BMessage styleArchive;
			ret = _Export(style, &styleArchive);
			if (ret < B_OK)
				break;
			ret = allStyles.AddMessage("style", &styleArchive);
			if (ret < B_OK)
				break;
		}
	
		if (ret == B_OK)
			ret = archive.AddMessage("styles", &allStyles);
	}

	// shapes
	if (ret == B_OK) {
		BMessage allShapes;
		ShapeContainer* shapes = icon->Shapes();
		int32 count = shapes->CountShapes();
		for (int32 i = 0; i < count; i++) {
			Shape* shape = shapes->ShapeAtFast(i);
			BMessage shapeArchive;
			ret = _Export(shape, paths, styles, &shapeArchive);
			if (ret < B_OK)
				break;
			ret = allShapes.AddMessage("shape", &shapeArchive);
			if (ret < B_OK)
				break;
		}
	
		if (ret == B_OK)
			ret = archive.AddMessage("shapes", &allShapes);
	}

	// prepend the magic number to the file which
	// later tells us that this file is one of us
	if (ret == B_OK) {
		ssize_t size = sizeof(uint32);
		uint32 magic = B_HOST_TO_BENDIAN_INT32(kNativeIconMagicNumber);
		ssize_t written = stream->Write(&magic, size);
		if (written != size) {
			if (written < 0)
				ret = (status_t)written;
			else
				ret = B_IO_ERROR;
		}
	}

	if (ret == B_OK)
		ret = archive.Flatten(stream);

	return ret;
}

// MIMEType
const char*
MessageExporter::MIMEType()
{
	return kNativeIconMimeType;
}

// #pragma mark -

// _Export
status_t
MessageExporter::_Export(const VectorPath* path, BMessage* into) const
{
	return path->Archive(into, true);
}

// _Export
status_t
MessageExporter::_Export(const Style* style, BMessage* into) const
{
	return style->Archive(into, true);
}

// _Export
status_t
MessageExporter::_Export(const Shape* shape,
						 const PathContainer* globalPaths,
						 const StyleContainer* globalStyles,
						 BMessage* into) const
{
	// NOTE: when the same path is used in two different
	// documents, and these are to be merged, each path
	// having a "globally unique" id would make it possible
	// to reference the same path across documents...
	// For now, we simply use the index of the path in the
	// globalPaths container.

	// index of used style
	Style* style = shape->Style();
	status_t ret = into->AddInt32("style ref", globalStyles->IndexOf(style));

	// indices of used paths
	if (ret == B_OK) {
		int32 count = shape->Paths()->CountPaths();
		for (int32 i = 0; i < count; i++) {
			VectorPath* path = shape->Paths()->PathAtFast(i);
			ret = into->AddInt32("path ref", globalPaths->IndexOf(path));
			if (ret < B_OK)
				break;
		}
	}

	// Shape properties
	if (ret == B_OK)
		ret = shape->Archive(into);

	return ret;
}

