/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PathManipulator.h"

#include <float.h>
#include <stdio.h>

#include <Catalog.h>
#include <Cursor.h>
#include <Locale.h>
#include <Message.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Window.h>

#include "cursors.h"
#include "support.h"

#include "CanvasView.h"

#include "AddPointCommand.h"
#include "ChangePointCommand.h"
//#include "CloseCommand.h"
#include "InsertPointCommand.h"
#include "FlipPointsCommand.h"
//#include "NewPathCommand.h"
#include "NudgePointsCommand.h"
//#include "RemovePathCommand.h"
#include "RemovePointsCommand.h"
//#include "ReversePathCommand.h"
//#include "SelectPathCommand.h"
//#include "SelectPointsCommand.h"
#include "SplitPointsCommand.h"
#include "TransformPointsBox.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-PathManipulator"
#define POINT_EXTEND 3.0
#define CONTROL_POINT_EXTEND 2.0
#define INSERT_DIST_THRESHOLD 7.0
#define MOVE_THRESHOLD 9.0


enum {
	UNDEFINED,

	NEW_PATH,

	ADD_POINT,
	INSERT_POINT,
	MOVE_POINT,
	MOVE_POINT_IN,
	MOVE_POINT_OUT,
	CLOSE_PATH,

	TOGGLE_SHARP,
	TOGGLE_SHARP_IN,
	TOGGLE_SHARP_OUT,

	REMOVE_POINT,
	REMOVE_POINT_IN,
	REMOVE_POINT_OUT,

	SELECT_POINTS,
	TRANSFORM_POINTS,
	TRANSLATE_POINTS,

	SELECT_SUB_PATH,
};

enum {
	MSG_TRANSFORM				= 'strn',
	MSG_REMOVE_POINTS			= 'srmp',
	MSG_UPDATE_SHAPE_UI			= 'udsi',

	MSG_SPLIT_POINTS			= 'splt',
	MSG_FLIP_POINTS				= 'flip',
};

inline const char*
string_for_mode(uint32 mode)
{
	switch (mode) {
		case UNDEFINED:
			return "UNDEFINED";
		case NEW_PATH:
			return "NEW_PATH";
		case ADD_POINT:
			return "ADD_POINT";
		case INSERT_POINT:
			return "INSERT_POINT";
		case MOVE_POINT:
			return "MOVE_POINT";
		case MOVE_POINT_IN:
			return "MOVE_POINT_IN";
		case MOVE_POINT_OUT:
			return "MOVE_POINT_OUT";
		case CLOSE_PATH:
			return "CLOSE_PATH";
		case TOGGLE_SHARP:
			return "TOGGLE_SHARP";
		case TOGGLE_SHARP_IN:
			return "TOGGLE_SHARP_IN";
		case TOGGLE_SHARP_OUT:
			return "TOGGLE_SHARP_OUT";
		case REMOVE_POINT:
			return "REMOVE_POINT";
		case REMOVE_POINT_IN:
			return "REMOVE_POINT_IN";
		case REMOVE_POINT_OUT:
			return "REMOVE_POINT_OUT";
		case SELECT_POINTS:
			return "SELECT_POINTS";
		case TRANSFORM_POINTS:
			return "TRANSFORM_POINTS";
		case TRANSLATE_POINTS:
			return "TRANSLATE_POINTS";
		case SELECT_SUB_PATH:
			return "SELECT_SUB_PATH";
	}
	return "<unknown mode>";
}

class PathManipulator::Selection : protected BList
{
public:
	inline Selection(int32 count = 20)
		: BList(count) {}
	inline ~Selection() {}

	inline void Add(int32 value)
		{
			if (value >= 0) {
				// keep the list sorted
				int32 count = CountItems();
				int32 index = 0;
				for (; index < count; index++) {
					if (IndexAt(index) > value) {
						break;
					}
				}
				BList::AddItem((void*)value, index);
			}
		}

	inline bool Remove(int32 value)
		{ return BList::RemoveItem((void*)value); }

	inline bool Contains(int32 value) const
		{ return BList::HasItem((void*)value); }

	inline bool IsEmpty() const
		{ return BList::IsEmpty(); }

	inline int32 IndexAt(int32 index) const
		{ return (int32)BList::ItemAt(index); }

	inline void MakeEmpty()
		{ BList::MakeEmpty(); }

	inline int32* Items() const
		{ return (int32*)BList::Items(); }

	inline const int32 CountItems() const
		{ return BList::CountItems(); }

	inline Selection& operator =(const Selection& other)
		{
			MakeEmpty();
			int32 count = other.CountItems();
			int32* items = other.Items();
			for (int32 i = 0; i < count; i++) {
				Add(items[i]);
			}
			return *this;
		}

	inline bool operator ==(const Selection& other)
		{
			if (other.CountItems() == CountItems()) {
				int32* items = Items();
				int32* otherItems = other.Items();
				for (int32 i = 0; i < CountItems(); i++) {
					if (items[i] != otherItems[i])
						return false;
					items++;
					otherItems++;
				}
				return true;
			} else
				return false;
		}

	inline bool operator !=(const Selection& other)
	{
		return !(*this == other);
	}
};


// constructor
PathManipulator::PathManipulator(VectorPath* path)
	: Manipulator(NULL),
	  fCanvasView(NULL),

	  fCommandDown(false),
	  fOptionDown(false),
	  fShiftDown(false),
	  fAltDown(false),

	  fClickToClose(false),

	  fMode(NEW_PATH),
	  fFallBackMode(SELECT_POINTS),

	  fMouseDown(false),

	  fPath(path),
	  fCurrentPathPoint(-1),

	  fChangePointCommand(NULL),
	  fInsertPointCommand(NULL),
	  fAddPointCommand(NULL),

	  fSelection(new Selection()),
	  fOldSelection(new Selection()),
	  fTransformBox(NULL),

	  fNudgeOffset(0.0, 0.0),
	  fLastNudgeTime(system_time()),
	  fNudgeCommand(NULL)
{
	fPath->Acquire();
	fPath->AddListener(this);
	fPath->AddObserver(this);
}

