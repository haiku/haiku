#ifndef SCROLL_BAR_WINDOW_H
#define SCROLL_BAR_WINDOW_H

#include <Window.h>

class ScrollBarView;

class ScrollBarWindow : public BWindow {
public:
	ScrollBarWindow(void);
	virtual ~ScrollBarWindow(void);
	virtual bool QuitRequested(void);
	
	ScrollBarView *sbview;
};

#endif // SCROLL_BAR_WINDOW_H
