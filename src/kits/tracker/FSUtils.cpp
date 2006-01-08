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

// Tracker file system calls.

// Note - APIs/code in FSUtils.h and FSUtils.cpp is slated for a major cleanup
// 	-- in other words, you will find a lot of ugly cruft in here

// ToDo:
// Move most of preflight error checks to the Model level and only keep those
// 	that have to do with size, reading/writing and name collisions.
// Get rid of all the BList based APIs, use BObjectLists.
// Clean up the error handling, push most of the user interaction out of the
// 	low level FS calls.

#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <Alert.h>
#include <Application.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>
#include <SymLink.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <fs_attr.h>
#include <fs_info.h>

#include "Attributes.h"
#include "Bitmaps.h"
#include "Commands.h"
#include "FSUndoRedo.h"
#include "FSUtils.h"
#include "InfoWindow.h"
#include "MimeTypes.h"
#include "Model.h"
#include "OverrideAlert.h"
#include "StatusWindow.h"
#include "Thread.h"
#include "Tracker.h"
#include "TrackerSettings.h"
#include "Utilities.h"


enum {
	kUserCanceled = B_ERRORS_END + 1,
	kCopyCanceled = kUserCanceled,
	kTrashCanceled
};

enum ConflictCheckResult {
	kCanceled = kUserCanceled,
	kPrompt,
	kReplace,
	kReplaceAll,
	kNoConflicts
};
 
