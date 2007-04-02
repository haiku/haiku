/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef ADD_POINT_COMMAND_H
#define ADD_POINT_COMMAND_H


#include "PathCommand.h"

#include <Point.h>


class AddPointCommand : public PathCommand {
 public:
								AddPointCommand(VectorPath* path,
												int32 index,
												const int32* selected,
												int32 count);
	virtual						~AddPointCommand();

	virtual	status_t			Perform();
	virtual status_t			Undo();
	virtual status_t			Redo();

	virtual void				GetName(BString& name);

 private:
			int32				fIndex;
			BPoint				fPoint;
			BPoint				fPointIn;
			BPoint				fPointOut;

			int32*				fOldSelection;
			int32				fOldSelectionCount;
};

#endif // ADD_POINT_COMMAND_H
