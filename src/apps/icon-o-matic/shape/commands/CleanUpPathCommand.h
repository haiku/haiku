/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef CLEAN_UP_PATH_COMMAND_H
#define CLEAN_UP_PATH_COMMAND_H

#include "PathCommand.h"
#include "VectorPath.h"

class CleanUpPathCommand : public PathCommand {
 public:
								CleanUpPathCommand(VectorPath* path);
	virtual						~CleanUpPathCommand();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			VectorPath			fOriginalPath;
};

#endif // CLEAN_UP_PATH_COMMAND_H
