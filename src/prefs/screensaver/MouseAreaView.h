#include <View.h>
#include "ScreenSaverPrefs.h"

class MouseAreaView : public BView
{
	public:	
	MouseAreaView(BRect frame, const char *name) : BView (frame,name,B_FOLLOW_NONE,B_WILL_DRAW) 
		{ SetViewColor(216,216,216); 
		currentDirection=NONE;}

	virtual void Draw(BRect update); 
	virtual void MouseUp(BPoint point);
	void DrawArrow(void);
	inline arrowDirection getDirection(void) {return currentDirection;}
	void setDirection(arrowDirection direction) {currentDirection=direction;Draw(BRect (0,0,100,100));}
	private:
	BRect screenArea;
	arrowDirection currentDirection;
};
