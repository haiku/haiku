//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <Application.h>
#include <Message.h>
#include <Messenger.h>
#include <NodeMonitor.h>
#include <ObjectList.h>

// Monitor
class Monitor : public BHandler {
public:
	Monitor(BMessenger target);
	virtual ~Monitor();

	virtual void MessageReceived(BMessage *message);

	BMessenger Target() const { return fTarget; }

private:
	BMessenger	fTarget;
};

// NodeMonitorApp
class NodeMonitorApp : public BApplication {
public:
	NodeMonitorApp();
	virtual ~NodeMonitorApp();

	virtual void MessageReceived(BMessage *message);

private:
	BObjectList<Monitor>	fMonitors;
};


// Monitor

// constructor
Monitor::Monitor(BMessenger target)
	: BHandler(),
	  fTarget(target)
{
}

// destructor
Monitor::~Monitor()
{
}

// MessageReceived
void
Monitor::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
			fTarget.SendMessage(message);
			break;
	}
}


// NodeMonitorApp

// constructor
NodeMonitorApp::NodeMonitorApp()
	: BApplication("application/x-vnd.obos-NodeMonitor"),
	  fMonitors(10, true)
{
}

// destructor
NodeMonitorApp::~NodeMonitorApp()
{
	Lock();
	for (int32 i = 0; Monitor *monitor = fMonitors.ItemAt(i); i++)
		RemoveHandler(monitor);
	Unlock();
}

// MessageReceived
void
NodeMonitorApp::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case 'wtch':
		{
			status_t error = B_BAD_VALUE;
			node_ref ref;
			BMessenger target;
			uint32 flags;
			if (message->FindInt32("device", &ref.device) == B_OK
				&& message->FindInt64("node", &ref.node) == B_OK
				&& message->FindMessenger("target", &target) == B_OK
				&& message->FindInt32("flags", (int32*)&flags) == B_OK) {
				Monitor *monitor = new Monitor(target);
				fMonitors.AddItem(monitor);
				AddHandler(monitor);
				error = watch_node(&ref, flags, monitor);
				if (error != B_OK) {
					RemoveHandler(monitor);
					fMonitors.RemoveItem(monitor);
				}
			}
			BMessage reply(B_SIMPLE_DATA);
			reply.AddInt32("result", error);
			message->SendReply(&reply);
			break;
		}
		case 'hctw':
		{
			status_t error = B_BAD_VALUE;
			BMessenger target;
			if (message->FindMessenger("target", &target) == B_OK) {
				error = B_OK;
				for (int32 i = fMonitors.CountItems();
					 Monitor *monitor = fMonitors.ItemAt(i);
					 i--) {
					if (monitor->Target() == target) {
						error = stop_watching(monitor->Target());
						RemoveHandler(monitor);
						delete fMonitors.RemoveItemAt(i);
					}
				}
			}
			BMessage reply(B_SIMPLE_DATA);
			reply.AddInt32("result", error);
			message->SendReply(&reply);
			break;
		}
	}
}


// main
int
main()
{
	NodeMonitorApp app;
	app.Run();
	return 0;
}

