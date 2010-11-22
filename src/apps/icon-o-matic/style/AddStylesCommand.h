/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef ADD_STYLES_COMMAND_H
#define ADD_STYLES_COMMAND_H


#include "Command.h"
#include "IconBuild.h"


_BEGIN_ICON_NAMESPACE
	class Style;
	class StyleContainer;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class AddStylesCommand : public Command {
 public:
								AddStylesCommand(
									StyleContainer* container,
									Style** const styles,
									int32 count,
									int32 index);
	virtual						~AddStylesCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			StyleContainer*		fContainer;
			Style**				fStyles;
			int32				fCount;
			int32				fIndex;
			bool				fStylesAdded;
};

#endif // ADD_STYLES_COMMAND_H
