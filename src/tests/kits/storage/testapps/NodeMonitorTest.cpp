/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Entry.h>
#include <File.h>
#include <Looper.h>
#include <NodeMonitor.h>

#include <stdio.h>
#include <string.h>


class Looper : public BLooper {
	public:
		Looper(node_ref& nodeRef);
		virtual ~Looper();

		virtual void MessageReceived(BMessage* message);

	private:
		node_ref	fNodeRef;
};


Looper::Looper(node_ref& nodeRef)
	:
	fNodeRef(nodeRef)
{
	status_t status = watch_node(&fNodeRef, B_WATCH_DIRECTORY, this);
	if (status != B_OK) {
		fprintf(stderr, "Could not watch file.\n");
		PostMessage(B_QUIT_REQUESTED);
	}
}


Looper::~Looper()
{
	status_t status = watch_node(&fNodeRef, B_STOP_WATCHING, this);
	if (status != B_OK)
		fprintf(stderr, "Could not stop watching: %s\n", strerror(status));
}


void
Looper::MessageReceived(BMessage* message)
{
	if (message->what == B_NODE_MONITOR)
		message->PrintToStream();
}


int
main()
{
	BEntry entry("/tmp", true);
	node_ref nodeRef;
	if (entry.GetNodeRef(&nodeRef) != B_OK) {
		fprintf(stderr, "Could not open /tmp.\n");
		return -1;
	}

	//printf("device: %ld, node: %Ld\n", nodeRef.device, nodeRef.node);

	// start looper
	Looper& looper = *new Looper(nodeRef);
	looper.Run();

	// run tests
	entry.SetTo("/tmp/_watch_test_");

	BFile file;
	status_t status = file.SetTo(&entry, B_CREATE_FILE | B_READ_WRITE);
	if (status != B_OK)
		fprintf(stderr, "could not create watch test.\n");
	file.Write("test", 4);
	file.Unset();

	if (entry.Remove() != B_OK)
		fprintf(stderr, "could not remove watch test.\n");

	snooze(500000LL);

	// quit looper
	looper.PostMessage(B_QUIT_REQUESTED);
	wait_for_thread(looper.Thread(), &status);

	return 0;
}
