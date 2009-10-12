// Sun, 18 Jun 2000
// Y.Takagi

#ifndef __LprSetupDlg_H
#define __LprSetupDlg_H

#include <Window.h>

#include "DialogWindow.h"

class BDirectory;
class LprSetupView;


class LprSetupDlg : public DialogWindow {
public:
					LprSetupDlg(BDirectory *);
					~LprSetupDlg() {}
	virtual void	MessageReceived(BMessage *message);

private:
	LprSetupView*	fSetupView;
};

#endif	// __LprSetupDlg_H
