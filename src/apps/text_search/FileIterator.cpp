/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include "FileIterator.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Directory.h>
#include <NodeInfo.h>
#include <Path.h>

#include "Model.h"

using std::nothrow;

// TODO: stippi: Check if this is a the best place to maintain a global
// list of files and folders for node monitoring. It should probably monitor
// every file that was grepped, as well as every visited (sub) folder.
// For the moment I don't know the life cycle of the FileIterator object.


FileIterator::FileIterator(Model* model) 
	: fDirectories(10),
	  fCurrentDir(new (nothrow) BDirectory(&model->fDirectory)),
	  fCurrentRef(0),
	  fModel(model)
{
	if (!fCurrentDir || !fDirectories.AddItem(fCurrentDir)) {
		// init error
		delete fCurrentDir;
		fCurrentDir = NULL;
	}
}


FileIterator::~FileIterator()
{
	for (int32 i = fDirectories.CountItems() - 1; i >= 0; i--)
		delete (BDirectory*)fDirectories.ItemAt(i);
}


bool
FileIterator::IsValid() const
{
	return fCurrentDir != NULL;
}


bool
FileIterator::GetNextName(char* buffer)
{
	BEntry entry;
	struct stat fileStat;

	while (true) {
		// Traverse the directory to get a new BEntry.
		// _GetNextEntry returns false if there are no
		// more entries, and we exit the loop.

		if (!_GetNextEntry(entry))
			return false;

		// If the entry is a subdir, then add it to the
		// list of directories and continue the loop. 
		// If the entry is a file and we can grep it
		// (i.e. it is a text file), then we're done
		// here. Otherwise, continue with the next entry.

		if (entry.GetStat(&fileStat) == B_OK) {
			if (S_ISDIR(fileStat.st_mode)) {
				// subdir
				_ExamineSubdir(entry);
			} else {
				// file or a (non-traversed) symbolic link
				if (_ExamineFile(entry, buffer))
					return true;
			}
		}
	}
}


// #pragma mark - private


bool
FileIterator::_GetNextEntry(BEntry& entry)
{
	if (fDirectories.CountItems() == 1)
		return _GetTopEntry(entry);
	else
		return _GetSubEntry(entry);
}


bool
FileIterator::_GetTopEntry(BEntry& entry)
{
	// If the user selected one or more files, we must look 
	// at the "refs" inside the message that was passed into 
	// our add-on's process_refs(). If the user didn't select 
	// any files, we will simply read all the entries from the 
	// current working directory.
	
	entry_ref fileRef;

	if (fModel->fSelectedFiles.FindRef("refs", fCurrentRef, &fileRef) == B_OK) {
		entry.SetTo(&fileRef, fModel->fRecurseLinks);
		++fCurrentRef;
		return true;
	} else if (fCurrentRef > 0) {
		// when we get here, we have processed
		// all the refs from the message
		return false;
	} else if (fCurrentDir != NULL) {
		// examine the whole directory
		return fCurrentDir->GetNextEntry(&entry,
			fModel->fRecurseLinks) == B_OK;
	}

	return false;
}


bool
FileIterator::_GetSubEntry(BEntry& entry)
{
	if (!fCurrentDir)
		return false;

	if (fCurrentDir->GetNextEntry(&entry, fModel->fRecurseLinks) == B_OK)
		return true;

	// If we get here, there are no more entries in 
	// this subdir, so return to the parent directory.

	fDirectories.RemoveItem(fCurrentDir);
	delete fCurrentDir;
	fCurrentDir = (BDirectory*)fDirectories.LastItem();

	return _GetNextEntry(entry);
}


void
FileIterator::_ExamineSubdir(BEntry& entry)
{
	if (!fModel->fRecurseDirs)
		return;

	if (fModel->fSkipDotDirs) {
		char nameBuf[B_FILE_NAME_LENGTH];
		if (entry.GetName(nameBuf) == B_OK) {
			if (*nameBuf == '.')
				return;
		}
	}

	BDirectory* dir = new (nothrow) BDirectory(&entry);
	if (dir == NULL || dir->InitCheck() != B_OK
		|| !fDirectories.AddItem(dir)) {
		// clean up
		delete dir;
		return;
	}

	fCurrentDir = dir;
}


bool
FileIterator::_ExamineFile(BEntry& entry, char* buffer)
{
	BPath path;
	if (entry.GetPath(&path) != B_OK)
		return false;

	strcpy(buffer, path.Path());

	if (!fModel->fTextOnly)
		return true;

	BNode node(&entry);
	BNodeInfo nodeInfo(&node);
	char mimeTypeString[B_MIME_TYPE_LENGTH];

	if (nodeInfo.GetType(mimeTypeString) == B_OK) {
		BMimeType mimeType(mimeTypeString);
		BMimeType superType;

		if (mimeType.GetSupertype(&superType) == B_OK) {
			if (strcmp("text", superType.Type()) == 0
				|| strcmp("message", superType.Type()) == 0) {
				return true;
			}
		}
	}

	return false;
}

