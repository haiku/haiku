/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef REMOVE_PATHS_COMMAND_H
#define REMOVE_PATHS_COMMAND_H


#include "Command.h"
#include "IconBuild.h"
#include "RemoveCommand.h"


class BList;

_BEGIN_ICON_NAMESPACE
	template <class Type> class Container;
	class VectorPath;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class RemovePathsCommand : public RemoveCommand<VectorPath> {
 public:
									RemovePathsCommand(
										Container<VectorPath>* container,
										const int32* indices,
										int32 count);
	virtual							~RemovePathsCommand();

	virtual	status_t				InitCheck();

	virtual	status_t				Perform();
	virtual status_t				Undo();

	virtual void					GetName(BString& name);

 private:
			BList*					fShapes;
};

#endif // REMOVE_PATHS_COMMAND_H
