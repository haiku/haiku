// States.h

#ifndef STATES_H
#define STATES_H

#include <GraphicsDefs.h>
#include <Point.h>

class BView;

enum {
	OBJECT_LINE = 0,
	OBJECT_RECT,
	OBJECT_ROUND_RECT,
	OBJECT_ELLIPSE,
	OBJECT_TRIANGLE,
	OBJECT_SHAPE,
};

class State {
 public:
						State(rgb_color color,
							  bool fill, float penSize);

			void		MouseDown(BPoint where);
			void		MouseUp(BPoint where);
			void		MouseMoved(BPoint where);
			bool		IsTracking() const
							{ return fTracking; }

			void		SetColor(rgb_color color);
			void		SetFill(bool fill);
			void		SetPenSize(float penSize);

	virtual	BRect		Bounds() const;
	virtual	void		Draw(BView* view) const = 0;

	static	State*		StateFor(int32 objectType,
								 rgb_color color,
								 bool fill, float penSize);

 protected:
			BRect		_ValidRect() const;

			bool		fValid;

			bool		fTracking;
			BPoint		fTrackingStart;
			BPoint		fLastMousePos;

			rgb_color	fColor;
			bool		fFill;
			float		fPenSize;
};

#endif // STATES_H
