// VolumeEvent.cpp

#include "VolumeEvent.h"

// constructor
VolumeEvent::VolumeEvent(uint32 type, vnode_id target)
	:
	BReferenceable(),
	fType(type),
	fTarget(target)
{
}

// destructor
VolumeEvent::~VolumeEvent()
{
}

// GetType
uint32
VolumeEvent::GetType() const
{
	return fType;
}

// SetTarget
void
VolumeEvent::SetTarget(vnode_id target)
{
	fTarget = target;
}

// GetTarget
vnode_id
VolumeEvent::GetTarget() const
{
	return fTarget;
}


// #pragma mark -

// constructor
ConnectionBrokenEvent::ConnectionBrokenEvent(vnode_id target)
	: VolumeEvent(CONNECTION_BROKEN_EVENT, target)
{
}

// destructor
ConnectionBrokenEvent::~ConnectionBrokenEvent()
{
}