namespace BPrivate {

static status_t FSDeleteFolder(BEntry *, CopyLoopControl *, bool updateStatus, 
	bool deleteTopDir = true, bool upateFileNameInStatus = false);
static status_t MoveEntryToTrash(BEntry *, BPoint *, Undo &undo);
static void LowLevelCopy(BEntry *, StatStruct *, BDirectory *, char *destName,
	CopyLoopControl *, BPoint *);
status_t DuplicateTask(BObjectList<entry_ref> *srcList);
static status_t MoveTask(BObjectList<entry_ref> *, BEntry *, BList *, uint32);
static status_t _DeleteTask(BObjectList<entry_ref> *, bool);
static status_t _RestoreTask(BObjectList<entry_ref> *);
status_t CalcItemsAndSize(BObjectList<entry_ref> *refList, int32 *totalCount, off_t *totalSize);
status_t MoveItem(BEntry *entry, BDirectory *destDir, BPoint *loc,
	uint32 moveMode, const char *newName, Undo &undo);
ConflictCheckResult PreFlightNameCheck(BObjectList<entry_ref> *srcList, const BDirectory *destDir,
	int32 *collisionCount);
status_t CheckName(uint32 moveMode, const BEntry *srcEntry, const BDirectory *destDir,
	bool multipleCollisions, ConflictCheckResult &);
void CopyAttributes(CopyLoopControl *control, BNode *srcNode, BNode* destNode, void *buffer,
	size_t bufsize);
void CopyPoseLocation(BNode *src, BNode *dest);
bool DirectoryMatchesOrContains(const BEntry *, directory_which);
bool DirectoryMatchesOrContains(const BEntry *, const char *additionalPath, directory_which);
bool DirectoryMatches(const BEntry *, directory_which);
bool DirectoryMatches(const BEntry *, const char *additionalPath, directory_which);

status_t empty_trash(void *);

#ifdef __HAIKU__
	#define OS_NAME	"Haiku"
#else
	#define OS_NAME "BeOS"
#endif


const char *kDeleteConfirmationStr = "Are you sure you want to delete the selected "
	"item(s)? This operation cannot be reverted.";

const char *kReplaceStr = "You are trying to replace the item:\n"
	"\t%s%s\n"
	"with:\n"
	"\t%s%s\n\n"
	"Would you like to replace it with the one you are %s?";

const char *kDirectoryReplaceStr = "An item named \"%s\" already exists in this folder. "
	"Would you like to replace it with the one you are %s?";

const char *kSymLinkReplaceStr = "An item named \"%s\" already exists in this folder. "
	"Would you like to replace it with the symbolic link you are creating?";

const char *kNoFreeSpace = "Sorry, there is not enough free space on the destination "
	"volume to copy the selection.";

const char *kFileErrorString = "Error copying file \"%s\":\n\t%s\n\nWould you like to continue?";
const char *kFolderErrorString = "Error copying folder \"%s\":\n\t%s\n\nWould you like to continue?";
const char *kFileDeleteErrorString = "There was an error deleting \"%s\":\n\t%s";
const char *kReplaceManyStr = "Some items already exist in this folder with "
	"the same names as the items you are %s.\n \nWould you like to replace them "
	"with the ones you are %s or be prompted for each one?";

const char *kFindAlternativeStr = "Would you like to find some other suitable application?";
const char *kFindApplicationStr = "Would you like to find a suitable application to "
	"open the file?";

// Skip these attributes when copying in Tracker
const char *kSkipAttributes[] = {
	kAttrPoseInfo,
	NULL
};


CopyLoopControl::~CopyLoopControl()
{
}


bool 
TrackerCopyLoopControl::FileError(const char *message, const char *name,
	status_t error, bool allowContinue)
{
	char buffer[512];
	sprintf(buffer, message, name, strerror(error));

	if (allowContinue) 
		return (new BAlert("", buffer, "Cancel", "OK", 0,
			B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go() != 0;

	(new BAlert("", buffer, "Cancel", 0, 0,
			B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
	return false;
}


void 
TrackerCopyLoopControl::UpdateStatus(const char *name, entry_ref, int32 count, 
	bool optional)
{
	if (gStatusWindow && gStatusWindow->HasStatus(fThread))
		gStatusWindow->UpdateStatus(fThread, const_cast<char *>(name),  
			count, optional);
}


bool 
TrackerCopyLoopControl::CheckUserCanceled()
{
	return gStatusWindow && gStatusWindow->CheckCanceledOrPaused(fThread);
}


TrackerCopyLoopControl::OverwriteMode
TrackerCopyLoopControl::OverwriteOnConflict(const BEntry *, const char *, 
	const BDirectory *, bool, bool)
{
	return kReplace;
}


bool 
TrackerCopyLoopControl::SkipEntry(const BEntry *, bool)
{
	// tracker makes no exceptions
	return false;
}


bool 
TrackerCopyLoopControl::SkipAttribute(const char *attributeName)
{
	for (const char **skipAttribute = kSkipAttributes; *skipAttribute;
		skipAttribute++) 
		if (strcmp(*skipAttribute, attributeName) == 0)
			return true;

	return false;
}


void 
CopyLoopControl::ChecksumChunk(const char *, size_t)
{
}


bool 
CopyLoopControl::ChecksumFile(const entry_ref *)
{
	return true;
}


bool
CopyLoopControl::SkipAttribute(const char*)
{
	return false;
}


bool
CopyLoopControl::PreserveAttribute(const char*)
{
	return false;
}


static BNode *
GetWritableNode(BEntry *entry, StatStruct *statBuf = 0)
{
	// utility call that works around the problem with BNodes not being
	// universally writeable
	// BNodes created on files will fail to WriteAttr because they do not
	// have the right r/w permissions

	StatStruct localStatbuf;
	
	if (!statBuf) {
		statBuf = &localStatbuf;
		if (entry->GetStat(statBuf) != B_OK)
			return 0;
	}

	if (S_ISREG(statBuf->st_mode))
		return new BFile(entry, O_RDWR);
	
	return new BNode(entry);
}


status_t
FSSetPoseLocation(ino_t destDirInode, BNode *destNode, BPoint point)
{
	PoseInfo poseInfo;
	poseInfo.fInvisible = false;
	poseInfo.fInitedDirectory = destDirInode;
	poseInfo.fLocation = point;

	status_t result = destNode->WriteAttr(kAttrPoseInfo, B_RAW_TYPE, 0,
		&poseInfo, sizeof(poseInfo));
		
	if (result == sizeof(poseInfo))
		return B_OK;
	
	return result;
}


status_t
FSSetPoseLocation(BEntry *entry, BPoint point)
{
	BNode node(entry);
	status_t result = node.InitCheck();
	if (result != B_OK)
		return result;
	
	BDirectory parent;
	result = entry->GetParent(&parent);
	if (result != B_OK)
		return result;
	
	node_ref destNodeRef;
	result = parent.GetNodeRef(&destNodeRef);
	if (result != B_OK)
		return result;
	
	return FSSetPoseLocation(destNodeRef.node, &node, point);
}


bool
FSGetPoseLocation(const BNode *node, BPoint *point)
{
	PoseInfo poseInfo;
	if (ReadAttr(node, kAttrPoseInfo, kAttrPoseInfoForeign,
		B_RAW_TYPE, 0, &poseInfo, sizeof(poseInfo), &PoseInfo::EndianSwap)
		== kReadAttrFailed)
		return false;
	
	if (poseInfo.fInitedDirectory == -1LL)
		return false;

	*point = poseInfo.fLocation;

	return true;
}


static void
SetUpPoseLocation(ino_t sourceParentIno, ino_t destParentIno,
	const BNode *sourceNode, BNode *destNode, BPoint *loc)
{
	BPoint point;
	if (!loc
		// we don't have a position yet
		&& sourceParentIno != destParentIno
		// we aren't  copying into the same directory
		&& FSGetPoseLocation(sourceNode, &point))
		// the original has a valid inited location
		loc = &point;
		// copy the originals location

	if (loc && loc != (BPoint *)-1) {
		// loc of -1 is used when copying/moving into a window in list mode
		// where copying positions would not work
		// ToSo:
		// should push all this logic to upper levels
		FSSetPoseLocation(destParentIno, destNode, *loc);
	}
}

	
void
FSMoveToFolder(BObjectList<entry_ref> *srcList, BEntry *destEntry,
	uint32 moveMode, BList *pointList)
{
	if (srcList->IsEmpty()) {
		delete srcList;
		delete pointList;
		delete destEntry;
		return;
	}

	LaunchInNewThread("MoveTask", B_NORMAL_PRIORITY, MoveTask, srcList, destEntry,
		pointList, moveMode);
}


void
FSDelete(entry_ref *ref, bool async, bool confirm)
{
	BObjectList<entry_ref> *list = new BObjectList<entry_ref>(1, true);
	list->AddItem(ref);
	FSDeleteRefList(list, async, confirm);
}


void
FSDeleteRefList(BObjectList<entry_ref> *list, bool async, bool confirm)
{
	if (async) 
		LaunchInNewThread("DeleteTask", B_NORMAL_PRIORITY, _DeleteTask, list, confirm);
	else
		_DeleteTask(list, confirm);
}


void
FSRestoreRefList(BObjectList<entry_ref> *list, bool async)
{
	if (async) 
		LaunchInNewThread("RestoreTask", B_NORMAL_PRIORITY, _RestoreTask, list);
	else
		_RestoreTask(list);
}


void
FSMoveToTrash(BObjectList<entry_ref> *srcList, BList *pointList, bool async)
{
	if (srcList->IsEmpty()) {
		delete srcList;
		delete pointList;
		return;
	}

	if (async) 
		LaunchInNewThread("MoveTask", B_NORMAL_PRIORITY, MoveTask, srcList,
			(BEntry *)0, pointList, kMoveSelectionTo);
	else
		MoveTask(srcList, 0, pointList, kMoveSelectionTo);
}


static bool
IsDisksWindowIcon(BEntry *entry)
{
	BPath path;
	if (entry->InitCheck() != B_OK || entry->GetPath(&path) != B_OK)
		return false;
	
	return strcmp(path.Path(), "/") == 0;
}

enum {
	kNotConfirmed,
	kConfirmedHomeMove,
	kConfirmedAll
};


bool
ConfirmChangeIfWellKnownDirectory(const BEntry *entry, const char *action,
	bool dontAsk, int32 *confirmedAlready)
{
	// Don't let the user casually move/change important files/folders
	//
	// This is a cheap replacement for having a real UID support turned
	// on and not running as root all the time

	if (confirmedAlready && *confirmedAlready == kConfirmedAll)
		return true;

	if (!DirectoryMatchesOrContains(entry, B_BEOS_DIRECTORY)
		&& !DirectoryMatchesOrContains(entry, B_USER_DIRECTORY))
		// quick way out
		return true;

	const char *warning = NULL;
	bool requireOverride = true;

	if (DirectoryMatches(entry, B_BEOS_DIRECTORY))
		warning = "If you %s the beos folder, you won't be able to "
			"boot " OS_NAME "! Are you sure you want to do this? To %s the folder "
			"anyway, hold down the Shift key and click \"Do it\".";
	else if (DirectoryMatchesOrContains(entry, B_BEOS_SYSTEM_DIRECTORY))
		warning = "If you %s the system folder or its contents, you "
			"won't be able to boot " OS_NAME "! Are you sure you want to do this? "
			"To %s the system folder or its contents anyway, hold down "
			"the Shift key and click \"Do it\".";
	else if (DirectoryMatches(entry, B_USER_DIRECTORY)) {
		warning = "If you %s the home folder, " OS_NAME " may not "
			"behave properly! Are you sure you want to do this? "
			"To %s the home anyway, click \"Do it\".";
		requireOverride = false;
	} else if (DirectoryMatchesOrContains(entry, B_USER_CONFIG_DIRECTORY)
		|| DirectoryMatchesOrContains(entry, B_COMMON_SETTINGS_DIRECTORY)) {
		
		if (DirectoryMatchesOrContains(entry, "beos_mime", B_USER_SETTINGS_DIRECTORY)
			|| DirectoryMatchesOrContains(entry, "beos_mime", B_COMMON_SETTINGS_DIRECTORY)) {
			warning = "If you %s the mime settings, " OS_NAME " may not "
				"behave properly! Are you sure you want to do this? "
				"To %s the mime settings anyway, click \"Do it\".";
			requireOverride = false;
		} else if (DirectoryMatches(entry, B_USER_CONFIG_DIRECTORY)) {
			warning = "If you %s the config folder, " OS_NAME " may not "
				"behave properly! Are you sure you want to do this? "
				"To %s the config folder anyway, click \"Do it\".";
			requireOverride = false;
		} else if (DirectoryMatches(entry, B_USER_SETTINGS_DIRECTORY)
			|| DirectoryMatches(entry, B_COMMON_SETTINGS_DIRECTORY)) {
			warning = "If you %s the settings folder, " OS_NAME " may not "
				"behave properly! Are you sure you want to do this? "
				"To %s the settings folder anyway, click \"Do it\".";
			requireOverride = false;
		}
	}
	
	if (!warning)
		return true;

	if (dontAsk)
		return false;

	if (confirmedAlready && *confirmedAlready == kConfirmedHomeMove
		&& !requireOverride)
		// we already warned about moving home this time around
		return true;

	char buffer[256];
	sprintf(buffer, warning, action, action);

	if ((new OverrideAlert("", buffer, "Do it", (requireOverride ? B_SHIFT_KEY : 0),
		"Cancel", 0, NULL, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go() == 1) {
		if (confirmedAlready)
			*confirmedAlready = kNotConfirmed;
		return false;
	}
	
	if (confirmedAlready) {
		if (!requireOverride)
			*confirmedAlready = kConfirmedHomeMove;
		else
			*confirmedAlready = kConfirmedAll;
	}

	return true;
}


static status_t
InitCopy(uint32 moveMode, BObjectList<entry_ref> *srcList, thread_id thread, 
	BVolume *dstVol, BDirectory *destDir, entry_ref *destRef,
	bool preflightNameCheck, int32 *collisionCount, ConflictCheckResult *preflightResult)
{
	if (dstVol->IsReadOnly()) {
		if (gStatusWindow)
			gStatusWindow->RemoveStatusItem(thread);

		(new BAlert("", "You can't move or copy items to read-only volumes.",
			"Cancel", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
		return B_ERROR;
	}

	int32 numItems = srcList->CountItems();
	int32 askOnceOnly = kNotConfirmed;
	for (int32 index = 0; index < numItems; index++) {
		// we could check for this while iterating through items in each of the copy
		// loops, except it takes forever to call CalcItemsAndSize
		BEntry entry((entry_ref *)srcList->ItemAt(index));
		if (IsDisksWindowIcon(&entry)) {

			const char *errorStr;
			if (moveMode == kCreateLink)
				errorStr = "You cannot create a link to the root directory.";
			else
				errorStr = "You cannot copy or move the root directory.";

			(new BAlert("", errorStr, "Cancel", 0, 0,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();	
			return B_ERROR;
		}
		if (moveMode == kMoveSelectionTo
			&& !ConfirmChangeIfWellKnownDirectory(&entry, "move", false, &askOnceOnly))
			return B_ERROR;
	}

	if (preflightNameCheck) {
		ASSERT(collisionCount);
		ASSERT(preflightResult);
		
		*preflightResult = kPrompt;
		*collisionCount = 0;

		*preflightResult = PreFlightNameCheck(srcList, destDir, collisionCount);
		if (*preflightResult == kCanceled)		// user canceled
			return B_ERROR;
	}

	// set up the status display
	switch (moveMode) {
		case kCopySelectionTo:
		case kDuplicateSelection:
			{
				if (gStatusWindow)
					gStatusWindow->CreateStatusItem(thread, kCopyState);

				int32 totalItems = 0;
				off_t totalSize = 0;
				if (CalcItemsAndSize(srcList, &totalItems, &totalSize) != B_OK)
					return B_ERROR;

				// check for free space before starting copy
				if ((totalSize + (4 * kKBSize)) >= dstVol->FreeBytes()) {
					(new BAlert("", kNoFreeSpace, "Cancel", 0, 0,
						B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
					return B_ERROR;
				}

				if (gStatusWindow)
					gStatusWindow->InitStatusItem(thread, totalItems, totalSize,
						destRef);
				break;
			}

		case kMoveSelectionTo:
		case kCreateLink:
			if (numItems > 10) {
				// this will be fast, only put up status if lots of items
				// moved, links created
				if (gStatusWindow) {
					gStatusWindow->CreateStatusItem(thread,
						moveMode == kMoveSelectionTo
							? kMoveState : kCreateLinkState);
					gStatusWindow->InitStatusItem(thread, numItems, numItems,
						destRef);
				}
			}
			break;
	}
	return B_OK;
}


// ToDo:
// get rid of this cruft
bool
delete_ref(void *ref)
{
	delete (entry_ref*)ref;
	return false;
}


bool
delete_point(void *point)
{
	delete (BPoint*)point;
	return false;
}


static status_t
MoveTask(BObjectList<entry_ref> *srcList, BEntry *destEntry, BList *pointList, uint32 moveMode)
{
	ASSERT(!srcList->IsEmpty());

	// extract information from src, dest models
	// ## note that we're assuming all items come from the same volume
	// ## by looking only at FirstItem here which is not a good idea
	dev_t srcVolumeDevice = srcList->FirstItem()->device;
	dev_t destVolumeDevice = srcVolumeDevice;

	StatStruct deststat;
	BVolume volume;
	entry_ref destRef;
	const entry_ref *destRefToCheck = NULL;

	bool destIsTrash = false;
	BDirectory destDir;
	BDirectory *destDirToCheck = NULL;
	bool needPreflightNameCheck = false;

	bool fromUndo = FSIsUndoMoveMode(moveMode);
	moveMode = FSMoveMode(moveMode);

	// if we're not passed a destEntry then we are supposed to move to trash
	if (destEntry) {
		destEntry->GetRef(&destRef);
		destRefToCheck = &destRef;

		destDir.SetTo(destEntry);
		destDir.GetStat(&deststat);
		destDirToCheck = &destDir;

		destVolumeDevice = deststat.st_dev;
		destIsTrash = FSIsTrashDir(destEntry);
		volume.SetTo(destVolumeDevice);
		
		needPreflightNameCheck = true;
	} else if (moveMode == kDuplicateSelection)
		volume.SetTo(srcVolumeDevice);
	else {
		// move is to trash
		destIsTrash = true;

		FSGetTrashDir(&destDir, srcVolumeDevice);
		volume.SetTo(srcVolumeDevice);

		BEntry entry;
		destDir.GetEntry(&entry);
		destDirToCheck = &destDir;

		entry.GetRef(&destRef);
		destRefToCheck = &destRef;
	}

	// change the move mode if needed
	if (moveMode == kMoveSelectionTo && srcVolumeDevice != destVolumeDevice)
		// move across volumes - copy instead
		moveMode = kCopySelectionTo;
	if (moveMode == kCopySelectionTo && destIsTrash)
		// cannot copy to trash
		moveMode = kMoveSelectionTo;

	// we need the undo object later on, so we create it no matter
	// if we really need it or not (it's very lightweight)
	MoveCopyUndo undo(srcList, destDir, pointList, moveMode);
	if (fromUndo)
		undo.Remove();

	thread_id thread = find_thread(NULL);
	ConflictCheckResult conflictCheckResult = kPrompt;
	int32 collisionCount = 0;
	status_t result = InitCopy(moveMode, srcList, thread, &volume, destDirToCheck,
		&destRef, needPreflightNameCheck, &collisionCount, &conflictCheckResult);
	
	int32 count = srcList->CountItems();
	if (result == B_OK) {
		for (int32 i = 0; i < count; i++) {
			BPoint *loc = (BPoint *)-1;
				// a loc of -1 forces autoplacement, rather than copying the
				// position of the original node
				// ToDo:
				// clean this mess up

			entry_ref *srcRef = srcList->ItemAt(i);

			if (moveMode == kDuplicateSelection) {
				BEntry entry(srcRef);
				entry.GetParent(&destDir);
				destDir.GetStat(&deststat);
				volume.SetTo(srcRef->device);
			}

			// handle case where item is dropped into folder it already lives in
			// which could happen if dragging from a query window
			if (moveMode != kCreateLink
				&& moveMode != kCreateRelativeLink
				&& moveMode != kDuplicateSelection
				&& !destIsTrash
				&& (srcRef->device == destRef.device
					&& srcRef->directory == deststat.st_ino))
				continue;

			if (gStatusWindow && gStatusWindow->CheckCanceledOrPaused(thread))
				break;

			BEntry sourceEntry(srcRef);
			if (sourceEntry.InitCheck() != B_OK) {
				BString error;
				error << "Error moving \"" << srcRef->name << "\".";
				(new BAlert("", error.String(), "Cancel", 0, 0,
					B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
				break;
			}

			// are we moving item to trash?
			if (destIsTrash) {
				if (pointList)
					loc = (BPoint *)pointList->ItemAt(i);

				result = MoveEntryToTrash(&sourceEntry, loc, undo);
				if (result != B_OK) {
					BString error;
					error << "Error moving \"" << srcRef->name << "\" to Trash. ("
						<< strerror(result) << ")";
					(new BAlert("", error.String(), "Cancel", 0, 0,
						B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
					break;
				}
				continue;
			}

			// resolve name collisions and hierarchy problems
			if (CheckName(moveMode, &sourceEntry, &destDir, collisionCount > 1,
				conflictCheckResult) != B_OK) {
				// we will skip the current item, because we got a conflict
				// and were asked to or because there was some conflict

				// update the status because item got skipped and the status
				// will not get updated by the move call
				if (gStatusWindow && gStatusWindow->HasStatus(thread))
					gStatusWindow->UpdateStatus(thread, srcRef->name, 1);

				continue;
			}

			// get location to place this item
			if (pointList && moveMode != kCopySelectionTo) {
				loc = (BPoint *)pointList->ItemAt(i);

				BNode *src_node = GetWritableNode(&sourceEntry);
				if (src_node && src_node->InitCheck() == B_OK) {
					PoseInfo poseInfo;
					poseInfo.fInvisible = false;
					poseInfo.fInitedDirectory = deststat.st_ino;
					poseInfo.fLocation = *loc;
					src_node->WriteAttr(kAttrPoseInfo, B_RAW_TYPE, 0,
						&poseInfo, sizeof(poseInfo));
				}
				delete src_node;
			}

			if (pointList)
 				loc = (BPoint*)pointList->ItemAt(i);

			result = MoveItem(&sourceEntry, &destDir, loc, moveMode, NULL, undo);
			if (result != B_OK)
				break;
		}
	}

	// duplicates of srcList, destFolder were created - dispose them
	delete srcList;
	delete destEntry;

	// delete file location list and all Points within
	if (pointList) {
		pointList->DoForEach(delete_point);
		delete pointList;
	}

	if (gStatusWindow)
		gStatusWindow->RemoveStatusItem(thread);

	return B_OK;
}

class FailWithAlert {
	public:
		static void FailOnError(status_t error, const char *string, const char *name = NULL)
		{
			if (error != B_OK) 
				throw FailWithAlert(error, string, name);
		}

		FailWithAlert(status_t error, const char *string, const char *name)
		:	fString(string),
			fName(name),
			fError(error)
		{
		}

		const char *fString;
		const char *fName;
		status_t fError;
};

class MoveError {
	public:
		static void FailOnError(status_t error)
		{
			if (error != B_OK) 
				throw MoveError(error);
		}

		MoveError(status_t error)
		:	fError(error)
		{ }

		status_t fError;
};


void
CopyFile(BEntry *srcFile, StatStruct *srcStat, BDirectory *destDir,
	CopyLoopControl *loopControl, BPoint *loc, bool makeOriginalName, Undo &undo)
{
	if (loopControl->SkipEntry(srcFile, true))
		return;

	node_ref node;
	destDir->GetNodeRef(&node);
	BVolume volume(node.device);

	// check for free space first
	if ((srcStat->st_size + kKBSize) >= volume.FreeBytes()) {
		loopControl->FileError(kNoFreeSpace, "", B_DEVICE_FULL, false);
		throw (status_t)B_DEVICE_FULL;
	}

	char destName[B_FILE_NAME_LENGTH];
	srcFile->GetName(destName);
	entry_ref ref;
	srcFile->GetRef(&ref);

	loopControl->UpdateStatus(destName, ref, 1024, true);

	if (makeOriginalName) {
		FSMakeOriginalName(destName, destDir, " copy");
		undo.UpdateEntry(srcFile, destName);
	}

	BEntry conflictingEntry;
	if (destDir->FindEntry(destName, &conflictingEntry) == B_OK) {
		switch (loopControl->OverwriteOnConflict(srcFile, destName, destDir, 
			false, false)) {
			case TrackerCopyLoopControl::kSkip:
				// we are about to ignore this entire directory
				return;

			case TrackerCopyLoopControl::kReplace:
				if (conflictingEntry.IsDirectory()) 
					// remove existing folder recursively
					ThrowOnError(FSDeleteFolder(&conflictingEntry, loopControl, false));
				else
					ThrowOnError(conflictingEntry.Remove());
				break;

			case TrackerCopyLoopControl::kMerge:
				// This flag implies that the attributes should be kept
				// on the file.  Just ignore it.
				break;
		}
	}

	try {
		LowLevelCopy(srcFile, srcStat, destDir, destName, loopControl, loc);
	} catch (status_t err) {
		if (err == kCopyCanceled)
			throw (status_t)err;

		if (err != B_OK) {
			if (!loopControl->FileError(kFileErrorString, destName, err, true))
				throw (status_t)err;
			else
				// user selected continue in spite of error, update status bar
				loopControl->UpdateStatus(NULL, ref, (int32)srcStat->st_size);
		}
	}
}


#ifdef _SILENTLY_CORRECT_FILE_NAMES
static bool
CreateFileSystemCompatibleName(const BDirectory *destDir, char *destName)
{
	// Is it a FAT32 file system? (this is the only one we currently now about)

	BEntry target;
	destDir->GetEntry(&target);
	entry_ref targetRef;
	fs_info info;
	if (target.GetRef(&targetRef) == B_OK
		&& fs_stat_dev(targetRef.device, &info) == B_OK
		&& !strcmp(info.fsh_name, "dos")) {
		bool wasInvalid = false;

		// it's a FAT32 file system, now check the name

		int32 length = strlen(destName) - 1;
		while (destName[length] == '.') {
			// invalid name, just cut off the dot at the end
			destName[length--] = '\0';
			wasInvalid = true;
		}

		char *invalid = destName;
		while ((invalid = strpbrk(invalid, "?<>\\:\"|*")) != NULL) {
			invalid[0] = '_';
			wasInvalid = true;
		}

		return wasInvalid;
	}

	return false;
}
#endif


static void
LowLevelCopy(BEntry *srcEntry, StatStruct *srcStat, BDirectory *destDir,
	char *destName, CopyLoopControl *loopControl, BPoint *loc)
{
	entry_ref ref;
	ThrowOnError(srcEntry->GetRef(&ref));

	if (S_ISLNK(srcStat->st_mode)) {
		// handle symbolic links
		BSymLink srcLink;
		BSymLink newLink;
		char linkpath[MAXPATHLEN];

		ThrowOnError(srcLink.SetTo(srcEntry));
		ThrowIfNotSize(srcLink.ReadLink(linkpath, MAXPATHLEN-1));

		ThrowOnError(destDir->CreateSymLink(destName, linkpath, &newLink));

		node_ref destNodeRef;
		destDir->GetNodeRef(&destNodeRef);
		// copy or write new pose location as a first thing
		SetUpPoseLocation(ref.directory, destNodeRef.node, &srcLink,
			&newLink, loc);

		BNodeInfo nodeInfo(&newLink);
		ThrowOnError(nodeInfo.SetType(B_LINK_MIMETYPE));

		newLink.SetPermissions(srcStat->st_mode);
		newLink.SetOwner(srcStat->st_uid);
		newLink.SetGroup(srcStat->st_gid);
		newLink.SetModificationTime(srcStat->st_mtime);
		newLink.SetCreationTime(srcStat->st_crtime);

		return;
	}

	BFile srcFile(srcEntry, O_RDONLY);
	ThrowOnInitCheckError(&srcFile);

	const size_t kMinBufferSize = 1024 * 128; 
	const size_t kMaxBufferSize = 1024 * 1024; 
 
	size_t bufsize = kMinBufferSize;
	if (bufsize < srcStat->st_size) {
		//	File bigger than the buffer size: determine an optimal buffer size 
		system_info sinfo; 
		get_system_info(&sinfo); 
		size_t freesize = static_cast<size_t>((sinfo.max_pages - sinfo.used_pages) * B_PAGE_SIZE);
		bufsize = freesize / 4;					// take 1/4 of RAM max 
		bufsize -= bufsize % (16 * 1024);		// Round to 16 KB boundaries 
		if (bufsize < kMinBufferSize)			// at least kMinBufferSize 
			bufsize = kMinBufferSize; 
		else if (bufsize > kMaxBufferSize)		// no more than kMaxBufferSize 
			bufsize = kMaxBufferSize; 
	} 

	BFile destFile(destDir, destName, O_RDWR | O_CREAT);
#ifdef _SILENTLY_CORRECT_FILE_NAMES
	if ((destFile.InitCheck() == B_BAD_VALUE || destFile.InitCheck() == B_NOT_ALLOWED)
		&& CreateFileSystemCompatibleName(destDir, destName))
		destFile.SetTo(destDir, destName, B_CREATE_FILE | B_READ_WRITE);
#endif

	ThrowOnInitCheckError(&destFile);

	node_ref destNodeRef;
	destDir->GetNodeRef(&destNodeRef);
	// copy or write new pose location as a first thing
	SetUpPoseLocation(ref.directory, destNodeRef.node, &srcFile,
		&destFile, loc);

	char *buffer = new char[bufsize];
	try {
		// copy data portion of file
		while (true) {
			if (loopControl->CheckUserCanceled()) {
				// if copy was canceled, remove partial destination file
				destFile.Unset();
	
				BEntry destEntry;
				if (destDir->FindEntry(destName, &destEntry) == B_OK)
					destEntry.Remove();

				throw (status_t)kCopyCanceled;
			}
	
			ASSERT(buffer);
			ssize_t bytes = srcFile.Read(buffer, bufsize);

			if (bytes > 0) {
				ssize_t updateBytes = 0; 
				if (bytes > 32 * 1024) { 
					// when copying large chunks, update after read and after write
					// to get better update granularity
					updateBytes = bytes / 2; 
					loopControl->UpdateStatus(NULL, ref, updateBytes, true); 
				} 
 
				loopControl->ChecksumChunk(buffer, (size_t)bytes); 
	 
				ssize_t result = destFile.Write(buffer, (size_t)bytes); 
				if (result != bytes)  
					throw (status_t)B_ERROR; 
	 
				loopControl->UpdateStatus(NULL, ref, bytes - updateBytes, true); 
			} else if (bytes < 0) 
				// read error
				throw (status_t)bytes;
			else
				// we are done
				break;
		}

		CopyAttributes(loopControl, &srcFile, &destFile, buffer, bufsize);
	} catch (...) {
		delete [] buffer;
		throw;
	}

	destFile.SetPermissions(srcStat->st_mode);
	destFile.SetOwner(srcStat->st_uid);
	destFile.SetGroup(srcStat->st_gid);
	destFile.SetModificationTime(srcStat->st_mtime);
	destFile.SetCreationTime(srcStat->st_crtime);

	delete [] buffer;

	if (!loopControl->ChecksumFile(&ref)) {
		// File no good.  Remove and quit.
		destFile.Unset();

		BEntry destEntry;
		if (destDir->FindEntry(destName, &destEntry) == B_OK)
			destEntry.Remove();
		throw (status_t)kUserCanceled;
	}
}


void
CopyAttributes(CopyLoopControl *control, BNode *srcNode, BNode *destNode, void *buffer,
	size_t bufsize)
{
	// ToDo:
	// Add error checking
	// prior to coyping attributes, make sure indices are installed

	// When calling CopyAttributes on files, have to make sure destNode
	// is a BFile opened R/W
	
	srcNode->RewindAttrs();
	char name[256];
	while (srcNode->GetNextAttrName(name) == B_OK) {
		// Check to see if this attribute should be skipped.
		if (control->SkipAttribute(name))
			continue;

		attr_info info;
		if (srcNode->GetAttrInfo(name, &info) != B_OK)
			continue;

		// Check to see if this attribute should be overwritten when it
		// already exists.
		if (control->PreserveAttribute(name)) {
			attr_info dest_info;
			if (destNode->GetAttrInfo(name, &dest_info) == B_OK)
				continue;
		}

		ssize_t bytes;
		ssize_t numToRead = (ssize_t)info.size;
		for (off_t offset = 0; numToRead > 0; offset += bytes) {
			size_t chunkSize = (size_t)numToRead;
			if (chunkSize > bufsize)
				chunkSize = bufsize;

			bytes = srcNode->ReadAttr(name, info.type, offset,
				buffer, chunkSize);

			if (bytes <= 0) 
				break;
			
			destNode->WriteAttr(name, info.type, offset, buffer, (size_t)bytes);

			numToRead -= bytes;
		}
	}
}


static void
CopyFolder(BEntry *srcEntry, BDirectory *destDir, CopyLoopControl *loopControl,
	BPoint *loc, bool makeOriginalName, Undo &undo)
{
	BDirectory newDir;
	BEntry entry;
	status_t err = B_OK;
	bool createDirectory = true;
	BEntry existingEntry;

	if (loopControl->SkipEntry(srcEntry, false))
		return;

	entry_ref ref;
	srcEntry->GetRef(&ref);

	char destName[B_FILE_NAME_LENGTH];
	strcpy(destName, ref.name);

	loopControl->UpdateStatus(ref.name, ref, 1024, true);

	if (makeOriginalName) {
		FSMakeOriginalName(destName, destDir, " copy");
		undo.UpdateEntry(srcEntry, destName);
	}

	if (destDir->FindEntry(destName, &existingEntry) == B_OK) {
		// some entry with a conflicting name is already present in destDir
		// decide what to do about it
		bool isDirectory = existingEntry.IsDirectory();

		switch (loopControl->OverwriteOnConflict(srcEntry, destName, destDir, 
			true, isDirectory)) {
			case TrackerCopyLoopControl::kSkip:
				// we are about to ignore this entire directory
				return;

			case TrackerCopyLoopControl::kReplace:
				if (isDirectory) 
					// remove existing folder recursively
					ThrowOnError(FSDeleteFolder(&existingEntry, loopControl, false));

				else
					// conflicting with a file or symbolic link, remove entry
					ThrowOnError(existingEntry.Remove());
				break;

			case TrackerCopyLoopControl::kMerge:
				ASSERT(isDirectory);
				// do not create a new directory, use the current one
				newDir.SetTo(&existingEntry);
				createDirectory = false;
				break;
		}
	}

	// loop through everything in src folder and copy it to new folder
	BDirectory srcDir(srcEntry);
	srcDir.Rewind();
	srcEntry->Unset();

	// create a new folder inside of destination folder
	if (createDirectory) {
	 	err = destDir->CreateDirectory(destName, &newDir);
#ifdef _SILENTLY_CORRECT_FILE_NAMES
	 	if (err == B_BAD_VALUE) {
	 		// check if it's an invalid name on a FAT32 file system
	 		if (CreateFileSystemCompatibleName(destDir, destName))
	 			err = destDir->CreateDirectory(destName, &newDir);
	 	}
#endif
	 	if (err != B_OK) {
			if (!loopControl->FileError(kFolderErrorString, destName, err, true))
				throw err;

			// will allow rest of copy to continue
			return;
		}
	}

	char *buffer;
	if (createDirectory && err == B_OK && (buffer = (char*)malloc(32768)) != 0) {
		CopyAttributes(loopControl, &srcDir, &newDir, buffer, 32768);
			// don't copy original pose location if new location passed
		free(buffer);
	}
	
	StatStruct statbuf;
	srcDir.GetStat(&statbuf);
	dev_t sourceDeviceID = statbuf.st_dev;

	// copy or write new pose location
	node_ref destNodeRef;
	destDir->GetNodeRef(&destNodeRef);
	SetUpPoseLocation(ref.directory, destNodeRef.node, &srcDir,
		&newDir, loc);

	while (srcDir.GetNextEntry(&entry) == B_OK) {

		if (loopControl->CheckUserCanceled())
			throw (status_t)kUserCanceled;

		entry.GetStat(&statbuf);
				
		if (S_ISDIR(statbuf.st_mode)) {

			// entry is a mount point, do not copy it
			if (statbuf.st_dev != sourceDeviceID) {
				PRINT(("Avoiding mount point %d, %d	\n", statbuf.st_dev, sourceDeviceID));
				continue;
			}
			
			CopyFolder(&entry, &newDir, loopControl, 0, false, undo);
		} else
			CopyFile(&entry, &statbuf, &newDir, loopControl, 0, false, undo);
	}
}


status_t
MoveItem(BEntry *entry, BDirectory *destDir, BPoint *loc, uint32 moveMode,
	const char *newName, Undo &undo)
{
	entry_ref ref;
	try {
		node_ref destNode;
		StatStruct statbuf;

		MoveError::FailOnError(entry->GetStat(&statbuf));
		MoveError::FailOnError(entry->GetRef(&ref));
		MoveError::FailOnError(destDir->GetNodeRef(&destNode));	

		if (moveMode == kCreateLink || moveMode == kCreateRelativeLink) {
			PoseInfo poseInfo;
			char name[B_FILE_NAME_LENGTH];
			strcpy(name, ref.name);

			BSymLink link;
			FSMakeOriginalName(name, destDir, " link");
			undo.UpdateEntry(entry, name);

			BPath path;
			entry->GetPath(&path);
			if (loc && loc != (BPoint *)-1) {
				poseInfo.fInvisible = false;
				poseInfo.fInitedDirectory = destNode.node;
				poseInfo.fLocation = *loc;
			}

			status_t err = B_ERROR;

			if (moveMode == kCreateRelativeLink) {
				if (statbuf.st_dev == destNode.device) {
					// relative link only works on the same device
					char oldwd[B_PATH_NAME_LENGTH];
					getcwd(oldwd, B_PATH_NAME_LENGTH);

					BEntry destEntry;
					destDir -> GetEntry(&destEntry);
					BPath destPath;
					destEntry.GetPath(&destPath);

					chdir(destPath.Path());
						// change working dir to target dir

					BString destString(destPath.Path());
					destString.Append("/");

					BString srcString(path.Path());
					srcString.RemoveLast(path.Leaf());

					// find index while paths are the same

					const char *src = srcString.String();
					const char *dest = destString.String();					
					const char *lastFolderSrc = src;
					const char *lastFolderDest = dest;

					while (*src && *dest && *src == *dest) {
						++src;
						if (*dest++ == '/') {
							lastFolderSrc = src;
							lastFolderDest = dest;
						}
					}
					src = lastFolderSrc;
					dest = lastFolderDest;

					BString source;
					if (*dest == '\0' && *src != '\0') {
						// source is deeper in the same tree than the target
						source.Append(src);
					} else if (*dest != '\0') {
						// target is deeper in the same tree than the source
						while (*dest) {
							if (*dest == '/')
								source.Prepend("../");
							++dest;
						}
						source.Append(src);
					}

					// else source and target are in the same dir

					source.Append(path.Leaf());
					err = destDir->CreateSymLink(name, source.String(), &link);

					chdir(oldwd);
						// change working dir back to original
				} else 
					moveMode = kCreateLink;
						// fall back to absolute link mode
			}

			if (moveMode == kCreateLink)
				err = destDir->CreateSymLink(name, path.Path(), &link);

			if (err == B_UNSUPPORTED) 
				throw FailWithAlert(err, "The target disk does not support creating links.", NULL);

			FailWithAlert::FailOnError(err, "Error creating link to \"%s\".", ref.name);

			if (loc && loc != (BPoint *)-1) 
				link.WriteAttr(kAttrPoseInfo, B_RAW_TYPE, 0, &poseInfo, sizeof(PoseInfo));

			BNodeInfo nodeInfo(&link);
			nodeInfo.SetType(B_LINK_MIMETYPE);
			return B_OK;
		}

		// if move is on same volume don't copy
		if (statbuf.st_dev == destNode.device && moveMode != kCopySelectionTo
			&& moveMode != kDuplicateSelection) {

			// for "Move" the size for status is always 1 - since file
			// size is irrelevant when simply moving to a new folder

			thread_id thread = find_thread(NULL);
			if (gStatusWindow && gStatusWindow->HasStatus(thread))
				gStatusWindow->UpdateStatus(thread, ref.name, 1);

			MoveError::FailOnError(entry->MoveTo(destDir, newName));
		} else {
			TrackerCopyLoopControl loopControl(find_thread(NULL));

			bool makeOriginalName = (moveMode == kDuplicateSelection);
			if (S_ISDIR(statbuf.st_mode))
				CopyFolder(entry, destDir, &loopControl, loc, makeOriginalName, undo);
			else
				CopyFile(entry, &statbuf, destDir, &loopControl, loc, makeOriginalName, undo);
		}
	} catch (status_t error) {
		// no alert, was already taken care of before
		return error;
	} catch (MoveError error) {
		BString errorString;
		errorString << "Error moving \"" << ref.name << '"';
		(new BAlert("", errorString.String(), "OK", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
		return error.fError;
	} catch (FailWithAlert error) {
		char buffer[256];
		if (error.fName)
			sprintf(buffer, error.fString, error.fName);
		else
			strcpy(buffer, error.fString);
		(new BAlert("", buffer, "OK", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();

		return error.fError;
	}
	
	return B_OK;
}


void
FSDuplicate(BObjectList<entry_ref> *srcList, BList *pointList)
{
	LaunchInNewThread("DupTask", B_NORMAL_PRIORITY, MoveTask, srcList, (BEntry *)NULL,
		pointList, kDuplicateSelection);
}


#if 0
status_t
FSCopyFolder(BEntry *srcEntry, BDirectory *destDir, CopyLoopControl *loopControl,
	BPoint *loc, bool makeOriginalName)
{
	try {
		CopyFolder(srcEntry, destDir, loopControl, loc, makeOriginalName);
	} catch (status_t error) {
		return error;
	}

	return B_OK;
}
#endif


status_t
FSCopyAttributesAndStats(BNode *srcNode, BNode *destNode)
{
	char *buffer = new char[1024];
	
	// copy the attributes
	srcNode->RewindAttrs();
	char name[256];
	while (srcNode->GetNextAttrName(name) == B_OK) {
		attr_info info;
		if (srcNode->GetAttrInfo(name, &info) != B_OK)
			continue;
			
		attr_info dest_info;
		if (destNode->GetAttrInfo(name, &dest_info) == B_OK)
			continue;
			
		ssize_t bytes;
		ssize_t numToRead = (ssize_t)info.size;
		for (off_t offset = 0; numToRead > 0; offset += bytes) {
			size_t chunkSize = (size_t)numToRead;
			if (chunkSize > 1024)
				chunkSize = 1024;

			bytes = srcNode->ReadAttr(name, info.type, offset, buffer, chunkSize);

			if (bytes <= 0) 
				break;
			
			destNode->WriteAttr(name, info.type, offset, buffer, (size_t)bytes);

			numToRead -= bytes;
		}
	}
	delete[] buffer;
		
	// copy the file stats
	struct stat srcStat;
	srcNode->GetStat(&srcStat);
	destNode->SetPermissions(srcStat.st_mode);
	destNode->SetOwner(srcStat.st_uid);
	destNode->SetGroup(srcStat.st_gid);
	destNode->SetModificationTime(srcStat.st_mtime);
	destNode->SetCreationTime(srcStat.st_crtime);
	
	return B_OK;
}


#if 0
status_t
FSCopyFile(BEntry* srcFile, StatStruct *srcStat, BDirectory* destDir,
	CopyLoopControl *loopControl, BPoint *loc, bool makeOriginalName)
{
	try {
		CopyFile(srcFile, srcStat, destDir, loopControl, loc, makeOriginalName);
	} catch (status_t error) {
		return error;
	}

	return B_OK;
}
#endif


static status_t
MoveEntryToTrash(BEntry *entry, BPoint *loc, Undo &undo)
{
	BDirectory trash_dir;
	entry_ref ref;
	status_t result = entry->GetRef(&ref);
	if (result != B_OK)
		return result;

	node_ref nodeRef;
	result = entry->GetNodeRef(&nodeRef);
	if (result != B_OK)
		return result;

	StatStruct statbuf;
	result = entry->GetStat(&statbuf);
	if (entry->GetStat(&statbuf) != B_OK)
		return result;

	// if it's a directory close the window and any child dir windows
	if (S_ISDIR(statbuf.st_mode)) {
		BDirectory dir(entry);

		// if it's a volume, try to unmount
		if (dir.IsRootDirectory()) {
			BVolume	volume(nodeRef.device);
			BVolume	boot;

			BVolumeRoster().GetBootVolume(&boot);
			if (volume == boot) {
				char name[B_FILE_NAME_LENGTH];
				volume.GetName(name);
				char buffer[256];
				sprintf(buffer, "Cannot unmount the boot volume \"%s\".", name);
				(new BAlert("", buffer, "Cancel", 0, 0,
					B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
			} else {
				BMessage message(kUnmountVolume);
				message.AddInt32("device_id", volume.Device());
				be_app->PostMessage(&message);
			}
			return B_OK;
		}

		// get trash directory on same volume as item being moved
		result = FSGetTrashDir(&trash_dir, nodeRef.device);
		if (result != B_OK)
			return result;

		// check hierarchy before moving
		BEntry trashEntry;
		trash_dir.GetEntry(&trashEntry);

		if (dir == trash_dir || dir.Contains(&trashEntry)) {
			(new BAlert("", "You cannot put the Trash, home or Desktop "
				"directory into the trash.", "OK", 0, 0,
					B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();

			// return no error so we don't get two dialogs
			return B_OK;
		}

		BMessage message(kCloseWindowAndChildren);

		node_ref parentNode;
		parentNode.device = statbuf.st_dev;
		parentNode.node = statbuf.st_ino;
		message.AddData("node_ref", B_RAW_TYPE, &parentNode, sizeof(node_ref));
		be_app->PostMessage(&message);
	} else {
		// get trash directory on same volume as item being moved
		result = FSGetTrashDir(&trash_dir, nodeRef.device);
		if (result != B_OK)
			return result;
	}

	// make sure name doesn't conflict with anything in trash already
	char name[B_FILE_NAME_LENGTH];
	strcpy(name, ref.name);
	if (trash_dir.Contains(name)) {
		FSMakeOriginalName(name, &trash_dir, " copy");
		undo.UpdateEntry(entry, name);
	}

	BNode *src_node = 0;
	if (loc && loc != (BPoint *)-1
		&& (src_node = GetWritableNode(entry, &statbuf)) != 0) {
		trash_dir.GetStat(&statbuf);
		PoseInfo poseInfo;
		poseInfo.fInvisible = false;
		poseInfo.fInitedDirectory = statbuf.st_ino;
		poseInfo.fLocation = *loc;
		src_node->WriteAttr(kAttrPoseInfo, B_RAW_TYPE, 0, &poseInfo,
			sizeof(poseInfo));
		delete src_node;
	}

	BNode node(entry);
	BPath path;
	// Get path of entry before it's moved to the trash
	// and write it to the file as an attribute
	if (node.InitCheck() == B_OK && entry->GetPath(&path) == B_OK) {
		BString originalPath(path.Path());
		node.WriteAttrString(kAttrOriginalPath, &originalPath);
	}

	MoveItem(entry, &trash_dir, loc, kMoveSelectionTo, name, undo);
	return B_OK;
}


ConflictCheckResult
PreFlightNameCheck(BObjectList<entry_ref> *srcList, const BDirectory *destDir,
	int32 *collisionCount)
{

	// count the number of name collisions in dest folder
	*collisionCount = 0;
	
	int32 count = srcList->CountItems();
	for (int32 i = 0; i < count; i++) {
		entry_ref *srcRef = srcList->ItemAt(i);
		BEntry entry(srcRef);
		BDirectory parent;
		entry.GetParent(&parent);

		if (parent != *destDir) {
			if (destDir->Contains(srcRef->name))
				(*collisionCount)++;
		}
	}

	// prompt user only if there is more than one collision, otherwise the
	// single collision case will be handled as a "Prompt" case by CheckName
	if (*collisionCount > 1) {
		entry_ref *srcRef = (entry_ref*)srcList->FirstItem();

		StatStruct statbuf;
		destDir->GetStat(&statbuf);
		
		const char *verb = (srcRef->device == statbuf.st_dev) ? "moving" : "copying";
		char replaceMsg[256];
		sprintf(replaceMsg, kReplaceManyStr, verb, verb);
		
		switch ((new BAlert("", replaceMsg, "Cancel", "Prompt", "Replace All"))->Go()) {
			case 0:
				return kCanceled;
			
			case 1:
				// user selected "Prompt"
				return kPrompt;

			case 2:
				// user selected "Replace All"
				return kReplaceAll;
		}
	}

	return kNoConflicts;
}


void
FileStatToString(StatStruct *stat, char *buffer, int32 length)
{
	tm timeData;			
	localtime_r(&stat->st_mtime, &timeData);
				
	sprintf(buffer, "\n\t(%Ld bytes, ", stat->st_size);
	uint32 pos = strlen(buffer);
	strftime(buffer + pos, length - pos,"%b %d %Y, %I:%M:%S %p)", &timeData);
}


status_t
CheckName(uint32 moveMode, const BEntry *sourceEntry, const BDirectory *destDir,
	bool multipleCollisions, ConflictCheckResult &replaceAll)
{
	if (moveMode == kDuplicateSelection)
		// when duplicating, we will never have a conflict
		return B_OK;

	// see if item already exists in destination dir
	status_t err = B_OK;
	char name[B_FILE_NAME_LENGTH];
	sourceEntry->GetName(name);
	bool sourceIsDirectory = sourceEntry->IsDirectory();

	BDirectory srcDirectory;
	if (sourceIsDirectory) {
		srcDirectory.SetTo(sourceEntry);
		BEntry destEntry;
		destDir->GetEntry(&destEntry);

		if (moveMode != kCreateLink
			&& moveMode != kCreateRelativeLink
			&& (srcDirectory == *destDir || srcDirectory.Contains(&destEntry))) {
			(new BAlert("", "You can't move a folder into itself "
				"or any of its own sub-folders.", "OK", 0, 0,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
			return B_ERROR;
		}
	}

	if (FSIsTrashDir(sourceEntry)) {
		(new BAlert("", "You can't move or copy the trash.",
			"OK", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
		return B_ERROR;
	}

	BEntry entry;
	if (destDir->FindEntry(name, &entry) != B_OK)
		// no conflict, return
		return B_OK;

	if (moveMode == kCreateLink	|| moveMode == kCreateRelativeLink) {
		// if we are creating link in the same directory, the conflict will
		// be handled later by giving the link a unique name
		sourceEntry->GetParent(&srcDirectory);
		
		if (srcDirectory == *destDir)
			return B_OK;
	}

	bool destIsDir = entry.IsDirectory();
	// be sure not to replace the parent directory of the item being moved
	if (destIsDir) {
		BDirectory test_dir(&entry);
		if (test_dir.Contains(sourceEntry)) {
			(new BAlert("", "You can't replace a folder "
				"with one of its sub-folders.", "OK", 0, 0,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
			return B_ERROR;
		}
	}

	if (moveMode != kCreateLink
		&& moveMode != kCreateRelativeLink
		&& destIsDir != sourceIsDirectory) {
		// ensure user isn't trying to replace a file with folder or vice versa
			(new BAlert("", sourceIsDirectory
				? "You cannot replace a file with a folder or a symbolic link."
				: "You cannot replace a folder or a symbolic link with a file.",
				"OK", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
			return B_ERROR;
		}

	if (replaceAll != kReplaceAll) {
		// prompt user to determine whether to replace or not

		char replaceMsg[512];
		
		if (moveMode == kCreateLink || moveMode == kCreateRelativeLink)
			sprintf(replaceMsg, kSymLinkReplaceStr, name);
		else if (sourceEntry->IsDirectory())
			sprintf(replaceMsg, kDirectoryReplaceStr, name,
				moveMode == kMoveSelectionTo ? "moving" : "copying");
		else {
			char sourceBuffer[96], destBuffer[96];
			StatStruct statBuffer;
			
			if (!sourceEntry->IsDirectory() && sourceEntry->GetStat(&statBuffer) == B_OK)
				FileStatToString(&statBuffer, sourceBuffer, 96);
			else
				sourceBuffer[0] = '\0';
			
			if (!entry.IsDirectory() && entry.GetStat(&statBuffer) == B_OK)
				FileStatToString(&statBuffer, destBuffer, 96);
			else
				destBuffer[0] = '\0';
			
			sprintf(replaceMsg, kReplaceStr, name, destBuffer, name, sourceBuffer,
				moveMode == kMoveSelectionTo ? "moving" : "copying");
		}

		// special case single collision (don't need Replace All shortcut)
		BAlert *alert;
		if (multipleCollisions)
			alert = new BAlert("", replaceMsg, "Skip", "Replace All",
				"Replace");
		else
			alert = new BAlert("", replaceMsg, "Cancel", "Replace");

		switch (alert->Go()) {
			case 0:		// user selected "Cancel" or "Skip"
				replaceAll = kCanceled;
				return B_ERROR;

			case 1:		// user selected "Replace" or "Replace All"
				replaceAll = kReplaceAll;
					// doesn't matter which since a single
					// collision "Replace" is equivalent to a
					// "Replace All"
				break;
		}
	}

	// delete destination item
	if (destIsDir) {
		TrackerCopyLoopControl loopControl(find_thread(NULL));
		err = FSDeleteFolder(&entry, &loopControl, false);
	} else
		err = entry.Remove();

	if (err != B_OK) {
		BString error;
		error << "There was a problem trying to replace \""
			<< name << "\". The item might be open or busy.";
		(new BAlert("", error.String(), "Cancel", 0, 0,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
	}

	return err;
}


status_t
FSDeleteFolder(BEntry *dir_entry, CopyLoopControl *loopControl, bool update_status, 
	bool delete_top_dir, bool upateFileNameInStatus)
{
	entry_ref	ref;
	BEntry		entry;
	BDirectory	dir;
	status_t	err;

	dir.SetTo(dir_entry);
	dir.Rewind();

	// loop through everything in folder and delete it, skipping trouble files
	for (;;) {
		if (dir.GetNextEntry(&entry) != B_OK)
			break;

		entry.GetRef(&ref);

		if (loopControl->CheckUserCanceled())
			return kTrashCanceled;

		if (entry.IsDirectory())
			err = FSDeleteFolder(&entry, loopControl, update_status, true,
				upateFileNameInStatus);
		else {
			err = entry.Remove();
			if (update_status)
				loopControl->UpdateStatus(upateFileNameInStatus ? ref.name : "", ref, 1, true);
		}

		if (err == kTrashCanceled)
			return kTrashCanceled;
		else if (err == B_OK)
			dir.Rewind();
		else 
			loopControl->FileError(kFileDeleteErrorString, ref.name, err, false);
	}

	if (loopControl->CheckUserCanceled())
		return kTrashCanceled;

	dir_entry->GetRef(&ref);

	if (update_status && delete_top_dir)
		loopControl->UpdateStatus(NULL, ref, 1);

	if (delete_top_dir)
		return dir_entry->Remove();
	else
		return B_OK;
}


void 
FSMakeOriginalName(BString &string, const BDirectory *destDir, const char *suffix)
{
	if (!destDir->Contains(string.String()))
		return;

	FSMakeOriginalName(string.LockBuffer(B_FILE_NAME_LENGTH),
		const_cast<BDirectory *>(destDir), suffix ? suffix : " copy");
	string.UnlockBuffer();
}


void
FSMakeOriginalName(char *name, BDirectory *destDir, const char *suffix)
{
	char		root[B_FILE_NAME_LENGTH];
	char		copybase[B_FILE_NAME_LENGTH];
	char		temp_name[B_FILE_NAME_LENGTH + 10];
	int32		fnum;

	// is this name already original?
	if (!destDir->Contains(name))
		return;

	// Determine if we're copying a 'copy'. This algorithm isn't perfect.
	// If you're copying a file whose REAL name ends with 'copy' then
	// this method will return "<filename> 1", not "<filename> copy"

	// However, it will correctly handle file that contain 'copy' 
	// elsewhere in their name.

	bool copycopy = false;		// are we copying a copy?
	int32 len = (int32)strlen(name);
	char *p = name + len - 1;	// get pointer to end os name

	// eat up optional numbers (if were copying "<filename> copy 34")
	while ((p > name) && isdigit(*p))
		p--;
	
	// eat up optional spaces
	while ((p > name) && isspace(*p))
		p--;

	// now look for the phrase " copy"
	if (p > name) {
		// p points to the last char of the word. For example, 'y' in 'copy'

		if ((p - 4 > name) && (strncmp(p - 4, suffix, 5) == 0)) {
			// we found 'copy' in the right place. 
			// so truncate after 'copy'
			*(p + 1) = '\0';
			copycopy = true;

			// save the 'root' name of the file, for possible later use.
			// that is copy everything but trailing " copy". Need to
			// NULL terminate after copy
			strncpy(root, name, (uint32)((p - name) - 4));
			root[(p - name) - 4] = '\0';
		}
	}

	if (!copycopy) {
		/*
		 The name can't be longer than B_FILE_NAME_LENGTH.
		 The algoritm adds " copy XX" to the name. That's 8 characters.
		 B_FILE_NAME_LENGTH already accounts for NULL termination so we
		 don't need to save an extra char at the end.
		*/
		if (strlen(name) > B_FILE_NAME_LENGTH - 8) {
			// name is too long - truncate it!
			name[B_FILE_NAME_LENGTH - 8] = '\0';
		}

		strcpy(root, name);		// save root name
		strcat(name, suffix);
	}

	strcpy(copybase, name);

	// if name already exists then add a number
	fnum = 1;
	strcpy(temp_name, name);
	while (destDir->Contains(temp_name)) {
		sprintf(temp_name, "%s %ld", copybase, ++fnum);

		if (strlen(temp_name) > (B_FILE_NAME_LENGTH - 1)) {
			/*
			 The name has grown too long. Maybe we just went from
			 "<filename> copy 9" to "<filename> copy 10" and that extra
			 character was too much. The solution is to further
			 truncate the 'root' name and continue.
			 ??? should we reset fnum or not ???
			*/
			root[strlen(root) - 1] = '\0';
			sprintf(temp_name, "%s%s %ld", root, suffix, fnum);
		}
	}
	
	ASSERT((strlen(temp_name) <= (B_FILE_NAME_LENGTH - 1)));
	strcpy(name, temp_name);
}


status_t
FSRecursiveCalcSize(BInfoWindow *wind, BDirectory *dir, off_t *running_size,
	int32 *fileCount, int32 *dirCount)
{
	thread_id tid = find_thread(NULL);

	dir->Rewind();
	BEntry entry;
	while (dir->GetNextEntry(&entry) == B_OK) {

		// be sure window hasn't closed
		if (wind && wind->StopCalc())
			return B_OK;

		if (gStatusWindow && gStatusWindow->CheckCanceledOrPaused(tid))
			return kUserCanceled;

		StatStruct statbuf;
		entry.GetStat(&statbuf);

		if (S_ISDIR(statbuf.st_mode)) {
			BDirectory subdir(&entry);
			(*dirCount)++;
			(*running_size) += 1024;
			status_t result;
			if ((result = FSRecursiveCalcSize(wind, &subdir, running_size,
				fileCount, dirCount)) != B_OK)
				return result;
		} else {
			(*fileCount)++;
			(*running_size) += statbuf.st_size + 1024;	// Add to compensate
														// for attributes.
		}
	}
	return B_OK;
}


status_t
CalcItemsAndSize(BObjectList<entry_ref> *refList, int32 *totalCount, off_t *totalSize)
{
	int32 fileCount = 0;
	int32 dirCount = 0;

	thread_id tid = find_thread(NULL);

	int32 num_items = refList->CountItems();
	for (int32 i = 0; i < num_items; i++) {
		entry_ref *ref = refList->ItemAt(i);
		BEntry entry(ref);
		StatStruct statbuf;
		entry.GetStat(&statbuf);

		if (gStatusWindow && gStatusWindow->CheckCanceledOrPaused(tid))
			return kUserCanceled;

		if (S_ISDIR(statbuf.st_mode)) {
			BDirectory dir(&entry);
			dirCount++;
			(*totalSize) += 1024;
			status_t result;
			if ((result = FSRecursiveCalcSize(NULL, &dir, totalSize, &fileCount,
				&dirCount)) != B_OK)
				return result;
		} else {
			fileCount++;
			(*totalSize) += statbuf.st_size + 1024;
		}
	}

	*totalCount += (fileCount + dirCount);
	return B_OK;
}


status_t
FSGetTrashDir(BDirectory *trash_dir, dev_t dev)
{

	BVolume volume(dev);
	status_t result = volume.InitCheck();
	if (result != B_OK)
		return result;
	
	BPath path;
	result = find_directory(B_TRASH_DIRECTORY, &path, true, &volume);
	if (result != B_OK)
		return result;
	
	result = trash_dir->SetTo(path.Path());
	if (result != B_OK)
		return result;

	// make trash invisible
	attr_info a_info;
	if (trash_dir->GetAttrInfo(kAttrPoseInfo, &a_info) != B_OK) {

		StatStruct sbuf;
		trash_dir->GetStat(&sbuf);

		// move trash to bottom left of main screen initially
		BScreen screen(B_MAIN_SCREEN_ID);
		BRect scrn_frame = screen.Frame();

		PoseInfo poseInfo;
		poseInfo.fInvisible = false;
		poseInfo.fInitedDirectory = sbuf.st_ino;
		poseInfo.fLocation = BPoint(scrn_frame.left + 20, scrn_frame.bottom - 60);
		trash_dir->WriteAttr(kAttrPoseInfo, B_RAW_TYPE, 0, &poseInfo,
			sizeof(PoseInfo));
	}

	return B_OK;
}


status_t
FSGetDeskDir(BDirectory *deskDir, dev_t dev)
{
	BVolume volume(dev);
	status_t result = volume.InitCheck();
	if (result != B_OK)
		return result;
		
	BPath path;
	result = find_directory(B_DESKTOP_DIRECTORY, &path, true, &volume);
	if (result != B_OK)
		return result;

	result = deskDir->SetTo(path.Path());
	if (result != B_OK)
		return result;

	// make desktop fInvisible
	PoseInfo poseInfo;
	poseInfo.fInvisible = true;
	poseInfo.fInitedDirectory = -1LL;
	deskDir->WriteAttr(kAttrPoseInfo, B_RAW_TYPE, 0, &poseInfo, sizeof(PoseInfo));

	size_t size;
	const void* data = GetTrackerResources()->
		LoadResource('ICON', kResDeskIcon, &size);

	if (data)
		deskDir->WriteAttr(kAttrLargeIcon, B_COLOR_8_BIT_TYPE, 0, data, size);

	data = GetTrackerResources()->
		LoadResource('MICN', kResDeskIcon, &size);

	if (data)
		deskDir->WriteAttr(kAttrMiniIcon, B_COLOR_8_BIT_TYPE, 0, data, size);
	
	return B_OK;
}


status_t
FSGetBootDeskDir(BDirectory *deskDir)
{
	BVolume	bootVol;
	BVolumeRoster().GetBootVolume(&bootVol);
	BPath path;

	status_t result = find_directory(B_DESKTOP_DIRECTORY, &path, true, &bootVol);
	if (result != B_OK)
		return result;

	return deskDir->SetTo(path.Path());
}


static bool
FSIsDirFlavor(const BEntry *entry, directory_which directoryType)
{
	StatStruct dir_stat;
	StatStruct entry_stat;
	BVolume volume;
	BPath path;

	if (entry->GetStat(&entry_stat) != B_OK)
		return false;

	if (volume.SetTo(entry_stat.st_dev) != B_OK)
		return false;

	if (find_directory(directoryType, &path, false, &volume) != B_OK)
		return false;

	stat(path.Path(), &dir_stat);

	return dir_stat.st_ino == entry_stat.st_ino
		&& dir_stat.st_dev == entry_stat.st_dev;
}


bool
FSIsPrintersDir(const BEntry *entry)
{
	return FSIsDirFlavor(entry, B_USER_PRINTERS_DIRECTORY);
}


bool
FSIsTrashDir(const BEntry *entry)
{
	return FSIsDirFlavor(entry, B_TRASH_DIRECTORY);
}


bool 
FSIsDeskDir(const BEntry *entry, dev_t device)
{
	BVolume volume(device);
	status_t result = volume.InitCheck();
	if (result != B_OK)
		return false;
		
	BPath path;
	result = find_directory(B_DESKTOP_DIRECTORY, &path, true, &volume);
	if (result != B_OK)
		return false;

	BEntry entryToCompare(path.Path());
	return entryToCompare == *entry;
}


bool
FSIsDeskDir(const BEntry *entry)
{
	entry_ref ref;
	if (entry->GetRef(&ref) != B_OK)
		return false;
	
	return FSIsDeskDir(entry, ref.device);
}


bool
FSIsSystemDir(const BEntry *entry)
{
	return FSIsDirFlavor(entry, B_BEOS_SYSTEM_DIRECTORY);
}


bool
FSIsBeOSDir(const BEntry *entry)
{
	return FSIsDirFlavor(entry, B_BEOS_DIRECTORY);
}


bool
FSIsHomeDir(const BEntry *entry)
{
	return FSIsDirFlavor(entry, B_USER_DIRECTORY);
}


bool 
DirectoryMatchesOrContains(const BEntry *entry, directory_which which)
{
	BPath path;
	if (find_directory(which, &path, false, NULL) != B_OK)
		return false;
	
	BEntry dirEntry(path.Path());
	if (dirEntry.InitCheck() != B_OK)
		return false;
	
	if (dirEntry == *entry)
		// root level match
		return true;
	
	BDirectory dir(&dirEntry);
	return dir.Contains(entry);
}


bool 
DirectoryMatchesOrContains(const BEntry *entry, const char *additionalPath,
	directory_which which)
{
	BPath path;
	if (find_directory(which, &path, false, NULL) != B_OK)
		return false;
	
	path.Append(additionalPath);
	BEntry dirEntry(path.Path());
	if (dirEntry.InitCheck() != B_OK)
		return false;
	
	if (dirEntry == *entry)
		// root level match
		return true;
	
	BDirectory dir(&dirEntry);
	return dir.Contains(entry);
}


bool 
DirectoryMatches(const BEntry *entry, directory_which which)
{
	BPath path;
	if (find_directory(which, &path, false, NULL) != B_OK)
		return false;
	
	BEntry dirEntry(path.Path());
	if (dirEntry.InitCheck() != B_OK)
		return false;
	
	return dirEntry == *entry;
}


bool 
DirectoryMatches(const BEntry *entry, const char *additionalPath, directory_which which)
{
	BPath path;
	if (find_directory(which, &path, false, NULL) != B_OK)
		return false;
	
	path.Append(additionalPath);
	BEntry dirEntry(path.Path());
	if (dirEntry.InitCheck() != B_OK)
		return false;
	
	return dirEntry == *entry;
}


extern status_t 
FSFindTrackerSettingsDir(BPath *path, bool autoCreate)
{
	status_t result = find_directory (B_USER_SETTINGS_DIRECTORY, path, autoCreate);
	if (result != B_OK)
		return result;
		
	path->Append("Tracker");

	return mkdir(path->Path(), 0777) ? B_OK : errno;
}


bool 
FSInTrashDir(const entry_ref *ref)
{
	BEntry entry(ref);
	if (entry.InitCheck() != B_OK)
		return false;

	BDirectory trashDir;
	if (FSGetTrashDir(&trashDir, ref->device) != B_OK)
		return false;
	
	return trashDir.Contains(&entry);
}


void
FSEmptyTrash()
{
	if (find_thread("_tracker_empty_trash_") == B_NAME_NOT_FOUND)
		resume_thread(spawn_thread(empty_trash, "_tracker_empty_trash_",
			B_NORMAL_PRIORITY, NULL));
}


status_t
empty_trash(void *)
{
	BVolumeRoster	roster;
	BVolume			volume;
	BEntry			entry;
	BDirectory		trash_dir;
	entry_ref		ref;
	thread_id		tid;
	status_t		err;
	int32			totalCount;
	off_t			totalSize;
	BObjectList<entry_ref>	srcList;

	// empty trash on all mounted volumes
	err = B_OK;

	tid = find_thread(NULL);
	if (gStatusWindow)
		gStatusWindow->CreateStatusItem(tid, kTrashState);

	// calculate the sum total of all items on all volumes in trash
	totalCount = 0;
	totalSize = 0;

	roster.Rewind();
	while (roster.GetNextVolume(&volume) == B_OK) {
		
		if (volume.IsReadOnly() || !volume.IsPersistent())
			continue;

		if (FSGetTrashDir(&trash_dir, volume.Device()) != B_OK)
			continue;

		trash_dir.GetEntry(&entry);
		entry.GetRef(&ref);
		srcList.AddItem(&ref);
		err = CalcItemsAndSize(&srcList, &totalCount, &totalSize);
		if (err != B_OK)
			break;

		srcList.MakeEmpty();

		// don't count trash directory itself
		totalCount--;
	}

	if (err == B_OK) {
		if (gStatusWindow)
			gStatusWindow->InitStatusItem(tid, totalCount, totalCount);
	
		roster.Rewind();
		while (roster.GetNextVolume(&volume) == B_OK) {
			TrackerCopyLoopControl loopControl(tid);
			
			if (volume.IsReadOnly() || !volume.IsPersistent())
				continue;
	
			if (FSGetTrashDir(&trash_dir, volume.Device()) != B_OK)
				continue;
	
			trash_dir.GetEntry(&entry);
			err = FSDeleteFolder(&entry, &loopControl, true, false);
		}
	}

	if (err != B_OK && err != kTrashCanceled && err != kUserCanceled) 
		(new BAlert("", "Error emptying Trash!", "OK", NULL, NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();

	if (gStatusWindow)
		gStatusWindow->RemoveStatusItem(find_thread(NULL));

	return B_OK;
}


status_t
_DeleteTask(BObjectList<entry_ref> *list, bool confirm)
{
	if (confirm) {
		bool dontMoveToTrash = TrackerSettings().DontMoveFilesToTrash();

		if (!dontMoveToTrash) {
			BAlert *alert = new BAlert("", kDeleteConfirmationStr,
				"Cancel", "Move to Trash", "Delete", B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
				B_WARNING_ALERT);
				
			alert->SetShortcut(0, B_ESCAPE);
			alert->SetShortcut(1, 'm');
			alert->SetShortcut(2, 'd');

			switch (alert->Go()) {
				case 0:
					delete list;
					return B_OK;
				case 1:
					FSMoveToTrash(list, NULL, false);
					return B_OK;
			}
		} else {
			BAlert *alert = new BAlert("", kDeleteConfirmationStr,
				"Cancel", "Delete", NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
				B_WARNING_ALERT);
			
			alert->SetShortcut(0, B_ESCAPE);
			alert->SetShortcut(1, 'd');

			if (!alert->Go()) {
				delete list;
				return B_OK;
			}
		}
	}

	thread_id thread = find_thread(NULL);
	if (gStatusWindow)
		gStatusWindow->CreateStatusItem(thread, kDeleteState);

	// calculate the sum total of all items on all volumes in trash
	int32 totalItems = 0;
	int64 totalSize = 0;

	status_t err = CalcItemsAndSize(list, &totalItems, &totalSize);
	if (err == B_OK) {
		if (gStatusWindow)
			gStatusWindow->InitStatusItem(thread, totalItems, totalItems);
		
		int32 count = list->CountItems();
		TrackerCopyLoopControl loopControl(thread);
		for (int32 index = 0; index < count; index++) {
			entry_ref ref(*list->ItemAt(index));
			BEntry entry(&ref);
			loopControl.UpdateStatus(ref.name, ref, 1, true);
			if (entry.IsDirectory())
				err = FSDeleteFolder(&entry, &loopControl, true, true, true);
			else 
				err = entry.Remove();
		}

		if (err != kTrashCanceled && err != kUserCanceled && err != B_OK) 
			(new BAlert("", "Error Deleting items", "OK", NULL, NULL,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
	}
	if (gStatusWindow)
		gStatusWindow->RemoveStatusItem(find_thread(NULL));

	delete list;

	return B_OK;
}

status_t
FSRecursiveCreateFolder(BPath path)
{
	BEntry entry(path.Path());
	if (entry.InitCheck() != B_OK) {
		BPath parentPath;
		status_t err = path.GetParent(&parentPath);
		if (err != B_OK)
			return err;

		err = FSRecursiveCreateFolder(parentPath);
		if (err != B_OK)
			return err;
	}
	
	entry.SetTo(path.Path());
	if (entry.Exists())
		return B_FILE_EXISTS;
	else {
		char name[B_FILE_NAME_LENGTH];
		BDirectory parent;

		entry.GetParent(&parent);
		entry.GetName(name);
		parent.CreateDirectory(name, NULL);
	}

	return B_OK;
}

status_t
_RestoreTask(BObjectList<entry_ref> *list)
{
	thread_id thread = find_thread(NULL);
	if (gStatusWindow)
		gStatusWindow->CreateStatusItem(thread, kRestoreFromTrashState);

	// calculate the sum total of all items that will be restored
	int32 totalItems = 0;
	int64 totalSize = 0;

	status_t err = CalcItemsAndSize(list, &totalItems, &totalSize);
	if (err == B_OK) {
		if (gStatusWindow)
			gStatusWindow->InitStatusItem(thread, totalItems, totalItems);

		int32 count = list->CountItems();
		TrackerCopyLoopControl loopControl(thread);
		for (int32 index = 0; index < count; index++) {
			entry_ref ref(*list->ItemAt(index));
			BEntry entry(&ref);
			BPath originalPath;

			loopControl.UpdateStatus(ref.name, ref, 1, true);

			if (FSGetOriginalPath(&entry, &originalPath) != B_OK)
				continue;

			BEntry originalEntry(originalPath.Path());
			BPath parentPath;
			err = originalPath.GetParent(&parentPath);
			if (err != B_OK)
				continue;
			BEntry parentEntry(parentPath.Path());

			if (parentEntry.InitCheck() != B_OK || !parentEntry.Exists()) {
				if (FSRecursiveCreateFolder(parentPath) == B_OK) {
					originalEntry.SetTo(originalPath.Path());
					if (entry.InitCheck() != B_OK)
						continue;
				}
			}
	
			if (!originalEntry.Exists()) {
				BDirectory dir(parentPath.Path());
				if (dir.InitCheck() == B_OK) {
					char leafName[B_FILE_NAME_LENGTH];
					originalEntry.GetName(leafName);
					if (entry.MoveTo(&dir, leafName) == B_OK) {
						BNode node(&entry);
						if (node.InitCheck() == B_OK)
							node.RemoveAttr(kAttrOriginalPath);
					}
				}
			}

			err = loopControl.CheckUserCanceled();
			if (err != B_OK)
				break;
		}
	}
	if (gStatusWindow)
		gStatusWindow->RemoveStatusItem(find_thread(NULL));

	delete list;

	return err;
}

void
FSCreateTrashDirs()
{
	BVolume volume;
	BVolumeRoster roster;

	roster.Rewind();
	while (roster.GetNextVolume(&volume) == B_OK) {
		
		if (volume.IsReadOnly() || !volume.IsPersistent())
			continue;
		
		BPath path;
		find_directory(B_DESKTOP_DIRECTORY, &path, true, &volume);
		find_directory(B_TRASH_DIRECTORY, &path, true, &volume);

		BDirectory trashDir;
		if (FSGetTrashDir(&trashDir, volume.Device()) == B_OK) {
			size_t size;
			const void* data = GetTrackerResources()->
				LoadResource('ICON', kResTrashIcon, &size);
			if (data) {
				trashDir.WriteAttr(kAttrLargeIcon, B_COLOR_8_BIT_TYPE, 0,
					data, size);
			}
			data = GetTrackerResources()->
				LoadResource('MICN', kResTrashIcon, &size);
			if (data) {
				trashDir.WriteAttr(kAttrMiniIcon, B_COLOR_8_BIT_TYPE, 0,
					data, size);
			}
		}
	}
}


status_t
FSCreateNewFolder(const entry_ref *ref)
{
	node_ref node;
	node.device = ref->device;
	node.node = ref->directory;

	BDirectory dir(&node);
	status_t result = dir.InitCheck();
	if (result != B_OK)
		return result;

	// ToDo: is that really necessary here?
	BString name(ref->name);
	FSMakeOriginalName(name, &dir, "-");

	BDirectory newDir;
	result = dir.CreateDirectory(name.String(), &newDir);
	if (result != B_OK)
		return result;

	BNodeInfo nodeInfo(&newDir);
	nodeInfo.SetType(B_DIR_MIMETYPE);

	return result;
}


status_t
FSCreateNewFolderIn(const node_ref *dirNode, entry_ref *newRef,
	node_ref *newNode)
{
	BDirectory dir(dirNode);
	status_t result = dir.InitCheck();
	if (result == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		strcpy(name, "New Folder");

		int32 fnum = 1;
		while (dir.Contains(name)) {
			// if base name already exists then add a number
			// ToDo:
			// move this logic ot FSMakeOriginalName
			if (++fnum > 9)
				sprintf(name, "New Folder%ld", fnum);
			else
				sprintf(name, "New Folder %ld", fnum);
		}

		BDirectory newDir;
		result = dir.CreateDirectory(name, &newDir);
		if (result == B_OK) {
			BEntry entry;
			newDir.GetEntry(&entry);
			entry.GetRef(newRef);
			entry.GetNodeRef(newNode);

			BNodeInfo nodeInfo(&newDir);
			nodeInfo.SetType(B_DIR_MIMETYPE);

			// add undo item
			NewFolderUndo undo(*newRef);
			return B_OK;
		}
	}

	(new BAlert("", "Sorry, could not create a new folder.", "Cancel", 0, 0,
		B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
	return result;
}


ReadAttrResult
ReadAttr(const BNode *node, const char *hostAttrName, const char *foreignAttrName,
	type_code type, off_t offset, void *buffer, size_t length,
	void (*swapFunc)(void *), bool isForeign)
{
	if (!isForeign && node->ReadAttr(hostAttrName, type, offset, buffer, length) == (ssize_t)length)
		return kReadAttrNativeOK;

	// PRINT(("trying %s\n", foreignAttrName));
	// try the other endianness	
	if (node->ReadAttr(foreignAttrName, type, offset, buffer, length) != (ssize_t)length)
		return kReadAttrFailed;

	// PRINT(("got %s\n", foreignAttrName));
	if (!swapFunc)
		return kReadAttrForeignOK;

	(swapFunc)(buffer);
		// run the endian swapper

	return kReadAttrForeignOK;
}


ReadAttrResult 
GetAttrInfo(const BNode *node, const char *hostAttrName, const char *foreignAttrName,
	type_code *type, size_t *size)
{
	attr_info info;
	
	if (node->GetAttrInfo(hostAttrName, &info) == B_OK) {
		if (type)
			*type = info.type;
		if (size)
			*size = (size_t)info.size;

		return kReadAttrNativeOK;
	}

	if (node->GetAttrInfo(foreignAttrName, &info) == B_OK) {
		if (type)
			*type = info.type;
		if (size)
			*size = (size_t)info.size;

		return kReadAttrForeignOK;
	}
	return kReadAttrFailed;
}

// launching code

static status_t
TrackerOpenWith(const BMessage *refs)
{
	BMessage clone(*refs);
	ASSERT(dynamic_cast<TTracker *>(be_app));
	ASSERT(clone.what);
	clone.AddInt32("launchUsingSelector", 0);
	// runs the Open With window
	be_app->PostMessage(&clone);

	return B_OK;
}

static void 
AsynchLaunchBinder(void (*func)(const entry_ref *, const BMessage *, bool on),
	const entry_ref *entry, const BMessage *message, bool on)
{
	Thread::Launch(NewFunctionObject(func, entry, message, on),
		B_NORMAL_PRIORITY, "LaunchTask");
}

static bool
SniffIfGeneric(const entry_ref *ref)
{
	BNode node(ref);
	char type[B_MIME_TYPE_LENGTH];
	BNodeInfo info(&node);
	if (info.GetType(type) == B_OK && strcasecmp(type, B_FILE_MIME_TYPE) != 0)
		// already has a type and it's not octet stream
		return false;
	
	BPath path(ref);
	if (path.Path()) {
		// force a mimeset
		node.RemoveAttr(kAttrMIMEType);
		update_mime_info(path.Path(), 0, 1, 1);
	}
	
	return true;
}

static void
SniffIfGeneric(const BMessage *refs)
{
	entry_ref ref;
	for (int32 index = 0; ; index++) {
		if (refs->FindRef("refs", index, &ref) != B_OK)
			break;
		SniffIfGeneric(&ref);
	}
}

static void
_TrackerLaunchAppWithDocuments(const entry_ref *appRef, const BMessage *refs, bool openWithOK)
{
	team_id team;

	status_t error = B_ERROR;
	BString alertString;

	for (int32 mimesetIt = 0; ; mimesetIt++) {
		error = be_roster->Launch(appRef, refs, &team);
		if (error == B_ALREADY_RUNNING)
			// app already running, not really an error
			error = B_OK;
		
		if (error == B_OK)
			break;

		if (mimesetIt > 0)
			break;
		
		// failed to open, try mimesetting the refs and launching again
		SniffIfGeneric(refs);
	}
	
	if (error == B_OK) {
		// close possible parent window, if specified
		const node_ref *nodeToClose = 0;
		int32 numBytes;
		refs->FindData("nodeRefsToClose", B_RAW_TYPE, (const void **)&nodeToClose, &numBytes);
		if (nodeToClose)
			dynamic_cast<TTracker *>(be_app)->CloseParent(*nodeToClose);
	} else {
		alertString << "Could not open \"" << appRef->name << "\" (" << strerror(error) << "). ";
		if (refs && openWithOK) {
			alertString << kFindAlternativeStr;
			if ((new BAlert("", alertString.String(), "Cancel", "Find", 0,
					B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go() == 1)
				error = TrackerOpenWith(refs);
		} else
			(new BAlert("", alertString.String(), "Cancel", 0, 0,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
	}
}

extern "C" char** environ;
extern "C"
#if !B_BEOS_VERSION_DANO
_IMPEXP_ROOT
#endif
status_t _kload_image_etc_(int argc, char **argv, char **envp,
	char *buf, int bufsize);


static status_t
LoaderErrorDetails(const entry_ref *app, BString &details)
{
	BPath path;
	BEntry appEntry(app, true);
	status_t result = appEntry.GetPath(&path);
	
	if (result != B_OK)
		return result;
	
	char *argv[2] = { const_cast<char *>(path.Path()), 0};

#ifdef __HAIKU__
	// ToDo: do this correctly!
	result = load_image(1, (const char **)argv, (const char **)environ);
	details.SetTo("ToDo: this is missing from Haiku");
#else
	result = _kload_image_etc_(1, argv, environ, details.LockBuffer(1024), 1024);
	details.UnlockBuffer();
#endif
	return B_OK;
}


static void
_TrackerLaunchDocuments(const entry_ref */*doNotUse*/, const BMessage *refs,
	bool openWithOK)
{
	BMessage copyOfRefs(*refs);

	entry_ref documentRef;
	if (copyOfRefs.FindRef("refs", &documentRef) != B_OK)
		// nothing to launch, we are done
		return;

	status_t error = B_ERROR;
	entry_ref app;
	BMessage *refsToPass = NULL;
	BString alertString;
	const char *alternative = 0;

	for (int32 mimesetIt = 0; ; mimesetIt++) {
		alertString = "";
		error = be_roster->FindApp(&documentRef, &app);

		if (error != B_OK && mimesetIt == 0) {
			SniffIfGeneric(&copyOfRefs);
			continue;
		}

		if (error != B_OK) {
			alertString << "Could not find an application to open \"" << documentRef.name
				<< "\" (" << strerror(error) << "). ";
			if (openWithOK)
				alternative = kFindApplicationStr;

			break;
		} else {
			BEntry appEntry(&app, true);
			for (int32 index = 0;;) {
				// remove the app itself from the refs received so we don't try
				// to open ourselves
				entry_ref ref;
				if (copyOfRefs.FindRef("refs", index, &ref) != B_OK)
					break;

				// deal with symlinks properly
				BEntry documentEntry(&ref, true);
				if (appEntry == documentEntry) {
					PRINT(("stripping %s, app %s \n", ref.name, app.name));
					copyOfRefs.RemoveData("refs", index);
				} else {
					PRINT(("leaving %s, app %s  \n", ref.name, app.name));
					index++;
				}
			}

			refsToPass = CountRefs(&copyOfRefs) > 0 ? &copyOfRefs: 0;
			team_id team;
			error = be_roster->Launch(&app, refsToPass, &team);
			if (error == B_ALREADY_RUNNING) 
				// app already running, not really an error
				error = B_OK;
			if (error == B_OK || mimesetIt != 0)
				break;

			SniffIfGeneric(&copyOfRefs);
		}
	}

	if (error != B_OK && alertString.Length() == 0) {
		BString loaderErrorString;
		bool openedDocuments = true;

		if (!refsToPass) {
			// we just double clicked the app itself, do not offer to
			// find a handling app
			openWithOK = false;
			openedDocuments = false;
		}

		if (error == B_LAUNCH_FAILED_EXECUTABLE && !refsToPass) {
			alertString << "Could not open \"" << app.name
				<< "\". The file is mistakenly marked as executable. ";

			if (!openWithOK) {
				// offer the possibility to change the permissions
				
				alertString << "\nShould this be fixed?";
				if ((new BAlert("", alertString.String(), "Cancel", "Proceed", 0,
						B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go() == 1) {
					BEntry entry(&documentRef);
					mode_t permissions;
					
					error = entry.GetPermissions(&permissions);
					if (error == B_OK)
						error = entry.SetPermissions(permissions & ~(S_IXUSR | S_IXGRP | S_IXOTH));
					if (error == B_OK) {
						// we updated the permissions, so let's try again
						_TrackerLaunchDocuments(NULL, refs, false);
						return;
					} else {
						alertString = "Could not update permissions of file \"";
						alertString << app.name << "\". " << strerror(error);
					}
				} else
					return;
			}

			alternative = kFindApplicationStr;
		} else if (error == B_LAUNCH_FAILED_APP_IN_TRASH) {
			alertString << "Could not open \"" << documentRef.name
				<< "\" because application \"" << app.name << "\" is in the trash. ";
			alternative = kFindAlternativeStr;
		} else if (error == B_LAUNCH_FAILED_APP_NOT_FOUND) {
			alertString << "Could not open \"" << documentRef.name << "\" "
				<< "(" << strerror(error) << "). ";
			alternative = kFindAlternativeStr;
		} else if (error == B_MISSING_SYMBOL
			&& LoaderErrorDetails(&app, loaderErrorString) == B_OK) {
			alertString << "Could not open \"" << documentRef.name << "\" ";
			if (openedDocuments)
				alertString << "with application \"" << app.name << "\" ";
			alertString << "(Missing symbol: " << loaderErrorString << "). \n";
			alternative = kFindAlternativeStr;
		} else if (error == B_MISSING_LIBRARY
			&& LoaderErrorDetails(&app, loaderErrorString) == B_OK) {
			alertString << "Could not open \"" << documentRef.name << "\" ";
			if (openedDocuments)
				alertString << "with application \"" << app.name << "\" ";
			alertString << "(Missing library: " << loaderErrorString << "). \n";
			alternative = kFindAlternativeStr;
		} else {
			alertString << "Could not open \"" << documentRef.name
				<< "\" with application \"" << app.name << "\" (" << strerror(error) << "). ";
			alternative = kFindAlternativeStr;
		}
	}

	if (error != B_OK) {
		if (openWithOK) {
			ASSERT(alternative);
			alertString << alternative;
			if ((new BAlert("", alertString.String(), "Cancel", "Find", 0,
					B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go() == 1)
				error = TrackerOpenWith(refs);
		} else 
			(new BAlert("", alertString.String(), "Cancel", 0, 0,
					B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
	}
}

// the following three calls don't return any reasonable error codes,
// should fix that, making them void

status_t 
TrackerLaunch(const entry_ref *appRef, const BMessage *refs, bool async, bool openWithOK)
{
	if (!async)
		_TrackerLaunchAppWithDocuments(appRef, refs, openWithOK);
	else
		AsynchLaunchBinder(&_TrackerLaunchAppWithDocuments, appRef, refs, openWithOK);

	return B_OK;
}

status_t 
TrackerLaunch(const entry_ref *appRef, bool async)
{
	if (!async)
		_TrackerLaunchAppWithDocuments(appRef, 0, false);
	else
		AsynchLaunchBinder(&_TrackerLaunchAppWithDocuments, appRef, 0, false);

	return B_OK;
}

status_t 
TrackerLaunch(const BMessage *refs, bool async, bool openWithOK)
{
	if (!async) 
		_TrackerLaunchDocuments(0, refs, openWithOK);
	else
		AsynchLaunchBinder(&_TrackerLaunchDocuments, 0, refs, openWithOK);

	return B_OK;
}

status_t
LaunchBrokenLink(const char *signature, const BMessage *refs)
{
	// This call is to support a hacky workaround for double-clicking
	// broken refs for cifs
	be_roster->Launch(signature, const_cast<BMessage *>(refs));
	return B_OK;
}

// external launch calls; need to be robust, work if Tracker is not running

#if !B_BEOS_VERSION_DANO
_IMPEXP_TRACKER
#endif
status_t 
FSLaunchItem(const entry_ref *application, const BMessage *refsReceived,
	bool async, bool openWithOK)
{
	return TrackerLaunch(application, refsReceived, async, openWithOK);
}


#if !B_BEOS_VERSION_DANO
_IMPEXP_TRACKER
#endif
status_t 
FSOpenWith(BMessage *listOfRefs)
{
	status_t result = B_ERROR;
	listOfRefs->what = B_REFS_RECEIVED;
	
	if (dynamic_cast<TTracker *>(be_app)) 
		result = TrackerOpenWith(listOfRefs);
	else 
		ASSERT(!"not yet implemented");

	return result;
}

// legacy calls, need for compatibility

void 
FSOpenWithDocuments(const entry_ref *executable, BMessage *documents)
{
	TrackerLaunch(executable, documents, true);
	delete documents;
}

status_t
FSLaunchUsing(const entry_ref *ref, BMessage *listOfRefs)
{
	BMessage temp(B_REFS_RECEIVED);
	if (!listOfRefs) {
		ASSERT(ref);
		temp.AddRef("refs", ref);
		listOfRefs = &temp;
	}
	FSOpenWith(listOfRefs);
	return B_OK;
}

status_t
FSLaunchItem(const entry_ref *ref, BMessage* message, int32, bool async)
{
	if (message) 
		message->what = B_REFS_RECEIVED;

	status_t result = TrackerLaunch(ref, message, async, true);
	delete message;
	return result;
}


void
FSLaunchItem(const entry_ref *ref, BMessage *message, int32 workspace)
{
	FSLaunchItem(ref, message, workspace, true);
}

// Get the original path of an entry in the trash
status_t
FSGetOriginalPath(BEntry *entry, BPath *result)
{
	status_t err;
	entry_ref ref;
	err = entry->GetRef(&ref);
	if (err != B_OK)
		return err;
	
	// Only call the routine for entries in the trash
	if (!FSInTrashDir(&ref))
		return B_ERROR;
		
	BNode node(entry);
	BString originalPath;
	if (node.ReadAttrString(kAttrOriginalPath, &originalPath) == B_OK) {
		// We're in luck, the entry has the original path in an attribute
		err = result->SetTo(originalPath.String());
		return err;
	}
	
	// Iterate the parent directories to find one with
	// the original path attribute
	BEntry parent(*entry);
	err = parent.InitCheck();
	if (err != B_OK)
		return err;

	// walk up the directory structure until we find a node
	// with original path attribute
	do {
		// move to the parent of this node
		err = parent.GetParent(&parent);
		if (err != B_OK)
			return err;
			
		// return if we are at the root of the trash
		if (FSIsTrashDir(&parent))
			return B_ENTRY_NOT_FOUND;
			
		// get the parent as a node
		err = node.SetTo(&parent);
		if (err != B_OK)
			return err;
	} while (node.ReadAttrString(kAttrOriginalPath, &originalPath) != B_OK);
	
	// Found the attribute, figure out there this file
	// used to live, based on the successfully-read attribute
	err = result->SetTo(originalPath.String());
	if (err != B_OK)
		return err;

	BPath path, pathParent;
	err = parent.GetPath(&pathParent);
	if (err != B_OK)
		return err;
	err = entry->GetPath(&path);
	if (err != B_OK)
		return err;
	result->Append(path.Path() + strlen(pathParent.Path()) + 1);
		// compute the new path by appending the offset of
		// the item we are locating, to the original path
		// of the parent
	return B_OK;
}

directory_which 
WellKnowEntryList::Match(const node_ref *node)
{
	const WellKnownEntry *result = MatchEntry(node);
	if (result)
		return result->which;
	
	return (directory_which)-1;
}

const WellKnowEntryList::WellKnownEntry *
WellKnowEntryList::MatchEntry(const node_ref *node)
{
	if (!self)
		self = new WellKnowEntryList();
	
	return self->MatchEntryCommon(node);
}

const WellKnowEntryList::WellKnownEntry *
WellKnowEntryList::MatchEntryCommon(const node_ref *node)
{	
	uint32 count = entries.size();
	for (uint32 index = 0; index < count; index++)
		if (*node == entries[index].node)
			return &entries[index];
	
	return NULL;
}


void
WellKnowEntryList::Quit()
{
	delete self;
	self = NULL;
}

void
WellKnowEntryList::AddOne(directory_which which, const char *name)
{
	BPath path;
	if (find_directory(which, &path, true) != B_OK)
		return;
	
	BEntry entry(path.Path());
	node_ref node;
	if (entry.GetNodeRef(&node) != B_OK)
		return;
	
	entries.push_back(WellKnownEntry(&node, which, name));
}

void 
WellKnowEntryList::AddOne(directory_which which, directory_which base,
	const char *extra, const char *name)
{
	BPath path;
	if (find_directory(base, &path, true) != B_OK)
		return;
	
	path.Append(extra);
	BEntry entry(path.Path());
	node_ref node;
	if (entry.GetNodeRef(&node) != B_OK)
		return;
	
	entries.push_back(WellKnownEntry(&node, which, name));
}

void 
WellKnowEntryList::AddOne(directory_which which, const char *path, const char *name)
{
	BEntry entry(path);
	node_ref node;
	if (entry.GetNodeRef(&node) != B_OK)
		return;
	
	entries.push_back(WellKnownEntry(&node, which, name));
}


WellKnowEntryList::WellKnowEntryList()
{
	AddOne(B_BEOS_DIRECTORY, "beos");
	AddOne((directory_which)B_BOOT_DISK, "/boot", "boot");
	AddOne(B_USER_DIRECTORY, "home");
	AddOne(B_BEOS_SYSTEM_DIRECTORY, "system");
	
	AddOne(B_BEOS_FONTS_DIRECTORY, "fonts");
	AddOne(B_COMMON_FONTS_DIRECTORY, "fonts");
	AddOne(B_USER_FONTS_DIRECTORY, "fonts");
	
	AddOne(B_BEOS_APPS_DIRECTORY, "apps");
	AddOne(B_APPS_DIRECTORY, "apps");
	AddOne((directory_which)B_USER_DESKBAR_APPS_DIRECTORY, B_USER_DESKBAR_DIRECTORY,
		"Applications", "apps");

	AddOne(B_BEOS_PREFERENCES_DIRECTORY, "preferences");
	AddOne(B_PREFERENCES_DIRECTORY, "preferences");
	AddOne((directory_which)B_USER_DESKBAR_PREFERENCES_DIRECTORY, B_USER_DESKBAR_DIRECTORY,
		"Preferences", "preferences");

	AddOne((directory_which)B_USER_MAIL_DIRECTORY, B_USER_DIRECTORY, "mail", "mail");
	
	AddOne((directory_which)B_USER_QUERIES_DIRECTORY, B_USER_DIRECTORY, "queries", "queries");


	
	AddOne(B_COMMON_DEVELOP_DIRECTORY, "develop");
	AddOne((directory_which)B_USER_DESKBAR_DEVELOP_DIRECTORY, B_USER_DESKBAR_DIRECTORY,
		"Development", "develop");
	
	AddOne(B_USER_CONFIG_DIRECTORY, "config");
	
	AddOne((directory_which)B_USER_PEOPLE_DIRECTORY, B_USER_DIRECTORY, "people", "people");
	
	AddOne((directory_which)B_USER_DOWNLOADS_DIRECTORY, B_USER_DIRECTORY, "Downloads",
		"Downloads");
}

WellKnowEntryList *WellKnowEntryList::self = NULL;

}
