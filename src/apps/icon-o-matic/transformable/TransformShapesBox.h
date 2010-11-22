/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef TRANSFORM_SHAPES_BOX_H
#define TRANSFORM_SHAPES_BOX_H


#include "CanvasTransformBox.h"
#include "IconBuild.h"


_BEGIN_ICON_NAMESPACE
	class Shape;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class TransformShapesBox : public CanvasTransformBox {
 public:
								TransformShapesBox(CanvasView* view,
												   const Shape** objects,
												   int32 count);
	virtual						~TransformShapesBox();

	// Observer interface (Manipulator is an Observer)
	virtual	void				ObjectChanged(const Observable* object);

	// TransformBox interface
	virtual	void				Update(bool deep = true);

	virtual	TransformCommand*	MakeCommand(const char* actionName,
											uint32 nameIndex);

	// TransformShapesBox
			Shape**				Shapes() const
									{ return fShapes; }
			int32				CountShapes() const
									{ return fCount; }

 private:
			Shape**				fShapes;
			int32				fCount;

			// saves the transformable objects transformation states
			// prior to this transformation
			double*				fOriginals;

			Transformable		fParentTransform;
};

#endif // TRANSFORM_SHAPES_BOX_H

