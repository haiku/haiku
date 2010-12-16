/*
 * Copyright 2009 Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PLItemsCommand.h"

#include <stdio.h>

#include <new>

#include "Playlist.h"
#include "PlaylistItem.h"


using std::nothrow;


PLItemsCommand::PLItemsCommand()
	:
	Command()
{
}


PLItemsCommand::~PLItemsCommand()
{
}


void
PLItemsCommand::_CleanUp(PlaylistItem**& items, int32 count, bool deleteItems)
{
	if (items == NULL)
		return;
	if (deleteItems) {
		for (int32 i = 0; i < count; i++)
			items[i]->ReleaseReference();
	}
	delete[] items;
	items = NULL;
}

