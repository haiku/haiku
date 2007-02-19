// FSObject.cpp

#include "FSObject.h"

// constructor
FSObject::FSObject()
	: Referencable(),
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

