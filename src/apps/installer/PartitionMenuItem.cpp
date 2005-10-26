/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <View.h>
#include <stdlib.h>
#include <string.h>

#include "PartitionMenuItem.h"

PartitionMenuItem::PartitionMenuItem(const char *name, const char *label, const char *menuLabel, 
	BMessage *msg, partition_id id) 
	: BMenuItem(label, msg)
{
	fID = id;
	fMenuLabel = strdup(menuLabel);
	fName = strdup(name);
}


PartitionMenuItem::~PartitionMenuItem()
{
	free(fMenuLabel);
	free(fName);
}
