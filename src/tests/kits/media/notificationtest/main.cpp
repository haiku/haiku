/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <stdio.h>
#include <List.h>
#include <Invoker.h>
#include <Alert.h>
#include <Application.h>
#include <MediaRoster.h>

BMediaRoster *roster;
BList watchlist;
bool globalwatch = true;
bool nodewatch = true;

void InitWatch();
void ExitWatch();
void StartWatch(media_node node);
void StopWatch(media_node node);

void InitWatch()
{
	if (globalwatch) {
		status_t rv;
		rv = roster->StartWatching(be_app_messenger);
		if (rv != B_OK) {
			printf("Globalwatch: StartWatching failed. result = %#lx\n",rv);
			return;
		}
	}
	if (nodewatch) {
		live_node_info out_live_nodes[100];
		int32 io_total_count = 100;
		status_t rv;

		rv = roster->GetLiveNodes(out_live_nodes, &io_total_count);
		if (rv != B_OK) {
			printf("GetLiveNodes failed. result = %#lx\n",rv);
			return;
		}
		
		for (int i = 0; i < io_total_count; i++)
			StartWatch(out_live_nodes[i].node);
	}
}

void ExitWatch()
{
	if (globalwatch) {
		status_t rv;
		rv = roster->StopWatching(be_app_messenger);
		if (rv != B_OK) {
			printf("Globalwatch: StopWatching failed. result = %#lx\n",rv);
			return;
		}
	}
	if (nodewatch) {
		media_node *mn;
		while ((mn = (media_node *) watchlist.ItemAt(0)) != 0)
			StopWatch(*mn);
	}
}

void StartWatch(media_node node)
{
	if (nodewatch) {
		status_t rv;
		rv = roster->StartWatching(be_app_messenger, node, B_MEDIA_WILDCARD);
		if (rv != B_OK) {
			printf("Nodewatch: StartWatching failed. result = %#lx\n",rv);
			return;
		}
		media_node *mn = new media_node;
		*mn = node;
		watchlist.AddItem(mn);
	}
}

void StopWatch(media_node node)
{
	if (nodewatch) {
		status_t rv;
		rv = roster->StopWatching(be_app_messenger, node, B_MEDIA_WILDCARD);
		if (rv != B_OK) {
			printf("Nodewatch: StopWatching failed. result = %#lx\n",rv);
			return;
		}
		media_node *mn;
		for (int i = 0; (mn = (media_node *) watchlist.ItemAt(i)) != 0; i++) {
			if (*mn == node) {
				watchlist.RemoveItem(mn);
				delete mn;
				break;
			}
		}
	}
}

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
	globalwatch = (1 == (new BAlert("","Global watching?","No","Yes"))->Go());
	nodewatch = (1 == (new BAlert("","Node watching?","No","Yes"))->Go());
	printf("Global watching : %s\n",globalwatch ? "Yes" : "No");
	printf("Node watching   : %s\n",nodewatch ? "Yes" : "No");
	InitWatch();
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
				ExitWatch();
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

