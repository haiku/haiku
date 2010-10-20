#include "NodeMonitorHandler.h"

/*
 * static util functions for the super lazy
 */


/* static */ status_t
NodeMonitorHandler::make_entry_ref(dev_t device, ino_t directory,
                                   const char * name, 
                                   entry_ref * ref)
{
	ref->device = device;
	ref->directory = directory;
	return ref->set_name(name);
}


/* static */ void
NodeMonitorHandler::make_node_ref(dev_t device, ino_t node, node_ref * ref)
{
	ref->device = device;
	ref->node = node;
}


/*
 * public functions: constructor, destructor, MessageReceived
 */

NodeMonitorHandler::NodeMonitorHandler(const char * name)
	: BHandler(name)
{
	// nothing to do
}


NodeMonitorHandler::~NodeMonitorHandler()
{
	// nothing to do
}


/* virtual */ void
NodeMonitorHandler::MessageReceived(BMessage * msg)
{
	status_t status = B_MESSAGE_NOT_UNDERSTOOD;
	if (msg->what == B_NODE_MONITOR) {
		int32 opcode;
		if (msg->FindInt32("opcode", &opcode) == B_OK) {
			switch (opcode) {
			case B_ENTRY_CREATED:
				status = HandleEntryCreated(msg);
				break;
			case B_ENTRY_REMOVED:
				status = HandleEntryRemoved(msg);
				break;
			case B_ENTRY_MOVED:
				status = HandleEntryMoved(msg);
				break;
			case B_STAT_CHANGED:
				status = HandleStatChanged(msg);
				break;
			case B_ATTR_CHANGED:
				status = HandleAttrChanged(msg);
				break;
			case B_DEVICE_MOUNTED:
				status = HandleDeviceMounted(msg);
				break;
			case B_DEVICE_UNMOUNTED:
				status = HandleDeviceUnmounted(msg);
				break;
			default:
				break;
			}
		}
	}
	if (status == B_MESSAGE_NOT_UNDERSTOOD) {
		inherited::MessageReceived(msg);
	}
}

/*
 * default virtual functions do nothing
 */

/* virtual */ void
NodeMonitorHandler::EntryCreated(const char *name, ino_t directory,
	dev_t device, ino_t node)
{
	// ignore
}


/* virtual */ void
NodeMonitorHandler::EntryRemoved(const char *name, ino_t directory,
	dev_t device, ino_t node)
{
	// ignore
}


/* virtual */ void
NodeMonitorHandler::EntryMoved(const char *name, const char *fromName,
	ino_t fromDirectory, ino_t toDirectory, dev_t device,ino_t node,
	dev_t nodeDevice)
{
	// ignore
}


/* virtual */ void
NodeMonitorHandler::StatChanged(ino_t node, dev_t device, int32 statFields)
{
	// ignore
}


/* virtual */ void
NodeMonitorHandler::AttrChanged(ino_t node, dev_t device)
{
	// ignore
}


/* virtual */ void
NodeMonitorHandler::DeviceMounted(dev_t new_device, dev_t device,
	ino_t directory)
{
	// ignore
}


/* virtual */ void
NodeMonitorHandler::DeviceUnmounted(dev_t new_device)
{
	// ignore
}


/*
 * private functions to rip apart the corresponding BMessage
 */

status_t
NodeMonitorHandler::HandleEntryCreated(BMessage * msg)
{
	const char *name;
	ino_t directory;
	dev_t device;
	ino_t node;
	if ((msg->FindString("name", &name) != B_OK) ||
        (msg->FindInt64("directory", &directory) != B_OK) ||
		(msg->FindInt32("device", &device) != B_OK) ||
		(msg->FindInt64("node", &node) != B_OK)) {
		return B_MESSAGE_NOT_UNDERSTOOD;
	}
	EntryCreated(name, directory, device, node);
	return B_OK;
}


status_t
NodeMonitorHandler::HandleEntryRemoved(BMessage * msg)
{
	const char *name;
	ino_t directory;
	dev_t device;
	ino_t node;
	if ((msg->FindString("name", &name) != B_OK) ||
		(msg->FindInt64("directory", &directory) != B_OK) ||
		(msg->FindInt32("device", &device) != B_OK) ||
		(msg->FindInt64("node", &node) != B_OK)) {
		return B_MESSAGE_NOT_UNDERSTOOD;
	}
	EntryRemoved(name, directory, device, node);
	return B_OK;
}


status_t
NodeMonitorHandler::HandleEntryMoved(BMessage * msg)
{
	const char *name;
	const char *fromName;
	ino_t fromDirectory;
	ino_t toDirectory;
	dev_t device;
	ino_t node;
	dev_t deviceNode;
	if ((msg->FindString("name", &name) != B_OK) ||
		(msg->FindString("from name", &fromName) != B_OK) ||
		(msg->FindInt64("from directory", &fromDirectory) != B_OK) ||
		(msg->FindInt64("to directory", &toDirectory) != B_OK) ||
		(msg->FindInt32("device", &device) != B_OK) ||
		(msg->FindInt32("node device", &deviceNode) != B_OK) ||
		(msg->FindInt64("node", &node) != B_OK)) {
		return B_MESSAGE_NOT_UNDERSTOOD;
	}
	EntryMoved(name, fromName, fromDirectory, toDirectory, device, node,
		deviceNode);
	return B_OK;
}


status_t
NodeMonitorHandler::HandleStatChanged(BMessage * msg)
{
	ino_t node;
	dev_t device;
	int32 statFields;
	if ((msg->FindInt64("node", &node) != B_OK) ||
		(msg->FindInt32("device", &device) != B_OK) ||
		(msg->FindInt32("fields", &statFields) != B_OK)) {
		return B_MESSAGE_NOT_UNDERSTOOD;
	}
	StatChanged(node, device, statFields);
	return B_OK;
}


status_t
NodeMonitorHandler::HandleAttrChanged(BMessage * msg)
{
	ino_t node;
	dev_t device;
	if ((msg->FindInt64("node", &node) != B_OK) ||
		(msg->FindInt32("device", &device) != B_OK)) {
		return B_MESSAGE_NOT_UNDERSTOOD;
	}
	AttrChanged(node, device);
	return B_OK;
}


status_t
NodeMonitorHandler::HandleDeviceMounted(BMessage * msg)
{
	dev_t new_device;
	dev_t device;
	ino_t directory;
	if ((msg->FindInt32("new device", &new_device) != B_OK) ||
		(msg->FindInt32("device", &device) != B_OK) ||
		(msg->FindInt64("directory", &directory) != B_OK)) {
		return B_MESSAGE_NOT_UNDERSTOOD;
	}
	DeviceMounted(new_device, device, directory);
	return B_OK;
}


status_t
NodeMonitorHandler::HandleDeviceUnmounted(BMessage * msg)
{
	dev_t new_device;
	if (msg->FindInt32("new device", &new_device) != B_OK) {
		return B_MESSAGE_NOT_UNDERSTOOD;
	}
	DeviceUnmounted(new_device);
	return B_OK;
}
