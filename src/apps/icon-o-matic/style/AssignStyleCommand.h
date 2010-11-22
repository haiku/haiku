/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef ASSIGN_STYLE_COMMAND_H
#define ASSIGN_STYLE_COMMAND_H


#include "Command.h"
#include "IconBuild.h"

#include <InterfaceDefs.h>


_BEGIN_ICON_NAMESPACE
	class Shape;
	class Style;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class AssignStyleCommand : public Command {
 public:
								AssignStyleCommand(Shape* shape,
												   Style* style);
	virtual						~AssignStyleCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			Shape*				fShape;
			Style*				fOldStyle;
			Style*				fNewStyle;
};

#endif // ASSIGN_STYLE_COMMAND_H