// destructor
PathManipulator::~PathManipulator()
{
	delete fChangePointCommand;
	delete fInsertPointCommand;
	delete fAddPointCommand;

	delete fSelection;
	delete fOldSelection;
	delete fTransformBox;

	delete fNudgeCommand;

	fPath->RemoveObserver(this);
	fPath->RemoveListener(this);
	fPath->Release();
}


// #pragma mark -

class StrokePathIterator : public VectorPath::Iterator {
 public:
					StrokePathIterator(CanvasView* canvasView,
									   BView* drawingView)
						: fCanvasView(canvasView),
						  fDrawingView(drawingView)
					{
						fDrawingView->SetHighColor(0, 0, 0, 255);
						fDrawingView->SetDrawingMode(B_OP_OVER);
					}
	virtual			~StrokePathIterator()
					{}

	virtual	void	MoveTo(BPoint point)
					{
						fBlack = true;
						fSkip = false;
						fDrawingView->SetHighColor(0, 0, 0, 255);

						fCanvasView->ConvertFromCanvas(&point);
						fDrawingView->MovePenTo(point);
					}
	virtual	void	LineTo(BPoint point)
					{
						fCanvasView->ConvertFromCanvas(&point);
						if (!fSkip) {
							if (fBlack)
								fDrawingView->SetHighColor(255, 255, 255, 255);
							else
								fDrawingView->SetHighColor(0, 0, 0, 255);
							fBlack = !fBlack;
	
							fDrawingView->StrokeLine(point);
						} else {
							fDrawingView->MovePenTo(point);
						}
						fSkip = !fSkip;
					}

 private:
	CanvasView*		fCanvasView;
	BView*			fDrawingView;
	bool			fBlack;
	bool			fSkip;
};

// Draw
void
PathManipulator::Draw(BView* into, BRect updateRect)
{
	// draw the Bezier curve, but only if not "editing",
	// the path is actually on top all other modifiers
	// TODO: make this customizable in the GUI

	#if __HAIKU__
	uint32 flags = into->Flags();
	into->SetFlags(flags | B_SUBPIXEL_PRECISE);
	#endif // __HAIKU__

	StrokePathIterator iterator(fCanvasView, into);
	fPath->Iterate(&iterator, fCanvasView->ZoomLevel());

	#if __HAIKU__
	into->SetFlags(flags);
	#endif // __HAIKU__

	into->SetLowColor(0, 0, 0, 255);
	BPoint point;
	BPoint pointIn;
	BPoint pointOut;
	rgb_color focusColor = (rgb_color){ 255, 0, 0, 255 };
	rgb_color highlightColor = (rgb_color){ 60, 60, 255, 255 };
	for (int32 i = 0; fPath->GetPointsAt(i, point, pointIn, pointOut); i++) {
		bool highlight = fCurrentPathPoint == i;
		bool selected = fSelection->Contains(i);
		rgb_color normal = selected ? focusColor : (rgb_color){ 0, 0, 0, 255 };
		into->SetLowColor(normal);
		into->SetHighColor(255, 255, 255, 255);
		// convert to view coordinate space
		fCanvasView->ConvertFromCanvas(&point);
		fCanvasView->ConvertFromCanvas(&pointIn);
		fCanvasView->ConvertFromCanvas(&pointOut);
		// connect the points belonging to one control point
		into->SetDrawingMode(B_OP_INVERT);
		into->StrokeLine(point, pointIn);
		into->StrokeLine(point, pointOut);
		// draw main control point
		if (highlight && (fMode == MOVE_POINT ||
						  fMode == TOGGLE_SHARP ||
						  fMode == REMOVE_POINT ||
						  fMode == SELECT_POINTS ||
						  fMode == CLOSE_PATH)) {

			into->SetLowColor(highlightColor);
		}

		into->SetDrawingMode(B_OP_COPY);
		BRect r(point, point);
		r.InsetBy(-POINT_EXTEND, -POINT_EXTEND);
		into->StrokeRect(r, B_SOLID_LOW);
		r.InsetBy(1.0, 1.0);
		into->FillRect(r, B_SOLID_HIGH);
		// draw in control point
		if (highlight && (fMode == MOVE_POINT_IN ||
						  fMode == TOGGLE_SHARP_IN ||
						  fMode == REMOVE_POINT_IN ||
						  fMode == SELECT_POINTS))
			into->SetLowColor(highlightColor);
		else
			into->SetLowColor(normal);
		if (selected) {
			into->SetHighColor(220, 220, 220, 255);
		} else {
			into->SetHighColor(170, 170, 170, 255);
		}
		if (pointIn != point) {
			r.Set(pointIn.x - CONTROL_POINT_EXTEND, pointIn.y - CONTROL_POINT_EXTEND,
				  pointIn.x + CONTROL_POINT_EXTEND, pointIn.y + CONTROL_POINT_EXTEND);
			into->StrokeRect(r, B_SOLID_LOW);
			r.InsetBy(1.0, 1.0);
			into->FillRect(r, B_SOLID_HIGH);
		}
		// draw out control point
		if (highlight && (fMode == MOVE_POINT_OUT ||
						  fMode == TOGGLE_SHARP_OUT ||
						  fMode == REMOVE_POINT_OUT ||
						  fMode == SELECT_POINTS))
			into->SetLowColor(highlightColor);
		else
			into->SetLowColor(normal);
		if (pointOut != point) {
			r.Set(pointOut.x - CONTROL_POINT_EXTEND, pointOut.y - CONTROL_POINT_EXTEND,
				  pointOut.x + CONTROL_POINT_EXTEND, pointOut.y + CONTROL_POINT_EXTEND);
			into->StrokeRect(r, B_SOLID_LOW);
			r.InsetBy(1.0, 1.0);
			into->FillRect(r, B_SOLID_HIGH);
		}
	}

	if (fTransformBox) {
		fTransformBox->Draw(into, updateRect);
	}
}

// #pragma mark -

