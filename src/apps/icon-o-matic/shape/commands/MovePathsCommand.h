/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef MOVE_PATHS_COMMAND_H
#define MOVE_PATHS_COMMAND_H


#include "IconBuild.h"
#include "MoveCommand.h"


_BEGIN_ICON_NAMESPACE
	template <class Type> class Container;
	class VectorPath;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class MovePathsCommand : public MoveCommand<VectorPath> {
 public:
							   	MovePathsCommand(
							   		Container<VectorPath>* container,
							   		VectorPath** paths,
							   		int32 count,
							   		int32 toIndex);
	virtual					   	~MovePathsCommand();

	virtual void			   	GetName(BString& name);
};

#endif // MOVE_PATHS_COMMAND_H
