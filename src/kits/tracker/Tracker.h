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
#ifndef _TRACKER_H
#define _TRACKER_H


#include <Application.h>
#include <Resources.h>
#include <Entry.h>

#include "LockingList.h"
#include "SettingsHandler.h"
#include "Utilities.h"

#include "tracker_private.h"

namespace BPrivate {

class BClipboardRefsWatcher;
class BContainerWindow;
class BDeskWindow;
class BInfoWindow;
class BTrashWatcher;
class ExtraAttributeLazyInstaller;
class MimeTypeList;
class Model;
class BooleanValueSetting;
class ScalarValueSetting;
class HexScalarValueSetting;
class TaskLoop;
class TrackerSettingsWindow;

typedef LockingList<BWindow> WindowList;
	// this is because MW can't handle nested templates


const uint32 kNextSpecifier = 'snxt';
const uint32 kPreviousSpecifier = 'sprv';
const uint32 B_ENTRY_SPECIFIER = 'sref';

#define kPropertyEntry "Entry"
#define kPropertySelection "Selection"



class TTracker : public BApplication {
	public:
		TTracker();
		virtual ~TTracker();

		// BApplication overrides
		virtual void Quit();
		virtual bool QuitRequested();
		virtual void ReadyToRun();
		virtual void MessageReceived(BMessage*);
		virtual void Pulse();
		virtual void RefsReceived(BMessage*);
		virtual void ArgvReceived(int32 argc, char** argv);

		MimeTypeList* MimeTypes() const;
			// list of mime types that have a description and do not have
			// themselves as a preferred handler (case of applications)

		bool TrashFull() const;
		bool IsTrashNode(const node_ref*) const;
		bool InTrashNode(const entry_ref*) const;

		void CloseParentWaitingForChildSoon(const entry_ref* child,
				const node_ref* parent);
			// closes parent, waits for child to open first

		void SelectChildInParentSoon(const entry_ref* child,
				const node_ref* parent);
			// waits till child shows up in parent and selects it

		void SelectPoseAtLocationSoon(node_ref parent, BPoint location);
			// Used to select next item when deleting in list view mode

		enum OpenSelector {
			kOpen,
			kOpenWith,
			kRunOpenWithWindow
		};

		bool EntryHasWindowOpen(const entry_ref*);
			// return true if there is an open window for an entry

		status_t NeedMoreNodeMonitors();
			// call if ran out of node monitors to allocate more
			// return false if already using all we can get
		static status_t WatchNode(const node_ref*, uint32 flags,
					BMessenger target);
			// cover call for watch_node; if first watch_node fails,
			// tries bumping the node monitor limit and calls watch_node
			// again

		TaskLoop* MainTaskLoop() const;
		BMessenger MountServer() const;

		bool QueryActiveForDevice(dev_t);
		void CloseActiveQueryWindows(dev_t);

		void SaveAllPoseLocations();

		void CloseParent(node_ref closeThis);

		void ShowSettingsWindow();

		BContainerWindow* FindContainerWindow(const node_ref*,
			int32 number = 0) const;
		BContainerWindow* FindContainerWindow(const entry_ref*,
			int32 number = 0) const;
		BContainerWindow* FindParentContainerWindow(const entry_ref*) const;
			// right now works just on plain windows, not on query windows

		BClipboardRefsWatcher* ClipboardRefsWatcher() const;

	protected:
		// scripting
		virtual BHandler* ResolveSpecifier(BMessage*, int32, BMessage*,
			int32, const char*);
		virtual status_t GetSupportedSuites(BMessage*);

		bool HandleScriptingMessage(BMessage*);

		bool ExecuteProperty(BMessage*, int32, const char*, BMessage*);
		bool CreateProperty(BMessage*, BMessage*, int32, const char*,
				BMessage*);
		bool DeleteProperty(BMessage*, int32,
				const char*, BMessage*);
		bool CountProperty(BMessage*, int32, const char*, BMessage*);
		bool GetProperty(BMessage*, int32, const char*, BMessage*);
		bool SetProperty(BMessage*, BMessage*, int32, const char*, BMessage*);

	private:
		// callbacks for ChildParentSoon calls
		bool CloseParentWaitingForChild(const entry_ref* child,
				const node_ref* parent);
		bool LaunchAndCloseParentIfOK(const entry_ref* launchThis,
				const node_ref* closeThis, const BMessage* messageToBundle);
		bool SelectChildInParent(const entry_ref* child,
				const node_ref* parent);
		void SelectPoseAtLocationInParent(node_ref parent, BPoint location);
		bool CloseParentWindowCommon(BContainerWindow*);

		void InitMimeTypes();
		bool InstallMimeIfNeeded(const char* type, int32 bitsID,
				const char* shortDescription, const char* longDescription,
				const char* preferredAppSignature, uint32 forceMask = 0);
			// used by InitMimeTypes - checks if a metamime of a given <type>
			// is installed and if it has all the specified attributes;
			// if not, the whole mime type is installed and all attributes
			// are set; nulls can be passed for attributes that don't matter;
			// returns true if anything had to be changed <forceMask> can be
			// used to forcibly set a metamime attribute, even if it exists

		void InstallDefaultTemplates();
		void InstallTemporaryBackgroundImages();

		void InstallIndices();
		void InstallIndices(dev_t);

		void CloseAllWindows();
		void CloseWindowAndChildren(const node_ref*);
		void CloseAllInWorkspace();
		void OpenInfoWindows(BMessage*);
		void MoveRefsToTrash(const BMessage*);
		void OpenContainerWindow(Model*, BMessage* refsList = NULL,
			OpenSelector openSelector = kOpen, uint32 openFlags = 0,
			bool checkAlreadyOpen = true,
			const BMessage* stateMessage = NULL);
			// pass either a Model or a list of entries to open
		void _OpenPreviouslyOpenedWindows(const char* pathFilter = NULL);

		void SetDefaultPrinter(const BMessage*);
		void EditQueries(const BMessage*);

		BInfoWindow* FindInfoWindow(const node_ref*) const;

		BDeskWindow* GetDeskWindow() const;

		status_t OpenRef(const entry_ref*, const node_ref* nodeToClose = NULL,
				const node_ref* nodeToSelect = NULL,
				OpenSelector selector = kOpen,
				const BMessage* messageToBundle = NULL);

		MimeTypeList*			fMimeTypeList;
		WindowList				fWindowList;
		BClipboardRefsWatcher*	fClipboardRefsWatcher;
		BTrashWatcher*			fTrashWatcher;
		TaskLoop*				fTaskLoop;
		int32 					fNodeMonitorCount;

		TrackerSettingsWindow*	fSettingsWindow;

		typedef BApplication _inherited;
};


inline TaskLoop*
TTracker::MainTaskLoop() const
{
	return fTaskLoop;
}


inline BClipboardRefsWatcher*
TTracker::ClipboardRefsWatcher() const
{
	return fClipboardRefsWatcher;
}

} // namespace BPrivate

using namespace BPrivate;

#endif	// _TRACKER_H
