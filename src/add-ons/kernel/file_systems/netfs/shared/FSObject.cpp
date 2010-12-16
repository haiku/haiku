// FSObject.cpp

#include "FSObject.h"

// constructor
FSObject::FSObject()
	:
	BReferenceable(),
	fRemoved(false)
{
}

// destructor
FSObject::~FSObject()
{
}

// MarkRemoved
void
FSObject::MarkRemoved()
{
	fRemoved = true;
}

// IsRemoved
bool
FSObject::IsRemoved() const
{
	return fRemoved;
}


void
FSObject::LastReferenceReleased()
{
	// don't delete
}
