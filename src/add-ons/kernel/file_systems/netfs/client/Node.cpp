// Node.cpp

#include "Node.h"

// constructor
Node::Node(Volume* volume, vnode_id id)
	: fVolume(volume),
	  fID(id),
	  fKnownToVFS(false)
{
}

// destructor
Node::~Node()
{
}

// SetKnownToVFS
void
Node::SetKnownToVFS(bool known)
{
	fKnownToVFS = known;
}

// IsKnownToVFS
bool
Node::IsKnownToVFS() const
{
	return fKnownToVFS;
}

