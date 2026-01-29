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


#include "QueryPoseView.h"

#include <new>

#include <Catalog.h>
#include <Debug.h>
#include <Locale.h>
#include <NodeMonitor.h>
#include <Query.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Window.h>

#include "Attributes.h"
#include "AttributeStream.h"
#include "AutoLock.h"
#include "Commands.h"
#include "FindPanel.h"
#include "FSUtils.h"
#include "MimeTypeList.h"
#include "MimeTypes.h"
#include "Tracker.h"

#include <fs_attr.h>
#include <query_private.h>


using std::nothrow;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "QueryPoseView"


// Currently filtering out Trash doesn't node monitor too well - if you
// remove an item from the Trash, it doesn't show up in the query result
// To do this properly, we would have to node monitor everything BQuery
// returns and after a node monitor re-chech if it should be part of
// query results and add/remove appropriately. Right now only moving to
// Trash is supported


//	#pragma mark - BQueryPoseView


BQueryPoseView::BQueryPoseView(Model* model)
	:
	BPoseView(model, kListMode),
	fRefFilter(new QueryRefFilter),
	fQueryListContainer(NULL),
	fCreateOldPoseList(false)
{
	SetRefFilter(fRefFilter);
}


BQueryPoseView::~BQueryPoseView()
{
	delete fRefFilter;
	delete fQueryListContainer;
}


void
BQueryPoseView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kFSClipboardChanges:
		{
			// poses have always to be updated for the query view
			UpdatePosesClipboardModeFromClipboard(message);
			break;
		}

		default:
			_inherited::MessageReceived(message);
			break;
	}
}


void
BQueryPoseView::AdoptSystemColors()
{
	SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR, ReadOnlyTint(B_DOCUMENT_BACKGROUND_COLOR));
	SetLowUIColor(ViewUIColor());
	SetHighUIColor(B_DOCUMENT_TEXT_COLOR);
}


bool
BQueryPoseView::HasSystemColors() const
{
	float tint = B_NO_TINT;
	float readOnlyTint = ReadOnlyTint(B_DOCUMENT_BACKGROUND_COLOR);

	return ViewUIColor(&tint) == B_DOCUMENT_BACKGROUND_COLOR && tint == readOnlyTint
		&& LowUIColor(&tint) == B_DOCUMENT_BACKGROUND_COLOR && tint == readOnlyTint
		&& HighUIColor(&tint) == B_DOCUMENT_TEXT_COLOR && tint == B_NO_TINT;
}


void
BQueryPoseView::EditQueries()
{
	BMessage message(kEditQuery);
	message.AddRef("refs", TargetModel()->EntryRef());
	BMessenger(kTrackerSignature, -1, 0).SendMessage(&message);
}


void
BQueryPoseView::SetupDefaultColumnsIfNeeded()
{
	// in case there were errors getting some columns
	if (CountColumns() != 0)
		return;

	AddColumn(new BColumn(B_TRANSLATE("Name"), 145,
		B_ALIGN_LEFT, kAttrStatName, B_STRING_TYPE, true, true));
	AddColumn(new BColumn(B_TRANSLATE("Location"), 225,
		B_ALIGN_LEFT, kAttrPath, B_STRING_TYPE, true, false));
	AddColumn(new BColumn(B_TRANSLATE("Size"), 80,
		B_ALIGN_RIGHT, kAttrStatSize, B_OFF_T_TYPE, true, false));
	AddColumn(new BColumn(B_TRANSLATE("Modified"), 150,
		B_ALIGN_LEFT, kAttrStatModified, B_TIME_TYPE, true, false));
}


void
BQueryPoseView::RestoreState(AttributeStreamNode* node)
{
	_inherited::RestoreState(node);
	fViewState->SetViewMode(kListMode);
}


void
BQueryPoseView::RestoreState(const BMessage &message)
{
	_inherited::RestoreState(message);
	fViewState->SetViewMode(kListMode);
}


void
BQueryPoseView::SavePoseLocations(BRect*)
{
}


void
BQueryPoseView::SetViewMode(uint32)
{
}


void
BQueryPoseView::OpenParent()
{
}


void
BQueryPoseView::Refresh()
{
	PRINT(("refreshing dynamic date query\n"));

	// cause the old AddPosesTask to die
	fAddPosesThreads.clear();
	delete fQueryListContainer;
	fQueryListContainer = NULL;
	fRefFilter->SetQueryListContainer(NULL);

	fCreateOldPoseList = true;
	AddPoses(TargetModel());
	TargetModel()->CloseNode();

	ResetOrigin();
	ResetPosePlacementHint();
}


