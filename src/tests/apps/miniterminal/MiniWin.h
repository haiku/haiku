#ifndef _MINI_WIN_H_
#define _MINI_WIN_H_

#include <Window.h>

class Arguments;
class MiniView;

class MiniWin : public BWindow {
public:
							MiniWin(const Arguments &args);
virtual						~MiniWin();

		MiniView			*View();

private:
		MiniView			*fView;
};

#endif
