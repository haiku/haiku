/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT/X11 license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */


#include "Snapshot.h"

#include <stdio.h>
#include <string.h>

#include <Directory.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Volume.h>


static const char* kFileType = B_FILE_MIME_TYPE;
static const char* kDirType = "application/x-vnd.Be-directory";
static const char* kVolumeType = "application/x-vnd.Be-volume";


FileInfo::FileInfo()
	:
	pseudo(false),
	size(0),
	count(0),
	parent(NULL),
	children()
{
}


FileInfo::~FileInfo()
{
	while (children.size() != 0) {
		FileInfo* child = *children.begin();
		delete child;
		children.erase(children.begin());
	}
}


void
FileInfo::GetPath(string& path) const
{
	if (pseudo) {
		path.assign(ref.name);
	} else {
		BEntry entry(&ref, true);
		BPath pathObj(&entry);
		path.assign(pathObj.Path());
	}
}


FileInfo*
FileInfo::FindChild(const char* name) const
{
	vector<FileInfo*>::const_iterator i = children.begin();
	while (i != children.end()) {
		if (strcmp((*i)->ref.name, name) == 0)
			return *i;
		i++;
	}

	return NULL;
}


BMimeType*
FileInfo::Type() const
{
	char mimeStr[B_MIME_TYPE_LENGTH] = { '\0' };
	if (parent == NULL) {
		// This is the volume's root directory; treat it as a volume type.
		strlcpy(mimeStr, kVolumeType, sizeof(mimeStr));
	} else {
		// Get the MIME type from the registrar.
		BNode node(&ref);
		if (node.InitCheck() == B_OK) {
			BNodeInfo nodeInfo(&node);
			if (nodeInfo.InitCheck() == B_OK) {
				status_t s = nodeInfo.GetType(mimeStr);
				if (s != B_OK && children.size() > 0) {
					if (s == B_ENTRY_NOT_FOUND) {
						// This status appears to be returned only for files on
						// BFS volumes (e.g., CDFS volumes return B_BAD_VALUE).
						//nodeInfo.SetType(kDirType);
					}
					strlcpy(mimeStr, kDirType, sizeof(mimeStr));
				}
			}
		}
	}

	if (strlen(mimeStr) == 0)
		strlcpy(mimeStr, kFileType, sizeof(mimeStr));

	return new BMimeType(mimeStr);
}


// #pragma mark -


VolumeSnapshot::VolumeSnapshot(const BVolume* volume)
{
	char nameBuffer[B_FILE_NAME_LENGTH];
	volume->GetName(nameBuffer);
	name = nameBuffer;

	capacity = volume->Capacity();
	freeBytes = volume->FreeBytes();
	rootDir = NULL;
	freeSpace = NULL;
}


VolumeSnapshot::~VolumeSnapshot()
{
	delete rootDir;
	delete freeSpace;
}
