/**StyledEdit: Application class*/
#ifndef STYLED_EDIT_APP
#define STYLED_EDIT_APP

#include <Application.h>
#include <Message.h>
#include <FilePanel.h>

class StyledEditWindow;

class StyledEditApp
	: public BApplication
{
public:
					StyledEditApp();
	virtual void 	MessageReceived(BMessage *message);
			void	ArgvReceived(int32 argc, const char *argv[], const char * cwd);
	virtual void	RefsReceived(BMessage *message);
	virtual void	ReadyToRun();

	virtual	void DispatchMessage(BMessage *an_event, BHandler *handler);

			int32	NumberOfWindows();
			void	OpenDocument();
			void	OpenDocument(entry_ref * ref);
			void	CloseDocument();
	
private:
	BFilePanel		*fOpenPanel;
	BMenu			*fOpenPanelEncodingMenu;
	uint32			fOpenAsEncoding;
	int32			fWindowCount;
	int32			fNext_Untitled_Window;
		
};

extern StyledEditApp * styled_edit_app;

#endif // STYLED_EDIT_APP
