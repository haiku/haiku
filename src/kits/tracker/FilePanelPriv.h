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
#ifndef	_FILE_PANEL_PRIV_H
#define _FILE_PANEL_PRIV_H


#include <FilePanel.h>

#include "ContainerWindow.h"
#include "PoseView.h"
#include "TaskLoop.h"


class BTextControl;
class BFilePanel;
class BRefFilter;
class BMessenger;
class BMenuField;

namespace BPrivate {

class BackgroundView;
class BDirMenu;
class AttributeStreamNode;
class BFilePanelPoseView;

class TFilePanel : public BContainerWindow {
public:
	TFilePanel(file_panel_mode = B_OPEN_PANEL,
		BMessenger* target = NULL, const BEntry* startDirectory = NULL,
		uint32 nodeFlavors = B_FILE_NODE | B_SYMLINK_NODE,
		bool multipleSelection = true, BMessage* = NULL, BRefFilter* = NULL,
		uint32 containerWindowFlags = 0,
		window_look look = B_DOCUMENT_WINDOW_LOOK,
		window_feel feel = B_NORMAL_WINDOW_FEEL,
		bool hideWhenDone = true);

	virtual ~TFilePanel();

	BFilePanelPoseView* PoseView() const;

	virtual	bool QuitRequested();
	virtual	void MenusBeginning();
	virtual	void MenusEnded();
	virtual	void DispatchMessage(BMessage* message, BHandler* handler);
	virtual	void ShowContextMenu(BPoint, const entry_ref*, BView*);

			void SetClientObject(BFilePanel*);
			void SetRefFilter(BRefFilter*);
			void SetSaveText(const char* text);
			void SetButtonLabel(file_panel_button, const char* text);
			void SetTo(const entry_ref* ref);
	virtual	void SelectionChanged();
			void HandleOpenButton();
			void HandleSaveButton();
			void Rewind();
			bool IsSavePanel() const;
			void Refresh();
			const BMessenger* Target() const;
	BRefFilter* Filter() const;

	void SetTarget(BMessenger);
	void SetMessage(BMessage* message);

	virtual	status_t GetNextEntryRef(entry_ref*);
	virtual	void MessageReceived(BMessage*);

	void SetHideWhenDone(bool);
	bool HidesWhenDone(void);

	bool TrackingMenu() const;

protected:
	BPoseView* NewPoseView(Model* model, BRect rect, uint32 viewMode);
	virtual	void Init(const BMessage* message = NULL);
	virtual	void SaveState(bool hide = true);
	virtual	void SaveState(BMessage &) const;
	virtual void RestoreState();
	virtual void RestoreWindowState(AttributeStreamNode*);
	virtual void RestoreWindowState(const BMessage&);
	virtual void RestoreState(const BMessage&);

	virtual	void AddFileContextMenus(BMenu*);
	virtual	void AddWindowContextMenus(BMenu*);
	virtual	void AddDropContextMenus(BMenu*);
	virtual	void AddVolumeContextMenus(BMenu*);

	virtual	void SetupNavigationMenu(const entry_ref*, BMenu*);
	virtual	void OpenDirectory();
	virtual	void OpenParent();
	virtual	void WindowActivated(bool state);

	static	filter_result FSFilter(BMessage*, BHandler**, BMessageFilter*);
	static	filter_result MessageDropFilter(BMessage*, BHandler**,
		BMessageFilter*);

			int32 ShowCenteredAlert(const char* text, const char* button1,
				const char* button2 = NULL, const char* button3 = NULL);

private:
			bool SwitchDirToDesktopIfNeeded(entry_ref &ref);
			bool CanOpenParent() const;
			void SwitchDirMenuTo(const entry_ref* ref);
			void AdjustButton();
			bool SelectChildInParent(const entry_ref* parent,
				const node_ref* child);
			void OpenSelectionCommon(BMessage*);

			bool			fIsSavePanel;
			uint32			fNodeFlavors;
			BackgroundView*	fBackView;
			BDirMenu*		fDirMenu;
			BMenuField*		fDirMenuField;
			BTextControl*	fTextControl;
			BMessenger		fTarget;
			BFilePanel*		fClientObject;
			int32			fSelectionIterator;
			BMessage*		fMessage;
			BString			fButtonText;
			bool			fHideWhenDone;
			bool			fIsTrackingMenu;

			typedef BContainerWindow _inherited;

			friend class BackgroundView;
};


class BFilePanelPoseView : public BPoseView {
public:
	BFilePanelPoseView(Model*, BRect, uint32 resizeMask = B_FOLLOW_ALL);

	virtual bool IsFilePanel() const;
	virtual bool FSNotification(const BMessage*);

	void SetIsDesktop(bool);

protected:
	// don't do any volume watching and memtamime watching in file panels for now
	virtual	void StartWatching();
	virtual	void StopWatching();

	virtual	void RestoreState(AttributeStreamNode*);
	virtual	void RestoreState(const BMessage &);
	virtual	void SavePoseLocations(BRect* = NULL);

	virtual	EntryListBase* InitDirentIterator(const entry_ref*);
	virtual	void AddPosesCompleted();
	virtual	bool IsDesktopView() const;

			void ShowVolumes(bool visible, bool showShared);

			void AdaptToVolumeChange(BMessage*);
			void AdaptToDesktopIntegrationChange(BMessage*);

private:
			bool fIsDesktop;
				// This flags makes the distinction between the Desktop as
				// the root of the world and "/boot/home/Desktop" to which
				// we might have navigated from the home dir.

			typedef BPoseView _inherited;
};


// inlines follow

inline bool
BFilePanelPoseView::IsFilePanel() const
{
	return true;
}


inline bool
TFilePanel::IsSavePanel() const
{
	return fIsSavePanel;
}


inline const BMessenger*
TFilePanel::Target() const
{
	return &fTarget;
}


inline void
TFilePanel::Refresh()
{
	fPoseView->Refresh();
}


inline bool
TFilePanel::HidesWhenDone(void)
{
	return fHideWhenDone;
}


inline void
TFilePanel::SetHideWhenDone(bool on)
{
	fHideWhenDone = on;
}


inline bool
TFilePanel::TrackingMenu() const
{
	return fIsTrackingMenu;
}

} // namespace BPrivate

using namespace BPrivate;

#endif
