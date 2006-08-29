/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "MessageImporter.h"

#include <new>
#include <stdio.h>

#include <Archivable.h>
#include <DataIO.h>
#include <Message.h>

#include "Icon.h"
#include "PathContainer.h"
#include "Shape.h"
#include "Style.h"
#include "StyleContainer.h"
#include "VectorPath.h"

using std::nothrow;

// constructor
MessageImporter::MessageImporter()
	: Importer()
{
}

// destructor
MessageImporter::~MessageImporter()
{
}

// Import
status_t
MessageImporter::Import(Icon* icon, BPositionIO* stream)
{
	status_t ret = Init(icon);
	if (ret < B_OK) {
		printf("MessageImporter::Import() - "
			   "Init() error: %s\n", strerror(ret));
		return ret;
	}

	BMessage archive;
	ret = archive.Unflatten(stream);
	if (ret < B_OK) {
		printf("MessageImporter::Import() - "
			   "error unflattening icon archive: %s\n", strerror(ret));
		return ret;
	}

	// paths
	PathContainer* paths = icon->Paths();
	ret = _ImportPaths(&archive, paths);
	if (ret < B_OK) {
		printf("MessageImporter::Import() - "
			   "error importing paths: %s\n", strerror(ret));
		return ret;
	}

	// styles
	StyleContainer* styles = icon->Styles();
	ret = _ImportStyles(&archive, styles);
	if (ret < B_OK) {
		printf("MessageImporter::Import() - "
			   "error importing styles: %s\n", strerror(ret));
		return ret;
	}

	// shapes
	ret = _ImportShapes(&archive, paths, styles, icon->Shapes());
	if (ret < B_OK) {
		printf("MessageImporter::Import() - "
			   "error importing shapes: %s\n", strerror(ret));
		return ret;
	}

	return B_OK;
}

// #pragma mark -

// _ImportPaths
status_t
MessageImporter::_ImportPaths(const BMessage* archive,
							  PathContainer* paths) const
{
	BMessage allPaths;
	status_t ret = archive->FindMessage("paths", &allPaths);
	if (ret < B_OK)
		return ret;

	BMessage pathArchive;
	for (int32 i = 0;
		 allPaths.FindMessage("path", i, &pathArchive) == B_OK; i++) {
		VectorPath* path = new (nothrow) VectorPath(&pathArchive);
		if (!path || !paths->AddPath(path)) {
			delete path;
			ret = B_NO_MEMORY;
		}
		if (ret < B_OK)
			break;
	}

	return ret;
}

// _ImportStyles
status_t
MessageImporter::_ImportStyles(const BMessage* archive,
							   StyleContainer* styles) const
{
	BMessage allStyles;
	status_t ret = archive->FindMessage("styles", &allStyles);
	if (ret < B_OK)
		return ret;

	BMessage styleArchive;
	for (int32 i = 0;
		 allStyles.FindMessage("style", i, &styleArchive) == B_OK; i++) {
		Style* style = new (nothrow) Style(&styleArchive);
		if (!style || !styles->AddStyle(style)) {
			delete style;
			ret = B_NO_MEMORY;
		}
		if (ret < B_OK)
			break;
	}

	return ret;
}

// _ImportShapes
status_t
MessageImporter::_ImportShapes(const BMessage* archive,
							   PathContainer* paths,
							   StyleContainer* styles,
							   ShapeContainer* shapes) const
{
	BMessage allShapes;
	status_t ret = archive->FindMessage("shapes", &allShapes);
	if (ret < B_OK)
		return ret;

	BMessage shapeArchive;
	for (int32 i = 0;
		 allShapes.FindMessage("shape", i, &shapeArchive) == B_OK; i++) {
		// find the right style
		int32 styleIndex;
		if (shapeArchive.FindInt32("style ref", &styleIndex) < B_OK) {
			printf("MessageImporter::_ImportShapes() - "
				   "Shape %ld doesn't reference a Style!", i);
			continue;
		}
		Style* style = styles->StyleAt(StyleIndexFor(styleIndex));
		if (!style) {
			printf("MessageImporter::_ImportShapes() - "
				   "Shape %ld wants Style %ld, which does not exist\n",
				i, styleIndex);
			continue;
		}

		// create shape
		Shape* shape = new (nothrow) Shape(style);
		if (!shape || shape->InitCheck() < B_OK || !shapes->AddShape(shape)) {
			delete shape;
			ret = B_NO_MEMORY;
		}
		if (ret < B_OK)
			break;

		// find the referenced paths
		int32 pathIndex;
		for (int32 j = 0;
			 shapeArchive.FindInt32("path ref", j, &pathIndex) == B_OK;
			 j++) {
			VectorPath* path = paths->PathAt(PathIndexFor(pathIndex));
			if (!path) {
				printf("MessageImporter::_ImportShapes() - "
					   "Shape %ld referenced path %ld, "
					   "which does not exist\n", i, pathIndex);
				continue;
			}
			shape->Paths()->AddPath(path);
		}

		// Shape properties
		if (ret == B_OK)
			shape->Unarchive(&shapeArchive);
	}

	return ret;
}

