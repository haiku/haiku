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

#include <InterfaceDefs.h>


namespace BPrivate {
namespace Icon {
	class Shape;
	class Style;
}
}
using namespace BPrivate::Icon;

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