// MouseDown
bool
PathManipulator::MouseDown(BPoint where)
{
	fMouseDown = true;

	if (fMode == TRANSFORM_POINTS) {
		if (fTransformBox) {
			fTransformBox->MouseDown(where);

//			if (!fTransformBox->IsRotating())
//				fCanvasView->SetAutoScrolling(true);
		}
		return true;
	}

	if (fMode == MOVE_POINT &&
		fSelection->CountItems() > 1 &&
		fSelection->Contains(fCurrentPathPoint)) {
		fMode = TRANSLATE_POINTS;
	}

	// apply the canvas view mouse filter depending on current mode
	if (fMode == ADD_POINT || fMode == TRANSLATE_POINTS)
		fCanvasView->FilterMouse(&where);

	BPoint canvasWhere = where;
	fCanvasView->ConvertToCanvas(&canvasWhere);

	// maybe we're changing some point, so we construct the
	// "ChangePointCommand" here so that the point is remembered
	// in its current state
	// apply the canvas view mouse filter depending on current mode
	delete fChangePointCommand;
	fChangePointCommand = NULL;
	switch (fMode) {
		case TOGGLE_SHARP:
		case TOGGLE_SHARP_IN:
		case TOGGLE_SHARP_OUT:
		case MOVE_POINT:
		case MOVE_POINT_IN:
		case MOVE_POINT_OUT:
		case REMOVE_POINT_IN:
		case REMOVE_POINT_OUT:
			fChangePointCommand = new ChangePointCommand(fPath,
														 fCurrentPathPoint,
														 fSelection->Items(),
														 fSelection->CountItems());
			_Select(fCurrentPathPoint, fShiftDown);
			break;
	}

	// at this point we init doing something
	switch (fMode) {
		case ADD_POINT:
			_AddPoint(canvasWhere);
			break;
		case INSERT_POINT:
			_InsertPoint(canvasWhere, fCurrentPathPoint);
			break;

		case TOGGLE_SHARP:
			_SetSharp(fCurrentPathPoint);
			// continue by dragging out the _connected_ in/out points
			break;
		case TOGGLE_SHARP_IN:
			_SetInOutConnected(fCurrentPathPoint, false);
			// continue by moving the "in" point
			_SetMode(MOVE_POINT_IN);
			break;
		case TOGGLE_SHARP_OUT:
			_SetInOutConnected(fCurrentPathPoint, false);
			// continue by moving the "out" point
			_SetMode(MOVE_POINT_OUT);
			break;

		case MOVE_POINT:
		case MOVE_POINT_IN:
		case MOVE_POINT_OUT:
			// the right thing happens since "fCurrentPathPoint"
			// points to the correct index
			break;

		case CLOSE_PATH:
//			SetClosed(true, true);
			break;

		case REMOVE_POINT:
			if (fPath->CountPoints() == 1) {
//				fCanvasView->Perform(new RemovePathCommand(this, fPath));
			} else {
				fCanvasView->Perform(new RemovePointsCommand(fPath,
															 fCurrentPathPoint,
															 fSelection->Items(),
															 fSelection->CountItems()));
				_RemovePoint(fCurrentPathPoint);
			}
			break;
		case REMOVE_POINT_IN:
			_RemovePointIn(fCurrentPathPoint);
			break;
		case REMOVE_POINT_OUT:
			_RemovePointOut(fCurrentPathPoint);
			break;

		case SELECT_POINTS: {
			// TODO: this works so that you can deselect all points
			// when clicking outside the path even if pressing shift
			// in case the path is open... a better way would be
			// to deselect all on mouse up, if the mouse has not moved
			bool appendSelection;
			if (fPath->IsClosed())
				appendSelection = fShiftDown;
			else
				appendSelection = fShiftDown && fCurrentPathPoint >= 0;

			if (!appendSelection) {
				fSelection->MakeEmpty();
				_UpdateSelection();
			}
			*fOldSelection = *fSelection;
			if (fCurrentPathPoint >= 0) {
				_Select(fCurrentPathPoint, appendSelection);
			}
			fCanvasView->BeginRectTracking(BRect(where, where),
				B_TRACK_RECT_CORNER);
			break;
		}
	}

	fTrackingStart = canvasWhere;
	// remember the subpixel position
	// so that MouseMoved() will work even before
	// the integer position becomes different
	fLastCanvasPos = where;
	fCanvasView->ConvertToCanvas(&fLastCanvasPos);

	// the reason to exclude the select mode
	// is that the BView rect tracking does not
	// scroll the rect starting point along with us
	// (since we're doing no real scrolling)
//	if (fMode != SELECT_POINTS)
//		fCanvasView->SetAutoScrolling(true);

	UpdateCursor();

	return true;
}

// MouseMoved
void
PathManipulator::MouseMoved(BPoint where)
{
	fCanvasView->FilterMouse(&where);
		// NOTE: only filter mouse coords in mouse moved, no other
		// mouse function
	BPoint canvasWhere = where;
	fCanvasView->ConvertToCanvas(&canvasWhere);

	// since the tablet is generating mouse moved messages
	// even if only the pressure changes (and not the actual mouse position)
	// we insert this additional check to prevent too much calculation
	if (fLastCanvasPos == canvasWhere)
		return;

	fLastCanvasPos = canvasWhere;

	if (fMode == TRANSFORM_POINTS) {
		if (fTransformBox) {
			fTransformBox->MouseMoved(where);
		}
		return;
	}

	if (fMode == CLOSE_PATH) {
		// continue by moving the point
		_SetMode(MOVE_POINT);
		delete fChangePointCommand;
		fChangePointCommand = new ChangePointCommand(fPath,
													 fCurrentPathPoint,
													 fSelection->Items(),
													 fSelection->CountItems());
	}

//	if (!fPrecise) {
//		float offset = fmod(fOutlineWidth, 2.0) / 2.0;
//		canvasWhere.point += BPoint(offset, offset);
//	}

	switch (fMode) {
		case ADD_POINT:
		case INSERT_POINT:
		case TOGGLE_SHARP:
			// drag the "out" control point, mirror the "in" control point
			fPath->SetPointOut(fCurrentPathPoint, canvasWhere, true);
			break;
		case MOVE_POINT:
			// drag all three control points at once
			fPath->SetPoint(fCurrentPathPoint, canvasWhere);
			break;
		case MOVE_POINT_IN:
			// drag in control point
			fPath->SetPointIn(fCurrentPathPoint, canvasWhere);
			break;
		case MOVE_POINT_OUT:
			// drag out control point
			fPath->SetPointOut(fCurrentPathPoint, canvasWhere);
			break;
		
		case SELECT_POINTS: {
			// change the selection
			BRect r;
			r.left = min_c(fTrackingStart.x, canvasWhere.x);
			r.top = min_c(fTrackingStart.y, canvasWhere.y);
			r.right = max_c(fTrackingStart.x, canvasWhere.x);
			r.bottom = max_c(fTrackingStart.y, canvasWhere.y);
			_Select(r);
			break;
		}

		case TRANSLATE_POINTS: {
			BPoint offset = canvasWhere - fTrackingStart;
			_Nudge(offset);
			fTrackingStart = canvasWhere;
			break;
		}
	}
}

