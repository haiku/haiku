#ifndef APR_WINDOW_H
#define APR_WINDOW_H

#include <Application.h>
#include <Window.h>
#include <Message.h>
#include <TabView.h>

class APRView;
class DecView;

class APRWindow : public BWindow 
{
public:
	APRWindow(BRect frame); 
	virtual	bool QuitRequested();
	virtual void WorkspaceActivated(int32 wkspc, bool is_active);
	BTabView *tabview;
	APRView *colors;
	DecView *decorators;
};

#endif
