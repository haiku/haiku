/*
 * Copyright 2007-2009 Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef IMPORT_PL_ITEMS_COMMAND_H
#define IMPORT_PL_ITEMS_COMMAND_H


#include "PLItemsCommand.h"

class BMessage;

class ImportPLItemsCommand : public PLItemsCommand {
public:
								ImportPLItemsCommand(
									Playlist* playlist,
									const BMessage* refsMessage,
									int32 toIndex,
									bool sortItems);
	virtual						~ImportPLItemsCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

private:
			Playlist*			fPlaylist;
			PlaylistItem**		fOldItems;
			int32				fOldCount;
			PlaylistItem**		fNewItems;
			int32				fNewCount;
			int32				fToIndex;
			int32				fPlaylingIndex;
			bool				fItemsAdded;
};

#endif // IMPORT_PL_ITEMS_COMMAND_H
