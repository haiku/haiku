/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <stdio.h>
#include <Invoker.h>
#include <Alert.h>
#include <Application.h>
#include <MediaRoster.h>

BMediaRoster *roster;

class App : public BApplication
{
public:
	App(const char *sig);
	void ReadyToRun();
	void MessageReceived(BMessage *msg);
};

App::App(const char *sig)
 :	BApplication(sig)
{
}

void
App::ReadyToRun()
{
	roster = BMediaRoster::Roster();
	printf("start watching result = %#x\n",roster->StartWatching(be_app_messenger));
	(new BAlert("","Click Quit to gracefully quit or Abort to abort :-)","Quit","Abort"))->Go(new BInvoker(new BMessage('quit'),be_app_messenger));
}

void
App::MessageReceived(BMessage *msg)
{
	if (msg->what == 0x42524157) // eat this
		return;

	printf("\n");
	switch (msg->what) {
		case 'quit':
			if (0 == msg->FindInt32("which")) // Quit
				printf("stop watching result = %#x\n",roster->StopWatching(be_app_messenger));
			be_app->PostMessage(B_QUIT_REQUESTED);
			return;
		case BMediaNode::B_NODE_FAILED_START:
			printf("BMediaNode::B_NODE_FAILED_START:\n");
			break;
		case BMediaNode::B_NODE_FAILED_STOP:
			printf("BMediaNode::B_NODE_FAILED_STOP:\n");
			break;
		case BMediaNode::B_NODE_FAILED_SEEK:
			printf("BMediaNode::B_NODE_FAILED_SEEK:\n");
			break;
		case BMediaNode::B_NODE_FAILED_SET_RUN_MODE:
			printf("BMediaNode::B_NODE_FAILED_SET_RUN_MODE:\n");
			break;
		case BMediaNode::B_NODE_FAILED_TIME_WARP:
			printf("BMediaNode::B_NODE_FAILED_TIME_WARP:\n");
			break;
		case BMediaNode::B_NODE_FAILED_PREROLL:
			printf("BMediaNode::B_NODE_FAILED_PREROLL:\n");
			break;
		case BMediaNode::B_NODE_FAILED_SET_TIME_SOURCE_FOR:
			printf("BMediaNode::B_NODE_FAILED_SET_TIME_SOURCE_FOR:\n");
			break;
		case BMediaNode::B_NODE_IN_DISTRESS:
			printf("BMediaNode::B_NODE_IN_DISTRESS:\n");
			break;
		case B_MEDIA_NODE_CREATED:
			printf("B_MEDIA_NODE_CREATED:\n");
			break;
		case B_MEDIA_NODE_DELETED:
			printf("B_MEDIA_NODE_DELETED:\n");
			break;
		case B_MEDIA_CONNECTION_MADE:
			printf("B_MEDIA_CONNECTION_MADE:\n");
			break;
		case B_MEDIA_CONNECTION_BROKEN:
			printf("B_MEDIA_CONNECTION_BROKEN:\n");
			break;
		case B_MEDIA_BUFFER_CREATED:
			printf("B_MEDIA_BUFFER_CREATED:\n");
			break;
		case B_MEDIA_BUFFER_DELETED:
			printf("B_MEDIA_BUFFER_DELETED:\n");
			break;
		case B_MEDIA_TRANSPORT_STATE:
			printf("B_MEDIA_TRANSPORT_STATE:\n");
			break;
		case B_MEDIA_PARAMETER_CHANGED:
			printf("B_MEDIA_PARAMETER_CHANGED:\n");
			break;
		case B_MEDIA_FORMAT_CHANGED:
			printf("B_MEDIA_FORMAT_CHANGED:\n");
			break;
		case B_MEDIA_WEB_CHANGED:
			printf("B_MEDIA_WEB_CHANGED:\n");
			break;
		case B_MEDIA_DEFAULT_CHANGED:
			printf("B_MEDIA_DEFAULT_CHANGED:\n");
			break;
		case B_MEDIA_NEW_PARAMETER_VALUE:
			printf("B_MEDIA_NEW_PARAMETER_VALUE:\n");
			break;
		case B_MEDIA_NODE_STOPPED:
			printf("B_MEDIA_NODE_STOPPED:\n");
			break;
		case B_MEDIA_FLAVORS_CHANGED:
			printf("B_MEDIA_FLAVORS_CHANGED:\n");
			break;
		default:
			printf("unknown message:\n");
	}
	msg->PrintToStream();
}

int main()
{
	App app("application/x-vnd.OpenBeOS-NotificationTest");	
	app.Run();
	return 0;
}

