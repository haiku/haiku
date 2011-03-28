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

#ifndef	FS_UTILS_H
#define	FS_UTILS_H

#include <FindDirectory.h>
#include <List.h>
#include <Point.h>
#include <StorageDefs.h>

#include <vector>

#include "Model.h"
#include "ObjectList.h"

// Note - APIs/code in FSUtils.h and FSUtils.cpp is slated for a major cleanup
// 	-- in other words, you will find a lot of ugly cruft in here

class BDirectory;
class BEntry;
class BList;
class BFile;

namespace BPrivate {

class BInfoWindow;

//! Controls the copy engine; may be overriden to specify how conflicts are
// handled, etc.
class CopyLoopControl {
public:
	virtual						~CopyLoopControl();

	virtual	void				Init(uint32 jobKind);
	virtual	void				Init(int32 totalItems, off_t totalSize,
									const entry_ref* destDir = NULL,
									bool showCount = true);

	//! Inform that a file error occurred while copying <name>.
	// \return \c True if user decided to continue
	virtual	bool				FileError(const char* message,
									const char* name, status_t error,
									bool allowContinue);

	virtual	void				UpdateStatus(const char* name,
									const entry_ref& ref, int32 count,
									bool optional = false);

	//! \return \c true if canceled
	virtual	bool				CheckUserCanceled();

			enum OverwriteMode {
				kSkip,			// do not replace, go to next entry
				kReplace,		// remove entry before copying new one
				kMerge			// for folders: leave existing folder, update
								// contents leaving nonconflicting items
								// for files: save original attributes on file.
			};

	//! Override to always overwrite, never overwrite, let user decide,
	// compare dates, etc.
	virtual	OverwriteMode		OverwriteOnConflict(const BEntry* srcEntry,
									const char* destName,
									const BDirectory* destDir,
									bool srcIsDir, bool dstIsDir);

	//! Override to prevent copying of a given file or directory
	virtual	bool				SkipEntry(const BEntry *, bool file);

	//! During a file copy, this is called every time a chunk of data
	// is copied.  Users may override to keep a running checksum.
	virtual	void				ChecksumChunk(const char* block, size_t size);

	//! This is called when a file is finished copying.  Users of this
	// class may override to verify that the checksum they've been
	// computing in ChecksumChunk matches.  If this returns true,
	// the copy will continue.  If false, if will abort.
	virtual	bool				ChecksumFile(const entry_ref*);

	virtual	bool				SkipAttribute(const char* attributeName);
	virtual	bool				PreserveAttribute(const char* attributeName);
};


//! This is the Tracker copy-specific version of CopyLoopControl.
class TrackerCopyLoopControl : public CopyLoopControl {
public:
								TrackerCopyLoopControl();
								TrackerCopyLoopControl(uint32 jobKind);
								TrackerCopyLoopControl(int32 totalItems,
									off_t totalSize);
	virtual						~TrackerCopyLoopControl();

	virtual	void				Init(uint32 state);
	virtual	void				Init(int32 totalItems, off_t totalSize,
									const entry_ref* destDir = NULL,
									bool showCount = true);

	virtual	bool				FileError(const char *message,
									const char *name, status_t error,
									bool allowContinue);

	virtual	void				UpdateStatus(const char *name,
									const entry_ref& ref, int32 count,
									bool optional = false);

	virtual	bool				CheckUserCanceled();

	virtual	bool				SkipAttribute(const char *attributeName);


	// One can specify an entry_ref list with the source entries. This will
	// then trigger the feature to pull additional source entries from the
	// status window, such that the user can drop additional items onto the
	// progress display of the ongoing copy process to copy these items to
	// the same target directory.
			typedef BObjectList<entry_ref> EntryList;

			void				SetSourceList(EntryList* list);

private:
			thread_id			fThread;

