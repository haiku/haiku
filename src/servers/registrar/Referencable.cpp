// Referencable.cpp

#include "Debug.h"
#include "Referencable.h"

// constructor
Referencable::Referencable(bool deleteWhenUnreferenced)
	: fReferenceCount(1),
	  fDeleteWhenUnreferenced(deleteWhenUnreferenced)
{
}

// destructor
Referencable::~Referencable()
{
}

// AddReference
void
Referencable::AddReference()
{
	atomic_add(&fReferenceCount, 1);
}

// RemoveReference
bool
Referencable::RemoveReference()
{
	bool unreferenced = (atomic_add(&fReferenceCount, -1) == 1);
	if (fDeleteWhenUnreferenced && unreferenced)
		delete this;
	return unreferenced;
}

// CountReferences
int32
Referencable::CountReferences() const
{
	return fReferenceCount;
}

