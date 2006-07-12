/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef ADD_STYLES_COMMAND_H
#define ADD_STYLES_COMMAND_H

#include "Command.h"

class Style;
class StyleManager;

class AddStylesCommand : public Command {
 public:
								AddStylesCommand(
									StyleManager* container,
									Style** const styles,
									int32 count,
									int32 index);
	virtual						~AddStylesCommand();
	
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			StyleManager*		fContainer;
			Style**				fStyles;
			int32				fCount;
			int32				fIndex;
			bool				fStylesAdded;
};

#endif // ADD_STYLES_COMMAND_H
