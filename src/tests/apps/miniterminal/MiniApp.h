#ifndef _MINI_APP_H_
#define _MINI_APP_H_

#include <Application.h>

class MiniWin;

class MiniApp : public BApplication {
public:
							MiniApp(BRect bounds);
virtual						~MiniApp();

virtual	void				ReadyToRun();

private:
		MiniWin				*fWindow;
};

#endif