// MouseUp
Command*
PathManipulator::MouseUp()
{
	// prevent carrying out actions more than once by only
	// doing it if "fMouseDown" is true at the point of
	// entering this function
	if (!fMouseDown)
		return NULL;
	fMouseDown = false;

	if (fMode == TRANSFORM_POINTS) {
		if (fTransformBox) {
			return fTransformBox->MouseUp();
		}
		return NULL;
	}

	Command* command = NULL;

	switch (fMode) {

		case ADD_POINT:
			command = fAddPointCommand;
			fAddPointCommand = NULL;
			_SetMode(MOVE_POINT_OUT);
			break;

		case INSERT_POINT:
			command = fInsertPointCommand;
			fInsertPointCommand = NULL;
			break;

		case SELECT_POINTS:
			if (*fSelection != *fOldSelection) {
//				command = new SelectPointsCommand(this, fPath,
//												  fOldSelection->Items(),
//												  fOldSelection->CountItems(),
//												  fSelection->Items(),
//												  fSelection->CountItems()));
			}
			fCanvasView->EndRectTracking();
			break;

		case TOGGLE_SHARP:
		case TOGGLE_SHARP_IN:
		case TOGGLE_SHARP_OUT:
		case MOVE_POINT:
		case MOVE_POINT_IN:
		case MOVE_POINT_OUT:
		case REMOVE_POINT_IN:
		case REMOVE_POINT_OUT:
			command = fChangePointCommand;
			fChangePointCommand = NULL;
			break;

		case TRANSLATE_POINTS:
			if (!fNudgeCommand) {
				// select just the point that was clicked
				*fOldSelection = *fSelection;
				if (fCurrentPathPoint >= 0) {
					_Select(fCurrentPathPoint, fShiftDown);
				}
				if (*fSelection != *fOldSelection) {
//					command = new SelectPointsCommand(this, fPath,
//													  fOldSelection->Items(),
//													  fOldSelection->CountItems(),
//													  fSelection->Items(),
//													  fSelection->CountItems()));
				}
			} else {
				command = _FinishNudging();
			}
			break;
	}

	return command;
}

// MouseOver
bool
PathManipulator::MouseOver(BPoint where)
{
	if (fMode == TRANSFORM_POINTS) {
		if (fTransformBox) {
			return fTransformBox->MouseOver(where);
		}
		return false;
	}

	BPoint canvasWhere = where;
	fCanvasView->ConvertToCanvas(&canvasWhere);

	// since the tablet is generating mouse moved messages
	// even if only the pressure changes (and not the actual mouse position)
	// we insert this additional check to prevent too much calculation
	if (fMouseDown && fLastCanvasPos == canvasWhere)
		return false;

	fLastCanvasPos = canvasWhere;

	// hit testing
	// (use a subpixel mouse pos)
	fCanvasView->ConvertToCanvas(&where);
	_SetModeForMousePos(where);

	// TODO: always true?
	return true;
}

// DoubleClicked
bool
PathManipulator::DoubleClicked(BPoint where)
{
	return false;
}

// ShowContextMenu
bool
PathManipulator::ShowContextMenu(BPoint where)
{
	// Change the selection to the current point if it isn't currently
	// selected. This could will only be chosen if the user right-clicked
	// a path point directly. 
	if (fCurrentPathPoint >= 0 && !fSelection->Contains(fCurrentPathPoint)) {
		fSelection->MakeEmpty();
		_UpdateSelection();
		*fOldSelection = *fSelection;
		_Select(fCurrentPathPoint, false);
	}

	BPopUpMenu* menu = new BPopUpMenu("context menu", false, false);
	BMessage* message;
	BMenuItem* item;

	bool hasSelection = fSelection->CountItems() > 0;

	if (fCurrentPathPoint < 0) {
		message = new BMessage(B_SELECT_ALL);
		item = new BMenuItem(B_TRANSLATE("Select All"), message, 'A');
		menu->AddItem(item);
	
		menu->AddSeparatorItem();
	}

	message = new BMessage(MSG_TRANSFORM);
	item = new BMenuItem(B_TRANSLATE("Transform"), message, 'T');
	item->SetEnabled(hasSelection);
	menu->AddItem(item);

	message = new BMessage(MSG_SPLIT_POINTS);
	item = new BMenuItem(B_TRANSLATE("Split"), message);
	item->SetEnabled(hasSelection);
	menu->AddItem(item);

	message = new BMessage(MSG_FLIP_POINTS);
	item = new BMenuItem(B_TRANSLATE("Flip"), message);
	item->SetEnabled(hasSelection);
	menu->AddItem(item);

	message = new BMessage(MSG_REMOVE_POINTS);
	item = new BMenuItem(B_TRANSLATE("Remove"), message);
	item->SetEnabled(hasSelection);
	menu->AddItem(item);

	// go
	menu->SetTargetForItems(fCanvasView);
	menu->SetAsyncAutoDestruct(true);
	menu->SetFont(be_plain_font);
	where = fCanvasView->ConvertToScreen(where);
	BRect mouseRect(where, where);
	mouseRect.InsetBy(-10.0, -10.0);
	where += BPoint(5.0, 5.0);
	menu->Go(where, true, false, mouseRect, true);

	return true;
}

// #pragma mark -

// Bounds
BRect
PathManipulator::Bounds()
{
	BRect r = _ControlPointRect();
	fCanvasView->ConvertFromCanvas(&r);
	return r;
}

// TrackingBounds
BRect
PathManipulator::TrackingBounds(BView* withinView)
{
	return withinView->Bounds();
}

// #pragma mark -

