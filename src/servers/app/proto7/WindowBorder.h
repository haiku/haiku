#ifndef _WINBORDER_H_
#define _WINBORDER_H_

#include <Rect.h>
#include <String.h>
#include "Layer.h"

class ServerWindow;
class Decorator;

class WindowBorder : public Layer
{
public:
	WindowBorder(ServerWindow *win, const char *bordertitle);
	~WindowBorder(void);
	void MouseDown(BPoint pt, int32 buttons, int32 modifiers);
	void MouseMoved(BPoint pt, int32 buttons, int32 modifiers);
	void MouseUp(BPoint pt, int32 buttons, int32 modifiers);
	void Draw(BRect update);
	void SystemColorsUpdated(void);
	void SetDecorator(Decorator *newdecor);
	ServerWindow *Window(void) const;
	void RequestDraw(void);
	void MoveToBack(void);
	void MoveToFront(void);
	void InvalidateLowerSiblings(BRect r);

	ServerWindow *swin;
	BString *title;
	Decorator *decor;
	int32 flags;
	BRect frame, clientframe;
	int32 mbuttons,kmodifiers;
	BPoint mousepos;
	bool update;
	bool hresizewin,vresizewin;
};

extern bool is_moving_window, is_resizing_window;
#endif