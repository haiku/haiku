/**StyledEdit: Application class*/
#ifndef STYLED_EDIT_APP
#define STYLED_EDIT_APP

#ifndef _APPLICATION_H
#include <Application.h>
#endif

#ifndef STYLED_EDIT_WINDOW_H
#include "StyledEditWindow.h"
#endif

class StyledEditApp: public BApplication{
	public:
						StyledEditApp();
		virtual void 	MessageReceived(BMessage *message);
		virtual void	RefsReceived(BMessage *message);
				int32	NumberOfWindows();
						  
		
	private:
		BFilePanel			*fOpenPanel;
		int32				fWindowCount;
		int32				fNext_Untitled_Window;
		
};
#endif





