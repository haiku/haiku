/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>

class TestApp : public BApplication {
public:
	TestApp()
		: BApplication("application/x-vnd.haiku.negative-shutdown-reply")
	{
	}

	virtual bool QuitRequested()
	{
		return false;
	}
};

int
main()
{
	TestApp app;
	app.Run();

	return 0;
}
