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


class Selection;

namespace BPrivate {
namespace Icon {
	class Shape;
	class ShapeContainer;
}
}
using namespace BPrivate::Icon;

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
