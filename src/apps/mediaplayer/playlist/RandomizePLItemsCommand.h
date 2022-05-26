/*
 * Copyright © 2008-2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef RANDOMIZE_PL_ITEMS_COMMAND_H
#define RANDOMIZE_PL_ITEMS_COMMAND_H


#include <List.h>

#include "PLItemsCommand.h"


class RandomizePLItemsCommand : public PLItemsCommand {
public:
								RandomizePLItemsCommand(
									Playlist* playlist,
									BList indices);
	virtual						~RandomizePLItemsCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

private:
			status_t			_Sort(bool random);

			Playlist*			fPlaylist;
			int32				fCount;
			PlaylistItem**		fItems;
			int32*				fListIndices;
			int32*				fRandomInternalIndices;
};

#endif // RANDOMIZE_PL_ITEMS_COMMAND_H
