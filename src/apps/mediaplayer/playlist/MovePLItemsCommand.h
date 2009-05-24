/*
 * Copyright 2007-2009 Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MOVE_PL_ITEMS_COMMAND_H
#define MOVE_PL_ITEMS_COMMAND_H


#include "PLItemsCommand.h"

class MovePLItemsCommand : public PLItemsCommand {
 public:
								MovePLItemsCommand(
									Playlist* playlist,
									const int32* indices,
									int32 count,
									int32 toIndex);
	virtual						~MovePLItemsCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			Playlist*			fPlaylist;
			PlaylistItem**		fItems;
			int32*				fIndices;
			int32				fToIndex;
			int32				fCount;
};

#endif // MOVE_PL_ITEMS_COMMAND_H
