// NodeMonitor.h

#ifndef NET_FS_NODE_MONITOR_H
#define NET_FS_NODE_MONITOR_H

#include <Looper.h>

struct node_ref;
class NodeMonitoringEvent;
class NodeMonitorListener;

// NodeMonitor
class NodeMonitor : public BLooper {
public:
								NodeMonitor(NodeMonitorListener* listener);
	virtual						~NodeMonitor();

	virtual	void				MessageReceived(BMessage* message);

			status_t			StartWatching(const node_ref& ref);
			status_t			StopWatching(const node_ref& ref);

private:
			status_t			_IncreaseLimit();

private:
			NodeMonitorListener* fListener;
			int32				fCurrentNodeMonitorLimit;
};

// NodeMonitorListener
class NodeMonitorListener {
public:
								NodeMonitorListener();
	virtual						~NodeMonitorListener();

	virtual	void				ProcessNodeMonitoringEvent(
									NodeMonitoringEvent* event) = 0;
};

#endif	// NET_FS_NODE_MONITOR_H
