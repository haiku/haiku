/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef PERSPECTIVE_BOX_H
#define PERSPECTIVE_BOX_H

#include <List.h>
#include <Point.h>
#include <Referenceable.h>

#include "Manipulator.h"


namespace PerspectiveBoxStates {
	class DragState;
}

class CanvasView;
class Command;
class PerspectiveCommand;
class PerspectiveBox;
class PerspectiveTransformer;


class PerspectiveBoxListener {
public:
								PerspectiveBoxListener() {}
	virtual						~PerspectiveBoxListener() {}

	virtual	void				PerspectiveBoxDeleted(
									const PerspectiveBox* box) = 0;
};


class PerspectiveBox : public Manipulator {
public:
								PerspectiveBox(CanvasView* view,
									PerspectiveTransformer* parent);
	virtual						~PerspectiveBox();

	// Manipulator interface
	virtual	void				Draw(BView* into, BRect updateRect);

	virtual	bool				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where);
	virtual	Command*			MouseUp();
	virtual	bool				MouseOver(BPoint where);

	virtual	BRect				Bounds();
	virtual	BRect				TrackingBounds(BView* withinView);

	virtual	void				ModifiersChanged(uint32 modifiers);

	virtual	bool				UpdateCursor();

	virtual	void				AttachedToView(BView* view);
	virtual	void				DetachedFromView(BView* view);

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// PerspectiveTransformBox
			void				TransformTo(BPoint leftTop, BPoint rightTop,
									BPoint leftBottom, BPoint rightBottom);

	virtual	void				Update(bool deep = true);

			Command*			FinishTransaction();

	// Listener support
			bool				AddListener(PerspectiveBoxListener* listener);
			bool				RemoveListener(PerspectiveBoxListener* listener);

private:
			void				_NotifyDeleted() const;
			PerspectiveBoxStates::DragState* _DragStateFor(
									BPoint canvasWhere, float canvasZoom);
			void				_StrokeBWLine(BView* into,
									BPoint from, BPoint to) const;
			void				_StrokeBWPoint(BView* into,
									BPoint point, double angle) const;

private:
			BPoint				fLeftTop;
			BPoint				fRightTop;
			BPoint				fLeftBottom;
			BPoint				fRightBottom;

			PerspectiveCommand*	fCurrentCommand;
			PerspectiveBoxStates::DragState* fCurrentState;

			bool				fDragging;
			BPoint				fMousePos;
			uint32				fModifiers;

			BList				fListeners;

			BRect				fPreviousBox;

			// "static" state objects
			CanvasView*			fCanvasView;
			BReference<PerspectiveTransformer> fPerspective;

			PerspectiveBoxStates::DragState* fDragLTState;
			PerspectiveBoxStates::DragState* fDragRTState;
			PerspectiveBoxStates::DragState* fDragLBState;
			PerspectiveBoxStates::DragState* fDragRBState;
};

#endif // PERSPECTIVE_BOX_H
