/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef FLIP_POINTS_COMMAND_H
#define FLIP_POINTS_COMMAND_H

#include "PathCommand.h"

class BPoint;

class FlipPointsCommand : public PathCommand {
 public:
								FlipPointsCommand(VectorPath* path,
												  const int32* indices,
												  int32 count);
	virtual						~FlipPointsCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			int32*				fIndex;
			int32				fCount;
};

#endif // FLIP_POINTS_COMMAND_H
