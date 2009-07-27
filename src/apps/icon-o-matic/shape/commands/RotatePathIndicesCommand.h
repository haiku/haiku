/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ROTATE_PATH_INDICES_COMMAND_H
#define ROTATE_PATH_INDICES_COMMAND_H

#include "PathCommand.h"

class RotatePathIndicesCommand : public PathCommand {
public:
								RotatePathIndicesCommand(VectorPath* path,
									bool clockWise);
	virtual						~RotatePathIndicesCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

private:
			status_t			_Rotate(bool clockWise);

			bool				fClockWise;
};

#endif // ROTATE_PATH_INDICES_COMMAND_H
