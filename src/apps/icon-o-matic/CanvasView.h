/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef CANVAS_VIEW_H
#define CANVAS_VIEW_H

#include "StateView.h"

class CanvasView : public StateView {
 public:
								CanvasView(BRect frame);
	virtual						~CanvasView();

 protected:
	// StateView interface
	virtual	bool				_HandleKeyDown(uint32 key, uint32 modifiers);

 private:
};

#endif // CANVAS_VIEW_H
