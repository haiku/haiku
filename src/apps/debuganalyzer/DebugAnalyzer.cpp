/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <stdlib.h>

#include <new>

#include <Application.h>

#include "MainWindow.h"


static const char* const kSignature = "application/x-vnd.Haiku-DebugAnalyzer";


class DebugAnalyzer : public BApplication {
public:
	DebugAnalyzer()
		:
		BApplication(kSignature)
	{
	}

	virtual void ReadyToRun()
	{
		MainWindow* window;
		try {
			window = new MainWindow;
		} catch (std::bad_alloc) {
			exit(1);
		}

		window->Show();
	}

private:
};


int
main(int argc, const char* const* argv)
{
	DebugAnalyzer app;
	app.Run();

	return 0;
}
