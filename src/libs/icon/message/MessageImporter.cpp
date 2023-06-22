/*
 * Copyright 2006-2010, 2023, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */


#include "MessageImporter.h"

#include <new>
#include <stdio.h>

#include <Archivable.h>
#ifdef ICON_O_MATIC
#include <Bitmap.h>
#endif
#include <ByteOrder.h>
#include <DataIO.h>
#include <Message.h>

#include "Container.h"
#include "Defines.h"
#include "Icon.h"
#include "PathSourceShape.h"
#ifdef ICON_O_MATIC
#include "ReferenceImage.h"
#endif
#include "Shape.h"
#include "Style.h"
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
	Container<VectorPath>* paths = icon->Paths();
	ret = _ImportPaths(&archive, paths);
	if (ret < B_OK) {
		printf("MessageImporter::Import() - "
			   "error importing paths: %s\n", strerror(ret));
		return ret;
	}

	// styles
	Container<Style>* styles = icon->Styles();
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
	Container<VectorPath>* paths) const
{
	BMessage allPaths;
	status_t ret = archive->FindMessage("paths", &allPaths);
	if (ret < B_OK)
		return ret;

	BMessage pathArchive;
	for (int32 i = 0;
		 allPaths.FindMessage("path", i, &pathArchive) == B_OK; i++) {
		VectorPath* path = new (nothrow) VectorPath(&pathArchive);
		if (!path || !paths->AddItem(path)) {
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
	Container<Style>* styles) const
{
	BMessage allStyles;
	status_t ret = archive->FindMessage("styles", &allStyles);
	if (ret < B_OK)
		return ret;

	BMessage styleArchive;
	for (int32 i = 0;
		 allStyles.FindMessage("style", i, &styleArchive) == B_OK; i++) {
		Style* style = new (nothrow) Style(&styleArchive);
		if (!style || !styles->AddItem(style)) {
			delete style;
			ret = B_NO_MEMORY;
		}
		if (ret < B_OK)
			break;
	}

	return ret;
}


status_t
MessageImporter::_ImportShapes(const BMessage* archive, Container<VectorPath>* paths,
	Container<Style>* styles, Container<Shape>* shapes) const
{
	BMessage allShapes;
	status_t ret = archive->FindMessage("shapes", &allShapes);
	if (ret < B_OK)
		return ret;

	BMessage shapeArchive;
	for (int32 i = 0;
		 allShapes.FindMessage("shape", i, &shapeArchive) == B_OK; i++) {
		int32 type;
		status_t typeFound = shapeArchive.FindInt32("type", &type);
		if (typeFound != B_OK || type == PathSourceShape::archive_code) {
				// Type not being found shows an older format that did not support
				// reference images

			// find the right style
			int32 styleIndex;
			if (shapeArchive.FindInt32("style ref", &styleIndex) < B_OK) {
				printf("MessageImporter::_ImportShapes() - "
					   "Shape %" B_PRId32 " doesn't reference a Style!", i);
				continue;
			}
	#ifdef ICON_O_MATIC
			Style* style = styles->ItemAt(StyleIndexFor(styleIndex));
	#else
			Style* style = styles->ItemAt(styleIndex);
	#endif
			if (style == NULL) {
				printf("MessageImporter::_ImportShapes() - "
					   "Shape %" B_PRId32 " wants Style %" B_PRId32 ", which does not exist\n",
					i, styleIndex);
				continue;
			}

			// create shape
			PathSourceShape* shape = new (nothrow) PathSourceShape(style);
			if (shape == NULL || shape->InitCheck() < B_OK || !shapes->AddItem(shape)) {
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
				VectorPath* path = paths->ItemAt(PathIndexFor(pathIndex));
	#else
				VectorPath* path = paths->ItemAt(pathIndex);
	#endif
				if (path == NULL) {
					printf("MessageImporter::_ImportShapes() - "
						   "Shape %" B_PRId32 " referenced path %" B_PRId32 ", "
						   "which does not exist\n", i, pathIndex);
					continue;
				}
				shape->Paths()->AddItem(path);
			}

			// Shape properties
			if (ret == B_OK)
				shape->Unarchive(&shapeArchive);
		}
	#ifdef ICON_O_MATIC
		else if (type == ReferenceImage::archive_code) {
			ReferenceImage* shape = new (nothrow) ReferenceImage(&shapeArchive);
			if (shape == NULL || shape->InitCheck() < B_OK || !shapes->AddItem(shape)) {
				delete shape;
				ret = B_NO_MEMORY;
			}
		}
	#endif // ICON_O_MATIC
	}

	return ret;
}
