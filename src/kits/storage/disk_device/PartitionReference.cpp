/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "PartitionReference.h"


// constructor
PartitionReference::PartitionReference(partition_id id, uint32 changeCounter)
	: Referenceable(true),	// delete when unreferenced
	  fID(id),
	  fChangeCounter(changeCounter)
{
}


// destructor
PartitionReference::~PartitionReference()
{
}


// PartitionID
partition_id
PartitionReference::PartitionID() const
{
	return fID;
}


// SetPartitionID
void
PartitionReference::SetPartitionID(partition_id id)
{
	fID = id;
}


// ChangeCounter
uint32
PartitionReference::ChangeCounter() const
{
	return fChangeCounter;
}


// SetChangeCounter
void
PartitionReference::SetChangeCounter(uint32 counter)
{
	fChangeCounter = counter;
}
