/*
 * Copyright 2003, Michael Phipps. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _ScreenSaverWindow_H
#define _ScreenSaverWindow_H
#include <FilePanel.h>
#include <Picture.h>
#include "PasswordWindow.h"
#include "ScreenSaverPrefs.h"

class MouseAreaView;
class PreviewView;

class ScreenSaverWin: public BWindow {
public:
	ScreenSaverWin();
	virtual void MessageReceived(BMessage *message);
	virtual bool QuitRequested();
	void PopulateScreenSaverList();
	void LoadSettings();
	virtual ~ScreenSaverWin();

private:
	void SetupForm();
	void SetupTab1();
	void SetupTab2();
	void UpdateStatus();
	void SaverSelected();

	static int CompareSSListItems(const void* left, const void* right);

	ScreenSaverPrefs fPrefs;
	int fFadeState,fNoFadeState;
	BView *fSampleView;
  
	BView *fTab1,*fTab2;
	BTabView *fTabView;
	BBox *fModuleSettingsBox;

	PreviewView *fPreviewDisplay;
	BListView *fListView1;
	BList *fAddonList;
	BString fSelectedAddonFileName;
	image_id fCurrentAddon;
  
	BButton *fTestButton;
	BButton *fAddButton;
	BBox *fEnableScreenSaverBox;
	BSlider *fPasswordSlider;
	BSlider *fTurnOffSlider;
	BSlider *fRunSlider;
	BStringView *fStringView1;
	BCheckBox *fEnableCheckbox;
	BCheckBox *fPasswordCheckbox;
	BCheckBox *fTurnOffScreenCheckBox;
	BStringView *fTurnOffMinutes;
	BStringView *fRunMinutes;
	BStringView *fPasswordMinutes;
	BButton *fPasswordButton;
	BStringView *fFadeNowString;
	BStringView *fFadeNowString2;
	BStringView *fDontFadeString;
	BStringView *fDontFadeString2;
	BPicture fSamplePicture;
	MouseAreaView *fFadeNow,*fFadeNever;
	PasswordWindow *fPwWin;
	BMessenger *fPwMessenger;
  
	BMessage fSettings;
	BFilePanel *fFilePanel;
	BView *fSettingsArea;
};

#endif // _ScreenSaverWindow_H
