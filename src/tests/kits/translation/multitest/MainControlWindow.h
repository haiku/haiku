// MainControlWindow.h

#ifndef MAIN_CONTROL_WINDOW_H
#define MAIN_CONTROL_WINDOW_H

#include <Window.h>
#include <Button.h>

#define ADD_VIEW_BUTTON_ID 1
#define ADD_TRANSLATORS_BUTTON_ID 2

class MainControlWindow : public BWindow {
public:
	MainControlWindow();
	
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage *pMsg);
	
private:
	BButton *fbtnAddView;
	BButton *fbtnAddTranslators;
};

#endif