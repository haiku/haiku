#ifndef _MINI_APP_H_
#define _MINI_APP_H_

#include <Application.h>

class Arguments;
class MiniWin;

class MiniApp : public BApplication {
public:
							MiniApp(const Arguments &args);
virtual						~MiniApp();

virtual	void				ReadyToRun();

private:
		MiniWin				*fWindow;
};

#endif
