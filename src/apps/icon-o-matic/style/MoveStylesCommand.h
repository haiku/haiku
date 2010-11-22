/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef MOVE_STYLES_COMMAND_H
#define MOVE_STYLES_COMMAND_H


#include "Command.h"
#include "IconBuild.h"


// TODO: make a templated "move items" command?

_BEGIN_ICON_NAMESPACE
	class Style;
	class StyleContainer;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class MoveStylesCommand : public Command {
 public:
								MoveStylesCommand(
									StyleContainer* container,
									Style** styles,
									int32 count,
									int32 toIndex);
	virtual						~MoveStylesCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			StyleContainer*		fContainer;
			Style**				fStyles;
			int32*				fIndices;
			int32				fToIndex;
			int32				fCount;
};

#endif // MOVE_STYLES_COMMAND_H
