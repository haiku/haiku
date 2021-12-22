/*****************************************************************************/
// ImageView
// Written by Michael Wilber, Haiku Translation Kit Team
//
// ImageView.h
//
// BView class for showing images.  Images can be dropped on this view
// from the tracker and images viewed in this view can be dragged
// to the tracker to be saved.
//
//
// Copyright (c) 2003 Haiku Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include <View.h>
#include <Bitmap.h>
#include <Path.h>
#include <Entry.h>
#include <TranslatorRoster.h>

class ImageView : public BView {
public:
	ImageView(BRect rect, const char *name);
	~ImageView();

	void AttachedToWindow();
	void Draw(BRect rect);
	void FrameResized(float width, float height);
	void MouseDown(BPoint point);
	void MouseMoved(BPoint point, uint32 state, const BMessage *pmsg);
	void MouseUp(BPoint point);
	void MessageReceived(BMessage *pmsg);

	void SetImage(BMessage *pmsg);
	bool HasImage() { return fpbitmap ? true : false; };

	void FirstPage();
	void LastPage();
	void NextPage();
	void PrevPage();

private:
	void UpdateInfoWindow(const BPath &path, BMessage &ioExtension,
		const translator_info &info, BTranslatorRoster *proster);
	void ReDraw();
	void AdjustScrollBars();
	void SaveImageAtDropLocation(BMessage *pmsg);

	BTranslatorRoster *SelectTranslatorRoster(BTranslatorRoster &roster);

	entry_ref fcurrentRef;
	int32 fdocumentIndex;
	int32 fdocumentCount;
	BBitmap *fpbitmap;
};

#endif // #ifndef IMAGEVIEW_H
