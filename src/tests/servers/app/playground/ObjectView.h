// ObjectView.h

#ifndef OBJECT_VIEW_H
#define OBJECT_VIEW_H

#include <List.h>
#include <View.h>

class State;

enum {
	MSG_OBJECT_COUNT_CHANGED	= 'obcc',
	MSG_OBJECT_ADDED			= 'obad',
};

class ObjectView : public BView {
 public:
							ObjectView(BRect frame, const char* name,
									   uint32 resizeFlags, uint32 flags);
	virtual					~ObjectView();

							// BView
	virtual	void			AttachedToWindow();
	virtual	void			DetachedFromWindow();

	virtual	void			Draw(BRect updateRect);

	virtual	void			MouseDown(BPoint where);
	virtual	void			MouseUp(BPoint where);
	virtual	void			MouseMoved(BPoint where, uint32 transit,
								   const BMessage* dragMessage);

	virtual	void			MessageReceived(BMessage* message);

							// ObjectView
			void			SetState(State* state);

			void			SetObjectType(int32 type);
			int32			ObjectType() const
								{ return fObjectType; }

			void			AddObject(State* state);
			void			RemoveObject(State* state);
			int32			CountObjects() const;
			void			MakeEmpty();

			void			SetStateColor(rgb_color color);
			rgb_color		StateColor() const
								{ return fColor; }

			void			SetStateDrawingMode(drawing_mode mode);
			drawing_mode	StateDrawingMode() const
								{ return fDrawingMode; }

			void			SetStateFill(bool fill);
			bool			StateFill() const
								{ return fFill; }

			void			SetStatePenSize(float penSize);
			float			StatePenSize() const
								{ return fPenSize; }	

 private:
			State*			fState;
			int32			fObjectType;

			BList			fStateList;

			rgb_color		fColor;
			drawing_mode	fDrawingMode;
			bool			fFill;
			float			fPenSize;

			bool			fScrolling;
			bool			fInitiatingDrag;
			BPoint			fLastMousePos;
};

#endif // OBJECT_VIEW_H