// MessageReceived
bool
PathManipulator::MessageReceived(BMessage* message, Command** _command)
{
	bool result = true;
	switch (message->what) {
		case MSG_TRANSFORM:
			if (!fSelection->IsEmpty())
				_SetMode(TRANSFORM_POINTS);
			break;
		case MSG_REMOVE_POINTS:
			*_command = _Delete();
			break;
		case MSG_SPLIT_POINTS:
			*_command = new SplitPointsCommand(fPath,
											   fSelection->Items(),
											   fSelection->CountItems());
			break;
		case MSG_FLIP_POINTS:
			*_command = new FlipPointsCommand(fPath,
											  fSelection->Items(),
											  fSelection->CountItems());
			break;
		case B_SELECT_ALL: {
			*fOldSelection = *fSelection;
			fSelection->MakeEmpty();
			int32 count = fPath->CountPoints();
			for (int32 i = 0; i < count; i++)
				fSelection->Add(i);
			if (*fOldSelection != *fSelection) {
//				*_command = new SelectPointsCommand(this, fPath,
//												   fOldSelection->Items(),
//												   fOldSelection->CountItems(),
//												   fSelection->Items(),
//												   fSelection->CountItems()));
				count = fSelection->CountItems();
				int32 indices[count];
				memcpy(indices, fSelection->Items(), count * sizeof(int32));
				_Select(indices, count);
			}
			break;
		}
		default:
			result = false;
			break;
	}
	return result;
}


// ModifiersChanged
void
PathManipulator::ModifiersChanged(uint32 modifiers)
{
	fCommandDown = modifiers & B_COMMAND_KEY;
	fOptionDown = modifiers & B_CONTROL_KEY;
	fShiftDown = modifiers & B_SHIFT_KEY;
	fAltDown = modifiers & B_OPTION_KEY;

	if (fTransformBox) {
		fTransformBox->ModifiersChanged(modifiers);
		return;
	}
	// reevaluate mode
	if (!fMouseDown)
		_SetModeForMousePos(fLastCanvasPos);
}

// HandleKeyDown
bool
PathManipulator::HandleKeyDown(uint32 key, uint32 modifiers, Command** _command)
{
	bool result = true;

	float nudgeDist = 1.0;
	if (modifiers & B_SHIFT_KEY)
		nudgeDist /= fCanvasView->ZoomLevel();

	switch (key) {
		// commit
		case B_RETURN:
			if (fTransformBox) {
				_SetTransformBox(NULL);
			}// else
//				_Perform();
			break;
		// cancel
		case B_ESCAPE:
			if (fTransformBox) {
				fTransformBox->Cancel();
				_SetTransformBox(NULL);
			} else if (fFallBackMode == NEW_PATH) {
				fFallBackMode = SELECT_POINTS;
				_SetTransformBox(NULL);
			}// else
//				_Cancel();
			break;
		case 't':
		case 'T':
			if (!fSelection->IsEmpty())
				_SetMode(TRANSFORM_POINTS);
			else
				result = false;
			break;
		// nudging
		case B_UP_ARROW:
			_Nudge(BPoint(0.0, -nudgeDist));
			break;
		case B_DOWN_ARROW:
			_Nudge(BPoint(0.0, nudgeDist));
			break;
		case B_LEFT_ARROW:
			_Nudge(BPoint(-nudgeDist, 0.0));
			break;
		case B_RIGHT_ARROW:
			_Nudge(BPoint(nudgeDist, 0.0));
			break;

		case B_DELETE:
			if (!fSelection->IsEmpty())
				*_command = _Delete();
			else
				result = false;
			break;

		default:
			result = false;
	}
	return result;
}

// HandleKeyUp
bool
PathManipulator::HandleKeyUp(uint32 key, uint32 modifiers, Command** _command)
{
	bool handled = true;
	switch (key) {
		// nudging
		case B_UP_ARROW:
		case B_DOWN_ARROW:
		case B_LEFT_ARROW:
		case B_RIGHT_ARROW:
			*_command = _FinishNudging();
			break;
		default:
			handled = false;
			break;
	}
	return handled;
}

// UpdateCursor
bool
PathManipulator::UpdateCursor()
{
	if (fTransformBox)
		return fTransformBox->UpdateCursor();

	const uchar* cursorData;
	switch (fMode) {
		case ADD_POINT:
			cursorData = kPathAddCursor;
			break;
		case INSERT_POINT:
			cursorData = kPathInsertCursor;
			break;
		case MOVE_POINT:
		case MOVE_POINT_IN:
		case MOVE_POINT_OUT:
		case TRANSLATE_POINTS:
			cursorData = kPathMoveCursor;
			break;
		case CLOSE_PATH:
			cursorData = kPathCloseCursor;
			break;
		case TOGGLE_SHARP:
		case TOGGLE_SHARP_IN:
		case TOGGLE_SHARP_OUT:
			cursorData = kPathSharpCursor;
			break;
		case REMOVE_POINT:
		case REMOVE_POINT_IN:
		case REMOVE_POINT_OUT:
			cursorData = kPathRemoveCursor;
			break;
		case SELECT_POINTS:
			cursorData = kPathSelectCursor;
			break;

		case SELECT_SUB_PATH:
			cursorData = B_HAND_CURSOR;
			break;

		case UNDEFINED:
		default:
			cursorData = kStopCursor;
			break;
	}
	BCursor cursor(cursorData);
	fCanvasView->SetViewCursor(&cursor, true);
	fCanvasView->Sync();

	return true;
}

// AttachedToView
void
PathManipulator::AttachedToView(BView* view)
{
	fCanvasView = dynamic_cast<CanvasView*>(view);
}

// DetachedFromView
void
PathManipulator::DetachedFromView(BView* view)
{
	fCanvasView = NULL;
}

// #pragma mark -

// ObjectChanged
void
PathManipulator::ObjectChanged(const Observable* object)
{
	// TODO: refine VectorPath listener interface and
	// implement more efficiently
	BRect currentBounds = _ControlPointRect();
	_InvalidateCanvas(currentBounds | fPreviousBounds);
	fPreviousBounds = currentBounds;

	// reevaluate mode
	if (!fMouseDown && !fTransformBox)
		_SetModeForMousePos(fLastCanvasPos);
}

// #pragma mark -

// PointAdded
void
PathManipulator::PointAdded(int32 index)
{
	ObjectChanged(fPath);
}

// PointRemoved
void
PathManipulator::PointRemoved(int32 index)
{
	fSelection->Remove(index);
	ObjectChanged(fPath);
}

