/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef REMOVE_SHAPES_COMMAND_H
#define REMOVE_SHAPES_COMMAND_H


#include "Command.h"
#include "IconBuild.h"


_BEGIN_ICON_NAMESPACE
	class Shape;
	class ShapeContainer;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class RemoveShapesCommand : public Command {
 public:
								RemoveShapesCommand(
									ShapeContainer* container,
									int32* const indices,
									int32 count);
	virtual						~RemoveShapesCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			ShapeContainer*		fContainer;
			Shape**				fShapes;
			int32*				fIndices;
			int32				fCount;
			bool				fShapesRemoved;
};

#endif // REMOVE_SHAPES_COMMAND_H
