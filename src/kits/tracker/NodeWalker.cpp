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

// all the following calls are pure viruals, should not get called
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
	:	fDirs(20),
		fTopIndex(-1),
		fTopDir(0),
		fIncludeTopDir(includeTopDirectory),
		fOriginalIncludeTopDir(includeTopDirectory),
		fJustFile(0),
		fOriginalJustFile(0)
{
}


TNodeWalker::TNodeWalker(const char* path, bool includeTopDirectory)
	:	fDirs(20),
		fTopIndex(-1),
		fTopDir(0),
		fIncludeTopDir(includeTopDirectory),
		fOriginalIncludeTopDir(includeTopDirectory),
		fJustFile(0),
		fOriginalDirCopy(path),
		fOriginalJustFile(0)
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
	:	fDirs(20),
		fTopIndex(-1),
		fTopDir(0),
		fIncludeTopDir(includeTopDirectory),
		fOriginalIncludeTopDir(includeTopDirectory),
		fJustFile(0),
		fOriginalDirCopy(ref),
		fOriginalJustFile(0)
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
	:	fDirs(20),
		fTopIndex(-1),
		fTopDir(0),
		fIncludeTopDir(includeTopDirectory),
		fOriginalIncludeTopDir(includeTopDirectory),
		fJustFile(0),
		fOriginalDirCopy(*dir),
		fOriginalJustFile(0)
{
	fTopDir = new BDirectory(*dir);
	fTopIndex++;
	fDirs.AddItem(fTopDir);
}


TNodeWalker::TNodeWalker()
	:	fDirs(20),
		fTopIndex(-1),
		fTopDir(0),
		fIncludeTopDir(false),
		fOriginalIncludeTopDir(false),
		fJustFile(0),
		fOriginalJustFile(0)
{
}

TNodeWalker::TNodeWalker(const char* path)
	:	fDirs(20),
		fTopIndex(-1),
		fTopDir(0),
		fIncludeTopDir(false),
		fOriginalIncludeTopDir(false),
		fJustFile(0),
		fOriginalDirCopy(path),
		fOriginalJustFile(0)
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
	:	fDirs(20),
		fTopIndex(-1),
		fTopDir(0),
		fIncludeTopDir(false),
		fOriginalIncludeTopDir(false),
		fJustFile(0),
		fOriginalDirCopy(ref),
		fOriginalJustFile(0)
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
	:	fDirs(20),
		fTopIndex(-1),
		fTopDir(0),
		fIncludeTopDir(false),
		fOriginalIncludeTopDir(false),
		fJustFile(0),
		fOriginalDirCopy(*dir),
		fOriginalJustFile(0)
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

	if (fTopIndex == -1)
		// done
		return B_ENTRY_NOT_FOUND;

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
	if (fJustFile) {
		*entry = *fJustFile;
		fJustFile = 0;
		return B_OK;
	}

	if (!fTopDir)
		// done
		return B_ENTRY_NOT_FOUND;

	// If requested to include the top directory, return that first.
	if (fIncludeTopDir) {
		fIncludeTopDir = false;
		return fTopDir->GetEntry(entry);
	}
	
	// Get the next entry.
	status_t err = fTopDir->GetNextEntry(entry, traverse);

	if (err != B_OK) {
		err = PopDirCommon();
		if (err != B_OK)
			return err;
		return GetNextEntry(entry, traverse);
	}
	// See if this entry is a directory. If it is then push it onto the
	// stack
	entry_ref ref;
	err = entry->GetRef(&ref);

	if (err == B_OK && fTopDir->Contains(ref.name, B_DIRECTORY_NODE))
		PushDirCommon(&ref);

	return err;
}

status_t
TNodeWalker::GetNextRef(entry_ref* ref)
{
	if (fJustFile) {
		fJustFile->GetRef(ref);
		fJustFile = 0;
		return B_OK;
	}

	if (!fTopDir)
		// done
		return B_ENTRY_NOT_FOUND;

	// If requested to include the top directory, return that first.
	if (fIncludeTopDir) {
		fIncludeTopDir = false;
		BEntry entry;
		status_t err = fTopDir->GetEntry(&entry);
		if (err == B_OK)
			err = entry.GetRef(ref);
		return err;
	}
	
	// Get the next entry.
	status_t err = fTopDir->GetNextRef(ref);
	if (err != B_OK) {
		err = PopDirCommon();
		if (err != B_OK)
			return err;
		return GetNextRef(ref);
	}
	// See if this entry is a directory. If it is then push it onto the
	// stack

	if (fTopDir->Contains(ref->name, B_DIRECTORY_NODE))
		PushDirCommon(ref);

	return B_OK;
}

static int32
build_dirent(const BEntry* source, struct dirent* ent,
	size_t size, int32 count)
{
	entry_ref ref;
	source->GetRef(&ref);
		
	size_t recordLength = strlen(ref.name) + sizeof(dirent);
	if (recordLength > size || count <= 0)
		// can't fit in buffer, bail
		return 0;

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
	if (fJustFile) {
		if (!count)
			return 0;

		// simulate GetNextDirents by building a single dirent structure
		int32 result = build_dirent(fJustFile, ent, size, count);
		fJustFile = 0;
		return result;
	}

	if (!fTopDir)
		// done
		return 0;

	// If requested to include the top directory, return that first.
	if (fIncludeTopDir) {
		fIncludeTopDir = false;
		BEntry entry;
		if (fTopDir->GetEntry(&entry) < B_OK)
			return 0;

		return build_dirent(fJustFile, ent, size, count);
	}
	
	// Get the next entry.
	int32 result = fTopDir->GetNextDirents(ent, size, count);

	if (!result) {
		status_t err = PopDirCommon();
		if (err != B_OK)
			return 0;

		return GetNextDirents(ent, size, count);
	}

	// push any directories in the returned entries onto the stack
	for (int32 i = 0; i < result; i++) {
		if (fTopDir->Contains(ent->d_name, B_DIRECTORY_NODE)) {
			entry_ref ref(ent->d_dev, ent->d_ino, ent->d_name);
			PushDirCommon(&ref);
		}
		ent = (dirent*)((char*)ent + ent->d_reclen);
	}

	return result;
}

