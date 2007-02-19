// NodeMonitoringEvent.h

#ifndef NET_FS_NODE_MONITORING_EVENT_H
#define NET_FS_NODE_MONITORING_EVENT_H

#include "DLList.h"
#include "Referencable.h"
#include "String.h"

class BMessage;

// NodeMonitoringEvent
struct NodeMonitoringEvent : public Referencable,
	public DLListLinkImpl<NodeMonitoringEvent> {
								NodeMonitoringEvent();
	virtual						~NodeMonitoringEvent();

	virtual	status_t			Init(const BMessage* message) = 0;

			int32				opcode;
			bigtime_t			time;
			Referencable*		queryHandler;	// only for query notifications
			port_id				remotePort;		//
			int32				remoteToken;	//
};

// EntryCreatedEvent
struct EntryCreatedEvent : NodeMonitoringEvent {
			status_t			Init(const BMessage* message);

			dev_t				volumeID;
			ino_t				directoryID;
			ino_t				nodeID;
			String				name;
};

// EntryRemovedEvent
struct EntryRemovedEvent : NodeMonitoringEvent {
			status_t			Init(const BMessage* message);

			dev_t				volumeID;
			ino_t				directoryID;
			dev_t				nodeVolumeID;	// == volumeID, unless this
												// event is manually generated
												// for an entry pointing to the
												// root of a FS
			ino_t				nodeID;
			String				name;			// filled in later
};

// EntryMovedEvent
struct EntryMovedEvent : NodeMonitoringEvent {
			status_t			Init(const BMessage* message);

			dev_t				volumeID;
			ino_t				fromDirectoryID;
			ino_t				toDirectoryID;
			dev_t				nodeVolumeID;	// usually == volumeID, unless
												// the node is the root dir of
												// a volume (the VolumeManager)
												// fixes this field
			ino_t				nodeID;
			String				fromName;	// filled in later
			String				toName;
};

// StatChangedEvent
struct StatChangedEvent : NodeMonitoringEvent {
			status_t			Init(const BMessage* message);

			dev_t				volumeID;
			ino_t				nodeID;
};

// AttributeChangedEvent
struct AttributeChangedEvent : NodeMonitoringEvent {
			status_t			Init(const BMessage* message);

			dev_t				volumeID;
			ino_t				nodeID;
			String				attribute;
};

// VolumeMountedEvent
struct VolumeMountedEvent : NodeMonitoringEvent {
			status_t			Init(const BMessage* message);

			dev_t				newVolumeID;
			dev_t				volumeID;
			ino_t				directoryID;
};

// VolumeUnmountedEvent
struct VolumeUnmountedEvent : NodeMonitoringEvent {
			status_t			Init(const BMessage* message);

			dev_t				volumeID;
};


#endif	// NET_FS_NODE_MONITORING_EVENT_H