void
BQueryPoseView::AddPosesCompleted()
{
	ASSERT(Window()->IsLocked());

	PoseList* oldPoseList = fQueryListContainer->OldPoseList();
	if (oldPoseList != NULL) {
		int32 count = oldPoseList->CountItems();
		for (int32 index = count - 1; index >= 0; index--) {
			BPose* pose = oldPoseList->ItemAt(index);
			DeletePose(pose->TargetModel()->NodeRef());
		}
		fQueryListContainer->ClearOldPoseList();
	}

	_inherited::AddPosesCompleted();
}


// When using dynamic dates, such as "today", need to refresh the query
// window every now and then

EntryListBase*
BQueryPoseView::InitDirentIterator(const entry_ref* ref)
{
	BEntry entry(ref);
	if (entry.InitCheck() != B_OK)
		return NULL;

	Model sourceModel(&entry, true);
	if (sourceModel.InitCheck() != B_OK)
		return NULL;

	ASSERT(sourceModel.IsQuery());

	// old pose list is used for finding poses that no longer match a
	// dynamic date query during a Refresh call
	PoseList* oldPoseList = NULL;
	if (fCreateOldPoseList) {
		oldPoseList = new PoseList(10);
		oldPoseList->AddList(fPoseList);
	}

	fQueryListContainer = new QueryEntryListCollection(&sourceModel, this,
		oldPoseList);
	if (fQueryListContainer->InitCheck() != B_OK) {
		delete fQueryListContainer;
		fQueryListContainer = NULL;
		return NULL;
	}
	fCreateOldPoseList = false;
	fRefFilter->SetQueryListContainer(fQueryListContainer);

	TTracker::WatchNode(sourceModel.NodeRef(), B_WATCH_NAME | B_WATCH_STAT
		| B_WATCH_ATTR, this);

	if (fQueryListContainer->DynamicDateQuery()) {
		// calculate the time to trigger the query refresh - next midnight

		time_t now = time(0);

		time_t nextMidnight = now + 60 * 60 * 24;
			// move ahead by a day
		tm timeData;
		localtime_r(&nextMidnight, &timeData);
		timeData.tm_sec = 0;
		timeData.tm_min = 0;
		timeData.tm_hour = 0;
		nextMidnight = mktime(&timeData);

		time_t nextHour = now + 60 * 60;
			// move ahead by a hour
		localtime_r(&nextHour, &timeData);
		timeData.tm_sec = 0;
		timeData.tm_min = 0;
		nextHour = mktime(&timeData);

		PRINT(("%" B_PRIdTIME " minutes, %" B_PRIdTIME " seconds till next hour\n",
			(nextHour - now) / 60, (nextHour - now) % 60));

		time_t nextMinute = now + 60;
			// move ahead by a minute
		localtime_r(&nextMinute, &timeData);
		timeData.tm_sec = 0;
		nextMinute = mktime(&timeData);

		PRINT(("%" B_PRIdTIME " seconds till next minute\n", nextMinute - now));

		bigtime_t delta;
		if (fQueryListContainer->DynamicDateRefreshEveryMinute())
			delta = nextMinute - now;
		else if (fQueryListContainer->DynamicDateRefreshEveryHour())
			delta = nextHour - now;
		else
			delta = nextMidnight - now;

#if DEBUG
		int32 secondsTillMidnight = (nextMidnight - now);
		int32 minutesTillMidnight = secondsTillMidnight/60;
		secondsTillMidnight %= 60;
		int32 hoursTillMidnight = minutesTillMidnight/60;
		minutesTillMidnight %= 60;

		PRINT(("%" B_PRId32 " hours, %" B_PRId32 " minutes, %" B_PRId32
			" seconds till midnight\n", hoursTillMidnight, minutesTillMidnight,
			secondsTillMidnight));

		int32 refreshInSeconds = delta % 60;
		int32 refreshInMinutes = delta / 60;
		int32 refreshInHours = refreshInMinutes / 60;
		refreshInMinutes %= 60;

		PRINT(("next refresh in %" B_PRId32 " hours, %" B_PRId32 "minutes, %"
			B_PRId32 " seconds\n", refreshInHours, refreshInMinutes,
			refreshInSeconds));
#endif

		// bump up to microseconds
		delta *= 1000000;

		TTracker* tracker = dynamic_cast<TTracker*>(be_app);
		ThrowOnAssert(tracker != NULL);

		tracker->MainTaskLoop()->RunLater(
			NewLockingMemberFunctionObject(&BQueryPoseView::Refresh, this),
			delta);
	}

	return fQueryListContainer;
}


