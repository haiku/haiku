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

#include "InitialIterator.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Directory.h>

#include "Model.h"

using std::nothrow;

// TODO: stippi: Check if this is a the best place to maintain a global
// list of files and folders for node monitoring. It should probably monitor
// every file that was grepped, as well as every visited (sub) folder.
// For the moment I don't know the life cycle of the InitialIterator object.


InitialIterator::InitialIterator(const Model* model) 
	: FileIterator(),
	  fDirectories(10),
	  fCurrentDir(new (nothrow) BDirectory(&model->fDirectory)),
	  fCurrentRef(0),

	  fSelectedFiles(model->fSelectedFiles),

	  fRecurseDirs(model->fRecurseDirs),
	  fRecurseLinks(model->fRecurseLinks),
	  fSkipDotDirs(model->fSkipDotDirs),
	  fTextOnly(model->fTextOnly)
{
	if (!fCurrentDir || !fDirectories.AddItem(fCurrentDir)) {
		// init error
		delete fCurrentDir;
		fCurrentDir = NULL;
	}
}


InitialIterator::~InitialIterator()
{
	for (int32 i = fDirectories.CountItems() - 1; i >= 0; i--)
		delete (BDirectory*)fDirectories.ItemAt(i);
}


bool
InitialIterator::IsValid() const
{
	return fCurrentDir != NULL;
}


bool
InitialIterator::GetNextName(char* buffer)
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
				if (_ExamineFile(entry, buffer, fTextOnly))
					return true;
			}
		}
	}
}


bool
InitialIterator::NotifyNegatives() const
{
	return false;
}


bool
InitialIterator::GetTopEntry(BEntry& entry)
{
	// If the user selected one or more files, we must look 
	// at the "refs" inside the message that was passed into 
	// our add-on's process_refs(). If the user didn't select 
	// any files, we will simply read all the entries from the 
	// current working directory.
	
	entry_ref fileRef;

	if (fSelectedFiles.FindRef("refs", fCurrentRef, &fileRef) == B_OK) {
		entry.SetTo(&fileRef, fRecurseLinks);
		++fCurrentRef;
		return true;
	} else if (fCurrentRef > 0) {
		// when we get here, we have processed
		// all the refs from the message
		return false;
	} else if (fCurrentDir != NULL) {
		// examine the whole directory
		return fCurrentDir->GetNextEntry(&entry, fRecurseLinks) == B_OK;
	}

	return false;
}


bool
InitialIterator::FollowSubdir(BEntry& entry) const
{
	if (!fRecurseDirs)
		return false;

	if (fSkipDotDirs) {
		char nameBuf[B_FILE_NAME_LENGTH];
		if (entry.GetName(nameBuf) == B_OK) {
			if (*nameBuf == '.')
				return false;
		}
	}

	return true;
}


// #pragma mark - private


bool
InitialIterator::_GetNextEntry(BEntry& entry)
{
	if (fDirectories.CountItems() == 1)
		return GetTopEntry(entry);
	else
		return _GetSubEntry(entry);
}


bool
InitialIterator::_GetSubEntry(BEntry& entry)
{
	if (!fCurrentDir)
		return false;

	if (fCurrentDir->GetNextEntry(&entry, fRecurseLinks) == B_OK)
		return true;

	// If we get here, there are no more entries in 
	// this subdir, so return to the parent directory.

	fDirectories.RemoveItem(fCurrentDir);
	delete fCurrentDir;
	fCurrentDir = (BDirectory*)fDirectories.LastItem();

	return _GetNextEntry(entry);
}


void
InitialIterator::_ExamineSubdir(BEntry& entry)
{
	if (!FollowSubdir(entry))
		return;

	BDirectory* dir = new (nothrow) BDirectory(&entry);
	if (dir == NULL || dir->InitCheck() != B_OK || !fDirectories.AddItem(dir)) {
		// clean up
		delete dir;
		return;
	}

	fCurrentDir = dir;
}
