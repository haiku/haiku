// States.cpp

#include <View.h>

#include "States.h"

// constructor
State::State(rgb_color color, bool fill, float penSize)
	: fValid(false),
	  fTracking(false),
	  fTrackingStart(-1.0, -1.0),
	  fLastMousePos(-1.0, -1.0),
	  fColor(color),
	  fFill(fill),
	  fPenSize(penSize)
{
}

// MouseDown
void
State::MouseDown(BPoint where)
{
	fTracking = true;
	fTrackingStart = fLastMousePos = where;
}

// MouseUp
void
State::MouseUp(BPoint where)
{
	fTracking = false;
}

// MouseMoved
void
State::MouseMoved(BPoint where)
{
	if (fTracking) {
		fLastMousePos = where;
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

// Bounds
BRect
State::Bounds() const
{
	if (fValid) {
		BRect r = _ValidRect();
		if (!fFill) {
			float inset = -ceilf(fPenSize / 2.0);
			r.InsetBy(inset, inset);
		}
		return r;
	}
	return BRect(0.0, 0.0, -1.0, -1.0);
}

// _ValidRect
BRect
State::_ValidRect() const
{
	return BRect(min_c(fTrackingStart.x, fLastMousePos.x),
				 min_c(fTrackingStart.y, fLastMousePos.y),
				 max_c(fTrackingStart.x, fLastMousePos.x),
				 max_c(fTrackingStart.y, fLastMousePos.y));
}

// LineState
class LineState : public State {
 public:
					LineState(rgb_color color, bool fill, float penSize)
						: State(color, fill, penSize) {}

	virtual	BRect	Bounds() const
					{
						if (fValid) {
							BRect r = _ValidRect();
							r.InsetBy(-fPenSize, -fPenSize);
							return r;
						}
						return BRect(0.0, 0.0, -1.0, -1.0);
					}
	virtual	void	Draw(BView* view) const
					{
						if (fValid) {
							view->SetHighColor(fColor);
							view->SetPenSize(fPenSize);
							view->StrokeLine(fTrackingStart, fLastMousePos);
						}
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
							view->SetHighColor(fColor);
							if (fFill)
								view->FillRect(_ValidRect());
							else {
								view->SetPenSize(fPenSize);
								view->StrokeRect(_ValidRect());
							}
						}
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
							view->SetHighColor(fColor);
							BRect r = _ValidRect();
							float radius = min_c(r.Width() / 3.0, r.Height() / 3.0);
							if (fFill)
								view->FillRoundRect(r, radius, radius);
							else {
								view->SetPenSize(fPenSize);
								view->StrokeRoundRect(r, radius, radius);
							}
						}
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
							view->SetHighColor(fColor);
							if (fFill)
								view->FillEllipse(_ValidRect());
							else {
								view->SetPenSize(fPenSize);
								view->StrokeEllipse(_ValidRect());
							}
						}
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

