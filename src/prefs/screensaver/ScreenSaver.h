#ifndef _ScreenSaver_H
#define _ScreenSaver_H
#include <Picture.h>
#include "Constants.h"
#include "pwWindow.h"

class MouseAreaView;
class PreviewView;

class ScreenSaver: public BWindow {
public:
  ScreenSaver(void) : BWindow(BRect(50,50,500,385),"OBOS Screen Saver Preferences",B_TITLED_WINDOW,B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
  {
	pwWin=NULL;
	sampleView=NULL;
	SetupForm();
  }
  virtual void MessageReceived(BMessage *message);
  virtual bool QuitRequested(void);
  virtual ~ScreenSaver(void) {};

private:
  void SetupForm(void);
  void setupTab1(void);
  void setupTab2(void);
  void updateStatus(void);
  void loadSettings(BMessage *msg);
  void saveSettings(void);

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
  
};

#endif // _ScreenSaver_H
