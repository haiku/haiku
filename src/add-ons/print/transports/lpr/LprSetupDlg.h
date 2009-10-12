// Sun, 18 Jun 2000
// Y.Takagi

#ifndef __LprSetupDlg_H
#define __LprSetupDlg_H

#include <Window.h>

class BDirectory;

class LprSetupDlg : public BWindow {
public:
					LprSetupDlg(BDirectory *);
					~LprSetupDlg() {}
	virtual bool	QuitRequested();
	virtual void	MessageReceived(BMessage *message);
			int 	Go();

private:
	int    fResult;
	sem_id fExitSemaphore;
};

#endif	// __LprSetupDlg_H
