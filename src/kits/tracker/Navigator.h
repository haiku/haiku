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
#ifndef _NAVIGATOR_H_
#define _NAVIGATOR_H_


#include "Model.h"

#include <PictureButton.h>
#include <View.h>


class BTextControl;
class BEntry;

namespace BPrivate {

enum NavigationAction
{
	kActionSet,
	kActionForward,
	kActionBackward,
	kActionUp,
	kActionLocation,
	kActionUpdatePath,

	kNavigatorCommandBackward = 'NVBW',
	kNavigatorCommandForward = 'NVFW',
	kNavigatorCommandUp = 'NVUP',
	kNavigatorCommandLocation = 'NVLC'
};


// Custom BPictureButton which takes
// bitmap resource IDs as arguments
class BNavigatorButton : public BPictureButton {
public:
	BNavigatorButton(BRect rect, const char* name, BMessage* message, int32 resIDon,
		int32 resIDoff, int32 resIDdisabled);
					 
	~BNavigatorButton();
	
	virtual	void AttachedToWindow();

	void SetPicture(BBitmap*, bool enabled, bool on);

private:
	int32 fResIDOn;
	int32 fResIDOff;
	int32 fResIDDisabled;
};


class BNavigator : public BView {
public:
	BNavigator(const Model* model, BRect rect,
		uint32 resizeMask = B_FOLLOW_LEFT_RIGHT);
	~BNavigator();

	void UpdateLocation(const Model* newmodel, int32 action);

	static float CalcNavigatorHeight(void);

	BContainerWindow* Window() const;

protected:
	virtual void Draw(BRect rect);
	virtual void MessageReceived(BMessage* msg);
	virtual void AttachedToWindow();

	void GoForward(bool option); // is option key held down?
	void GoBackward(bool option);
	void GoUp(bool option);
	void SendNavigationMessage(NavigationAction, BEntry*, bool option);

	void GoTo();

private:
	BPath				fPath;
	BNavigatorButton*	fBack;
	BNavigatorButton*	fForw;
	BNavigatorButton*	fUp;
	BTextControl*		fLocation;

	BObjectList<BPath> fBackHistory;
	BObjectList<BPath> fForwHistory;

	typedef BView _inherited;
};


inline
BContainerWindow*
BNavigator::Window() const
{
	return dynamic_cast<BContainerWindow*>(_inherited::Window());
}


} // namespace BPrivate

using namespace BPrivate;

#endif	// _NAVIGATOR_H_