// PointChanged
void
PathManipulator::PointChanged(int32 index)
{
	ObjectChanged(fPath);
}

// PathChanged
void
PathManipulator::PathChanged()
{
	ObjectChanged(fPath);
}

// PathClosedChanged
void
PathManipulator::PathClosedChanged()
{
	ObjectChanged(fPath);
}

// PathReversed
void
PathManipulator::PathReversed()
{
	// reverse selection along with path
	int32 count = fSelection->CountItems();
	int32 pointCount = fPath->CountPoints();
	if (count > 0) {
		Selection temp;
		for (int32 i = 0; i < count; i++) {
			temp.Add((pointCount - 1) - fSelection->IndexAt(i));
		}
		*fSelection = temp;
	}

	ObjectChanged(fPath);
}

// #pragma mark -

// ControlFlags
uint32
PathManipulator::ControlFlags() const
{
	uint32 flags = 0;

//	flags |= SHAPE_UI_FLAGS_CAN_REVERSE_PATH;
//
//	if (!fSelection->IsEmpty())
//		flags |= SHAPE_UI_FLAGS_HAS_SELECTION;
//	if (fPath->CountPoints() > 1)
//		flags |= SHAPE_UI_FLAGS_CAN_CLOSE_PATH;
//	if (fPath->IsClosed())
//		flags |= SHAPE_UI_FLAGS_PATH_IS_CLOSED;
//	if (fTransformBox)
//		flags |= SHAPE_UI_FLAGS_IS_TRANSFORMING;

	return flags;
}

// ReversePath
void
PathManipulator::ReversePath()
{
	int32 count = fSelection->CountItems();
	int32 pointCount = fPath->CountPoints();
	if (count > 0) {
		Selection temp;
		for (int32 i = 0; i < count; i++) {
			temp.Add((pointCount - 1) - fSelection->IndexAt(i));
		}
		*fSelection = temp;
	}
	fPath->Reverse();
}

// #pragma mark -

// _SetMode
void
PathManipulator::_SetMode(uint32 mode)
{
	if (fMode != mode) {
//printf("switching mode: %s -> %s\n", string_for_mode(fMode), string_for_mode(mode));
		fMode = mode;

		if (fMode == TRANSFORM_POINTS) {
			_SetTransformBox(new TransformPointsBox(fCanvasView,
													this,
													fPath,
													fSelection->Items(),
													fSelection->CountItems()));
//			fCanvasView->Perform(new EnterTransformPointsCommand(this,
//														  fSelection->Items(),
//														  fSelection->CountItems()));
		} else {
			if (fTransformBox)
				_SetTransformBox(NULL);
		}

		if (BWindow* window = fCanvasView->Window()) {
			window->PostMessage(MSG_UPDATE_SHAPE_UI);
		}
		UpdateCursor();
	}
}


// _SetTransformBox
void
PathManipulator::_SetTransformBox(TransformPointsBox* transformBox)
{
	if (fTransformBox == transformBox)
		return;

	BRect dirty(LONG_MAX, LONG_MAX, LONG_MIN, LONG_MIN);
	if (fTransformBox) {
		// get rid of transform box display
		dirty = fTransformBox->Bounds();
		delete fTransformBox;
	}

	fTransformBox = transformBox;

	// TODO: this is weird, fMode should only be set in _SetMode, not
	// here as well, also this method could be called this way
	// _SetModeForMousePos -> _SetMode -> _SetTransformBox
	// and then below it does _SetModeForMousePos again...
	if (fTransformBox) {
		fTransformBox->MouseMoved(fLastCanvasPos);
		if (fMode != TRANSFORM_POINTS) {
			fMode = TRANSFORM_POINTS;
		}
		dirty = dirty | fTransformBox->Bounds();
	} else {
		if (fMode == TRANSFORM_POINTS) {
			_SetModeForMousePos(fLastCanvasPos);
		}
	}

	if (dirty.IsValid()) {
		dirty.InsetBy(-8, -8);
		fCanvasView->Invalidate(dirty);
	}
}

// _AddPoint
void
PathManipulator::_AddPoint(BPoint where)
{
	if (fPath->AddPoint(where)) {
		fCurrentPathPoint = fPath->CountPoints() - 1;

		delete fAddPointCommand;
		fAddPointCommand = new AddPointCommand(fPath, fCurrentPathPoint,
											   fSelection->Items(),
											   fSelection->CountItems());

		_Select(fCurrentPathPoint, fShiftDown);
	}
}

// scale_point
BPoint
scale_point(BPoint a, BPoint b, float scale)
{
	return BPoint(a.x + (b.x - a.x) * scale,
				  a.y + (b.y - a.y) * scale);
}

// _InsertPoint
void
PathManipulator::_InsertPoint(BPoint where, int32 index)
{
	double scale;

	BPoint point;
	BPoint pointIn;
	BPoint pointOut;

	BPoint previous;
	BPoint previousOut;
	BPoint next;
	BPoint nextIn;

	if (fPath->FindBezierScale(index - 1, where, &scale)
		&& scale >= 0.0 && scale <= 1.0
		&& fPath->GetPoint(index - 1, scale, point)) {

		fPath->GetPointAt(index - 1, previous);
		fPath->GetPointOutAt(index - 1, previousOut);
		fPath->GetPointAt(index, next);
		fPath->GetPointInAt(index, nextIn);

		where = scale_point(previousOut, nextIn, scale);

		previousOut = scale_point(previous, previousOut, scale);
		nextIn = scale_point(next, nextIn, 1 - scale);
		pointIn = scale_point(previousOut, where, scale);
		pointOut = scale_point(nextIn, where, 1 - scale);
		
		if (fPath->AddPoint(point, index)) {

			fPath->SetPointIn(index, pointIn);
			fPath->SetPointOut(index, pointOut);

			delete fInsertPointCommand;
			fInsertPointCommand = new InsertPointCommand(fPath, index,
														 fSelection->Items(),
														 fSelection->CountItems());

			fPath->SetPointOut(index - 1, previousOut);
			fPath->SetPointIn(index + 1, nextIn);

			fCurrentPathPoint = index;
			_ShiftSelection(fCurrentPathPoint, 1);
			_Select(fCurrentPathPoint, fShiftDown);
		}
	}
}

