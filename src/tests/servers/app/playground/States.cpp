// States.cpp

#include <View.h>

#include "States.h"

// constructor
State::State(rgb_color color, bool fill, float penSize)
	: fValid(false),
	  fEditing(true),
	  fTracking(TRACKING_NONE),
	  fStartPoint(-1.0, -1.0),
	  fEndPoint(-1.0, -1.0),
	  fColor(color),
	  fFill(fill),
	  fPenSize(penSize)
{
}

// MouseDown
void
State::MouseDown(BPoint where)
{
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
State::MouseUp(BPoint where)
{
	fTracking = TRACKING_NONE;
}

// MouseMoved
void
State::MouseMoved(BPoint where)
{
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
	if (fColor.alpha < 255)
		view->SetDrawingMode(B_OP_ALPHA);
	else
		view->SetDrawingMode(B_OP_OVER);

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
					LineState(rgb_color color, bool fill, float penSize)
						: State(color, fill, penSize) {}

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
					RectState(rgb_color color, bool fill, float penSize)
						: State(color, fill, penSize) {}

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
					RoundRectState(rgb_color color, bool fill, float penSize)
						: State(color, fill, penSize) {}

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
					EllipseState(rgb_color color, bool fill, float penSize)
						: State(color, fill, penSize) {}

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
State::StateFor(int32 objectType, rgb_color color, bool fill, float penSize)
{
	switch (objectType) {
		case OBJECT_LINE:
			return new LineState(color, fill, penSize);
		case OBJECT_RECT:
			return new RectState(color, fill, penSize);
		case OBJECT_ROUND_RECT:
			return new RoundRectState(color, fill, penSize);
		case OBJECT_ELLIPSE:
			return new EllipseState(color, fill, penSize);
		default:
			return NULL;
	}
}