void
BQueryPoseView::ReturnDirentIterator(EntryListBase* iterator)
{
	// Do nothing. We keep our fIterator around.
}


uint32
BQueryPoseView::WatchNewNodeMask()
{
	// B_QUERY_WATCH_ALL suffices.
	return 0;
}


bool
BQueryPoseView::AttributeChanged(const BMessage* message)
{
	BMessage alteredMessage;
	const char* attrName;
	if (message->FindString("attr", &attrName) == B_OK) {
		if (strcmp(attrName, "last_modified") == 0 || strcmp(attrName, "size") == 0) {
			// BPoseView handles changes to these under B_STAT_UPDATE,
			// so make this message look like that one.
			alteredMessage = *message;
			alteredMessage.RemoveName("attr");
			message = &alteredMessage;
		}
	}

	return BPoseView::AttributeChanged(message);
}


const char*
BQueryPoseView::SearchForType() const
{
	if (!fSearchForMimeType.Length()) {
		BModelOpener opener(TargetModel());
		BString buffer;
		attr_info attrInfo;

		// read the type of files we are looking for
		status_t status
			= TargetModel()->Node()->GetAttrInfo(kAttrQueryInitialMime,
				&attrInfo);
		if (status == B_OK) {
			TargetModel()->Node()->ReadAttrString(kAttrQueryInitialMime,
				&buffer);
		}

		TTracker* tracker = dynamic_cast<TTracker*>(be_app);
		if (tracker != NULL && buffer.Length() > 0) {
			const ShortMimeInfo* info = tracker->MimeTypes()->FindMimeType(
				buffer.String());
			if (info != NULL)
				fSearchForMimeType = info->InternalName();
		}

		if (!fSearchForMimeType.Length())
			fSearchForMimeType = B_FILE_MIMETYPE;
	}

	return fSearchForMimeType.String();
}


bool
BQueryPoseView::ActiveOnDevice(dev_t device) const
{
	int32 count = fQueryListContainer->QueryList()->CountItems();
	for (int32 index = 0; index < count; index++) {
		if (fQueryListContainer->QueryList()->ItemAt(index)->TargetDevice() == device)
			return true;
	}

	return false;
}


//	#pragma mark - QueryRefFilter


QueryRefFilter::QueryRefFilter()
{
}


QueryRefFilter::~QueryRefFilter()
{
}


bool
QueryRefFilter::Filter(const entry_ref* ref, BNode* node, stat_beos* st,
	const char* filetype)
{
	if (fQueryListContainer == NULL)
		return true;

	TTracker* tracker = dynamic_cast<TTracker*>(be_app);
	if (!fQueryListContainer->ShowResultsFromTrash()
			&& tracker != NULL && tracker->InTrashNode(ref))
		return false;
	if (!fQueryListContainer->PathFilter(ref))
		return false;
	return true;
}


//	#pragma mark - QueryEntryListCollection


