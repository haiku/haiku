// States.cpp

#include <View.h>

#include "States.h"

// constructor
State::State()
	: fValid(false),
	  fEditing(true),
	  fTracking(TRACKING_NONE),
	  fStartPoint(-1.0, -1.0),
	  fEndPoint(-1.0, -1.0),
	  fColor((rgb_color){ 0, 0, 0, 255 }),
	  fDrawingMode(B_OP_COPY),
	  fFill(true),
	  fPenSize(1.0)
{
}

// destructor
State::~State()
{
}

// Init
void
State::Init(rgb_color color, drawing_mode mode, bool fill, float penSize)
{
	fColor = color;
	fDrawingMode = mode;
	fFill = fill;
	fPenSize = penSize;
}

// MouseDown
void
State::MouseDown(BPoint where)
{
	where.x = floorf(where.x + 0.5);
	where.y = floorf(where.y + 0.5);

	if (_HitTest(where, fStartPoint)) {
		fTracking = TRACKING_START;
		fClickOffset = fStartPoint - where;
	} else if (_HitTest(where, fEndPoint)) {
		fTracking = TRACKING_END;
		fClickOffset = fEndPoint - where;
	} else if (!fValid) {
		fTracking = TRACKING_END;
		fStartPoint = fEndPoint = where;
		fClickOffset.Set(0.0, 0.0);
	}
}

// MouseUp
void
State::MouseUp()
{
	fTracking = TRACKING_NONE;
}

// MouseMoved
void
State::MouseMoved(BPoint where)
{
	where.x = floorf(where.x + 0.5);
	where.y = floorf(where.y + 0.5);

	if (fTracking == TRACKING_START) {
		fStartPoint = where + fClickOffset;
		fValid = true;
	} else if (fTracking == TRACKING_END) {
		fEndPoint = where + fClickOffset;
		fValid = true;
	}
}

// SetColor
void
State::SetColor(rgb_color color)
{
	fColor = color;
}

// SetDrawingMode
void
State::SetDrawingMode(drawing_mode mode)
{
	fDrawingMode = mode;
}

// SetFill
void
State::SetFill(bool fill)
{
	fFill = fill;
}

// SetPenSize
void
State::SetPenSize(float penSize)
{
	fPenSize = penSize;
}

// SetEditing
void
State::SetEditing(bool editing)
{
	fEditing = editing;
}

// Bounds
BRect
State::Bounds() const
{
	if (fValid) {
		BRect r = _ValidRect();
		float inset = -2.0; // for the dots
		if (!SupportsFill() || !fFill) {
			inset = min_c(inset, -ceilf(fPenSize / 2.0));
		}
		r.InsetBy(inset, inset);
		return r;
	}
	return BRect(0.0, 0.0, -1.0, -1.0);
}

// Draw
void
State::Draw(BView* view) const
{
	if (fValid && fEditing) {
		_RenderDot(view, fStartPoint);
		_RenderDot(view, fEndPoint);
	}
}

// _ValidRect
BRect
State::_ValidRect() const
{
	return BRect(min_c(fStartPoint.x, fEndPoint.x),
				 min_c(fStartPoint.y, fEndPoint.y),
				 max_c(fStartPoint.x, fEndPoint.x),
				 max_c(fStartPoint.y, fEndPoint.y));
}

// _RenderDot
void
State::_RenderDot(BView* view, BPoint where) const
{
	view->SetHighColor(0, 0, 0, 255);
	view->SetPenSize(1.0);
	view->SetDrawingMode(B_OP_COPY);
	BRect r(where, where);
	r.InsetBy(-2.0, -2.0);
	view->StrokeRect(r);
	view->SetHighColor(255, 255, 255, 255);
	r.InsetBy(1.0, 1.0);
	view->FillRect(r);
}

// _AdjustViewState
void
State::_AdjustViewState(BView* view) const
{
	view->SetDrawingMode(fDrawingMode);
	view->SetHighColor(fColor);

	if (!SupportsFill() || !fFill)
		view->SetPenSize(fPenSize);
}

// _HitTest
bool
State::_HitTest(BPoint where, BPoint point) const
{
	BRect r(point, point);
	r.InsetBy(-8.0, -8.0);
	return r.Contains(where);
}

// LineState
class LineState : public State {
 public:
					LineState()
						: State() {}

	virtual	void	Draw(BView* view) const
					{
						if (fValid) {
							_AdjustViewState(view);
							view->StrokeLine(fStartPoint, fEndPoint);
						}
						State::Draw(view);
					}
	virtual	bool	SupportsFill() const
					{
						return false;
					}
};

// RectState
class RectState : public State {
 public:
					RectState()
						: State() {}

	virtual	void	Draw(BView* view) const
					{
						if (fValid) {
							_AdjustViewState(view);
							if (fFill)
								view->FillRect(_ValidRect());
							else
								view->StrokeRect(_ValidRect());
						}
						State::Draw(view);
					}
};

// RoundRectState
class RoundRectState : public State {
 public:
					RoundRectState()
						: State() {}

	virtual	void	Draw(BView* view) const
					{
						if (fValid) {
							_AdjustViewState(view);
							BRect r = _ValidRect();
							float radius = min_c(r.Width() / 3.0, r.Height() / 3.0);
							if (fFill)
								view->FillRoundRect(r, radius, radius);
							else
								view->StrokeRoundRect(r, radius, radius);
						}
						State::Draw(view);
					}
};

// EllipseState
class EllipseState : public State {
 public:
					EllipseState()
						: State() {}

	virtual	void	Draw(BView* view) const
					{
						if (fValid) {
							_AdjustViewState(view);
							if (fFill)
								view->FillEllipse(_ValidRect());
							else
								view->StrokeEllipse(_ValidRect());
						}
						State::Draw(view);
					}
};

// StateFor
State*
State::StateFor(int32 objectType, rgb_color color, drawing_mode mode,
				bool fill, float penSize)
{
	State* state = NULL;
	switch (objectType) {
		case OBJECT_LINE:
			state = new LineState();
			break;
		case OBJECT_RECT:
			state = new RectState();
			break;
		case OBJECT_ROUND_RECT:
			state = new RoundRectState();
			break;
		case OBJECT_ELLIPSE:
			state = new EllipseState();
			break;
		default:
			break;
	}
	if (state)
		state->Init(color, mode, fill, penSize);
	return state;
}

