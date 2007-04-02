/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef SET_GRADIENT_COMMAND_H
#define SET_GRADIENT_COMMAND_H


#include "Command.h"


namespace BPrivate {
namespace Icon {
	class Gradient;
	class Style;
}
}
using namespace BPrivate::Icon;

class SetGradientCommand : public Command {
 public:
								SetGradientCommand(Style* style,
												   const Gradient* gradient);
	virtual						~SetGradientCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

	virtual	bool				CombineWithNext(const Command* next);

 private:
			Style*				fStyle;
			Gradient*			fGradient;
};

#endif // CHANGE_GRADIENT_COMMAND_H
