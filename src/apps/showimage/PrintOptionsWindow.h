/*
 * Copyright 2003-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer, laplace@haiku-os.org
 */
#ifndef PRINT_OPTIONS_WINDOW_H
#define PRINT_OPTIONS_WINDOW_H


#include <Messenger.h>
#include <RadioButton.h>
#include <Rect.h>
#include <TextControl.h>
#include <Window.h>


class PrintOptions {
public:
					PrintOptions();
	
	// bounds of the image
	BRect 			Bounds() const { return fBounds; }
	void 			SetBounds(BRect bounds);

	enum Option {
		kFitToPage,
		kZoomFactor,
		kDPI,
		kWidth,
		kHeight,
		kNumberOfOptions
	};
	enum Option		Option() const { return fOption; }
	void 			SetOption(enum Option op) { fOption = op; }
	
	// ZoomFactor = 72.0 / dpi
	float 			ZoomFactor() const { return fZoomFactor; }
	void 			SetZoomFactor(float z);
	float 			DPI() const { return fDPI; }
	void 			SetDPI(float dpi);
	
	// Setting width/height updates height/width to keep aspect ratio
	float 			Width() const { return fWidth; }
	float 			Height() const { return fHeight; }
	void 			SetWidth(float width);
	void 			SetHeight(float height);
	
private:
	BRect 			fBounds;
	enum Option		fOption;
	float 			fZoomFactor;
	float 			fDPI;
	float 			fWidth, fHeight; // 1/72 Inches
};

class PrintOptionsWindow : public BWindow {
public:
								PrintOptionsWindow(BPoint at,
									PrintOptions* options, BWindow* listener);
								~PrintOptionsWindow();

	void 			MessageReceived(BMessage* msg);

private:
	BRadioButton* 				AddRadioButton(const char* name, const char* label,
									uint32 what, bool selected);

	BTextControl* 				AddTextControl(const char* name, const char* label,
									float value, uint32 what);

	void						Setup();
	enum PrintOptions::Option	MsgToOption(uint32 what);
	bool						GetValue(BTextControl* text, float* value);
	void						SetValue(BTextControl* text, float value);
	
	PrintOptions* 				fPrintOptions;
	PrintOptions 				fCurrentOptions;
	BMessenger 					fListener;
	status_t 					fStatus;
	BTextControl* 				fZoomFactor;
	BTextControl* 				fDPI;
	BTextControl* 				fWidth;
	BTextControl* 				fHeight;

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
};


#endif

