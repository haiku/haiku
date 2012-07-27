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
#ifndef WALKER_H
#define WALKER_H


#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#include <VolumeRoster.h>
#include <Volume.h>
#include <List.h>
#include <EntryList.h>
#include <Directory.h>
#include <Entry.h>
#include <Query.h>

#include "ObjectList.h"


namespace BTrackerPrivate {

class TWalker : public BEntryList {
	// adds a virtual destructor that is severely missing in BEntryList
	// BEntryList should never be used polymorphically because of that

public:
	virtual ~TWalker();
	
	virtual	status_t GetNextEntry(BEntry*, bool traverse = false) = 0;
	virtual	status_t GetNextRef(entry_ref*) = 0;
	virtual	int32 GetNextDirents(struct dirent*, size_t,
		int32 count = INT_MAX) = 0;
	virtual	status_t Rewind() = 0;
	virtual	int32 CountEntries() = 0;
};


class TNodeWalker : public TWalker {
// TNodeWalker supports iterating a single volume, starting from a specified
// entry; if passed a non-directory entry it returns just that one entry
public:
	TNodeWalker(bool includeTopDirectory);
	TNodeWalker(const char* path, bool includeTopDirectory);
	TNodeWalker(const entry_ref* ref, bool includeTopDirectory);
	TNodeWalker(const BDirectory* dir, bool includeTopDirectory);
	virtual ~TNodeWalker();

	// Backwards compatibility with Tracker compiled for R5 (remove when this
	// gets integrated into the official release).
	TNodeWalker();
	TNodeWalker(const char* path);
	TNodeWalker(const entry_ref* ref);
	TNodeWalker(const BDirectory* dir);
						
	virtual	status_t GetNextEntry(BEntry*, bool traverse = false);
	virtual	status_t GetNextRef(entry_ref*);
	virtual	int32 GetNextDirents(struct dirent*, size_t,
		int32 count = INT_MAX);
	virtual	status_t Rewind();

protected:
	status_t PopDirCommon();
	void PushDirCommon(const entry_ref*);

private:
	virtual	int32 CountEntries();
		// don't know how to do that, have just a fake stub here

protected:
	BObjectList<BDirectory> fDirs;
	int32 fTopIndex;
	BDirectory* fTopDir;
	bool fIncludeTopDir;
	bool fOriginalIncludeTopDir;

private:
	BEntry* fJustFile;
	BDirectory fOriginalDirCopy;
	BEntry* fOriginalJustFile;
		// keep around to support Rewind
};


class TVolWalker : public TNodeWalker {
// TNodeWalker supports iterating over all the mounted volumes;
// non-attribute and read-only volumes may optionaly be filtered out
public:
	TVolWalker(bool knows_attr = true, bool writable = true,
		bool includeTopDirectory = true);
	virtual ~TVolWalker();

	virtual status_t GetNextEntry(BEntry*, bool traverse = false);
	virtual status_t GetNextRef(entry_ref*);
	virtual int32 GetNextDirents(struct dirent*, size_t,
		int32 count = INT_MAX);
	virtual status_t Rewind();
	
	virtual status_t NextVolume();
		// skips to the next volume
		// Note: it would be cool to return const BVolume*
		// that way a subclass could implement a volume filter -
		// it would just override, call inherited for as long as there
		// are volumes and it does not like them
		// we would have to give up the status_t then, which might be
		// ok

private:
	BVolumeRoster fVolRoster;
	BVolume fVol;
	bool fKnowsAttr;
	bool fWritable;

	typedef	TNodeWalker _inherited;
};


class TQueryWalker : public TWalker {
public:
	TQueryWalker(const char* predicate);
	virtual ~TQueryWalker();
	
	// Does an in-fix walk of all entries
	virtual status_t GetNextEntry(BEntry*, bool traverse = false);
	virtual status_t GetNextRef(entry_ref*);
	virtual int32 GetNextDirents(struct dirent*, size_t,
		int32 count = INT_MAX);
	
	virtual	status_t NextVolume();
	// skips to the next volume
	virtual	status_t Rewind();

private:
	virtual int32 CountEntries();
	// can't count
	
	BQuery fQuery;
	BVolumeRoster fVolRoster;
	BVolume fVol;
	bigtime_t fTime;
	const char* fPredicate;

	typedef TQueryWalker _inherited;
};

}	// namespace BTrackerPrivate

using namespace BTrackerPrivate;

#endif	// WALKER_H
