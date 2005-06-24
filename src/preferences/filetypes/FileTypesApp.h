#ifndef FILE_TYPES_APP
#define FILE_TYPES_APP

#include <Application.h>
#include <Message.h>
#include <FilePanel.h>

class FileTypesWindow;

class FileTypesApp
	: public BApplication
{
public:
					FileTypesApp();
	virtual void 	MessageReceived(BMessage *message);
	virtual	void	ArgvReceived(int32 argc, char ** argv) { BApplication::ArgvReceived(argc, argv); }
	virtual	void	ArgvReceived(int32 argc, const char *argv[], const char * cwd);
	virtual void	RefsReceived(BMessage *message);
	virtual void	ReadyToRun();

	virtual	void	DispatchMessage(BMessage *an_event, BHandler *handler);

private:
	BFilePanel *	OpenPanel();

	FileTypesWindow * fWindow;
	BFilePanel		* fOpenPanel;

	bool			fArgvOkay;
};

extern FileTypesApp * file_types_app;

#endif // FILE_TYPES_APP
