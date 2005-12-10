
#ifndef DRAW_VIEW_H
#define DRAW_VIEW_H

#include <View.h>

class Desktop;

class DrawView : public BView {
 public:
								DrawView(BRect frame);
	virtual						~DrawView();

	virtual void				MouseDown(BPoint where); 
	virtual void				MouseUp(BPoint where); 
	virtual void				MouseMoved(BPoint where, uint32 code,
										   const BMessage* dragMessage);

			void				SetDesktop(Desktop* desktop);

 private:
			Desktop*			fDesktop;
};

#endif // DRAW_VIEW_H

