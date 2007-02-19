// VolumeEvent.h

#ifndef NETFS_VOLUME_EVENT_H
#define NETFS_VOLUME_EVENT_H

#include <fsproto.h>

#include "DLList.h"
#include "Referencable.h"

// event types
enum {
	CONNECTION_BROKEN_EVENT,
};

// VolumeEvent
class VolumeEvent : public Referencable, public DLListLinkImpl<VolumeEvent> {
public:
								VolumeEvent(uint32 type, vnode_id target = -1);
	virtual						~VolumeEvent();

			uint32				GetType() const;

			void				SetTarget(vnode_id target);
			vnode_id			GetTarget() const;
				// a node ID identifying the target volume (usually the ID
				// of the root node)

private:
			uint32				fType;
			vnode_id			fTarget;
};

// ConnectionBrokenEvent
class ConnectionBrokenEvent : public VolumeEvent {
public:
								ConnectionBrokenEvent(vnode_id target = -1);
	virtual						~ConnectionBrokenEvent();
};

#endif	// NETFS_VOLUME_EVENT_H
