// AlertTestWindow.h

#ifndef ALERT_TEST_WINDOW_H
#define ALERT_TEST_WINDOW_H

#include <Window.h>
#include <Button.h>
#include <StringView.h>

#define MSG_RUN_BUTTON 'mRNB'

class AlertTestWindow : public BWindow {
public:
	AlertTestWindow(BRect frame);
	virtual void MessageReceived(BMessage *message);
	virtual bool QuitRequested();

private:
	void Test();
	
	char fAlertType;
	BButton *fRunButton;
	BStringView *fTitleView;
};

#endif // #ifndef ALERT_TEST_WINDOW_H

