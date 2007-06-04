/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef REMOVE_PL_ITEMS_COMMAND_H
#define REMOVE_PL_ITEMS_COMMAND_H


#include "Command.h"

class Playlist;

class RemovePLItemsCommand : public Command {
 public:
								RemovePLItemsCommand(
									Playlist* playlist,
									const int32* indices,
									int32 count);
	virtual						~RemovePLItemsCommand();
	
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			Playlist*			fPlaylist;
			entry_ref*			fRefs;
			int32*				fIndices;
			int32				fCount;
};

#endif // REMOVE_PL_ITEMS_COMMAND_H
