#ifndef _MINI_WIN_H_
#define _MINI_WIN_H_

#include <Window.h>

class MiniView;

class MiniWin : public BWindow {
public:
							MiniWin(BRect bounds);
virtual						~MiniWin();

		MiniView			*View();

private:
		MiniView			*fView;
};

#endif
