/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef ADD_STYLES_COMMAND_H
#define ADD_STYLES_COMMAND_H


#include "AddCommand.h"
#include "IconBuild.h"


_BEGIN_ICON_NAMESPACE
	class Style;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class AddStylesCommand : public AddCommand<Style> {
 public:
								AddStylesCommand(
									Container<Style>* container,
									const Style* const* styles,
									int32 count,
									int32 index);
	virtual						~AddStylesCommand();

	virtual void				GetName(BString& name);
};

#endif // ADD_STYLES_COMMAND_H
