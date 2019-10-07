/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef MOVE_PATHS_COMMAND_H
#define MOVE_PATHS_COMMAND_H


#include "Command.h"
#include "IconBuild.h"


// TODO: make a templated "move items" command?

_BEGIN_ICON_NAMESPACE
	class VectorPath;
	class PathContainer;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class MovePathsCommand : public Command {
 public:
								MovePathsCommand(
									PathContainer* container,
									VectorPath** paths,
									int32 count,
									int32 toIndex);
	virtual						~MovePathsCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			PathContainer*		fContainer;
			VectorPath**		fPaths;
			int32*				fIndices;
			int32				fToIndex;
			int32				fCount;
};

#endif // MOVE_PATHS_COMMAND_H
