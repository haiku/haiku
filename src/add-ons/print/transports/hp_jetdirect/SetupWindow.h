#ifndef SETUPWINDOW_H
#define SETUPWINDOW_H

#include <Window.h>

class BDirectory;

class SetupWindow : public BWindow {
public:
							SetupWindow(BDirectory* printerDirectory);

	virtual bool 			QuitRequested();
	virtual void 			MessageReceived(BMessage* message);
			int 			Go();

private:
			int  fResult;
			long fExitSem;
};

#endif	// SETUPWINDOW_H
