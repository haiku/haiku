#ifndef _VIEWDRIVER_H_
#define _VIEWDRIVER_H_

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <GraphicsCard.h>
#include <Cursor.h>
#include <Message.h>
#include "DisplayDriver.h"

class BBitmap;
class PortLink;
class VDWindow;
class ServerCursor;

class VDView : public BView
{
public:
	VDView(BRect bounds);
	~VDView(void);
	void AttachedToWindow(void);
	void Draw(BRect rect);
	void MouseDown(BPoint pt);
	void MouseMoved(BPoint pt, uint32 transit, const BMessage *msg);
	void MouseUp(BPoint pt);
		
	BBitmap *viewbmp;
	BView *drawview;
	PortLink *serverlink;

protected:
	friend VDWindow;
	
	int hide_cursor;
	BBitmap *cursor;
	
	BRect cursorframe, oldcursorframe;
};

class VDWindow : public BWindow
{
public:
	VDWindow(void);
	~VDWindow(void);
	void MessageReceived(BMessage *msg);
	bool QuitRequested(void);
	void WindowActivated(bool active);
	
	void SafeMode(void);
	void Reset(void);
	void SetScreen(uint32 space);
	void Clear(uint8 red,uint8 green,uint8 blue);

	VDView *view;
	BCursor *pick, *cross;
};

class ViewDriver : public DisplayDriver
{
public:
	ViewDriver(void);
	virtual ~ViewDriver(void);

	virtual void Initialize(void);
	virtual void Shutdown(void);
	virtual void SafeMode(void);
	virtual void Reset(void);
	virtual void SetScreen(uint32 space);
	virtual void Clear(uint8 red,uint8 green,uint8 blue);

	virtual void StrokeRect(BRect rect,rgb_color color);
	virtual void FillRect(BRect rect, rgb_color color);
	virtual void StrokeEllipse(BRect rect,rgb_color color);
	virtual void FillEllipse(BRect rect,rgb_color color);
	virtual void Blit(BPoint dest, ServerBitmap *sourcebmp, ServerBitmap *destbmp);

	virtual void SetCursor(int32 value);
	virtual void SetCursor(ServerCursor *cursor);
	virtual void ShowCursor(void);
	virtual void HideCursor(void);
	virtual bool IsCursorHidden(void);
	virtual void MoveCursorTo(int x, int y);
	VDWindow *screenwin;
protected:
	int hide_cursor;
};

#endif