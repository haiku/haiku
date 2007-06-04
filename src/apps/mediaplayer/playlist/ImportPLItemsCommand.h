/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef IMPORT_PL_ITEMS_COMMAND_H
#define IMPORT_PL_ITEMS_COMMAND_H


#include "Command.h"

class BMessage;
class Playlist;

class ImportPLItemsCommand : public Command {
 public:
								ImportPLItemsCommand(
									Playlist* playlist,
									const BMessage* refsMessage,
									int32 toIndex);
	virtual						~ImportPLItemsCommand();
	
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			Playlist*			fPlaylist;
			entry_ref*			fOldRefs;
			int32				fOldCount;
			entry_ref*			fNewRefs;
			int32				fNewCount;
			int32				fToIndex;
};

#endif // IMPORT_PL_ITEMS_COMMAND_H
