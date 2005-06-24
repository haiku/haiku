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
	fFadeState(0),fNoFadeState(0), 
	fSampleView(NULL), 
	fTab1(NULL),fTab2(NULL), 
		fTabView(NULL), fModuleSettingsBox(NULL),
		fPreviewDisplay(NULL), fListView1(NULL), 
		fAddonList(NULL), fSelectedAddonFileName(NULL), 
		fCurrentAddon(NULL), fTestButton(NULL), 
		fAddButton(NULL), fEnableScreenSaverBox(NULL), 
		fPasswordSlider(NULL), fTurnOffSlider(NULL),
		fRunSlider(NULL), fStringView1(NULL), 
		fEnableCheckbox(NULL), fPasswordCheckbox(NULL), 
		fTurnOffScreenCheckBox(NULL),
		fTurnOffMinutes(NULL), fRunMinutes(NULL), 
		fPasswordMinutes(NULL), fPasswordButton(NULL), 
		fFadeNowString(NULL),
		fFadeNowString2(NULL), 
		fDontFadeString(NULL), fDontFadeString2(NULL), 
		fFadeNow(NULL),fFadeNever(NULL),
		fPwWin(NULL), 
		fPwMessenger(NULL), fFilePanel(NULL) ,
		fSettingsArea(NULL) {
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
	pwWindow *fPwWin;
	BMessenger *fPwMessenger;
  
	BMessage fSettings;
	BFilePanel *fFilePanel;
	BView *fSettingsArea;
};

#endif // _ScreenSaver_H
