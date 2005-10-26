/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <View.h>
#include <stdlib.h>
#include <string.h>

#include "PartitionMenuItem.h"

PartitionMenuItem::PartitionMenuItem(const char *label, BMessage *msg, partition_id id) 
	: BMenuItem(label, msg)
{
	fID = id;
}


PartitionMenuItem::~PartitionMenuItem()
{
}

