#ifndef FILE_TYPE_APP
#define FILE_TYPE_APP

#include <Application.h>
#include <FilePanel.h>

class FileTypeWindow;

class FileTypeApp
	: public BApplication
{
public:
					FileTypeApp();
	virtual void 	MessageReceived(BMessage *message);
			void	ArgvReceived(int32 argc, const char *argv[], const char * cwd);
	virtual void	RefsReceived(BMessage *message);
	virtual void	ReadyToRun();

	virtual	void	DispatchMessage(BMessage *an_event, BHandler *handler);

private:
	BFilePanel *	OpenPanel();
	void			PrintUsage(const char * execname);

	BWindow			* fWindow;
	BFilePanel		* fOpenPanel;

	bool			fArgvOkay;
};

extern FileTypeApp * file_type_app;

#endif // FILE_TYPE_APP