// _SetInOutConnected
void
PathManipulator::_SetInOutConnected(int32 index, bool connected)
{
	fPath->SetInOutConnected(index, connected);
}

// _SetSharp
void
PathManipulator::_SetSharp(int32 index)
{
	BPoint p;
	fPath->GetPointAt(index, p);
	fPath->SetPoint(index, p, p, p, true);
}

// _RemoveSelection
void
PathManipulator::_RemoveSelection()
{
	// NOTE: copy selection since removing points will
	// trigger notifications, and that will influence the
	// selection
	Selection selection = *fSelection;
	int32 count = selection.CountItems();
	for (int32 i = 0; i < count; i++) {
		if (!fPath->RemovePoint(selection.IndexAt(i) - i))
			break;
	}

	fPath->SetClosed(fPath->IsClosed() && fPath->CountPoints() > 1);

	fSelection->MakeEmpty();
}


// _RemovePoint
void
PathManipulator::_RemovePoint(int32 index)
{
	if (fPath->RemovePoint(index)) {
		_Deselect(index);
		_ShiftSelection(index + 1, -1);
	}
}

// _RemovePointIn
void
PathManipulator::_RemovePointIn(int32 index)
{
	BPoint p;
	if (fPath->GetPointAt(index, p)) {
		fPath->SetPointIn(index, p);
		fPath->SetInOutConnected(index, false);
	}
}

// _RemovePointOut
void
PathManipulator::_RemovePointOut(int32 index)
{
	BPoint p;
	if (fPath->GetPointAt(index, p)) {
		fPath->SetPointOut(index, p);
		fPath->SetInOutConnected(index, false);
	}
}

// _Delete
Command*
PathManipulator::_Delete()
{
	Command* command = NULL;
	if (!fMouseDown) {
		// make sure we apply an on-going transformation before we proceed
		if (fTransformBox) {
			_SetTransformBox(NULL);
		}

		if (fSelection->CountItems() == fPath->CountPoints()) {
//			command = new RemovePathCommand(fPath);
		} else {
			command = new RemovePointsCommand(fPath,
											  fSelection->Items(),
											  fSelection->CountItems());
			_RemoveSelection();
		}

		_SetModeForMousePos(fLastCanvasPos);
	}

	return command;
}

// #pragma mark -

// _Select
void
PathManipulator::_Select(BRect r)
{
	BPoint p;
	BPoint pIn;
	BPoint pOut;
	int32 count = fPath->CountPoints();
	Selection temp;
	for (int32 i = 0; i < count && fPath->GetPointsAt(i, p, pIn, pOut); i++) {
		if (r.Contains(p) || r.Contains(pIn) || r.Contains(pOut)) {
			temp.Add(i);
		}
	}
	// merge old and new selection
	count = fOldSelection->CountItems();
	for (int32 i = 0; i < count; i++) {
		int32 index = fOldSelection->IndexAt(i);
		if (temp.Contains(index))
			temp.Remove(index);
		else
			temp.Add(index);
	}
	if (temp != *fSelection) {
		*fSelection = temp;
		_UpdateSelection();
	}
}

// _Select
void
PathManipulator::_Select(int32 index, bool extend)
{
	if (!extend)
		fSelection->MakeEmpty();
	if (fSelection->Contains(index))
		fSelection->Remove(index);
	else
		fSelection->Add(index);
	// TODO: this can lead to unnecessary invalidation (maybe need to investigate)
	_UpdateSelection();
}

// _Select
void
PathManipulator::_Select(const int32* indices, int32 count, bool extend)
{
	if (extend) {
		for (int32 i = 0; i < count; i++) {
			if (!fSelection->Contains(indices[i]))
				fSelection->Add(indices[i]);
		}
	} else {
		fSelection->MakeEmpty();
		for (int32 i = 0; i < count; i++) {
			fSelection->Add(indices[i]);
		}
	}
	_UpdateSelection();
}

// _Deselect
void
PathManipulator::_Deselect(int32 index)
{
	if (fSelection->Contains(index)) {
		fSelection->Remove(index);
		_UpdateSelection();
	}
}

// _ShiftSelection
void
PathManipulator::_ShiftSelection(int32 startIndex, int32 direction)
{
	int32 count = fSelection->CountItems();
	if (count > 0) {
		int32* selection = fSelection->Items();
		for (int32 i = 0; i < count; i++) {
			if (selection[i] >= startIndex) {
				selection[i] += direction;
			}
		}
	}
	_UpdateSelection();
}

// _IsSelected
bool
PathManipulator::_IsSelected(int32 index) const
{
	return fSelection->Contains(index);
}

// #pragma mark -

// _InvalidateCanvas
void
PathManipulator::_InvalidateCanvas(BRect rect) const
{
	// convert from canvas to view space
	fCanvasView->ConvertFromCanvas(&rect);
	fCanvasView->Invalidate(rect);
}

// _InvalidateHighlightPoints
void
PathManipulator::_InvalidateHighlightPoints(int32 newIndex, uint32 newMode)
{
	BRect oldRect = _ControlPointRect(fCurrentPathPoint, fMode);
	BRect newRect = _ControlPointRect(newIndex, newMode);
	if (oldRect.IsValid())
		_InvalidateCanvas(oldRect);
	if (newRect.IsValid())
		_InvalidateCanvas(newRect);
}

// _UpdateSelection
void
PathManipulator::_UpdateSelection() const
{
	_InvalidateCanvas(_ControlPointRect());
	if (BWindow* window = fCanvasView->Window()) {
		window->PostMessage(MSG_UPDATE_SHAPE_UI);
	}
}

// _ControlPointRect
BRect
PathManipulator::_ControlPointRect() const
{
	BRect r = fPath->ControlPointBounds();
	r.InsetBy(-POINT_EXTEND, -POINT_EXTEND);
	return r; 
}

