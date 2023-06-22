/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef MOVE_SHAPES_COMMAND_H
#define MOVE_SHAPES_COMMAND_H


#include "IconBuild.h"
#include "MoveCommand.h"


_BEGIN_ICON_NAMESPACE
	class Shape;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class MoveShapesCommand : public MoveCommand<Shape> {
 public:
								MoveShapesCommand(
									Container<Shape>* container,
									Shape** shapes,
									int32 count,
									int32 toIndex);
	virtual						~MoveShapesCommand();

	virtual void				GetName(BString& name);
};

#endif // MOVE_SHAPES_COMMAND_H
