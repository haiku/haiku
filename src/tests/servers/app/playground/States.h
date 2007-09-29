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
							State();
	virtual					~State();

			void			Init(rgb_color color, drawing_mode mode,
								 bool fill, float penSize);

			void			MouseDown(BPoint where);
			void			MouseUp();
			void			MouseMoved(BPoint where);
			bool			IsTracking() const
								{ return fTracking; }

			void			SetColor(rgb_color color);
			rgb_color		Color() const
								{ return fColor; }
			void			SetDrawingMode(drawing_mode mode);
			void			SetFill(bool fill);
			void			SetPenSize(float penSize);

			void			SetEditing(bool editing);

			BRect			Bounds() const;
	virtual	void			Draw(BView* view) const;
	virtual	bool			SupportsFill() const
								{ return true; }

	static	State*			StateFor(int32 objectType,
									 rgb_color color, drawing_mode mode,
									 bool fill, float penSize);

 protected:
			BRect			_ValidRect() const;
			void			_RenderDot(BView* view, BPoint where) const;
			void			_AdjustViewState(BView* view) const;

			bool			_HitTest(BPoint where, BPoint point) const;

			bool			fValid;

			bool			fEditing;

			enum {
				TRACKING_NONE = 0,
				TRACKING_START,
				TRACKING_END
			};

			uint32			fTracking;
			BPoint			fClickOffset;

			BPoint			fStartPoint;
			BPoint			fEndPoint;

			rgb_color		fColor;
			drawing_mode	fDrawingMode;
			bool			fFill;
			float			fPenSize;
};

#endif // STATES_H
