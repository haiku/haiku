#ifndef SETUPWINDOW_H
#define SETUPWINDOW_H

#include <Window.h>

class BDirectory;

class SetupWindow : public BWindow {
public:
	SetupWindow(BDirectory *);
	~SetupWindow() {}
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage *message);
	int Go();

private:
	int  result;
	long semaphore;
};

#endif	// SETUPWINDOW_H
