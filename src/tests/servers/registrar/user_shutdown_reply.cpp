/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <Alert.h>
#include <Application.h>

class TestApp : public BApplication {
public:
	TestApp()
		: BApplication("application/x-vnd.haiku.user-shutdown-reply")
	{
	}

	virtual bool QuitRequested()
	{
		BAlert *alert = new BAlert("Quit App?",
			"Quit application user_shutdown_reply?",
			"Quit", "Cancel", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		int32 result = alert->Go();

		return (result == 0);
	}
};

int
main()
{
	TestApp app;
	app.Run();

	return 0;
}
