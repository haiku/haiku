// NodeMonitor.cpp

#include <new>

#include <Message.h>
#include <Node.h>
#include <NodeMonitor.h>

#include "AutoLocker.h"
#include "Debug.h"
#include "NodeMonitor.h"
#include "NodeMonitoringEvent.h"

// node monitor constants
static const int32 kDefaultNodeMonitorLimit = 4096;
static const int32 kNodeMonitorLimitIncrement = 512;

// private BeOS syscall to set the node monitor slot limit
extern "C" int _kset_mon_limit_(int num);

// constructor
NodeMonitor::NodeMonitor(NodeMonitorListener* listener)
	: BLooper("node monitor", B_DISPLAY_PRIORITY, 1000),
	  fListener(listener),
	  fCurrentNodeMonitorLimit(kDefaultNodeMonitorLimit)
{
	// set the initial limit -- just to be sure
	_kset_mon_limit_(fCurrentNodeMonitorLimit);

	// start volume watching
	watch_node(NULL, B_WATCH_MOUNT, this);
}

// destructor
NodeMonitor::~NodeMonitor()
{
	// stop volume watching
	stop_watching(this);
}

// MessageReceived
void
NodeMonitor::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
		{
			NodeMonitoringEvent* event = NULL;
			int32 opcode;
			if (message->FindInt32("opcode", &opcode) == B_OK) {
				switch (opcode) {
					case B_ENTRY_CREATED:
						event = new(nothrow) EntryCreatedEvent;
						break;
					case B_ENTRY_REMOVED:
						event = new(nothrow) EntryRemovedEvent;
						break;
					case B_ENTRY_MOVED:
						event = new(nothrow) EntryMovedEvent;
						break;
					case B_STAT_CHANGED:
						event = new(nothrow) StatChangedEvent;
						break;
					case B_ATTR_CHANGED:
						event = new(nothrow) AttributeChangedEvent;
						break;
					case B_DEVICE_MOUNTED:
						event = new(nothrow) VolumeMountedEvent;
						break;
					case B_DEVICE_UNMOUNTED:
						event = new(nothrow) VolumeUnmountedEvent;
						break;
				}
			}
			if (event) {
				if (event->Init(message) == B_OK)
					fListener->ProcessNodeMonitoringEvent(event);
				else
					delete event;
			}
			break;
		}
		default:
			BLooper::MessageReceived(message);
	}
}

// StartWatching
status_t
NodeMonitor::StartWatching(const node_ref& ref)
{
	uint32 flags = B_WATCH_ALL;
	status_t error = watch_node(&ref, flags, this);
	// If starting to watch the node fail, we allocate more node
	// monitoring slots and try again.
	if (error != B_OK) {
		error = _IncreaseLimit();
		if (error == B_OK)
			error = watch_node(&ref, flags, this);
	}
if (error == B_OK) {
PRINT(("NodeMonitor: started watching node: (%ld, %lld)\n", ref.device,
ref.node));
}
	return error;
}

// StopWatching
status_t
NodeMonitor::StopWatching(const node_ref& ref)
{
PRINT(("NodeMonitor: stopped watching node: (%ld, %lld)\n", ref.device,
ref.node));
	return watch_node(&ref, B_STOP_WATCHING, this);
}

// _IncreaseLimit
status_t
NodeMonitor::_IncreaseLimit()
{
	AutoLocker<BLooper> _(this);

	int32 newLimit = fCurrentNodeMonitorLimit + kNodeMonitorLimitIncrement;
	status_t error = _kset_mon_limit_(newLimit);
	if (error == B_OK)
		fCurrentNodeMonitorLimit = newLimit;
	return error;
}


// #pragma mark -

// constructor
NodeMonitorListener::NodeMonitorListener()
{
}

// destructor
NodeMonitorListener::~NodeMonitorListener()
{
}
