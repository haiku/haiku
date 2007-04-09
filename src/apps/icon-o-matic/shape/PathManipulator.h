/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef PATH_MANIPULATOR_H
#define PATH_MANIPULATOR_H

#include "Manipulator.h"
#include "VectorPath.h"

class AddPointCommand;
class CanvasView;
class ChangePointCommand;
class InsertPointCommand;
class NudgePointsCommand;
class TransformPointsBox;

//class PathSelection {
// public:
//								PathSelection();
//	virtual						~PathSelection();
//
//	virtual	PathSelection*		Clone() const;
//	virtual	bool				SetTo(const PathSelection* other);
//
//	virtual	Command*			Delete();
//};

class PathManipulator : public Manipulator,
						public PathListener {
 public:
								PathManipulator(VectorPath* path);
	virtual						~PathManipulator();

	// Manipulator interface
	virtual	void				Draw(BView* into, BRect updateRect);

	virtual	bool				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where);
	virtual	Command*			MouseUp();
	virtual	bool				MouseOver(BPoint where);
	virtual	bool				DoubleClicked(BPoint where);

	virtual	bool				ShowContextMenu(BPoint where);

	virtual	BRect				Bounds();
	virtual	BRect				TrackingBounds(BView* withinView);

	virtual	bool				MessageReceived(BMessage* message,
												Command** _command);

	virtual	void				ModifiersChanged(uint32 modifiers);
	virtual	bool				HandleKeyDown(uint32 key, uint32 modifiers,
											  Command** _command);
	virtual	bool				HandleKeyUp(uint32 key, uint32 modifiers,
											Command** _command);

	virtual	bool				UpdateCursor();

	virtual	void				AttachedToView(BView* view);
	virtual	void				DetachedFromView(BView* view);

	// Observer interface (Manipulator)
	virtual	void				ObjectChanged(const Observable* object);

	// PathListener interface
	virtual	void				PointAdded(int32 index);
	virtual	void				PointRemoved(int32 index);
	virtual	void				PointChanged(int32 index);
	virtual	void				PathChanged();
	virtual	void				PathClosedChanged();
	virtual	void				PathReversed();

	// PathManipulator
			uint32				ControlFlags() const;

//			PathSelection*		Selection() const;
//
			// path manipulation
			void				ReversePath();

 private:
			friend	class		PathCommand;
			friend	class		PointSelection;
			friend	class		EnterTransformPointsCommand;
			friend	class		ExitTransformPointsCommand;
			friend	class		TransformPointsCommand;
//			friend	class		NewPathCommand;
//			friend	class		RemovePathCommand;
			friend	class		ReversePathCommand;
//			friend	class		SelectPathCommand;

			void				_SetMode(uint32 mode);
			void				_SetTransformBox(
									TransformPointsBox* transformBox);

			// BEGIN functions that need to be undoable
			void				_AddPoint(BPoint where);
			void				_InsertPoint(BPoint where, int32 index);
			void				_SetInOutConnected(int32 index,
												   bool connected);
			void				_SetSharp(int32 index);

			void				_RemoveSelection();
			void				_RemovePoint(int32 index);
			void				_RemovePointIn(int32 index);
			void				_RemovePointOut(int32 index);

			Command*			_Delete();

			void				_Select(BRect canvasRect);
			void				_Select(int32 index, bool extend = false);
			void				_Select(const int32* indices,
										int32 count, bool extend = false);
			void				_Deselect(int32 index);
			void				_ShiftSelection(int32 startIndex,
												int32 direction);
			bool				_IsSelected(int32 index) const;
			// END functions that need to be undoable

			void				_InvalidateCanvas(BRect rect) const;
			void				_InvalidateHighlightPoints(int32 newIndex,
														   uint32 newMode);

			void				_UpdateSelection() const;

			BRect				_ControlPointRect() const;
			BRect				_ControlPointRect(int32 index,
												  uint32 mode) const;
			void				_GetChangableAreas(BRect* pathArea,
												   BRect* controlPointArea) const;

			void				_SetModeForMousePos(BPoint canvasWhere);

			void				_Nudge(BPoint direction);
			Command*			_FinishNudging();

			CanvasView*			fCanvasView;

			bool				fCommandDown;
			bool				fOptionDown;
			bool				fShiftDown;
			bool				fAltDown;

			bool				fClickToClose;

			uint32				fMode;
			uint32				fFallBackMode;

			bool				fMouseDown;
			BPoint				fTrackingStart;
			BPoint				fLastCanvasPos;

			VectorPath*			fPath;
			int32				fCurrentPathPoint;
			BRect				fPreviousBounds;

			ChangePointCommand*	fChangePointCommand;
			InsertPointCommand*	fInsertPointCommand;
			AddPointCommand*	fAddPointCommand;

			class Selection;
			Selection*			fSelection;
			Selection*			fOldSelection;
			TransformPointsBox*	fTransformBox;

			// stuff needed for nudging
			BPoint				fNudgeOffset;
			bigtime_t			fLastNudgeTime;
			NudgePointsCommand*	fNudgeCommand;
};

#endif	// SHAPE_STATE_H
