/*****************************************************************************/
// ShowImageView
// Written by Fernando Francisco de Oliveira, Michael Wilber
//
// ShowImageView.h
//
//
// Copyright (c) 2003 OpenBeOS Project
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

#ifndef _ShowImageView_h
#define _ShowImageView_h

#include <View.h>

class ShowImageView : public BView {
public:
	ShowImageView(BRect rect, const char *name, uint32 resizingMode,
		uint32 flags);
	~ShowImageView();
	
	void SetBitmap(BBitmap *pbitmap);
	BBitmap *GetBitmap();
	
	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);
	virtual void FrameResized(float width, float height);
	virtual void MessageReceived(BMessage *pmsg);
	
	void FixupScrollBars();
	
private:
	BBitmap *fpbitmap;
};

#endif /* _ShowImageView_h */
