/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef ADD_TRANSFORMERS_COMMAND_H
#define ADD_TRANSFORMERS_COMMAND_H

#include "Command.h"

class Shape;
class Transformer;

// TODO: make a templated "add items" command?

class AddTransformersCommand : public Command {
 public:
								AddTransformersCommand(
									Shape* container,
									Transformer** const transformers,
									int32 count,
									int32 index);
	virtual						~AddTransformersCommand();
	
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			Shape*				fContainer;
			Transformer**		fTransformers;
			int32				fCount;
			int32				fIndex;
			bool				fTransformersAdded;
};

#endif // ADD_TRANSFORMERS_COMMAND_H
