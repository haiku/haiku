/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>

#include <Application.h>
#include <Entry.h>
#include <NodeMonitor.h>


class Application : public BApplication {
public:
								Application();
	virtual						~Application();

protected:
	virtual	void				ArgvReceived(int32 argCount, char** args);
	virtual void				ReadyToRun();
	virtual	void				MessageReceived(BMessage* message);

private:
			bool				fWatchingNode;
};


Application::Application()
	:
	BApplication("application/x-vnd.test-node-monitor-test"),
	fWatchingNode(false)
{
}


Application::~Application()
{
}


void
Application::ArgvReceived(int32 argCount, char** args)
{
	uint32 flags = B_WATCH_STAT;

	for (int32 i = 0; i < argCount; i++) {
		BEntry entry(args[i]);
		if (!entry.Exists()) {
			fprintf(stderr, "Entry does not exist: %s\n", args[i]);
			continue;
		}

		node_ref nodeRef;
		entry.GetNodeRef(&nodeRef);
		if (watch_node(&nodeRef, flags, this) == B_OK)
			fWatchingNode = true;
	}
}


void
Application::ReadyToRun()
{
	if (!fWatchingNode)
		Quit();
}


void
Application::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
			message->PrintToStream();
			break;

		default:
			BApplication::MessageReceived(message);
	}
}


// #pragma mark -


int
main(int argc, char** argv)
{
	Application app;
	app.Run();

	return 0;
}
