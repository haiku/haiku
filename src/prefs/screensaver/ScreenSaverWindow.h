#ifndef _ScreenSaver_H
#define _ScreenSaver_H
#include <FilePanel.h>
#include <Picture.h>
#include "Constants.h"
#include "passwordWindow.h"
#include "ScreenSaverPrefs.h"

class MouseAreaView;
class PreviewView;

class ScreenSaverWin: public BWindow {
public:
	ScreenSaverWin(void) : BWindow(BRect(50,50,500,385),"OBOS Screen Saver Preferences",B_TITLED_WINDOW,B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_RESIZABLE) , 
	fadeState(0),noFadeState(0), 
	sampleView(NULL), 
	tab1(NULL),tab2(NULL), 
		tabView(NULL), ModuleSettingsBox(NULL),
		previewDisplay(NULL), ListView1(NULL), 
		AddonList(NULL), SelectedAddonFileName(NULL), 
		currentAddon(NULL), TestButton(NULL), 
		AddButton(NULL), EnableScreenSaverBox(NULL), 
		PasswordSlider(NULL), TurnOffSlider(NULL),
		RunSlider(NULL), StringView1(NULL), 
		EnableCheckbox(NULL), PasswordCheckbox(NULL), 
		TurnOffScreenCheckBox(NULL),
		TurnOffMinutes(NULL), RunMinutes(NULL), 
		PasswordMinutes(NULL), PasswordButton(NULL), 
		FadeNowString(NULL),
		FadeNowString2(NULL), 
		DontFadeString(NULL), DontFadeString2(NULL), 
		fadeNow(NULL),fadeNever(NULL),
		pwWin(NULL), 
		pwMessenger(NULL), filePanel(NULL) ,
		settingsArea(NULL) {
	SetupForm();
	}
	virtual void MessageReceived(BMessage *message);
	virtual bool QuitRequested(void);
	void populateScreenSaverList(void);
	void LoadSettings(void);
	virtual ~ScreenSaverWin(void) {};

private:
	void SetupForm(void);
	void setupTab1(void);
	void setupTab2(void);
	void updateStatus(void);
	void SaverSelected(void);

	ScreenSaverPrefs prefs;
	int fadeState,noFadeState;
	BView *sampleView;
  
	BView *tab1,*tab2;
	BTabView *tabView;
	BBox *ModuleSettingsBox;

	PreviewView *previewDisplay;
	BListView *ListView1;
	BList *AddonList;
	BString SelectedAddonFileName;
	image_id currentAddon;
  
	BButton *TestButton;
	BButton *AddButton;
	BBox *EnableScreenSaverBox;
	BSlider *PasswordSlider;
	BSlider *TurnOffSlider;
	BSlider *RunSlider;
	BStringView *StringView1;
	BCheckBox *EnableCheckbox;
	BCheckBox *PasswordCheckbox;
	BCheckBox *TurnOffScreenCheckBox;
	BStringView *TurnOffMinutes;
	BStringView *RunMinutes;
	BStringView *PasswordMinutes;
	BButton *PasswordButton;
	BStringView *FadeNowString;
	BStringView *FadeNowString2;
	BStringView *DontFadeString;
	BStringView *DontFadeString2;
	BPicture samplePicture;
	MouseAreaView *fadeNow,*fadeNever;
	pwWindow *pwWin;
	BMessenger *pwMessenger;
  
	BMessage settings;
	BFilePanel *filePanel;
	BView *settingsArea;
};

#endif // _ScreenSaver_H
