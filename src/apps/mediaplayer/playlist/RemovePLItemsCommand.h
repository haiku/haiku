/*
 * Copyright © 2007-2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef REMOVE_PL_ITEMS_COMMAND_H
#define REMOVE_PL_ITEMS_COMMAND_H


#include "PLItemsCommand.h"

class RemovePLItemsCommand : public PLItemsCommand {
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
			PlaylistItem**		fItems;
			int32*				fIndices;
			int32				fCount;
			bool				fMoveFilesToTrash;
			bool				fMoveErrorShown;
			bool				fItemsRemoved;
};

#endif // REMOVE_PL_ITEMS_COMMAND_H
