/*
 * Copyright 2006-2010, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef TRANSFORM_GRADIENT_COMMAND_H
#define TRANSFORM_GRADIENT_COMMAND_H


#include "IconBuild.h"
#include "TransformBox.h"
#include "TransformCommand.h"


_BEGIN_ICON_NAMESPACE
	class Gradient;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class TransformGradientCommand : public TransformCommand,
	public TransformBoxListener {
public:
								TransformGradientCommand(
									TransformBox* box, Gradient* gradient,
									BPoint pivot, BPoint translation,
									double rotation, double xScale,
									double yScale, const char* name,
									int32 nameIndex);
	virtual						~TransformGradientCommand();

	// Command interface
	virtual	status_t			InitCheck();

	// TransformBoxListener interface
	virtual	void				TransformBoxDeleted(const TransformBox* box);

protected:
 	// TransformCommand interface
	virtual	status_t			_SetTransformation(BPoint pivotDiff,
									BPoint translationDiff,
									double rotationDiff, double xScaleDiff,
									double yScaleDiff) const;

			TransformBox*		fTransformBox;
			Gradient*			fGradient;
};


#endif // TRANSFORM_GRADIENT_COMMAND_H
