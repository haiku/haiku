#ifndef APR_WINDOW_H
#define APR_WINDOW_H

#include <Application.h>
#include <Window.h>
#include <Message.h>
#include <TabView.h>

class APRView;
class DecView;
class CurView;

class APRWindow : public BWindow 
{
public:
	APRWindow(BRect frame); 
	virtual	bool QuitRequested();
	virtual void WorkspaceActivated(int32 wkspc, bool is_active);
	virtual void MessageReceived(BMessage *msg);
	BTabView *tabview;
	APRView *colors;
	DecView *decorators;
	CurView *cursors;
};

#endif