			EntryList*			fSourceList;
};


#define B_DESKTOP_DIR_NAME "Desktop"

#ifndef _IMPEXP_TRACKER
#define _IMPEXP_TRACKER
#endif
_IMPEXP_TRACKER status_t FSCopyAttributesAndStats(BNode *, BNode *);

_IMPEXP_TRACKER void FSDuplicate(BObjectList<entry_ref> *srcList, BList *pointList);
_IMPEXP_TRACKER void FSMoveToFolder(BObjectList<entry_ref> *srcList, BEntry *, uint32 moveMode,
	BList *pointList = NULL);
_IMPEXP_TRACKER void FSMakeOriginalName(char *name, BDirectory *destDir, const char *suffix);
_IMPEXP_TRACKER bool FSIsTrashDir(const BEntry *);
_IMPEXP_TRACKER bool FSIsPrintersDir(const BEntry *);
_IMPEXP_TRACKER bool FSIsDeskDir(const BEntry *);
_IMPEXP_TRACKER bool FSIsHomeDir(const BEntry *);
_IMPEXP_TRACKER bool FSIsRootDir(const BEntry *);
_IMPEXP_TRACKER void FSMoveToTrash(BObjectList<entry_ref> *srcList, BList *pointList = NULL,
	bool async = true);
	// Deprecated

void FSDeleteRefList(BObjectList<entry_ref> *, bool, bool confirm = true);
void FSDelete(entry_ref *, bool, bool confirm = true);
void FSRestoreRefList(BObjectList<entry_ref> *list, bool async);

_IMPEXP_TRACKER status_t FSLaunchItem(const entry_ref *application, const BMessage *refsReceived,
	bool async, bool openWithOK);
	// Preferred way of launching; only pass an actual application in <application>, not
	// a document; to open documents with the preferred app, pase 0 in <application> and
	// stuff all the document refs into <refsReceived>
	// Consider having silent mode that does not show alerts, just returns error code

_IMPEXP_TRACKER status_t FSOpenWith(BMessage *listOfRefs);
	// runs the Open With window; pas a list of refs

_IMPEXP_TRACKER void FSEmptyTrash();
_IMPEXP_TRACKER status_t FSCreateNewFolderIn(const node_ref *destDir, entry_ref *newRef,
	node_ref *new_node);
_IMPEXP_TRACKER void FSCreateTrashDirs();
_IMPEXP_TRACKER status_t FSGetTrashDir(BDirectory *trashDir, dev_t volume);
_IMPEXP_TRACKER status_t FSGetDeskDir(BDirectory *deskDir);
_IMPEXP_TRACKER status_t FSRecursiveCalcSize(BInfoWindow *,
	CopyLoopControl* loopControl, BDirectory *, off_t *runningSize,
	int32 *fileCount, int32 *dirCount);

bool FSInTrashDir(const entry_ref *);

// doesn't need to be exported
bool FSGetPoseLocation(const BNode *node, BPoint *point);
status_t FSSetPoseLocation(BEntry *entry, BPoint point);
status_t FSSetPoseLocation(ino_t destDirInode, BNode *destNode, BPoint point);
status_t FSGetBootDeskDir(BDirectory *deskDir);

status_t FSGetOriginalPath(BEntry *entry, BPath *path);

enum ReadAttrResult {
	kReadAttrFailed,
	kReadAttrNativeOK,
	kReadAttrForeignOK
};

ReadAttrResult ReadAttr(const BNode *, const char *hostAttrName, const char *foreignAttrName,
	type_code , off_t , void *, size_t , void (*swapFunc)(void *) = 0,
	bool isForeign = false);
	// Endian swapping ReadAttr call; endianness is determined by trying first the
	// native attribute name, then the foreign one; an endian swapping function can
	// be passed, if null data won't be swapped; if <isForeign> set the foreign endianness
	// will be read directly without first trying the native one

ReadAttrResult GetAttrInfo(const BNode *, const char *hostAttrName, const char *foreignAttrName,
	type_code * = NULL, size_t * = NULL);

status_t FSCreateNewFolder(const entry_ref *);
status_t FSRecursiveCreateFolder(const char *path);
void FSMakeOriginalName(BString &name, const BDirectory *destDir, const char *suffix = 0);

status_t TrackerLaunch(const entry_ref *app, bool async);
status_t TrackerLaunch(const BMessage *refs, bool async, bool okToRunOpenWith = true);
status_t TrackerLaunch(const entry_ref *app, const BMessage *refs, bool async,
	bool okToRunOpenWith = true);
status_t LaunchBrokenLink(const char *, const BMessage *);

status_t FSFindTrackerSettingsDir(BPath *, bool autoCreate = true);

bool FSIsDeskDir(const BEntry *);

// two separate ifYouDoAction and toDoAction versions are needed for localization
// purposes. The first one is used in "If you do action ..." sentence,
// the second one in the "To do action" sentence.
bool ConfirmChangeIfWellKnownDirectory(const BEntry *entry,
	const char *ifYouDoAction, const char *toDoAction,
	bool dontAsk = false, int32 *confirmedAlready = NULL);

bool CheckDevicesEqual(const entry_ref *entry, const Model *targetModel);

// Deprecated calls use newer calls above instead
_IMPEXP_TRACKER void FSLaunchItem(const entry_ref *, BMessage * = NULL, int32 workspace = -1);
_IMPEXP_TRACKER status_t FSLaunchItem(const entry_ref *, BMessage *,
	int32 workspace, bool asynch);
_IMPEXP_TRACKER void FSOpenWithDocuments(const entry_ref *executableToLaunch,
	BMessage *documentEntryRefs);
_IMPEXP_TRACKER status_t FSLaunchUsing(const entry_ref *ref, BMessage *listOfRefs);


// some extra directory_which values
// move these to FindDirectory.h
const uint32 B_USER_MAIL_DIRECTORY = 3500;
const uint32 B_USER_QUERIES_DIRECTORY = 3501;
const uint32 B_USER_PEOPLE_DIRECTORY = 3502;
const uint32 B_USER_DOWNLOADS_DIRECTORY = 3503;
const uint32 B_USER_DESKBAR_APPS_DIRECTORY = 3504;
const uint32 B_USER_DESKBAR_PREFERENCES_DIRECTORY = 3505;
const uint32 B_USER_DESKBAR_DEVELOP_DIRECTORY = 3506;
const uint32 B_BOOT_DISK = 3507;
	// map /boot into the directory_which enum for convenience

class WellKnowEntryList {
	// matches up names, id's and node_refs of well known entries in the
	// system hierarchy
	public:
		struct WellKnownEntry {
			WellKnownEntry(const node_ref *node, directory_which which, const char *name)
				:
				node(*node),
				which(which),
				name(name)
			{
			}

			// mwcc needs these explicitly to use vector
			WellKnownEntry(const WellKnownEntry &clone)
				:
				node(clone.node),
				which(clone.which),
				name(clone.name)
			{
			}

			WellKnownEntry()
			{
			}

			node_ref node;
			directory_which which;
			BString name;
		};

		static directory_which Match(const node_ref *);
		static const WellKnownEntry *MatchEntry(const node_ref *);
		static void Quit();

	private:
		const WellKnownEntry *MatchEntryCommon(const node_ref *);
		WellKnowEntryList();
		void AddOne(directory_which, const char *name);
		void AddOne(directory_which, const char *path, const char *name);
		void AddOne(directory_which, directory_which base, const char *extension,
			const char *name);

		std::vector<WellKnownEntry> entries;
		static WellKnowEntryList *self;
};

#if B_BEOS_VERSION_DANO
#undef _IMPEXP_TRACKER
#endif

} // namespace BPrivate

using namespace BPrivate;

#endif	/* FS_UTILS_H */
