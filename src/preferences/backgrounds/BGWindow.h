/*

"Backgrounds" by Jerome Duval (jerome.duval@free.fr)

(C)2002 OpenBeOS under MIT license

*/

#ifndef BG_WINDOW_H
#define BG_WINDOW_H

#include <Application.h>
#include <Window.h>
#include "BGView.h"

#define WINDOW_TITLE				"Backgrounds"

class BGWindow : public BWindow 
{
public:
	BGWindow(BRect frame, bool standalone = true);
	BGView *GetView() {return fBGView;}
	void ProcessRefs(entry_ref dir_ref, BMessage* msg);
protected:
	virtual	bool QuitRequested();
	virtual void WorkspaceActivated(int32 oldWorkspaces, bool active);
	BGView *fBGView;
	bool fIsStandalone;
};

#endif
