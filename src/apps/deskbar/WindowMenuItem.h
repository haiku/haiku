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

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered
trademarks of Be Incorporated in the United States and other countries. Other
brand product names are registered trademarks or trademarks of their respective
holders.
All rights reserved.
*/
#ifndef WINDOWMENUITEM_H
#define WINDOWMENUITEM_H


#include <MenuItem.h>
#include <String.h>
#include <WindowInfo.h>


class BBitmap;

// Individual windows of an application item for WindowMenu,
// sub of TeamMenuItem all DB positions
class TWindowMenuItem : public BMenuItem {
public:
								TWindowMenuItem(const char* title, int32 id,
									bool mini, bool currentWorkSpace,
									bool dragging = false);

			void				ExpandedItem(bool state);
			void				SetTo(const char* title, int32 id, bool mini,
									bool currentWorkSpace,
									bool dragging = false);
			int32				ID();
			void				SetRequireUpdate();
			bool				RequiresUpdate();
			bool				ChangedState();

	virtual	void				SetLabel(const char* string);
			const char*			FullTitle() const;

	static	int32				InsertIndexFor(BMenu* menu, int32 startIndex,
									TWindowMenuItem* item);

protected:
			void				Initialize(const char* title);
	virtual void				GetContentSize(float* width, float* height);
	virtual void				DrawContent();
	virtual status_t			Invoke(BMessage* message = NULL);
	virtual void				Draw();

private:
			int32				fID;
			bool				fMini;
			bool				fCurrentWorkSpace;
			const BBitmap*		fBitmap;
			float				fTitleWidth;
			float				fTitleAscent;
			float				fTitleDescent;
			bool				fDragging;
			bool				fExpanded;
			bool				fRequireUpdate;
			bool				fModified;
			BString				fFullTitle;
};


#endif	/* WINDOWMENUITEM_H */
