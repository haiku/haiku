#ifndef SCROLL_BAR_APP_H
#define SCROLL_BAR_APP_H

#include <Application.h>

class ScrollBarWindow;

class ScrollBarApp : public BApplication
{
	public:
		ScrollBarApp();
		virtual ~ScrollBarApp();
		virtual void ReadyToRun();
	private:
		ScrollBarWindow * window;
};

extern ScrollBarApp * scroll_bar_app;

#endif // SCROLL_BAR_APP_H
