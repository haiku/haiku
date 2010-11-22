/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef ADD_SHAPES_COMMAND_H
#define ADD_SHAPES_COMMAND_H


#include "Command.h"
#include "IconBuild.h"


class Selection;

_BEGIN_ICON_NAMESPACE
	class Shape;
	class ShapeContainer;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class AddShapesCommand : public Command {
 public:
								AddShapesCommand(
									ShapeContainer* container,
									Shape** const shapes,
									int32 count,
									int32 index,
									Selection* selection);
	virtual						~AddShapesCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			ShapeContainer*		fContainer;
			Shape**				fShapes;
			int32				fCount;
			int32				fIndex;
			bool				fShapesAdded;

			Selection*			fSelection;
};

#endif // ADD_SHAPES_COMMAND_H
