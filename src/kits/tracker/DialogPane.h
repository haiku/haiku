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
#ifndef _DIALOG_PANE_H
#define _DIALOG_PANE_H


#include <Control.h>
#include <ObjectList.h>


namespace BPrivate {

class PaneSwitch : public BControl {
public:
								PaneSwitch(BRect frame, const char* name,
									bool leftAligned = true,
									uint32 resizeMask
										= B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE);

								PaneSwitch(const char* name,
									bool leftAligned = true,
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE);

	virtual						~PaneSwitch();

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 code, const BMessage*);
	virtual	void				MouseUp(BPoint where);

	virtual	void				GetPreferredSize(float* _width,
									float* _height);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

			void				SetLabels(const char* labelOn,
									const char* labelOff);

protected:
			enum State {
				kCollapsed,
				kPressed,
				kExpanded
			};

	virtual	void				DrawInState(PaneSwitch::State state);

private:
			bool				fLeftAligned;
			bool				fPressing;

			char*				fLabelOn;
			char*				fLabelOff;
};

} // namespace BPrivate

using namespace BPrivate;


#endif	// _DIALOG_PANE_H
