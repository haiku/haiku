/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef TRANSFORM_BOX_H
#define TRANSFORM_BOX_H

#include "ChannelTransform.h"
#include "Manipulator.h"

class Command;
class StateView;
class DragState;
class TransformCommand;

class TransformBox : public ChannelTransform,
					 public Manipulator {
 public:
								TransformBox(StateView* view,
											 BRect box);
	virtual						~TransformBox();

	// Manipulator interface
	virtual	void				Draw(BView* into, BRect updateRect);

	virtual	bool				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where);
	virtual	Command*			MouseUp();
	virtual	bool				MouseOver(BPoint where);
	virtual	bool				DoubleClicked(BPoint where);

	virtual	BRect				Bounds();
	virtual	BRect				TrackingBounds(BView* withinView);

	virtual	void				ModifiersChanged(uint32 modifiers);
	virtual	bool				HandleKeyDown(uint32 key, uint32 modifiers,
											  Command** _command);
	virtual	bool				HandleKeyUp(uint32 key, uint32 modifiers,
											Command** _command);

	virtual	void				UpdateCursor();

	virtual	void				AttachedToView(BView* view);
	virtual	void				DetachedFromView(BView* view);

	// TransformBox
	virtual	void				Update(bool deep = true);

			void				OffsetPivot(BPoint offset);
			void				SetBox(BRect box);
			BRect				Box() const
									{ return fOriginalBox; }

			Command*			FinishTransaction();

			void				NudgeBy(BPoint offset);
			bool				IsNudging() const
									{ return fNudging; }
			Command*			FinishNudging();

	virtual	void				TransformFromCanvas(BPoint& point) const;
	virtual	void				TransformToCanvas(BPoint& point) const;


	virtual	TransformCommand*	MakeAction(const char* actionName,
										   uint32 nameIndex) const = 0;

			bool				IsRotating() const
									{ return fCurrentState == fRotateState; }
	virtual	double				ViewSpaceRotation() const;

 private:
			DragState*			_DragStateFor(BPoint canvasWhere,
											  float canvasZoom);
			void				_StrokeBWLine(BView* into,
											  BPoint from, BPoint to) const;
			void				_StrokeBWPoint(BView* into,
											   BPoint point, double angle) const;

			BRect				fOriginalBox;

			BPoint				fLeftTop;
			BPoint				fRightTop;
			BPoint				fLeftBottom;
			BPoint				fRightBottom;

			BPoint				fPivot;
			BPoint				fPivotOffset;

			TransformCommand*	fCurrentCommand;
			DragState*			fCurrentState;

			bool				fDragging;
			BPoint				fMousePos;
			uint32				fModifiers;

			bool				fNudging;

 protected:
			// "static" state objects
			void				_SetState(DragState* state);

			StateView*			fView;

			DragState*			fDragLTState;
			DragState*			fDragRTState;
			DragState*			fDragLBState;
			DragState*			fDragRBState;

			DragState*			fDragLState;
			DragState*			fDragRState;
			DragState*			fDragTState;
			DragState*			fDragBState;

			DragState*			fRotateState;
			DragState*			fTranslateState;
			DragState*			fOffsetCenterState;
};

#endif // TRANSFORM_BOX_H
