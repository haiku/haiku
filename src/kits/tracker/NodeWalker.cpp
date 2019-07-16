/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/


#include <Debug.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <SupportDefs.h>

#include "NodeWalker.h"


namespace BTrackerPrivate {

TWalker::~TWalker()
{
}


// all the following calls are pure virtuals, should not get called
status_t
TWalker::GetNextEntry(BEntry*, bool )
{
	TRESPASS();
	return B_ERROR;
}


status_t
TWalker::GetNextRef(entry_ref*)
{
	TRESPASS();
	return B_ERROR;
}


int32
TWalker::GetNextDirents(struct dirent*, size_t, int32)
{
	TRESPASS();
	return 0;
}


status_t
TWalker::Rewind()
{
	TRESPASS();
	return B_ERROR;
}


int32
TWalker::CountEntries()
{
	TRESPASS();
	return -1;
}


TNodeWalker::TNodeWalker(bool includeTopDirectory)
	:
	fDirs(20),
	fTopIndex(-1),
	fTopDir(NULL),
	fIncludeTopDir(includeTopDirectory),
	fOriginalIncludeTopDir(includeTopDirectory),
	fJustFile(NULL),
	fOriginalJustFile(NULL)
{
}


TNodeWalker::TNodeWalker(const char* path, bool includeTopDirectory)
	:
	fDirs(20),
	fTopIndex(-1),
	fTopDir(NULL),
	fIncludeTopDir(includeTopDirectory),
	fOriginalIncludeTopDir(includeTopDirectory),
	fJustFile(NULL),
	fOriginalDirCopy(path),
	fOriginalJustFile(NULL)
{
	if (fOriginalDirCopy.InitCheck() != B_OK) {
		// not a directory, set up walking a single file
		fJustFile = new BEntry(path);
		if (fJustFile->InitCheck() != B_OK) {
			delete fJustFile;
			fJustFile = NULL;
		}
		fOriginalJustFile = fJustFile;
	} else {
		fTopDir = new BDirectory(fOriginalDirCopy);
		fTopIndex++;
		fDirs.AddItem(fTopDir);
	}
}


TNodeWalker::TNodeWalker(const entry_ref* ref, bool includeTopDirectory)
	:
	fDirs(20),
	fTopIndex(-1),
	fTopDir(NULL),
	fIncludeTopDir(includeTopDirectory),
	fOriginalIncludeTopDir(includeTopDirectory),
	fJustFile(NULL),
	fOriginalDirCopy(ref),
	fOriginalJustFile(NULL)
{
	if (fOriginalDirCopy.InitCheck() != B_OK) {
		// not a directory, set up walking a single file
		fJustFile = new BEntry(ref);
		if (fJustFile->InitCheck() != B_OK) {
			delete fJustFile;
			fJustFile = NULL;
		}
		fOriginalJustFile = fJustFile;
	} else {
		fTopDir = new BDirectory(fOriginalDirCopy);
		fTopIndex++;
		fDirs.AddItem(fTopDir);
	}
}


TNodeWalker::TNodeWalker(const BDirectory* dir, bool includeTopDirectory)
	:
	fDirs(20),
	fTopIndex(-1),
	fTopDir(NULL),
	fIncludeTopDir(includeTopDirectory),
	fOriginalIncludeTopDir(includeTopDirectory),
	fJustFile(NULL),
	fOriginalDirCopy(*dir),
	fOriginalJustFile(NULL)
{
	fTopDir = new BDirectory(*dir);
	fTopIndex++;
	fDirs.AddItem(fTopDir);
}


TNodeWalker::TNodeWalker()
	:
	fDirs(20),
	fTopIndex(-1),
	fTopDir(NULL),
	fIncludeTopDir(false),
	fOriginalIncludeTopDir(false),
	fJustFile(NULL),
	fOriginalJustFile(NULL)
{
}


TNodeWalker::TNodeWalker(const char* path)
	:
	fDirs(20),
	fTopIndex(-1),
	fTopDir(NULL),
	fIncludeTopDir(false),
	fOriginalIncludeTopDir(false),
	fJustFile(NULL),
	fOriginalDirCopy(path),
	fOriginalJustFile(NULL)
{
	if (fOriginalDirCopy.InitCheck() != B_OK) {
		// not a directory, set up walking a single file
		fJustFile = new BEntry(path);
		if (fJustFile->InitCheck() != B_OK) {
			delete fJustFile;
			fJustFile = NULL;
		}
		fOriginalJustFile = fJustFile;
	} else {
		fTopDir = new BDirectory(fOriginalDirCopy);
		fTopIndex++;
		fDirs.AddItem(fTopDir);
	}
}


TNodeWalker::TNodeWalker(const entry_ref* ref)
	:
	fDirs(20),
	fTopIndex(-1),
	fTopDir(NULL),
	fIncludeTopDir(false),
	fOriginalIncludeTopDir(false),
	fJustFile(NULL),
	fOriginalDirCopy(ref),
	fOriginalJustFile(NULL)
{
	if (fOriginalDirCopy.InitCheck() != B_OK) {
		// not a directory, set up walking a single file
		fJustFile = new BEntry(ref);
		if (fJustFile->InitCheck() != B_OK) {
			delete fJustFile;
			fJustFile = NULL;
		}
		fOriginalJustFile = fJustFile;
	} else {
		fTopDir = new BDirectory(fOriginalDirCopy);
		fTopIndex++;
		fDirs.AddItem(fTopDir);
	}
}

TNodeWalker::TNodeWalker(const BDirectory* dir)
	:
	fDirs(20),
	fTopIndex(-1),
	fTopDir(NULL),
	fIncludeTopDir(false),
	fOriginalIncludeTopDir(false),
	fJustFile(NULL),
	fOriginalDirCopy(*dir),
	fOriginalJustFile(NULL)
{
	fTopDir = new BDirectory(*dir);
	fTopIndex++;
	fDirs.AddItem(fTopDir);
}


TNodeWalker::~TNodeWalker()
{
	delete fOriginalJustFile;

	for (;;) {
		BDirectory* directory = fDirs.RemoveItemAt(fTopIndex--);
		if (directory == NULL)
			break;

		delete directory;
	}
}


status_t
TNodeWalker::PopDirCommon()
{
	ASSERT(fTopIndex >= 0);

	// done with the old dir, pop it
	fDirs.RemoveItemAt(fTopIndex);
	fTopIndex--;
	delete fTopDir;
	fTopDir = NULL;

	if (fTopIndex == -1) {
		// done
		return B_ENTRY_NOT_FOUND;
	}

	// point to the new top dir
	fTopDir = fDirs.ItemAt(fTopIndex);

	return B_OK;
}


void
TNodeWalker::PushDirCommon(const entry_ref* ref)
{
	fTopDir = new BDirectory(ref);
		// OK to ignore error here. Will
		// catch at next call to GetNextEntry
	fTopIndex++;
	fDirs.AddItem(fTopDir);
}


status_t
TNodeWalker::GetNextEntry(BEntry* entry, bool traverse)
{
	if (fJustFile != NULL) {
		*entry = *fJustFile;
		fJustFile = 0;
		return B_OK;
	}

	if (fTopDir == NULL) {
		// done
		return B_ENTRY_NOT_FOUND;
	}

	// If requested to include the top directory, return that first.
	if (fIncludeTopDir) {
		fIncludeTopDir = false;
		return fTopDir->GetEntry(entry);
	}

	// Get the next entry.
	status_t result = fTopDir->GetNextEntry(entry, traverse);
	if (result != B_OK) {
		result = PopDirCommon();
		if (result != B_OK)
			return result;

		return GetNextEntry(entry, traverse);
	}
	// See if this entry is a directory. If it is then push it onto the
	// stack
	entry_ref ref;
	result = entry->GetRef(&ref);

	if (result == B_OK && fTopDir->Contains(ref.name, B_DIRECTORY_NODE))
		PushDirCommon(&ref);

	return result;
}


status_t
TNodeWalker::GetNextRef(entry_ref* ref)
{
	if (fJustFile != NULL) {
		fJustFile->GetRef(ref);
		fJustFile = 0;
		return B_OK;
	}

	if (fTopDir == NULL) {
		// done
		return B_ENTRY_NOT_FOUND;
	}

	// If requested to include the top directory, return that first.
	if (fIncludeTopDir) {
		fIncludeTopDir = false;
		BEntry entry;
		status_t err = fTopDir->GetEntry(&entry);
		if (err == B_OK)
			err = entry.GetRef(ref);
		return err;
	}

	// get the next entry
	status_t err = fTopDir->GetNextRef(ref);
	if (err != B_OK) {
		err = PopDirCommon();
		if (err != B_OK)
			return err;
		return GetNextRef(ref);
	}

	// See if this entry is a directory, if it is then push it onto the stack.
	if (fTopDir->Contains(ref->name, B_DIRECTORY_NODE))
		PushDirCommon(ref);

	return B_OK;
}


static int32
build_dirent(const BEntry* source, struct dirent* ent,
	size_t size, int32 count)
{
	if (source == NULL)
		return 0;

	entry_ref ref;
	source->GetRef(&ref);

	size_t recordLength = strlen(ref.name) + sizeof(dirent);
	if (recordLength > size || count <= 0) {
		// can't fit in buffer, bail
		return 0;
	}

	// info about this node
	ent->d_reclen = static_cast<ushort>(recordLength);
	strcpy(ent->d_name, ref.name);
	ent->d_dev = ref.device;
	ent->d_ino = ref.directory;

	// info about the parent
	BEntry parent;
	source->GetParent(&parent);
	if (parent.InitCheck() == B_OK) {
		entry_ref parentRef;
		parent.GetRef(&parentRef);
		ent->d_pdev = parentRef.device;
		ent->d_pino = parentRef.directory;
	} else {
		ent->d_pdev = 0;
		ent->d_pino = 0;
	}

	return 1;
}


int32
TNodeWalker::GetNextDirents(struct dirent* ent, size_t size, int32 count)
{
	if (fJustFile != NULL) {
		if (count == 0)
			return 0;

		// simulate GetNextDirents by building a single dirent structure
		int32 result = build_dirent(fJustFile, ent, size, count);
		fJustFile = 0;
		return result;
	}

	if (fTopDir == NULL) {
		// done
		return 0;
	}

	// If requested to include the top directory, return that first.
	if (fIncludeTopDir) {
		fIncludeTopDir = false;
		BEntry entry;
		if (fTopDir->GetEntry(&entry) < B_OK)
			return 0;

		return build_dirent(fJustFile, ent, size, count);
	}

	// get the next entry
	int32 nextDirent = fTopDir->GetNextDirents(ent, size, count);
	if (nextDirent == 0) {
		status_t result = PopDirCommon();
		if (result != B_OK)
			return 0;

		return GetNextDirents(ent, size, count);
	}

	// push any directories in the returned entries onto the stack
	for (int32 i = 0; i < nextDirent; i++) {
		if (fTopDir->Contains(ent->d_name, B_DIRECTORY_NODE)) {
			entry_ref ref(ent->d_dev, ent->d_ino, ent->d_name);
			PushDirCommon(&ref);
		}
		ent = (dirent*)((char*)ent + ent->d_reclen);
	}

	return nextDirent;
}


status_t
TNodeWalker::Rewind()
{
	if (fOriginalJustFile != NULL) {
		// single file mode, rewind by pointing to the original file
		fJustFile = fOriginalJustFile;
		return B_OK;
	}

	// pop all the directories and point to the initial one
	for (;;) {
		BDirectory* directory = fDirs.RemoveItemAt(fTopIndex--);
		if (directory == NULL)
			break;

		delete directory;
	}

	fTopDir = new BDirectory(fOriginalDirCopy);
	fTopIndex = 0;
	fIncludeTopDir = fOriginalIncludeTopDir;
	fDirs.AddItem(fTopDir);

	return fTopDir->Rewind();
		// rewind the directory
}

int32
TNodeWalker::CountEntries()
{
	// should not be calling this
	TRESPASS();
	return -1;
}


TVolWalker::TVolWalker(bool knowsAttributes, bool writable,
	bool includeTopDirectory)
	:
	TNodeWalker(includeTopDirectory),
	fKnowsAttr(knowsAttributes),
	fWritable(writable)
{
	// Get things initialized. Find first volume, or find the first volume
	// that supports attributes.
 	NextVolume();
}


TVolWalker::~TVolWalker()
{
}


status_t
TVolWalker::NextVolume()
{
	// The stack of directoies should be empty.
	ASSERT(fTopIndex == -1);
	ASSERT(fTopDir == NULL);

	status_t result;
	do {
		result = fVolRoster.GetNextVolume(&fVol);
		if (result != B_OK)
			break;
	} while ((fKnowsAttr && !fVol.KnowsAttr())
		|| (fWritable && fVol.IsReadOnly()));

	if (result == B_OK) {
		// Get the root directory to get things started. There's always
		// a root directory for a volume. So if there is an error then it
		// means that something is really bad, like the system is out of
		// memory.  In that case don't worry about truying to skip to the
		// next volume.
		fTopDir = new BDirectory();
		result = fVol.GetRootDirectory(fTopDir);
		fIncludeTopDir = fOriginalIncludeTopDir;
		fTopIndex = 0;
		fDirs.AddItem(fTopDir);
	}

	return result;
}

status_t
TVolWalker::GetNextEntry(BEntry* entry, bool traverse)
{
	if (fTopDir == NULL)
		return B_ENTRY_NOT_FOUND;

	// get the next entry
	status_t result = _inherited::GetNextEntry(entry, traverse);
	while (result != B_OK) {
		// we're done with the current volume, go to the next one
		result = NextVolume();
		if (result != B_OK)
			break;

		result = GetNextEntry(entry, traverse);
	}

	return result;
}


status_t
TVolWalker::GetNextRef(entry_ref* ref)
{
	if (fTopDir == NULL)
		return B_ENTRY_NOT_FOUND;

	// Get the next ref.
	status_t result = _inherited::GetNextRef(ref);

	while (result != B_OK) {
		// we're done with the current volume, go to the next one
		result = NextVolume();
		if (result != B_OK)
			break;
		result = GetNextRef(ref);
	}

	return result;
}


int32
TVolWalker::GetNextDirents(struct dirent* ent, size_t size, int32 count)
{
	if (fTopDir == NULL)
		return B_ENTRY_NOT_FOUND;

	// get the next dirent
	status_t result = _inherited::GetNextDirents(ent, size, count);
	while (result != B_OK) {
		// we're done with the current volume, go to the next one
		result = NextVolume();
		if (result != B_OK)
			break;

		result = GetNextDirents(ent, size, count);
	}

	return result;
}


status_t
TVolWalker::Rewind()
{
	fVolRoster.Rewind();
	return NextVolume();
}


TQueryWalker::TQueryWalker(const char* predicate)
	:
	TWalker(),
	fTime(0)
{
	fPredicate = strdup(predicate);
	NextVolume();
}


TQueryWalker::~TQueryWalker()
{
	free((char*)fPredicate);
	fPredicate = NULL;
}


status_t
TQueryWalker::GetNextEntry(BEntry* entry, bool traverse)
{
	status_t result;
	do {
		result = fQuery.GetNextEntry(entry, traverse);
		if (result == B_ENTRY_NOT_FOUND) {
			if (NextVolume() != B_OK)
				break;
		}
	} while (result == B_ENTRY_NOT_FOUND);

	return result;
}


status_t
TQueryWalker::GetNextRef(entry_ref* ref)
{
	status_t result;

	for (;;) {
		result = fQuery.GetNextRef(ref);
		if (result != B_ENTRY_NOT_FOUND)
			break;

		result = NextVolume();
		if (result != B_OK)
			break;
	}

	return result;
}


int32
TQueryWalker::GetNextDirents(struct dirent* ent, size_t size, int32 count)
{
	int32 result;

	for (;;) {
		result = fQuery.GetNextDirents(ent, size, count);
		if (result != 0)
			return result;

		if (NextVolume() != B_OK)
			return 0;
	}

	return result;
}


status_t
TQueryWalker::NextVolume()
{
	status_t result;
	do {
		result = fVolRoster.GetNextVolume(&fVol);
		if (result != B_OK)
			break;
	} while (!fVol.KnowsQuery());

	if (result == B_OK) {
		result = fQuery.Clear();
		result = fQuery.SetVolume(&fVol);
		result = fQuery.SetPredicate(fPredicate);
		result = fQuery.Fetch();
	}

	return result;
}


int32
TQueryWalker::CountEntries()
{
	// should not be calling this
	TRESPASS();
	return -1;
}


status_t
TQueryWalker::Rewind()
{
	fVolRoster.Rewind();
	return NextVolume();
}

}	// namespace BTrackerPrivate
