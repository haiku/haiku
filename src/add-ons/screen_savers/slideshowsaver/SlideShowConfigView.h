/*****************************************************************************/
// SlideShowConfigView
// Written by Michael Wilber
//
// SlideShowConfigView.h
//
// This BView based object displays the SlideShowSaver settings options
//
//
// Copyright (C) Haiku
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

#ifndef SLIDE_SHOW_CONFIG_VIEW_H
#define SLIDE_SHOW_CONFIG_VIEW_H

#include <View.h>
#include <CheckBox.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Button.h>
#include <FilePanel.h>
#include "LiveSettings.h"

// SlideShowSaver Settings
enum {
	CHANGE_CAPTION,
	CHANGE_BORDER,
	CHOOSE_DIRECTORY, CHANGE_DIRECTORY,
	CHANGE_DELAY
};
#define SAVER_SETTING_CAPTION	"slidesaver /caption"
#define SAVER_SETTING_BORDER	"slidesaver /border"
#define SAVER_SETTING_DIRECTORY	"slidesaver /directory"
#define SAVER_SETTING_DELAY		"slidesaver /delay"

class SlideShowConfigView : public BView {
public:
	SlideShowConfigView(const BRect &frame, const char *name, uint32 resize,
		uint32 flags, LiveSettings *settings);
		// sets up the view
		
	~SlideShowConfigView();
		// releases the settings
		
	virtual void AllAttached();
	virtual void MessageReceived(BMessage *message);

	virtual	void Draw(BRect area);
		// draws information about the slide show screen saver
private:
	BCheckBox *fShowCaption;
	BCheckBox *fShowBorder;
	BPopUpMenu *fDelayMenu;
	BMenuField *fDelayMenuField;
	BButton *fChooseFolder;
	
	BFilePanel *fFilePanel;
	
	LiveSettings *fSettings;
		// the actual settings for the screen saver,
		// shared with the screen saver
};

#endif // #ifndef SLIDE_SHOW_CONFIG_VIEW_H