// _ControlPointRect
BRect
PathManipulator::_ControlPointRect(int32 index, uint32 mode) const
{
	BRect rect(0.0, 0.0, -1.0, -1.0);
	if (index >= 0) {
		BPoint p, pIn, pOut;
		fPath->GetPointsAt(index, p, pIn, pOut);
		switch (mode) {
			case MOVE_POINT:
			case TOGGLE_SHARP:
			case REMOVE_POINT:
			case CLOSE_PATH:
				rect.Set(p.x, p.y, p.x, p.y);
				rect.InsetBy(-POINT_EXTEND, -POINT_EXTEND);
				break;
			case MOVE_POINT_IN:
			case TOGGLE_SHARP_IN:
			case REMOVE_POINT_IN:
				rect.Set(pIn.x, pIn.y, pIn.x, pIn.y);
				rect.InsetBy(-CONTROL_POINT_EXTEND, -CONTROL_POINT_EXTEND);
				break;
			case MOVE_POINT_OUT:
			case TOGGLE_SHARP_OUT:
			case REMOVE_POINT_OUT:
				rect.Set(pOut.x, pOut.y, pOut.x, pOut.y);
				rect.InsetBy(-CONTROL_POINT_EXTEND, -CONTROL_POINT_EXTEND);
				break;
			case SELECT_POINTS:
				rect.Set(min4(p.x, pIn.x, pOut.x, pOut.x),
						 min4(p.y, pIn.y, pOut.y, pOut.y),
						 max4(p.x, pIn.x, pOut.x, pOut.x),
						 max4(p.y, pIn.y, pOut.y, pOut.y));
				rect.InsetBy(-POINT_EXTEND, -POINT_EXTEND);
				break;
		}
	}
	return rect;
}

// #pragma mark -

// _SetModeForMousePos
void
PathManipulator::_SetModeForMousePos(BPoint where)
{
	uint32 mode = UNDEFINED;
	int32 index = -1;

	float zoomLevel = fCanvasView->ZoomLevel();

	// see if we're close enough at a control point
	BPoint point;
	BPoint pointIn;
	BPoint pointOut;
	for (int32 i = 0; fPath->GetPointsAt(i, point, pointIn, pointOut)
					  && mode == UNDEFINED; i++) {

		float distM = point_point_distance(point, where) * zoomLevel;
		float distIn = point_point_distance(pointIn, where) * zoomLevel;
		float distOut = point_point_distance(pointOut, where) * zoomLevel;
		
		if (distM < MOVE_THRESHOLD) {
			if (i == 0 && fClickToClose
				&& !fPath->IsClosed() && fPath->CountPoints() > 1) {
				mode = fCommandDown ? TOGGLE_SHARP :
							(fOptionDown ? REMOVE_POINT : CLOSE_PATH);
				index = i;
			} else {
				mode = fCommandDown ? TOGGLE_SHARP :
							(fOptionDown ? REMOVE_POINT : MOVE_POINT);
				index = i;
			}
		}
		if (distM - distIn > 0.00001
			&& distIn < MOVE_THRESHOLD) {
			mode = fCommandDown ? TOGGLE_SHARP_IN : 
						(fOptionDown ? REMOVE_POINT_IN : MOVE_POINT_IN);
			index = i;
		}
		if (distIn - distOut > 0.00001
			&& distOut < distM && distOut < MOVE_THRESHOLD) {
			mode = fCommandDown ? TOGGLE_SHARP_OUT :
						(fOptionDown ? REMOVE_POINT_OUT : MOVE_POINT_OUT);
			index = i;
		}
	}
	// selection mode overrides any other mode,
	// but we need to check for it after we know
	// the index of the point under the mouse (code above)
	int32 pointCount = fPath->CountPoints();
	if (fShiftDown && pointCount > 0) {
		mode = SELECT_POINTS;
	}

	// see if user wants to start new sub path
	if (fAltDown) {
		mode = NEW_PATH;
		index = -1;
	}

	// see if we're close enough at a line
	if (mode == UNDEFINED) {
		float distance;
		if (fPath->GetDistance(where, &distance, &index)) {
			if (distance < (INSERT_DIST_THRESHOLD / zoomLevel)) {
				mode = INSERT_POINT;
			}
		} else {
			// restore index, since it was changed by call above
			index = fCurrentPathPoint;
		}
	}

	// nope, still undefined mode, last fall back
	if (mode == UNDEFINED) {
		if (fFallBackMode == SELECT_POINTS) {
			if (fPath->IsClosed() && pointCount > 0) {
				mode = SELECT_POINTS;
				index = -1;
			} else {
				mode = ADD_POINT;
				index = pointCount - 1;
			}
		} else {
			// user had clicked "New Path" icon
			mode = fFallBackMode;
		}
	}
	// switch mode if necessary
	if (mode != fMode || index != fCurrentPathPoint) {
		// invalidate path display (to highlight the respective point)
		_InvalidateHighlightPoints(index, mode);
		_SetMode(mode);
		fCurrentPathPoint = index;
	}
}

// #pragma mark -

// _Nudge
void
PathManipulator::_Nudge(BPoint direction)
{
	bigtime_t now = system_time();
	if (now - fLastNudgeTime > 500000) {
		fCanvasView->Perform(_FinishNudging());
	}
	fLastNudgeTime = now;
	fNudgeOffset += direction;

	if (fTransformBox) {
		fTransformBox->NudgeBy(direction);
		return;
	}

	if (!fNudgeCommand) {

		bool fromSelection = !fSelection->IsEmpty();

		int32 count = fromSelection ? fSelection->CountItems()
									: fPath->CountPoints();
		int32 indices[count];
		control_point points[count];

		// init indices and points
		for (int32 i = 0; i < count; i++) {
			indices[i] = fromSelection ? fSelection->IndexAt(i) : i;
			fPath->GetPointsAt(indices[i],
							   points[i].point,
							   points[i].point_in,
							   points[i].point_out,
							   &points[i].connected);
		}

		fNudgeCommand = new NudgePointsCommand(fPath, indices, points, count);

		fNudgeCommand->SetNewTranslation(fNudgeOffset);
		fNudgeCommand->Redo();

	} else {
		fNudgeCommand->SetNewTranslation(fNudgeOffset);
		fNudgeCommand->Redo();
	}

	if (!fMouseDown)
		_SetModeForMousePos(fLastCanvasPos);
}

// _FinishNudging
Command*
PathManipulator::_FinishNudging()
{
	fNudgeOffset = BPoint(0.0, 0.0);

	Command* command;

	if (fTransformBox) {
		command = fTransformBox->FinishNudging();
	} else {
		command = fNudgeCommand;
		fNudgeCommand = NULL;
	}

	return command;
}


