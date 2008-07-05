/*
 * Copyright © 2008 Stephan Aßmus. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef RANDOMIZE_PL_ITEMS_COMMAND_H
#define RANDOMIZE_PL_ITEMS_COMMAND_H


#include "Command.h"

class Playlist;
struct entry_ref;

class RandomizePLItemsCommand : public Command {
 public:
								RandomizePLItemsCommand(
									Playlist* playlist,
									const int32* indices,
									int32 count);
	virtual						~RandomizePLItemsCommand();
	
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			status_t			_Sort(bool random);

			Playlist*			fPlaylist;
			entry_ref*			fRefs;
			int32*				fListIndices;
			int32*				fRandomInternalIndices;
			int32				fCount;
};

#endif // RANDOMIZE_PL_ITEMS_COMMAND_H
