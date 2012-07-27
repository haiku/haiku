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
#ifndef _QUERY_CONTAINER_WINDOW_H
#define _QUERY_CONTAINER_WINDOW_H


// Container window specificaly used for displaying BQueryPoseViews
// Adds query window specific menus


#include "ContainerWindow.h"


namespace BPrivate {

#define kQueryTemplates "DefaultQueryTemplates"

class BQueryPoseView;

class BQueryContainerWindow : public BContainerWindow {
public:
	BQueryContainerWindow(LockingList<BWindow>* windowList,
		uint32 containerWindowFlags,
		window_look look = B_DOCUMENT_WINDOW_LOOK,
		window_feel feel = B_NORMAL_WINDOW_FEEL,
		uint32 flags = B_WILL_ACCEPT_FIRST_CLICK | B_NO_WORKSPACE_ACTIVATION,
		uint32 workspace = B_CURRENT_WORKSPACE);

	BQueryPoseView* PoseView() const;
	bool ActiveOnDevice(dev_t) const;

protected:
	virtual	void CreatePoseView(Model*);
	virtual BPoseView* NewPoseView(Model* model, BRect rect, uint32 viewMode);
	virtual	void AddWindowMenu(BMenu* menu);
	virtual	void AddWindowContextMenus(BMenu* menu);

	virtual void SetUpDefaultState();

private:
	typedef BContainerWindow _inherited;
};

} // namespace BPrivate

using namespace BPrivate;

#endif	// _QUERY_CONTAINER_WINDOW_H
