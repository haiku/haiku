
#ifndef DRAWING_ENGINE_H
#define DRAWING_ENGINE_H

#include <View.h>

class Desktop;
class DrawView;

enum {
	MSG_INVALIDATE		= 'invl',
};

class DrawingEngine : public BView {
 public:
								DrawingEngine(BRect frame, DrawView* drawView);
	virtual						~DrawingEngine();

			bool				Lock();
			void				Unlock();

			void				CopyRegion(BRegion *region, int32 xOffset, int32 yOffset);

			void				MarkDirty(BRegion* region);
			void				MarkDirty(BRect rect);
			void				MarkDirty();

 private:
			DrawView*			fDrawView;
};

#define RUN_WITH_FRAME_BUFFER 0


#if RUN_WITH_FRAME_BUFFER
class DrawView : public BView {
#else
class DrawView : public DrawingEngine {
#endif

 public:
								DrawView(BRect frame);
	virtual						~DrawView();

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Draw(BRect updateRect);

	virtual void				MouseDown(BPoint where); 
	virtual void				MouseUp(BPoint where); 
	virtual void				MouseMoved(BPoint where, uint32 code,
										   const BMessage* dragMessage);

			void				SetDesktop(Desktop* desktop);
			DrawingEngine*		GetDrawingEngine() const
									{ return fDrawingEngine; }

 private:
			Desktop*			fDesktop;

#if RUN_WITH_FRAME_BUFFER
			BBitmap*			fFrameBuffer;
#endif
			DrawingEngine*		fDrawingEngine;
};

#endif // DRAWING_ENGINE_H