QueryEntryListCollection::QueryEntryListCollection(Model* model,
	BHandler* target, PoseList* oldPoseList)
	:
	fQueryListRep(new QueryListRep),
	fOldPoseList(oldPoseList)
{
	Rewind();
	attr_info info;
	BQuery query;

	BNode* modelNode = model->Node();
	if (modelNode == NULL) {
		fStatus = B_ERROR;
		return;
	}

	// read the actual query string
	fStatus = modelNode->GetAttrInfo(kAttrQueryString, &info);
	if (fStatus != B_OK)
		return;

	BString buffer;
	if (modelNode->ReadAttr(kAttrQueryString, B_STRING_TYPE, 0,
		buffer.LockBuffer((int32)info.size),
			(size_t)info.size) != info.size) {
		fStatus = B_ERROR;
		return;
	}

	buffer.UnlockBuffer();

	// read the extra options
	MoreOptionsStruct saveMoreOptions;
	if (ReadAttr(modelNode, kAttrQueryMoreOptions,
			kAttrQueryMoreOptionsForeign, B_RAW_TYPE, 0, &saveMoreOptions,
			sizeof(MoreOptionsStruct),
			&MoreOptionsStruct::EndianSwap) != kReadAttrFailed) {
		fQueryListRep->fShowResultsFromTrash = saveMoreOptions.searchTrash;
	}

	fStatus = query.SetPredicate(buffer.String());

	fQueryListRep->fDynamicDateQuery = false;

	fQueryListRep->fRefreshEveryHour = false;
	fQueryListRep->fRefreshEveryMinute = false;

	if (modelNode->ReadAttr(kAttrDynamicDateQuery, B_BOOL_TYPE, 0,
			&fQueryListRep->fDynamicDateQuery,
			sizeof(bool)) != sizeof(bool)) {
		fQueryListRep->fDynamicDateQuery = false;
	}

	if (fQueryListRep->fDynamicDateQuery) {
		// only refresh every minute on debug builds
		fQueryListRep->fRefreshEveryMinute = buffer.IFindFirst("second") != -1
			|| buffer.IFindFirst("minute") != -1;
		fQueryListRep->fRefreshEveryHour = fQueryListRep->fRefreshEveryMinute
			|| buffer.IFindFirst("hour") != -1;

#if !DEBUG
		// don't refresh every minute unless we are running debug build
		fQueryListRep->fRefreshEveryMinute = false;
#endif
	}

	if (fStatus != B_OK)
		return;

	bool searchAllVolumes = true;
	status_t result = B_OK;

	// get volumes to perform query on
	if (modelNode->GetAttrInfo(kAttrQueryVolume, &info) == B_OK) {
		char* buffer = NULL;

		if ((buffer = (char*)malloc((size_t)info.size)) != NULL
			&& modelNode->ReadAttr(kAttrQueryVolume, B_MESSAGE_TYPE, 0,
				buffer, (size_t)info.size) == info.size) {
			BMessage message;
			if (message.Unflatten(buffer) == B_OK) {
				for (int32 index = 0; ;index++) {
					ASSERT(index < 100);
					BVolume volume;
						// match a volume with the info embedded in
						// the message
					result = MatchArchivedVolume(&volume, &message, index);
					if (result == B_OK) {
						// start the query on this volume
						result = FetchOneQuery(&query, target,
							&fQueryListRep->fQueryList, &volume);
						if (result != B_OK)
							continue;

						searchAllVolumes = false;
					} else if (result != B_DEV_BAD_DRIVE_NUM) {
						// if B_DEV_BAD_DRIVE_NUM, the volume just isn't
						// mounted this time around, keep looking for more
						// if other error, bail
						break;
					}
				}
			}
		}

		free(buffer);
	}

	if (searchAllVolumes) {
		// no specific volumes embedded in query, search everything
		BVolumeRoster roster;
		BVolume volume;

		roster.Rewind();
		while (roster.GetNextVolume(&volume) == B_OK)
			if (volume.IsPersistent() && volume.KnowsQuery()) {
				result = FetchOneQuery(&query, target,
					&fQueryListRep->fQueryList, &volume);
				if (result != B_OK)
					continue;
			}
	}

	if (modelNode->GetAttrInfo("_trk/directories", &info) == B_OK) {
		BString bufferString;
		char* buffer = bufferString.LockBuffer(info.size);
		if (modelNode->ReadAttr("_trk/directories", B_MESSAGE_TYPE, 0,
				buffer, info.size) != info.size) {
			fStatus = B_ERROR;
			return;
		}

		BMessage message;
		result = message.Unflatten(buffer);
		if (result != B_OK) {
			fStatus = result;
			return;
		}

		int32 count;
		if ((result = message.GetInfo("refs", NULL, &count)) != B_OK)
			count = 0;

		for (int32 i = 0; i < count; i++) {
			entry_ref ref;
			if ((result = message.FindRef("refs", i, &ref)) != B_OK)
				continue;

			BEntry entry(&ref, true);
			if (entry.InitCheck() != B_OK || !entry.Exists() || !entry.IsDirectory())
				continue;

			BPath path;
			if ((result = entry.GetPath(&path)) != B_OK)
				continue;

			BString pathString = path.Path();
			pathString.Append("/");

			// check for duplicates
			if (fQueryListRep->fPathFilters.IndexOf(pathString) >= 0)
				continue;

			fQueryListRep->fPathFilters.Add(pathString);
		}
	}

	fStatus = B_OK;
}


status_t
QueryEntryListCollection::FetchOneQuery(const BQuery* copyThis,
	BHandler* target, BObjectList<BQuery, true>* list, BVolume* volume)
{
	BQuery* query = new (nothrow) BQuery;
	if (query == NULL)
		return B_NO_MEMORY;

	// have to fake a copy constructor here because BQuery doesn't have
	// a copy constructor
	BString buffer;
	const_cast<BQuery*>(copyThis)->GetPredicate(&buffer);
	query->SetPredicate(buffer.String());

	query->SetVolume(volume);
	query->SetTarget(BMessenger(target));
	query->SetFlags(B_QUERY_WATCH_ALL);

	status_t result = query->Fetch();
	if (result != B_OK) {
		PRINT(("fetch error %s\n", strerror(result)));
		delete query;
		return result;
	}

	list->AddItem(query);
	return B_OK;
}


