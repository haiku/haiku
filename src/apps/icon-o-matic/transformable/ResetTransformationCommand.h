/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef RESET_TRANSFORM_COMMAND_H
#define RESET_TRANSFORM_COMMAND_H

#include "Command.h"

class Transformable;

class ResetTransformationCommand : public Command {
 public:
								ResetTransformationCommand(
										Transformable** const objects,
										int32 count);
	virtual						~ResetTransformationCommand();
	
	// Command interface
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual	status_t			Undo();

	virtual void				GetName(BString& name);

 protected:
			Transformable**		fObjects;
			double*				fOriginals;
			int32				fCount;
};

#endif // RESET_TRANSFORM_COMMAND_H
