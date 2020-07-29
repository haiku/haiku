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

// Implementation for the public FilePanel object.


#include <sys/resource.h>

#include <BeBuild.h>
#include <Debug.h>
#include <FilePanel.h>
#include <Looper.h>
#include <Screen.h>
#include <Window.h>

#include "AutoLock.h"
#include "Commands.h"
#include "FilePanelPriv.h"


// prototypes for some private kernel calls that will some day be public
#ifndef _IMPEXP_ROOT
#	define _IMPEXP_ROOT
#endif


//	#pragma mark - BFilePanel


BFilePanel::BFilePanel(file_panel_mode mode, BMessenger* target,
	const entry_ref* ref, uint32 nodeFlavors, bool multipleSelection,
	BMessage* message, BRefFilter* filter, bool modal,
	bool hideWhenDone)
{
	// boost file descriptor limit so file panels in other apps don't have
	// problems
	struct rlimit rl;
	rl.rlim_cur = 512;
	rl.rlim_max = RLIM_SAVED_MAX;
	setrlimit(RLIMIT_NOFILE, &rl);

	BEntry startDir(ref);
	fWindow = new TFilePanel(mode, target, &startDir, nodeFlavors,
		multipleSelection, message, filter, 0, B_DOCUMENT_WINDOW_LOOK,
		modal ? B_MODAL_APP_WINDOW_FEEL : B_NORMAL_WINDOW_FEEL,
		hideWhenDone);

	static_cast<TFilePanel*>(fWindow)->SetClientObject(this);

	fWindow->SetIsFilePanel(true);
}


BFilePanel::~BFilePanel()
{
	if (fWindow->Lock())
		fWindow->Quit();
}


void
BFilePanel::Show()
{
	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return;

	// if the window is already showing, don't jerk the workspaces around,
	// just pull it to us
	uint32 workspace = 1UL << (uint32)current_workspace();
	uint32 windowWorkspaces = fWindow->Workspaces();
	if (!(windowWorkspaces & workspace)) {
		// window in a different workspace, reopen in current
		fWindow->SetWorkspaces(workspace);
	}

	// Position the file panel like an alert
	BWindow* parent = dynamic_cast<BWindow*>(
		BLooper::LooperForThread(find_thread(NULL)));
	const BRect frame = parent != NULL ? parent->Frame()
		: BScreen(fWindow).Frame();

	fWindow->MoveTo(fWindow->AlertPosition(frame));
	if (!IsShowing())
		fWindow->Show();

	fWindow->Activate();

#if 1
	// The Be Book gives the names for some of the child views so that apps
	// could move them around if they needed to, but we have most in layouts,
	// so once the window has been opened, we have to forcibly resize "PoseView"
	// (fBackView) to fully invalidate its layout in case any of the controls
	// in it have been moved.
	fWindow->FindView("PoseView")->ResizeBy(1, 1);
	fWindow->FindView("PoseView")->ResizeBy(-1, -1);
#endif
}


void
BFilePanel::Hide()
{
	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return;

	if (!fWindow->IsHidden())
		fWindow->QuitRequested();
}


bool
BFilePanel::IsShowing() const
{
	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return false;

	return !fWindow->IsHidden();
}


void
BFilePanel::SendMessage(const BMessenger* messenger, BMessage* message)
{
	messenger->SendMessage(message);
}


file_panel_mode
BFilePanel::PanelMode() const
{
	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return B_OPEN_PANEL;

	if (static_cast<TFilePanel*>(fWindow)->IsSavePanel())
		return B_SAVE_PANEL;

	return B_OPEN_PANEL;
}


BMessenger
BFilePanel::Messenger() const
{
	BMessenger target;

	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return target;

	return *static_cast<TFilePanel*>(fWindow)->Target();
}


void
BFilePanel::SetTarget(BMessenger target)
{
	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return;

	static_cast<TFilePanel*>(fWindow)->SetTarget(target);
}


void
BFilePanel::SetMessage(BMessage* message)
{
	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return;

	static_cast<TFilePanel*>(fWindow)->SetMessage(message);
}


void
BFilePanel::Refresh()
{
	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return;

	static_cast<TFilePanel*>(fWindow)->Refresh();
}


BRefFilter*
BFilePanel::RefFilter() const
{
	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return 0;

	return static_cast<TFilePanel*>(fWindow)->Filter();
}


void
BFilePanel::SetRefFilter(BRefFilter* filter)
{
	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return;

	static_cast<TFilePanel*>(fWindow)->SetRefFilter(filter);
}


void
BFilePanel::SetButtonLabel(file_panel_button button, const char* text)
{
	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return;

	static_cast<TFilePanel*>(fWindow)->SetButtonLabel(button, text);
}


void
BFilePanel::SetNodeFlavors(uint32 flavors)
{
	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return;

	static_cast<TFilePanel*>(fWindow)->SetNodeFlavors(flavors);
}


void
BFilePanel::GetPanelDirectory(entry_ref* ref) const
{
	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return;

	*ref = *static_cast<TFilePanel*>(fWindow)->TargetModel()->EntryRef();
}


void
BFilePanel::SetSaveText(const char* text)
{
	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return;

	static_cast<TFilePanel*>(fWindow)->SetSaveText(text);
}


void
BFilePanel::SetPanelDirectory(const entry_ref* ref)
{
	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return;

	static_cast<TFilePanel*>(fWindow)->SetTo(ref);
}


void
BFilePanel::SetPanelDirectory(const char* path)
{
	entry_ref ref;
	status_t err = get_ref_for_path(path, &ref);
	if (err < B_OK)
	  return;

	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return;

	static_cast<TFilePanel*>(fWindow)->SetTo(&ref);
}


void
BFilePanel::SetPanelDirectory(const BEntry* entry)
{
	entry_ref ref;

	if (entry && entry->GetRef(&ref) == B_OK)
		SetPanelDirectory(&ref);
}


void
BFilePanel::SetPanelDirectory(const BDirectory* dir)
{
	BEntry	entry;

	if (dir && (dir->GetEntry(&entry) == B_OK))
		SetPanelDirectory(&entry);
}


BWindow*
BFilePanel::Window() const
{
	return fWindow;
}


void
BFilePanel::Rewind()
{
	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return;

	static_cast<TFilePanel*>(fWindow)->Rewind();
}


status_t
BFilePanel::GetNextSelectedRef(entry_ref* ref)
{
	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return B_ERROR;

	return static_cast<TFilePanel*>(fWindow)->GetNextEntryRef(ref);

}


void
BFilePanel::SetHideWhenDone(bool on)
{
	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return;

	static_cast<TFilePanel*>(fWindow)->SetHideWhenDone(on);
}


bool
BFilePanel::HidesWhenDone(void) const
{
	AutoLock<BWindow> lock(fWindow);
	if (!lock)
		return false;

	return static_cast<TFilePanel*>(fWindow)->HidesWhenDone();
}


void
BFilePanel::WasHidden()
{
	// hook function
}


void
BFilePanel::SelectionChanged()
{
	// hook function
}
