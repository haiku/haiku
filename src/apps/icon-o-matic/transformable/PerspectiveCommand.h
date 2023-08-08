/*
 * Copyright 2006, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#ifndef PERSPECTIVE_COMMAND_H
#define PERSPECTIVE_COMMAND_H


#include <Point.h>

#include "Command.h"
#include "PerspectiveBox.h"


class PerspectiveTransformer;


class PerspectiveCommand : public Command, public PerspectiveBoxListener {
public:
								PerspectiveCommand(PerspectiveBox* box,
									PerspectiveTransformer* transformer,
									BPoint leftTop, BPoint rightTop,
									BPoint leftBottom, BPoint rightBottom);
	virtual						~PerspectiveCommand();

	// Command interface
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();
	virtual status_t			Redo();

	virtual void				GetName(BString& name);

	// TransformBoxListener interface
	virtual	void				PerspectiveBoxDeleted(const PerspectiveBox* box);

	// PerspectiveCommand
			void				SetNewPerspective(
									BPoint leftTop, BPoint rightTop,
									BPoint leftBottom, BPoint rightBottom);

private:
			PerspectiveBox*		fTransformBox;
			PerspectiveTransformer*	fTransformer;

			BPoint				fOldLeftTop;
			BPoint				fOldRightTop;
			BPoint				fOldLeftBottom;
			BPoint				fOldRightBottom;

			BPoint				fNewLeftTop;
			BPoint				fNewRightTop;
			BPoint				fNewLeftBottom;
			BPoint				fNewRightBottom;
};

#endif // PERSPECTIVE_COMMAND_H
