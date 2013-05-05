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
#include <Entry.h>
#include <Path.h>

#include <new>
#include <string.h>

#include "EntryIterator.h"
#include "NodeWalker.h"
#include "ObjectList.h"


TWalkerWrapper::TWalkerWrapper(BTrackerPrivate::TWalker* walker)
	:
	fWalker(walker),
	fStatus(B_OK)
{
}


TWalkerWrapper::~TWalkerWrapper()
{
	delete fWalker;
}


status_t
TWalkerWrapper::InitCheck() const
{
	return fStatus;
}


status_t
TWalkerWrapper::GetNextEntry(BEntry* entry, bool traverse)
{
	fStatus = fWalker->GetNextEntry(entry, traverse);
	return fStatus;
}


status_t
TWalkerWrapper::GetNextRef(entry_ref* ref)
{
	fStatus = fWalker->GetNextRef(ref);
	return fStatus;
}


int32
TWalkerWrapper::GetNextDirents(struct dirent* buffer, size_t length,
	int32 count)
{
	int32 result = fWalker->GetNextDirents(buffer, length, count);
	fStatus = result < B_OK ? result : (result ? B_OK : B_ENTRY_NOT_FOUND);
	return result;
}


status_t
TWalkerWrapper::Rewind()
{
	return fWalker->Rewind();
}


int32
TWalkerWrapper::CountEntries()
{
	return fWalker->CountEntries();
}


//	#pragma mark -


EntryListBase::EntryListBase()
	:	fStatus(B_OK)
{
}


status_t
EntryListBase::InitCheck() const
{
	 return fStatus;
}


dirent*
EntryListBase::Next(dirent* ent)
{
	return (dirent*)((char*)ent + ent->d_reclen);
}


//	#pragma mark -


CachedEntryIterator::CachedEntryIterator(BEntryList* iterator,
		int32 numEntries, bool sortInodes)
	:
	fIterator(iterator),
	fEntryRefBuffer(NULL),
	fCacheSize(numEntries),
	fNumEntries(0),
	fIndex(0),
	fDirentBuffer(NULL),
	fCurrentDirent(NULL),
	fSortInodes(sortInodes),
	fSortedList(NULL),
	fEntryBuffer(NULL)
{
}


CachedEntryIterator::~CachedEntryIterator()
{
	delete [] fEntryRefBuffer;
	free(fDirentBuffer);
	delete fSortedList;
	delete [] fEntryBuffer;
}


status_t
CachedEntryIterator::GetNextEntry(BEntry* result, bool traverse)
{
	ASSERT(!fDirentBuffer);
	ASSERT(!fEntryRefBuffer);

	if (!fEntryBuffer) {
		fEntryBuffer = new BEntry [fCacheSize];
		ASSERT(fIndex == 0 && fNumEntries == 0);
	}
	if (fIndex >= fNumEntries) {
		// fill up the buffer or stop if error; keep error around
		// and return it when appropriate
		fStatus = B_OK;
		for (fNumEntries = 0; fNumEntries < fCacheSize; fNumEntries++) {
			fStatus = fIterator->GetNextEntry(&fEntryBuffer[fNumEntries],
				traverse);
			if (fStatus != B_OK)
				break;
		}
		fIndex = 0;
	}
	*result = fEntryBuffer[fIndex++];
	if (fIndex > fNumEntries) {
		// we are at the end of the cache we loaded up, time to return
		// an error, if we had one
		return fStatus;
	}

	return B_OK;
}


status_t
CachedEntryIterator::GetNextRef(entry_ref* ref)
{
	ASSERT(!fDirentBuffer);
	ASSERT(!fEntryBuffer);

	if (!fEntryRefBuffer) {
		fEntryRefBuffer = new entry_ref[fCacheSize];
		ASSERT(fIndex == 0 && fNumEntries == 0);
	}

	if (fIndex >= fNumEntries) {
		// fill up the buffer or stop if error; keep error around
		// and return it when appropriate
		fStatus = B_OK;
		for (fNumEntries = 0; fNumEntries < fCacheSize; fNumEntries++) {
			fStatus = fIterator->GetNextRef(&fEntryRefBuffer[fNumEntries]);
			if (fStatus != B_OK)
				break;
		}
		fIndex = 0;
	}
	*ref = fEntryRefBuffer[fIndex++];
	if (fIndex > fNumEntries)
		// we are at the end of the cache we loaded up, time to return
		// an error, if we had one
		return fStatus;

	return B_OK;
}


