/*
 * Copyright 2006-2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "MessageImporter.h"

#include <new>
#include <stdio.h>

#include <Archivable.h>
#include <ByteOrder.h>
#include <DataIO.h>
#include <Message.h>

#include "Defines.h"
#include "Icon.h"
#include "PathContainer.h"
#include "Shape.h"
#include "Style.h"
#include "StyleContainer.h"
#include "VectorPath.h"


using std::nothrow;


MessageImporter::MessageImporter()
#ifdef ICON_O_MATIC
	: Importer()
#endif
{
}


MessageImporter::~MessageImporter()
{
}


status_t
MessageImporter::Import(Icon* icon, BPositionIO* stream)
{
#ifdef ICON_O_MATIC
	status_t ret = Init(icon);
	if (ret < B_OK) {
		printf("MessageImporter::Import() - "
			   "Init() error: %s\n", strerror(ret));
		return ret;
	}
#else
	status_t ret;
#endif

	uint32 magic = 0;
	ssize_t size = sizeof(magic);
	off_t position = stream->Position();
	ssize_t read = stream->Read(&magic, size);
	if (read != size) {
		if (read < 0)
			ret = (status_t)read;
		else
			ret = B_IO_ERROR;
		return ret;
	}

	if (B_BENDIAN_TO_HOST_INT32(magic) != kNativeIconMagicNumber) {
		// this might be an old native icon file, where
		// we didn't prepend the magic number yet, seek back
		if (stream->Seek(position, SEEK_SET) != position) {
			printf("MessageImporter::Import() - "
				   "failed to seek back to beginning of stream\n");
			return B_IO_ERROR;
		}
	}

	BMessage archive;
	ret = archive.Unflatten(stream);
	if (ret != B_OK)
		return ret;

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


status_t
MessageImporter::_ImportShapes(const BMessage* archive, PathContainer* paths,
	StyleContainer* styles, ShapeContainer* shapes) const
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
#ifdef ICON_O_MATIC
		Style* style = styles->StyleAt(StyleIndexFor(styleIndex));
#else
		Style* style = styles->StyleAt(styleIndex);
#endif
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
#ifdef ICON_O_MATIC
			VectorPath* path = paths->PathAt(PathIndexFor(pathIndex));
#else
			VectorPath* path = paths->PathAt(pathIndex);
#endif
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
