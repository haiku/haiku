/*
 * Copyright 2006-2007, 2023, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef REMOVE_STYLES_COMMAND_H
#define REMOVE_STYLES_COMMAND_H


#include "IconBuild.h"
#include "RemoveCommand.h"


class BList;

_BEGIN_ICON_NAMESPACE
	class Style;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class RemoveStylesCommand : public RemoveCommand<Style> {
 public:
								RemoveStylesCommand(
									Container<Style>* container,
									int32* const indices,
									int32 count);
	virtual						~RemoveStylesCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			BList*				fShapes;
};

#endif // REMOVE_STYLES_COMMAND_H
