/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef CHANGE_POINT_COMMAND_H
#define CHANGE_POINT_COMMAND_H

#include <Point.h>

#include "PathCommand.h"

class ChangePointCommand : public PathCommand {
 public:
								ChangePointCommand(VectorPath* path,
												   int32 index,
												   const int32* selected,
												   int32 count);
	virtual						~ChangePointCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();
	virtual status_t			Redo();

	virtual void				GetName(BString& name);

 private:
			int32				fIndex;

			BPoint				fPoint;
			BPoint				fPointIn;
			BPoint				fPointOut;
			bool				fConnected;

			int32*				fOldSelection;
			int32				fOldSelectionCount;
};

#endif // CHANGE_POINT_COMMAND_H
