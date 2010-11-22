/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef TRANSFORM_GRADIENT_BOX_H
#define TRANSFORM_GRADIENT_BOX_H


#include "IconBuild.h"
#include "TransformBox.h"


class CanvasView;

_BEGIN_ICON_NAMESPACE
	class Gradient;
	class Shape;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class TransformGradientBox : public TransformBox {
 public:
								TransformGradientBox(CanvasView* view,
													 Gradient* gradient,
													 Shape* parent);
	virtual						~TransformGradientBox();

	// Observer interface (Manipulator is an Observer)
	virtual	void				ObjectChanged(const Observable* object);

	// TransformBox interface
	virtual	void				Update(bool deep = true);

	virtual	void				TransformFromCanvas(BPoint& point) const;
	virtual	void				TransformToCanvas(BPoint& point) const;
	virtual	float				ZoomLevel() const;
	virtual	double				ViewSpaceRotation() const;

	virtual	TransformCommand*	MakeCommand(const char* actionName,
											uint32 nameIndex);

	// TransformGradientBox
			Command*			Perform();
			Command*			Cancel();
 private:
			CanvasView*			fCanvasView;

			Shape*				fShape;
			Gradient*			fGradient;
			int32				fCount;

			// saves the transformable object transformation states
			// prior to this transformation
			double				fOriginals[matrix_size];
};

#endif // TRANSFORM_GRADIENT_BOX_H

