#ifndef _WINBORDER_H_
#define _WINBORDER_H_

#include <Rect.h>
#include <String.h>
#include "Layer.h"

class ServerWindow;
class Decorator;
class DisplayDriver;

class WinBorder : public Layer
{
public:
	WinBorder(BRect r, const char *name, int32 resize, int32 flags, ServerWindow *win);
	~WinBorder(void);
	void RequestDraw(void);
	void MoveBy(BPoint pt);
	void MoveBy(float x, float y);
	void ResizeBy(BPoint pt);
	void ResizeBy(float x, float y);
	void MouseDown(int8 *buffer);
	void MouseMoved(int8 *buffer);
	void MouseUp(int8 *buffer);
	void Draw(BRect update);
	void UpdateColors(void);
	void UpdateDecorator(void);
	void UpdateFont(void);
	void UpdateScreen(void);
	void RebuildRegions(bool recursive=true);
	void Activate(bool active=false);
	ServerWindow *Window(void) { return _win; }
protected:
	ServerWindow *_win;
	BString *_title;
	Decorator *_decorator;
	int32 _flags;
	BRect _frame, _clientframe;
	int32 _mbuttons,_kmodifiers;
	BPoint _mousepos;
	bool _update;
	bool _hresizewin,_vresizewin;
};

bool is_moving_window(void);
void set_is_moving_window(bool state);
bool is_resizing_window(void);
void set_is_resizing_window(bool state);
void set_active_winborder(WinBorder *win);
WinBorder * get_active_winborder(void);

#endif
