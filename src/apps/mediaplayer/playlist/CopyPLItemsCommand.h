/*
 * Copyright © 2007-2009 Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef COPY_PL_ITEMS_COMMAND_H
#define COPY_PL_ITEMS_COMMAND_H


#include <List.h>

#include "PLItemsCommand.h"


class CopyPLItemsCommand : public PLItemsCommand {
public:
								CopyPLItemsCommand(
									Playlist* playlist,
									BList indices,
									int32 toIndex);
	virtual						~CopyPLItemsCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

private:
			Playlist*			fPlaylist;
			int32				fCount;
			PlaylistItem**		fItems;
			int32				fToIndex;
			bool				fItemsCopied;
};

#endif // COPY_PL_ITEMS_COMMAND_H