QueryEntryListCollection::~QueryEntryListCollection()
{
	delete fQueryListRep;
	delete fOldPoseList;
}


//	#pragma mark - QueryEntryListCollection


void
QueryEntryListCollection::ClearOldPoseList()
{
	delete fOldPoseList;
	fOldPoseList = NULL;
}


status_t
QueryEntryListCollection::GetNextEntry(BEntry* entry, bool traverse)
{
	status_t result = B_ERROR;

	for (int32 count = fQueryListRep->fQueryList.CountItems();
			fQueryListRep->fQueryListIndex < count;
			fQueryListRep->fQueryListIndex++) {
		for (;;) {
			result = fQueryListRep->fQueryList.
				ItemAt(fQueryListRep->fQueryListIndex)->
					GetNextEntry(entry, traverse);
			if (result != B_OK)
				break;
			if (!fQueryListRep->fPathFilters.IsEmpty()) {
				entry_ref ref;
				if ((result = entry->GetRef(&ref)) != B_OK)
					continue;
				if (!PathFilter(&ref)) {
					result = B_ERROR;
					continue;
				}
			}
			return result;
		}
	}

	return result;
}


int32
QueryEntryListCollection::GetNextDirents(struct dirent* buffer, size_t length,
	int32 count)
{
	// If path-filtering, we only read one dirent at a time.
	if (!fQueryListRep->fPathFilters.IsEmpty())
		count = 1;

	int32 result = 0;

	for (int32 queryCount = fQueryListRep->fQueryList.CountItems();
			fQueryListRep->fQueryListIndex < queryCount;
			fQueryListRep->fQueryListIndex++) {
		for (;;) {
			result = fQueryListRep->fQueryList.
				ItemAt(fQueryListRep->fQueryListIndex)->
					GetNextDirents(buffer, length, count);
			if (result <= 0)
				break;
			if (!fQueryListRep->fPathFilters.IsEmpty()) {
				entry_ref ref(buffer->d_pdev, buffer->d_pino, buffer->d_name);
				if (!PathFilter(&ref)) {
					result = 0;
					continue;
				}
			}
			return result;
		}
	}

	return result;
}


status_t
QueryEntryListCollection::GetNextRef(entry_ref* ref)
{
	status_t result = B_ERROR;

	for (int32 count = fQueryListRep->fQueryList.CountItems();
			fQueryListRep->fQueryListIndex < count;
			fQueryListRep->fQueryListIndex++) {
		for (;;) {
			result = fQueryListRep->fQueryList.
				ItemAt(fQueryListRep->fQueryListIndex)->GetNextRef(ref);
			if (result != B_OK)
				break;
			if (!fQueryListRep->fPathFilters.IsEmpty()) {
				if (!PathFilter(ref)) {
					result = B_ERROR;
					continue;
				}
			}
			return result;
		}
	}

	return result;
}


bool
QueryEntryListCollection::PathFilter(const entry_ref* ref) const
{
	// We perform path filtering here so that PoseView doesn't create Poses
	// to track the state of nodes removed by the path filters. (We still
	// must perform filtering in the RefFilter too, however, so that live
	// queries are filtered properly.)

	if (fQueryListRep->fPathFilters.IsEmpty())
		return true;

	BPath path(ref);
	if (path.InitCheck() != B_OK)
		return false;
	const char* pathStr = path.Path();

	for (int32 i = 0; i < fQueryListRep->fPathFilters.CountStrings(); i++) {
		BString filterPath = fQueryListRep->fPathFilters.StringAt(i);
		if (strncmp(filterPath.String(), pathStr, filterPath.Length()) == 0)
			return true;
	}

	return false;
}


status_t
QueryEntryListCollection::Rewind()
{
	fQueryListRep->fQueryListIndex = 0;
	for (int32 i = 0; i < fQueryListRep->fQueryList.CountItems(); i++)
		fQueryListRep->fQueryList.ItemAt(i)->Rewind();

	return B_OK;
}


int32
QueryEntryListCollection::CountEntries()
{
	return 0;
}


bool
QueryEntryListCollection::ShowResultsFromTrash() const
{
	return fQueryListRep->fShowResultsFromTrash;
}


bool
QueryEntryListCollection::DynamicDateQuery() const
{
	return fQueryListRep->fDynamicDateQuery;
}


bool
QueryEntryListCollection::DynamicDateRefreshEveryHour() const
{
	return fQueryListRep->fRefreshEveryHour;
}


bool
QueryEntryListCollection::DynamicDateRefreshEveryMinute() const
{
	return fQueryListRep->fRefreshEveryMinute;
}
