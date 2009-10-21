/*
 * Copyright 2008, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 1998-2007, Matthijs Hollemans.
 *
 * Distributed under the terms of the MIT License.
 */


#include "FileIterator.h"

#include <string.h>

#include <Entry.h>
#include <NodeInfo.h>
#include <Path.h>


FileIterator::FileIterator()
{
}


FileIterator::~FileIterator()
{
}


bool
FileIterator::_ExamineFile(BEntry& entry, char* buffer, bool textFilesOnly)
{
	BPath path;
	if (entry.GetPath(&path) != B_OK)
		return false;

	strcpy(buffer, path.Path());

	if (!textFilesOnly)
		return true;

	BMimeType mimeType;
	BNode node(&entry);
	BNodeInfo nodeInfo(&node);
	char mimeTypeString[B_MIME_TYPE_LENGTH];

	if (nodeInfo.GetType(mimeTypeString) != B_OK) {
		// try to get a MIME type before failing
		if (BMimeType::GuessMimeType(path.Path(), &mimeType) != B_OK)
			return false;

		nodeInfo.SetType(mimeType.Type());
	} else
		mimeType.SetTo(mimeTypeString);

	BMimeType superType;
	if (mimeType.GetSupertype(&superType) == B_OK) {
		if (strcmp("text", superType.Type()) == 0
			|| strcmp("message", superType.Type()) == 0) {
			return true;
		}
	}

	return false;
}

