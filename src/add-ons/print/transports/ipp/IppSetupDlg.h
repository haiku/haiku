// Sun, 18 Jun 2000
// Y.Takagi

#ifndef __IppSetupDlg_H
#define __IppSetupDlg_H

#include <Window.h>

class BDirectory;

class IppSetupDlg : public BWindow {
public:
	IppSetupDlg(BDirectory *);
	~IppSetupDlg() {}
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage *message);
	int Go();

private:
	int  result;
	long semaphore;
};

#endif	// __IppSetupDlg_H
