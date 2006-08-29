/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef CANVAS_TRANSFORM_BOX_H
#define CANVAS_TRANSFORM_BOX_H

#include "TransformBox.h"

class CanvasView;

class CanvasTransformBox : public TransformBox {
 public:
								CanvasTransformBox(CanvasView* view);
	virtual						~CanvasTransformBox();

	// TransformBox interface
	virtual	void				TransformFromCanvas(BPoint& point) const;
	virtual	void				TransformToCanvas(BPoint& point) const;
	virtual	float				ZoomLevel() const;
	virtual	double				ViewSpaceRotation() const;

 private:
			CanvasView*			fCanvasView;
			Transformable		fParentTransform;
};

#endif // TRANSFORM_SHAPES_BOX_H