/*static*/ int
CachedEntryIterator::_CompareInodes(const dirent* ent1, const dirent* ent2)
{
	if (ent1->d_ino < ent2->d_ino)
		return -1;
	if (ent1->d_ino == ent2->d_ino)
		return 0;

	return 1;
}


int32
CachedEntryIterator::GetNextDirents(struct dirent* ent, size_t size,
	int32 count)
{
	ASSERT(!fEntryRefBuffer);
	if (!fDirentBuffer) {
		fDirentBuffer = (dirent*)malloc(kDirentBufferSize);
		ASSERT(fIndex == 0 && fNumEntries == 0);
		ASSERT(size > sizeof(dirent) + B_FILE_NAME_LENGTH);
	}

	if (!count)
		return 0;

	if (fIndex >= fNumEntries) {
		// we are out of stock, cache em up
		fCurrentDirent = fDirentBuffer;
		int32 bufferRemain = kDirentBufferSize;
		for (fNumEntries = 0; fNumEntries < fCacheSize; ) {
			int32 count = fIterator->GetNextDirents(fCurrentDirent,
				bufferRemain, 1);

			if (count <= 0)
				break;

			fNumEntries += count;

			int32 currentDirentSize = fCurrentDirent->d_reclen;
			bufferRemain -= currentDirentSize;
			ASSERT(bufferRemain >= 0);

			if ((size_t)bufferRemain
					< (sizeof(dirent) + B_FILE_NAME_LENGTH)) {
				// cant fit a big entryRef in the buffer, just bail
				// and start from scratch
				break;
			}

			fCurrentDirent
				= (dirent*)((char*)fCurrentDirent + currentDirentSize);
		}
		fCurrentDirent = fDirentBuffer;
		if (fSortInodes) {
			if (!fSortedList)
				fSortedList = new BObjectList<dirent>(fCacheSize);
			else
				fSortedList->MakeEmpty();

			for (int32 count = 0; count < fNumEntries; count++) {
				fSortedList->AddItem(fCurrentDirent, 0);
				fCurrentDirent = Next(fCurrentDirent);
			}
			fSortedList->SortItems(&_CompareInodes);
			fCurrentDirent = fDirentBuffer;
		}
		fIndex = 0;
	}
	if (fIndex >= fNumEntries) {
		// we are done, no more dirents left
		return 0;
	}

	if (fSortInodes)
		fCurrentDirent = fSortedList->ItemAt(fIndex);

	fIndex++;
	uint32 currentDirentSize = fCurrentDirent->d_reclen;
	ASSERT(currentDirentSize <= size);
	if (currentDirentSize > size)
		return 0;

	memcpy(ent, fCurrentDirent, currentDirentSize);

	if (!fSortInodes)
		fCurrentDirent = (dirent*)((char*)fCurrentDirent + currentDirentSize);

	return 1;
}


status_t
CachedEntryIterator::Rewind()
{
	fIndex = 0;
	fNumEntries = 0;
	fCurrentDirent = NULL;
	fStatus = B_OK;

	delete fSortedList;
	fSortedList = NULL;

	return fIterator->Rewind();
}


int32
CachedEntryIterator::CountEntries()
{
	return fIterator->CountEntries();
}


void
CachedEntryIterator::SetTo(BEntryList* iterator)
{
	fIndex = 0;
	fNumEntries = 0;
	fStatus = B_OK;
	fIterator = iterator;
}


//	#pragma mark -


CachedDirectoryEntryList::CachedDirectoryEntryList(const BDirectory &dir)
	: CachedEntryIterator(0, 40, true),
	fDir(dir)
{
	fStatus = fDir.InitCheck();
	SetTo(&fDir);
}


