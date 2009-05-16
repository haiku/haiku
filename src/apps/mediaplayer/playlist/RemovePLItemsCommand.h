/*
 * Copyright 2007-2009, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef REMOVE_PL_ITEMS_COMMAND_H
#define REMOVE_PL_ITEMS_COMMAND_H


#include "Command.h"

struct entry_ref;
class Playlist;

class RemovePLItemsCommand : public Command {
 public:
								RemovePLItemsCommand(
									Playlist* playlist,
									const int32* indices,
									int32 count,
									bool moveFilesToTrash = false);
	virtual						~RemovePLItemsCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			Playlist*			fPlaylist;
			entry_ref*			fRefs;
			BString*			fNamesInTrash;
			int32*				fIndices;
			int32				fCount;
			bool				fMoveFilesToTrash;
			bool				fMoveErrorShown;
};

#endif // REMOVE_PL_ITEMS_COMMAND_H
