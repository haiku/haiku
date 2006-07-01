/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef REMOVE_POINTS_COMMAND_H
#define REMOVE_POINTS_COMMAND_H

#include "PathCommand.h"

class BPoint;

class RemovePointsCommand : public PathCommand {
 public:
								// for removing the point clicked on
								// independent of selection
								RemovePointsCommand(VectorPath* path,
													int32 index,
													const int32* selection,
													int32 count);
								// for removing the selection
								RemovePointsCommand(VectorPath* path,
													const int32* selection,
													int32 count);
	virtual						~RemovePointsCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();
	virtual status_t			Redo();

	virtual void				GetName(BString& name);

 private:
			void				_Init(const int32* indices, int32 count,
									  const int32* selection,
									  int32 selectionCount);

			int32*				fIndex;
			BPoint*				fPoint;
			BPoint*				fPointIn;
			BPoint*				fPointOut;
			bool*				fConnected;
			int32				fCount;

			bool				fWasClosed;

			int32*				fOldSelection;
			int32				fOldSelectionCount;
};

#endif // REMOVE_POINTS_COMMAND_H
