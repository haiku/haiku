#ifndef _NODE_MONITOR_HANDLER_H
#define _NODE_MONITOR_HANDLER_H

#include <Entry.h>
#include <Handler.h>
#include <Message.h>
#include <NodeMonitor.h>

namespace BPrivate {
namespace Storage {

class NodeMonitorHandler : public BHandler {
private:
	typedef BHandler inherited;
public:
			NodeMonitorHandler(const char * name = "NodeMonitorHandler");
	virtual	~NodeMonitorHandler();

	virtual void	MessageReceived(BMessage * msg);

	// useful utility functions
static status_t make_entry_ref(dev_t device, ino_t directory,
				               const char * name, 
				               entry_ref * ref);
static void		make_node_ref(dev_t device, ino_t node, node_ref * ref);

protected:
	// hooks for subclass
	virtual void	EntryCreated(const char *name, ino_t directory,
						dev_t device, ino_t node);
	virtual void	EntryRemoved(const char *name, ino_t directory,
						dev_t device, ino_t node);
	virtual void	EntryMoved(const char *name, const char *fromName,
						ino_t fromDirectory, ino_t toDirectory, dev_t device,
						ino_t node, dev_t nodeDevice);
	virtual void	StatChanged(ino_t node, dev_t device, int32 statFields);
	virtual void	AttrChanged(ino_t node, dev_t device);
	virtual void	DeviceMounted(dev_t new_device, dev_t device,
						ino_t directory);
	virtual void	DeviceUnmounted(dev_t new_device);

private:
	status_t	HandleEntryCreated(BMessage * msg);
	status_t	HandleEntryRemoved(BMessage * msg);
	status_t	HandleEntryMoved(BMessage * msg);
	status_t	HandleStatChanged(BMessage * msg);
	status_t	HandleAttrChanged(BMessage * msg);
	status_t	HandleDeviceMounted(BMessage * msg);
	status_t	HandleDeviceUnmounted(BMessage * msg);
};

};	// namespace Storage
};	// namespace BPrivate

using namespace BPrivate::Storage;

#endif // _NODE_MONITOR_HANDLER_H
