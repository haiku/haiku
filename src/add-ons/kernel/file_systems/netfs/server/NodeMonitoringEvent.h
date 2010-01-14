// NodeMonitoringEvent.h

#ifndef NET_FS_NODE_MONITORING_EVENT_H
#define NET_FS_NODE_MONITORING_EVENT_H

#include <HashString.h>
#include <Referenceable.h>
#include <util/DoublyLinkedList.h>

class BMessage;

// NodeMonitoringEvent
struct NodeMonitoringEvent : public BReferenceable,
	public DoublyLinkedListLinkImpl<NodeMonitoringEvent> {
								NodeMonitoringEvent();
	virtual						~NodeMonitoringEvent();

	virtual	status_t			Init(const BMessage* message) = 0;

			int32				opcode;
			bigtime_t			time;
			BReferenceable*		queryHandler;	// only for query notifications
			port_id				remotePort;		//
			int32				remoteToken;	//
};

// EntryCreatedEvent
struct EntryCreatedEvent : NodeMonitoringEvent {
			status_t			Init(const BMessage* message);

			dev_t				volumeID;
			ino_t				directoryID;
			ino_t				nodeID;
			HashString			name;
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
			HashString			name;			// filled in later
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
			HashString			fromName;	// filled in later
			HashString			toName;
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
			HashString			attribute;
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
