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
#ifndef _QUERY_POSE_VIEW_H
#define _QUERY_POSE_VIEW_H


#include "EntryIterator.h"
#include "PoseView.h"


class BQuery;

namespace BPrivate {

class BQueryContainerWindow;
class QueryEntryListCollection;

class BQueryPoseView : public BPoseView {
public:
	BQueryPoseView(Model*, BRect, uint32 resizeMask = B_FOLLOW_ALL);
	virtual ~BQueryPoseView();

	virtual void MessageReceived(BMessage* message);

	const char* SearchForType() const;
	BQueryContainerWindow* ContainerWindow() const;
	bool ActiveOnDevice(dev_t) const;

	void Refresh();
		// makes queries that are static but need to appear dynamic
		// live - for instance for a query that contains a
		// date == today - RestartQuery gets called on midnight to update
		// the contents

protected:
	virtual void AttachedToWindow();
	virtual void RestoreState(AttributeStreamNode*);
	virtual void RestoreState(const BMessage&);
	virtual void SavePoseLocations(BRect* = NULL);
	virtual void SetUpDefaultColumnsIfNeeded();
	virtual void SetViewMode(uint32);
	virtual void OpenParent();
	virtual void EditQueries();
	virtual EntryListBase* InitDirentIterator(const entry_ref*);
	virtual uint32 WatchNewNodeMask();
	virtual bool ShouldShowPose(const Model*, const PoseInfo*);
	virtual void AddPosesCompleted();

private:
		// list of all the queries this PoseView represents
		// typically there will be one query per volume specified
		// QueryEntryListCollection provides the abstraction layer
		// defining the iterators for _add_poses_
	bool fShowResultsFromTrash;
	mutable BString fSearchForMimeType;

	BObjectList<BQuery>* fQueryList;
	QueryEntryListCollection* fQueryListContainer;

	bool fCreateOldPoseList;

	typedef BPoseView _inherited;
};


class QueryEntryListCollection : public EntryListBase {
	// This will become a replacement for BDirectory and QueryList in a
	// PoseView, allowing PoseView to have an arbitrary collection of
	// elements that behave as an EntryList
	// For now just manage a list of BQueries

	class QueryListRep {
	public:
		QueryListRep(BObjectList<BQuery>* queryList)
			:	fQueryList(queryList),
				fRefCount(0),
				fShowResultsFromTrash(0),
				fOldPoseList(NULL)
			{}

		~QueryListRep()
			{
				ASSERT(fRefCount <= 0);
				delete fQueryList;
				delete fOldPoseList;
			}

		BObjectList<BQuery>* OpenQueryList()
			{
				fRefCount++;
				return fQueryList;
			}

		bool CloseQueryList()
		{
			return atomic_add(&fRefCount, -1) == 0;
		}

		BObjectList<BQuery>* fQueryList;
		int32 fRefCount;
		bool fShowResultsFromTrash;
		int32 fQueryListIndex;
		bool fDynamicDateQuery;
		bool fRefreshEveryHour;
		bool fRefreshEveryMinute;

		PoseList* fOldPoseList;
			// when doing a Refresh, this list is used to detect poses that
			// are no longer a part of a fDynamicDateQuery and need to be
			// removed
	};

public:
	QueryEntryListCollection(Model*, BHandler* = NULL,
		PoseList* oldPoseList = NULL);
	virtual ~QueryEntryListCollection();

	QueryEntryListCollection* Clone();

	BObjectList<BQuery>* QueryList() const
		{ return fQueryListRep->fQueryList; }

	PoseList* OldPoseList() const
		{ return fQueryListRep->fOldPoseList; }
	void ClearOldPoseList();

	virtual status_t GetNextEntry(BEntry* entry, bool traverse = false);
	virtual status_t GetNextRef(entry_ref* ref);
	virtual int32 GetNextDirents(struct dirent* buffer, size_t length,
		int32 count = INT_MAX);

	virtual status_t Rewind();
	virtual int32 CountEntries();

	bool ShowResultsFromTrash() const;
	bool DynamicDateQuery() const;
	bool DynamicDateRefreshEveryHour() const;
	bool DynamicDateRefreshEveryMinute() const;

private:
	QueryEntryListCollection(const QueryEntryListCollection&);
		// only to be used by the Clone routine
	status_t FetchOneQuery(const BQuery*, BHandler* target,
		BObjectList<BQuery>*, BVolume*);

	QueryListRep* fQueryListRep;
};

} // namespace BPrivate

using namespace BPrivate;

#endif	// _QUERY_POSE_VIEW_H
