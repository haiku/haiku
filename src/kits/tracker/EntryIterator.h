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
#ifndef __ENTRY_ITERATOR__
#define __ENTRY_ITERATOR__


//  A lot of the code in here wouldn't be needed if the destructor
//  for BEntryList was virtual

// TODO: get rid of all BEntryList API's in here, replace them with
//       EntryListBase ones


#include <Directory.h>
#include "ObjectList.h"
#include "NodeWalker.h"


namespace BPrivate {

class EntryListBase : public BEntryList {
	// this is what BEntryList should have been
public:
	EntryListBase();
	virtual ~EntryListBase() {}

	virtual status_t InitCheck() const;

	virtual status_t GetNextEntry(BEntry* entry, bool traverse = false) = 0;
	virtual status_t GetNextRef(entry_ref* ref) = 0;
	virtual int32 GetNextDirents(struct dirent* buffer, size_t length,
		int32 count = INT_MAX) = 0;

	virtual status_t Rewind() = 0;
	virtual int32 CountEntries() = 0;

	static dirent* Next(dirent*);

protected:
	status_t fStatus;
};


class TWalkerWrapper : public EntryListBase {
	// this is to be able to use TWalker polymorfically as BEntryListBase
public:
	TWalkerWrapper(BTrackerPrivate::TWalker* walker);
	virtual ~TWalkerWrapper();

	virtual status_t InitCheck() const;
	virtual status_t GetNextEntry(BEntry* entry, bool traverse = false);
	virtual status_t GetNextRef(entry_ref* ref);
	virtual int32 GetNextDirents(struct dirent* buffer, size_t length,
		int32 count = INT_MAX);
	virtual status_t Rewind();
	virtual int32 CountEntries();

protected:
	BTrackerPrivate::TWalker* fWalker;
	status_t fStatus;
};


const int32 kDirentBufferSize = 10 * 1024;


class CachedEntryIterator : public EntryListBase {
public:
	// takes any iterator and runs it through a cache of a specified size
	// used to cluster entry_ref reads together, away from node accesses
	//
	// each chunk of iterators in the cache are then returned in an order,
	// sorted by their i-node number -- this turns out to give quite a bit
	// better performance over just using the order in which they show up
	// using the default BEntryList iterator subclass

	CachedEntryIterator(BEntryList* iterator, int32 numEntries,
		bool sortInodes = false);
		// CachedEntryIterator does not get to own the <iterator>
	virtual ~CachedEntryIterator();

	virtual status_t GetNextEntry(BEntry* entry, bool traverse = false);
	virtual status_t GetNextRef(entry_ref* ref);
	virtual int32 GetNextDirents(struct dirent* buffer, size_t length,
		int32 count = INT_MAX);

	virtual status_t Rewind();
	virtual int32 CountEntries();

	virtual void SetTo(BEntryList* iterator);
		// CachedEntryIterator does not get to own the <iterator>

private:
	static int _CompareInodes(const dirent* ent1, const dirent* ent2);

	BEntryList* fIterator;
	entry_ref* fEntryRefBuffer;
	int32 fCacheSize;
	int32 fNumEntries;
	int32 fIndex;

	dirent* fDirentBuffer;
	dirent* fCurrentDirent;
	bool fSortInodes;
	BObjectList<dirent>* fSortedList;

	BEntry* fEntryBuffer;
};


class DirectoryEntryList : public EntryListBase {
public:
	DirectoryEntryList(const BDirectory &);

	virtual status_t GetNextEntry(BEntry* entry, bool traverse = false);
	virtual status_t GetNextRef(entry_ref* ref);
	virtual int32 GetNextDirents(struct dirent* buffer, size_t length,
		int32 count = INT_MAX);

	virtual status_t Rewind();
	virtual int32 CountEntries();

private:
	BDirectory fDir;
};


class CachedDirectoryEntryList : public CachedEntryIterator {
	// this class is to work around not being able to delete
	// BEntryList polymorfically - need to have a special
	// caching entry list iterator for directories
public:
	CachedDirectoryEntryList(const BDirectory &);
	virtual ~CachedDirectoryEntryList();

private:
	BDirectory fDir;
};


class EntryIteratorList : public EntryListBase {
	// This wraps up several BEntryList style iterators and
	// iterates them all, going from one to the other as it finishes
	// up each of them
public:
	EntryIteratorList();
	virtual ~EntryIteratorList();

	void AddItem(BEntryList*);
		// list gets to own walkers

	virtual status_t GetNextEntry(BEntry* entry, bool traverse = false);
	virtual status_t GetNextRef(entry_ref* ref);
	virtual int32 GetNextDirents(struct dirent* buffer, size_t length,
		int32 count = INT_MAX);

	virtual status_t Rewind();
	virtual int32 CountEntries();

protected:
	BObjectList<BEntryList> fList;
	int32 fCurrentIndex;
};


class CachedEntryIteratorList : public CachedEntryIterator {
public:
	CachedEntryIteratorList(bool sortInodes = true);
	void AddItem(BEntryList* list);

protected:
	EntryIteratorList fIteratorList;
};

} // namespace BPrivate

using namespace BPrivate;

#endif
