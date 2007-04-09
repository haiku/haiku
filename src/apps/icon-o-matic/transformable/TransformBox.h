/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef TRANSFORM_BOX_H
#define TRANSFORM_BOX_H

#include <List.h>

#include "ChannelTransform.h"
#include "Manipulator.h"

class Command;
class StateView;
class DragState;
class TransformBox;
class TransformCommand;

class TransformBoxListener {
 public:
								TransformBoxListener();
	virtual						~TransformBoxListener();

	virtual	void				TransformBoxDeleted(
									const TransformBox* box) = 0;
};

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

	virtual	bool				UpdateCursor();

	virtual	void				AttachedToView(BView* view);
	virtual	void				DetachedFromView(BView* view);

	// TransformBox
	virtual	void				Update(bool deep = true);

			void				OffsetCenter(BPoint offset);
			BPoint				Center() const;
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
	virtual	float				ZoomLevel() const;


	virtual	TransformCommand*	MakeCommand(const char* actionName,
											uint32 nameIndex) = 0;

			bool				IsRotating() const
									{ return fCurrentState == fRotateState; }
	virtual	double				ViewSpaceRotation() const;

	// Listener support
			bool				AddListener(TransformBoxListener* listener);
			bool				RemoveListener(TransformBoxListener* listener);

 private:
			DragState*			_DragStateFor(BPoint canvasWhere,
											  float canvasZoom);
			void				_StrokeBWLine(BView* into,
											  BPoint from, BPoint to) const;
			void				_StrokeBWPoint(BView* into,
											   BPoint point,
											   double angle) const;

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

			BList				fListeners;

 protected:
			void				_NotifyDeleted() const;

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
