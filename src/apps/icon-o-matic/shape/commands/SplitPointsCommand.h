/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef SPLIT_POINTS_COMMAND_H
#define SPLIT_POINTS_COMMAND_H

#include "PathCommand.h"

class BPoint;

class SplitPointsCommand : public PathCommand {
 public:
								SplitPointsCommand(VectorPath* path,
												   const int32* indices,
												   int32 count);
	virtual						~SplitPointsCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			int32*				fIndex;
			BPoint*				fPoint;
			BPoint*				fPointIn;
			BPoint*				fPointOut;
			bool*				fConnected;
			int32				fCount;
};

#endif // SPLIT_POINTS_COMMAND_H
