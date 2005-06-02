#include <OS.h>
#include <View.h>
#include <Region.h>

class Layer;

class MyView: public BView
{
public:
						MyView(BRect frame, const char *name, uint32 resizingMode, uint32 flags);
	virtual				~MyView();

	virtual	void		Draw(BRect area);
	virtual void		MouseDown(BPoint where); 
	virtual void		MouseUp(BPoint where); 
	virtual void		MouseMoved(BPoint where, uint32 code, const BMessage *a_message);
	virtual	void		MessageReceived(BMessage*);

			void		CopyRegion(BRegion *region, int32 xOffset, int32 yOffset);
			void		RequestRedraw();			

			Layer*		FindLayer(Layer *lay, BPoint &where) const;

			Layer		*topLayer;
			BRegion		fRedrawReg;
private:
			void		DrawSubTree(Layer* lay);

			bool		fTracking;
			BPoint		fLastPos;
			Layer		*fMovingLayer;
			bool		fIsResize;
			bool		fIs2ndButton;
};