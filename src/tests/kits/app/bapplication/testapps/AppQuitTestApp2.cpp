// AppQuitTestApp2.cpp

#include <stdio.h>

#include <OS.h>

#include "CommonTestApp.h"

enum {
	MSG_QUIT	= 'quit',
};

class Quitter : public BHandler {
public:
	virtual void MessageReceived(BMessage *message)
	{
		if (message->what == MSG_QUIT)
			be_app->Quit();
	}
};

int
main()
{
// R5: doesn't set the error variable in case of success
#ifdef TEST_R5
	status_t error = B_OK;
#else
	status_t error = B_ERROR;
#endif
	CommonTestApp *app = new CommonTestApp(
		"application/x-vnd.obos-app-quit-testapp1", &error);
	init_connection();
	report("error: %lx\n", error);
	report("InitCheck(): %lx\n", app->InitCheck());
	app->SetReportDestruction(true);
	if (error == B_OK) {
		app->SetMessageHandler(new Quitter);
		app->PostMessage(MSG_QUIT, app);
		app->Run();
	}
	delete app;
	return 0;
}

