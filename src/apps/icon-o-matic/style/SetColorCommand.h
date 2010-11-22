/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef SET_COLOR_COMMAND_H
#define SET_COLOR_COMMAND_H


#include "Command.h"
#include "IconBuild.h"

#include <InterfaceDefs.h>


_BEGIN_ICON_NAMESPACE
	class Style;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class SetColorCommand : public Command {
 public:
								SetColorCommand(Style* style,
												const rgb_color& color);
	virtual						~SetColorCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

	virtual	bool				CombineWithNext(const Command* next);

 private:
			Style*				fStyle;
			rgb_color			fColor;
};

#endif // SET_COLOR_COMMAND_H
