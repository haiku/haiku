/*
 * Copyright 2009 Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PL_ITEMS_COMMAND_H
#define PL_ITEMS_COMMAND_H


#include "Command.h"

class Playlist;
class PlaylistItem;

class PLItemsCommand : public Command {
public:
								PLItemsCommand();
	virtual						~PLItemsCommand();

protected:
			void				_CleanUp(PlaylistItem**& items, int32 count,
									bool deleteItems);
};

#endif // PL_ITEMS_COMMAND_H
