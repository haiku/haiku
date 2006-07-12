/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef REMOVE_PATHS_COMMAND_H
#define REMOVE_PATHS_COMMAND_H

#include <List.h>

#include "Command.h"

class VectorPath;
class PathContainer;

class RemovePathsCommand : public Command {
 public:
								RemovePathsCommand(
									PathContainer* container,
									VectorPath** const paths,
									int32 count);
	virtual						~RemovePathsCommand();
	
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			PathContainer*		fContainer;
			struct PathInfo {
				VectorPath*		path;
				int32			index;
				BList			shapes;
			};
			PathInfo*			fInfos;
			int32				fCount;
			bool				fPathsRemoved;
};

#endif // REMOVE_PATHS_COMMAND_H
