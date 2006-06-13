/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef INSERT_POINT_COMMAND_H
#define INSERT_POINT_COMMAND_H

#include <Point.h>

#include "PathCommand.h"

class InsertPointCommand : public PathCommand {
 public:
								InsertPointCommand(VectorPath* path,
												   int32 index,
												   const int32* selected,
												   int32 count);
	virtual						~InsertPointCommand();

	virtual	status_t			Perform();
	virtual status_t			Undo();
	virtual status_t			Redo();

	virtual void				GetName(BString& name);

 private:
			int32				fIndex;
			BPoint				fPoint;
			BPoint				fPointIn;
			BPoint				fPointOut;

			BPoint				fPreviousOut;
			BPoint				fNextIn;

			int32*				fOldSelection;
			int32				fOldSelectionCount;
};

#endif // INSERT_POINT_COMMAND_H
