#ifndef MEDIA_PLAYER_APP
#define MEDIA_PLAYER_APP

#include <Application.h>
#include <Message.h>
#include <FilePanel.h>

class MediaPlayerApp
	: public BApplication
{
public:
					MediaPlayerApp();
	virtual void 	MessageReceived(BMessage *message);
	virtual	void	ArgvReceived(int32 argc, const char *argv[], const char * cwd);
	virtual void	RefsReceived(BMessage *message);
	virtual void	ReadyToRun();

	virtual	void	DispatchMessage(BMessage *an_event, BHandler *handler);

			int32	NumberOfWindows();
			void	OpenDocument();
			void	OpenDocument(entry_ref * ref);
			void	CloseDocument();
	
private:
	BFilePanel		*fOpenPanel;
	int32			fWindowCount;
		
};

extern MediaPlayerApp * media_player_app;

#endif // MEDIA_PLAYER_APP
