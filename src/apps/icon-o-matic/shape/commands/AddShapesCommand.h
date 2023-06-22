/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef ADD_SHAPES_COMMAND_H
#define ADD_SHAPES_COMMAND_H


#include "AddCommand.h"
#include "IconBuild.h"


_BEGIN_ICON_NAMESPACE
	class Shape;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class AddShapesCommand : public AddCommand<Shape> {
 public:
								AddShapesCommand(
									Container<Shape>* container,
									const Shape* const* shapes,
									int32 count,
									int32 index);
	virtual						~AddShapesCommand();

	virtual void				GetName(BString& name);
};

#endif // ADD_SHAPES_COMMAND_H
