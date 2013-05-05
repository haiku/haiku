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
#ifndef FILE_PERMISSIONS_VIEW_H
#define FILE_PERMISSIONS_VIEW_H


#include <CheckBox.h>
#include <TextControl.h>

#include "Model.h"


namespace BPrivate {

class FocusCheckBox : public BCheckBox {
	public:
		FocusCheckBox(BRect rect, const char* name, const char* label,
			BMessage* message)
			:	BCheckBox(rect, name, label, message)
			{
			}

		virtual void Draw(BRect rect)
		{
			BCheckBox::Draw(rect);

			if (IsFocus()) {
				SetHighColor(0, 0, 255);
				StrokeRect(BRect(2 , 4, 12, 14));
			}
		}
};


class FilePermissionsView : public BView {
	public:
		FilePermissionsView(BRect, Model*);

		void ModelChanged(Model*);

	protected:
		virtual void MessageReceived(BMessage*);
		virtual void AttachedToWindow();

	private:
		Model* fModel;

		FocusCheckBox* fReadUserCheckBox;
		FocusCheckBox* fReadGroupCheckBox;
		FocusCheckBox* fReadOtherCheckBox;

		FocusCheckBox* fWriteUserCheckBox;
		FocusCheckBox* fWriteGroupCheckBox;
		FocusCheckBox* fWriteOtherCheckBox;

		FocusCheckBox* fExecuteUserCheckBox;
		FocusCheckBox* fExecuteGroupCheckBox;
		FocusCheckBox* fExecuteOtherCheckBox;

		BTextControl* fOwnerTextControl;
		BTextControl* fGroupTextControl;

		typedef BView _inherited;
};


} // namespace BPrivate

using namespace BPrivate;

#endif	// FILE_PERMISSIONS_VIEW_H
