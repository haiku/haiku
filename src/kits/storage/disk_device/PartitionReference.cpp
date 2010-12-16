/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "PartitionReference.h"


// constructor
PartitionReference::PartitionReference(partition_id id, int32 changeCounter)
	:
	BReferenceable(),
	fID(id),
	fChangeCounter(changeCounter)
{
}


// destructor
PartitionReference::~PartitionReference()
{
}


// SetTo
void
PartitionReference::SetTo(partition_id id, int32 changeCounter)
{
	fID = id;
	fChangeCounter = changeCounter;
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
int32
PartitionReference::ChangeCounter() const
{
	return fChangeCounter;
}


// SetChangeCounter
void
PartitionReference::SetChangeCounter(int32 counter)
{
	fChangeCounter = counter;
}