CachedDirectoryEntryList::~CachedDirectoryEntryList()
{
}


//	#pragma mark -


DirectoryEntryList::DirectoryEntryList(const BDirectory &dir)
	:
	fDir(dir)
{
	fStatus = fDir.InitCheck();
}


status_t
DirectoryEntryList::GetNextEntry(BEntry* entry, bool traverse)
{
	fStatus = fDir.GetNextEntry(entry, traverse);
	return fStatus;
}


status_t
DirectoryEntryList::GetNextRef(entry_ref* ref)
{
	fStatus = fDir.GetNextRef(ref);
	return fStatus;
}


int32
DirectoryEntryList::GetNextDirents(struct dirent* buffer, size_t length,
	int32 count)
{
	fStatus = fDir.GetNextDirents(buffer, length, count);
	return fStatus;
}


status_t
DirectoryEntryList::Rewind()
{
	fStatus = fDir.Rewind();
	return fStatus;
}


int32
DirectoryEntryList::CountEntries()
{
	return fDir.CountEntries();
}


//	#pragma mark -


EntryIteratorList::EntryIteratorList()
	:
	fList(5, true),
	fCurrentIndex(0)
{
}


EntryIteratorList::~EntryIteratorList()
{
	int32 count = fList.CountItems();
	for (;count; count--) {
		// workaround for BEntryList not having a proper destructor
		BEntryList* entry = fList.RemoveItemAt(count - 1);
		EntryListBase* fixedEntry = dynamic_cast<EntryListBase*>(entry);

		if (fixedEntry)
			delete fixedEntry;
		else
			delete entry;
	}
}


void
EntryIteratorList::AddItem(BEntryList* walker)
{
	fList.AddItem(walker);
}


status_t
EntryIteratorList::GetNextEntry(BEntry* entry, bool traverse)
{
	while (true) {
		if (fCurrentIndex >= fList.CountItems()) {
			fStatus = B_ENTRY_NOT_FOUND;
			break;
		}

		fStatus = fList.ItemAt(fCurrentIndex)->GetNextEntry(entry, traverse);
		if (fStatus != B_ENTRY_NOT_FOUND)
			break;

		fCurrentIndex++;
	}
	return fStatus;
}


status_t
EntryIteratorList::GetNextRef(entry_ref* ref)
{
	while (true) {
		if (fCurrentIndex >= fList.CountItems()) {
			fStatus = B_ENTRY_NOT_FOUND;
			break;
		}

		fStatus = fList.ItemAt(fCurrentIndex)->GetNextRef(ref);
		if (fStatus != B_ENTRY_NOT_FOUND)
			break;

		fCurrentIndex++;
	}
	return fStatus;
}


int32
EntryIteratorList::GetNextDirents(struct dirent* buffer, size_t length,
	int32 count)
{
	int32 result = 0;
	while (true) {
		if (fCurrentIndex >= fList.CountItems()) {
			fStatus = B_ENTRY_NOT_FOUND;
			break;
		}

		result = fList.ItemAt(fCurrentIndex)->GetNextDirents(buffer, length,
			count);
		if (result > 0) {
			fStatus = B_OK;
			break;
		}

		fCurrentIndex++;
	}
	return result;
}


status_t
EntryIteratorList::Rewind()
{
	fCurrentIndex = 0;
	int32 count = fList.CountItems();
	for (int32 index = 0; index < count; index++)
		fStatus = fList.ItemAt(index)->Rewind();

	return fStatus;
}


int32
EntryIteratorList::CountEntries()
{
	int32 result = 0;

	int32 count = fList.CountItems();
	for (int32 index = 0; index < count; index++)
		result += fList.ItemAt(fCurrentIndex)->CountEntries();

	return result;
}


//	#pragma mark -


CachedEntryIteratorList::CachedEntryIteratorList(bool sortInodes)
	: CachedEntryIterator(NULL, 10, sortInodes)
{
	fStatus = B_OK;
	SetTo(&fIteratorList);
}


void
CachedEntryIteratorList::AddItem(BEntryList* walker)
{
	fIteratorList.AddItem(walker);
}
