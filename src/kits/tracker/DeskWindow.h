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
#ifndef	_DESK_WINDOW_H
#define _DESK_WINDOW_H


#include <Shelf.h>
#include <set>

#include "ContainerWindow.h"
#include "DesktopPoseView.h"


class BPopUpMenu;

namespace BPrivate {

class BDeskWindow : public BContainerWindow {
public:
	BDeskWindow(LockingList<BWindow>* windowList);
	virtual ~BDeskWindow();

	virtual	void Init(const BMessage* message = NULL);

	virtual	void Show();
	virtual	void Quit();
	virtual	void ScreenChanged(BRect, color_space);

	virtual	void CreatePoseView(Model*);

	virtual	bool ShouldAddMenus() const;
	virtual	bool ShouldAddScrollBars() const;
	virtual	bool ShouldAddContainerView() const;

	DesktopPoseView* PoseView() const;

	void UpdateDesktopBackgroundImages();
		// Desktop window has special background image handling
		
	void SaveDesktopPoseLocations();
	
protected:
	virtual	void AddWindowContextMenus(BMenu*);
	virtual BPoseView* NewPoseView(Model*, BRect, uint32);

	virtual void WorkspaceActivated(int32, bool);
	virtual	void MenusBeginning();
	virtual void MessageReceived(BMessage*);

private:
	BShelf* fDeskShelf;
		// shelf for replicant support
	BPopUpMenu* fTrashContextMenu;

	BRect fOldFrame;
	
	// in the desktop window addon shortcuts have to be added by AddShortcut
	// and we don't always get the MenusBeginning call to check for new
	// addons/update the shortcuts -- instead we need to node monitor the
	// addon directory and keep a dirty flag that triggers shortcut
	// reinstallation
	bool fShouldUpdateAddonShortcuts;
	std::set<uint32> fCurrentAddonShortcuts;
		// keeps track of which shortcuts are installed for Tracker addons
	
	typedef BContainerWindow _inherited;
};


inline DesktopPoseView*
BDeskWindow::PoseView() const
{
	return dynamic_cast<DesktopPoseView*>(_inherited::PoseView());
}

} // namespace BPrivate

using namespace BPrivate;

#endif
