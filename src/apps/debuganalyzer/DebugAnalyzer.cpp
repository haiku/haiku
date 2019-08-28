/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <new>

#include <Application.h>

#include <AutoDeleter.h>

#include "DataSource.h"
#include "MessageCodes.h"

#include "main_window/MainWindow.h"


static const char* const kSignature = "application/x-vnd.Haiku-DebugAnalyzer";


class DebugAnalyzer : public BApplication {
public:
	DebugAnalyzer()
		:
		BApplication(kSignature),
		fWindowCount(0)
	{
	}

	virtual void ReadyToRun()
	{
printf("ReadyToRun()\n");
		if (fWindowCount == 0 && _CreateWindow(NULL) != B_OK)
			PostMessage(B_QUIT_REQUESTED);
	}

	virtual void ArgvReceived(int32 argc, char** argv)
	{
		printf("ArgvReceived()\n");
		for (int32 i = 0; i < argc; i++)
			printf("  arg %" B_PRId32 ": \"%s\"\n", i, argv[i]);

		for (int32 i = 1; i < argc; i++) {
			PathDataSource* dataSource = new(std::nothrow) PathDataSource;
			if (dataSource == NULL) {
				// no memory
				fprintf(stderr, "DebugAnalyzer::ArgvReceived(): Out of "
					"memory!");
				return;
			}

			status_t error = dataSource->Init(argv[i]);
			if (error != B_OK) {
				fprintf(stderr, "Failed to create data source for path "
					"\"%s\": %s\n", argv[i], strerror(error));
				// TODO: Alert!
				continue;
			}

			_CreateWindow(dataSource);
		}

	}

	virtual void RefsReceived(BMessage* message)
	{
printf("RefsReceived()\n");
	}

private:
	status_t _CreateWindow(DataSource* dataSource)
	{
		ObjectDeleter<DataSource> dataSourceDeleter(dataSource);

		MainWindow* window;
		try {
			window = new MainWindow(dataSource);
		} catch (std::bad_alloc&) {
			fprintf(stderr, "DebugAnalyzer::_CreateWindow(): Out of memory!\n");
			return B_NO_MEMORY;
		}

		// the data source is owned by the window now
		dataSourceDeleter.Detach();

		window->Show();

		fWindowCount++;

		return B_OK;
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_WINDOW_QUIT:
				if (--fWindowCount == 0)
					PostMessage(B_QUIT_REQUESTED);
				break;
			default:
				BApplication::MessageReceived(message);
				break;
		}
	}

private:
	int32	fWindowCount;
};


int
main(int argc, const char* const* argv)
{
	DebugAnalyzer app;
	app.Run();

	return 0;
}
