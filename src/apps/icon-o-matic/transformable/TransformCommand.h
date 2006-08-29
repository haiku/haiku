/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef TRANSFORM_COMMAND_H
#define TRANSFORM_COMMAND_H

#include <Point.h>
#include <String.h>

#include "Command.h"

class TransformCommand : public Command {
 public:
								TransformCommand(BPoint pivot,
												 BPoint translation,
												 double rotation,
												 double xScale,
												 double yScale,

												 const char* actionName,
												 uint32 nameIndex);

								TransformCommand(const char* actionName,
												 uint32 nameIndex);

	virtual						~TransformCommand();

	// Command interface	
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();
	virtual status_t			Redo();

	virtual void				GetName(BString& name);

	// TransformCommand
			void				SetNewTransformation(BPoint pivot,
													 BPoint translation,
													 double rotation,
													 double xScale,
													 double yScale);

			void				SetNewTranslation(BPoint translation);
				// convinience for "nudging"

			void				SetName(const char* actionName,
										uint32 nameIndex);

 protected:
	virtual	status_t			_SetTransformation(BPoint pivot,
												   BPoint translation,
												   double rotation,
												   double xScale,
												   double yScale) const = 0;

			BPoint				fOldPivot;
			BPoint				fOldTranslation;
			double				fOldRotation;
			double				fOldXScale;
			double				fOldYScale;

			BPoint				fNewPivot;
			BPoint				fNewTranslation;
			double				fNewRotation;
			double				fNewXScale;
			double				fNewYScale;

			BString				fName;
			uint32				fNameIndex;
};

#endif // TRANSFORM_COMMAND_H
