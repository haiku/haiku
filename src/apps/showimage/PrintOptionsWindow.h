/*****************************************************************************/
// PrintOptionsWindow
// Written by Michael Pfeiffer
//
// PrintOptionsWindow.h
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

#ifndef _PrintOptionsWindow_h
#define _PrintOptionsWindow_h

#include <Window.h>
#include <RadioButton.h>
#include <TextControl.h>
#include <Messenger.h>
#include <Rect.h>

class PrintOptions {
public:
	enum Option {
		kFitToPage,
		kZoomFactor,
		kDPI,
		kWidth,
		kHeight,
		kNumberOfOptions
	};

	PrintOptions();
	
	// bounds of the image
	BRect Bounds() const { return fBounds; }
	void SetBounds(BRect bounds);

	Option Option() const { return fOption; }
	void SetOption(Option op) { fOption = op; }
	// zoomFactor = 72.0 / dpi
	float ZoomFactor() const { return fZoomFactor; }
	void SetZoomFactor(float z);
	float DPI() const { return fDPI; }
	void SetDPI(float dpi);
	// setting width/height updates height/width to keep aspect ratio
	float Width() const { return fWidth; }
	float Height() const { return fHeight; }
	void SetWidth(float width);
	void SetHeight(float height);
	
private:
	BRect fBounds;
	Option fOption;
	float fZoomFactor;
	float fDPI;
	float fWidth, fHeight; // 1/72 Inches
};

class PrintOptionsWindow : public BWindow
{
public:
	PrintOptionsWindow(BPoint at, PrintOptions* options, BWindow* listener);
	~PrintOptionsWindow();

	void MessageReceived(BMessage* msg);

private:
	BRadioButton* AddRadioButton(BView* view, BPoint& at, const char* name, const char* label, uint32 what, bool selected);
	BTextControl* AddTextControl(BView* view, BPoint& at, const char* name, const char* label, float value, float divider, uint32 what);
	void Setup();
	enum PrintOptions::Option MsgToOption(uint32 what);
	bool GetValue(BTextControl* text, float* value);
	void SetValue(BTextControl* text, float value);
	
	enum {
		kMsgOK = 'mPOW',
		kMsgFitToPageSelected,
		kMsgZoomFactorSelected,
		kMsgDPISelected,
		kMsgWidthAndHeightSelected,
		
		kMsgZoomFactorChanged,
		kMsgDPIChanged,
		kMsgWidthChanged,
		kMsgHeightChanged,
		
		kMsgJobSetup,

		kIndent = 5,
		kLineSkip = 5,
	};
	
	PrintOptions* fPrintOptions;
	PrintOptions fCurrentOptions;
	BMessenger fListener;
	status_t fStatus;
	BTextControl* fZoomFactor;
	BTextControl* fDPI;
	BTextControl* fWidth;
	BTextControl* fHeight;
};

#endif
