/*
 * Copyright 2006-2007, 2023, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef MOVE_STYLES_COMMAND_H
#define MOVE_STYLES_COMMAND_H


#include "IconBuild.h"
#include "MoveCommand.h"


_BEGIN_ICON_NAMESPACE
	class Style;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class MoveStylesCommand : public MoveCommand<Style> {
 public:
								MoveStylesCommand(
									Container<Style>* container,
									Style** styles,
									int32 count,
									int32 toIndex);
	virtual						~MoveStylesCommand();

	virtual void				GetName(BString& name);
};

#endif // MOVE_STYLES_COMMAND_H