status_t
TNodeWalker::Rewind()
{
	if (fOriginalJustFile) {
		// single file mode, rewind by pointing to the original file
		fJustFile = fOriginalJustFile;
		return B_OK;
	}

	// pop all the directories and point to the initial one
	for (;;) {
		BDirectory* directory = fDirs.RemoveItemAt(fTopIndex--);
		if (!directory)
			break;
		delete directory;
	}

	fTopDir = new BDirectory(fOriginalDirCopy);
	fTopIndex = 0;
	fIncludeTopDir = fOriginalIncludeTopDir;
	fDirs.AddItem(fTopDir);
	// rewind the directory
	return fTopDir->Rewind();
}

int32
TNodeWalker::CountEntries()
{
	// should not be calling this
	TRESPASS();
	return -1;
}

TVolWalker::TVolWalker(bool knowsAttributes, bool writable, bool includeTopDirectory)
	: 	TNodeWalker(includeTopDirectory),
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
	status_t err;

	// The stack of directoies should be empty.
	ASSERT(fTopIndex == -1);
	ASSERT(fTopDir == NULL);

	do {
		err = fVolRoster.GetNextVolume(&fVol);
		if (err != B_OK)
			break;
	} while ((fKnowsAttr && !fVol.KnowsAttr()) || (fWritable && fVol.IsReadOnly()));

	if (err == B_OK) {
		// Get the root directory to get things started. There's always
		// a root directory for a volume. So if there is an error then it
		// means that something is really bad, like the system is out of
		// memory.  In that case don't worry about truying to skip to the
		// next volume.
		fTopDir = new BDirectory();
		err = fVol.GetRootDirectory(fTopDir);
		fIncludeTopDir = fOriginalIncludeTopDir;
		fTopIndex = 0;
		fDirs.AddItem(fTopDir);
	}

	return err;
}

status_t
TVolWalker::GetNextEntry(BEntry* entry, bool traverse)
{
	if (!fTopDir)
		return B_ENTRY_NOT_FOUND;

	// Get the next entry.
	status_t err = _inherited::GetNextEntry(entry, traverse);

	while (err != B_OK) {
		// We're done with the current volume. Go to the next one
		err = NextVolume();
		if (err != B_OK)
			break;
		err = GetNextEntry(entry, traverse);
	}

	return err;
}

status_t
TVolWalker::GetNextRef(entry_ref* ref)
{
	if (!fTopDir)
		return B_ENTRY_NOT_FOUND;

	// Get the next ref.
	status_t err = _inherited::GetNextRef(ref);

	while (err != B_OK) {
		// We're done with the current volume. Go to the next one
		err = NextVolume();
		if (err != B_OK)
			break;
		err = GetNextRef(ref);
	}

	return err;
}

int32
TVolWalker::GetNextDirents(struct dirent* ent, size_t size, int32 count)
{
	if (!fTopDir)
		return B_ENTRY_NOT_FOUND;

	// Get the next dirent.
	status_t err = _inherited::GetNextDirents(ent, size, count);

	while (err != B_OK) {
		// We're done with the current volume. Go to the next one
		err = NextVolume();
		if (err != B_OK)
			break;
		err = GetNextDirents(ent, size, count);
	}

	return err;
}

status_t
TVolWalker::Rewind()
{
	fVolRoster.Rewind();
	return NextVolume();
}

TQueryWalker::TQueryWalker(const char* predicate)
	: TWalker(), fQuery(), fVolRoster(), fVol()
{
	fPredicate = strdup(predicate);
	NextVolume();
}

TQueryWalker::~TQueryWalker()
{
	free((char*) fPredicate);
	fPredicate = NULL;
}

status_t
TQueryWalker::GetNextEntry(BEntry* entry, bool traverse)
{
	status_t err;

	do {
		err = fQuery.GetNextEntry(entry, traverse);
		if (err == B_ENTRY_NOT_FOUND) {
			if (NextVolume() != B_OK)
				break;
		}
	} while (err == B_ENTRY_NOT_FOUND);

	return err;
}

status_t
TQueryWalker::GetNextRef(entry_ref* ref)
{
	status_t err;

	for (;;) {
		err = fQuery.GetNextRef(ref);
		if (err != B_ENTRY_NOT_FOUND)
			break;

		err = NextVolume();
		if (err != B_OK)
			break;
	}

	return err;
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
	status_t err;
	do {
		err = fVolRoster.GetNextVolume(&fVol);
		if (err)
			break;
	} while (!fVol.KnowsQuery());


	if (err == B_OK) {
		err = fQuery.Clear();
		err = fQuery.SetVolume(&fVol);
		err = fQuery.SetPredicate(fPredicate);
		err = fQuery.Fetch();
	}

	return err;
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
