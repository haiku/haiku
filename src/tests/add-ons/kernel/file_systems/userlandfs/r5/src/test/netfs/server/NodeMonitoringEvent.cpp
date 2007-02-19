// NodeMonitoringEvent.cpp

#include <Message.h>
#include <NodeMonitor.h>

#include "Debug.h"
#include "NodeMonitoringEvent.h"

// NodeMonitoringEvent

// constructor
NodeMonitoringEvent::NodeMonitoringEvent()
	: Referencable(true),
	  time(system_time()),
	  queryHandler(NULL),
	  remotePort(-1),
	  remoteToken(-1)
{
}

// destructor
NodeMonitoringEvent::~NodeMonitoringEvent()
{
	if (queryHandler)
		queryHandler->RemoveReference();
}


// EntryCreatedEvent

// Init
status_t
EntryCreatedEvent::Init(const BMessage* message)
{
	opcode = B_ENTRY_CREATED;
	const char* localName;
	if (message->FindInt32("device", &volumeID) != B_OK
		|| message->FindInt64("directory", &directoryID) != B_OK
		|| message->FindInt64("node", &nodeID) != B_OK
		|| message->FindString("name", &localName) != B_OK) {
		return B_BAD_VALUE;
	}
	if (!name.SetTo(localName))
		return B_NO_MEMORY;
	return B_OK;
}


// EntryRemovedEvent

// Init
status_t
EntryRemovedEvent::Init(const BMessage* message)
{
	opcode = B_ENTRY_REMOVED;
	if (message->FindInt32("device", &volumeID) != B_OK
		|| message->FindInt64("directory", &directoryID) != B_OK
		|| message->FindInt64("node", &nodeID) != B_OK) {
		return B_BAD_VALUE;
	}
	nodeVolumeID = volumeID;
	return B_OK;
}


// EntryMovedEvent

// Init
status_t
EntryMovedEvent::Init(const BMessage* message)
{
	opcode = B_ENTRY_MOVED;
	const char* localName;
	if (message->FindInt32("device", &volumeID) != B_OK
		|| message->FindInt64("from directory", &fromDirectoryID) != B_OK
		|| message->FindInt64("to directory", &toDirectoryID) != B_OK
		|| message->FindInt64("node", &nodeID) != B_OK
		|| message->FindString("name", &localName) != B_OK) {
		return B_BAD_VALUE;
	}
	nodeVolumeID = volumeID;
	if (!toName.SetTo(localName))
		return B_NO_MEMORY;
	return B_OK;
}


// StatChangedEvent

// Init
status_t
StatChangedEvent::Init(const BMessage* message)
{
	opcode = B_STAT_CHANGED;
	if (message->FindInt32("device", &volumeID) != B_OK
		|| message->FindInt64("node", &nodeID)) {
		return B_BAD_VALUE;
	}
	return B_OK;
}


// AttributeChangedEvent

// Init
status_t
AttributeChangedEvent::Init(const BMessage* message)
{
	opcode = B_ATTR_CHANGED;
	const char* localName;
	if (message->FindInt32("device", &volumeID) != B_OK
		|| message->FindInt64("node", &nodeID)
		|| message->FindString("attr", &localName) != B_OK) {
		return B_BAD_VALUE;
	}
	if (!attribute.SetTo(localName))
		return B_NO_MEMORY;
	return B_OK;
}


// VolumeMountedEvent

// Init
status_t
VolumeMountedEvent::Init(const BMessage* message)
{
	opcode = B_DEVICE_MOUNTED;
	if (message->FindInt32("new device", &newVolumeID) != B_OK
		|| message->FindInt32("device", &volumeID) != B_OK
		|| message->FindInt64("directory", &directoryID) != B_OK) {
		return B_BAD_VALUE;
	}
	return B_OK;
}


// VolumeUnmountedEvent

// Init
status_t
VolumeUnmountedEvent::Init(const BMessage* message)
{
	opcode = B_DEVICE_UNMOUNTED;
	if (message->FindInt32("device", &volumeID) != B_OK)
		return B_BAD_VALUE;
	return B_OK;
}

