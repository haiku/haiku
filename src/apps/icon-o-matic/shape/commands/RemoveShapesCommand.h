/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef REMOVE_SHAPES_COMMAND_H
#define REMOVE_SHAPES_COMMAND_H


#include "IconBuild.h"
#include "RemoveCommand.h"


_BEGIN_ICON_NAMESPACE
	template <class Type> class Container;
	class Shape;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class RemoveShapesCommand : public RemoveCommand<Shape> {
 public:
								RemoveShapesCommand(
									Container<Shape>* container,
									const int32* indices,
									int32 count);
	virtual						~RemoveShapesCommand();

	virtual void				GetName(BString& name);
};

#endif // REMOVE_SHAPES_COMMAND_H
