/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>


class DelayShutdownApp : public BApplication {
public:
	DelayShutdownApp()
		:
		BApplication("application/x-vnd.Haiku-DelayShutdown"),
		fDelay(5LL),
		fQuit(false)
	{
	}

	virtual	void ArgvReceived(int32 argc, char* argv[])
	{
		if (argc <= 1) {
			_PrintUsageAndQuit();
			return;
		}

		int32 index = 1;

		while (index < argc) {
			if (strcmp(argv[index], "-d") == 0) {
				if (index + 1 < argc)
					fDelay = atoi(argv[++index]);
				else {
					_PrintUsageAndQuit();
					return;
				}
			} else if (strcmp(argv[index], "-q") == 0) {
				fQuit = true;
			} else {
				_PrintUsageAndQuit();
				return;
			}
			index++;
		}
	}

	virtual bool QuitRequested()
	{
		snooze(fDelay * 1000000);
		return fQuit;
	}

private:
	void _PrintUsageAndQuit()
	{
		printf("usage:\n"
			"\t-d <x>  - wait x seconds before replying the quit\n"
			"\t          request.\n"
			"\t-q      - respond positively to the quit request\n"
			"\t          (default: don't quit).\n");
		PostMessage(B_QUIT_REQUESTED);
	}

	bigtime_t	fDelay;
	bool		fQuit;
};


int
main(int argc, char* argv[])
{
	DelayShutdownApp app;
	app.Run();

	return 0;
}
