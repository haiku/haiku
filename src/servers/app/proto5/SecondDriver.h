#ifndef _SECDRIVER_H_
#define _SECDRIVER_H_

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <GraphicsCard.h>
#include <GraphicsDefs.h>	// for pattern struct
#include <Cursor.h>
#include <Message.h>
#include "DisplayDriver.h"

#include <Accelerant.h>
#include <GraphicsCard.h>
#include <image.h>

class BBitmap;
class PortLink;
class VDWindow;
class ServerCursor;


class SecondDriver:public DisplayDriver{
public:
	SecondDriver(void);
	virtual ~SecondDriver(void);

	virtual void Initialize(void);		// Sets the driver
	virtual bool IsInitialized(void);
	virtual void Shutdown(void);		// You never know when you'll need this
	
	virtual void SafeMode(void);	// Easy-access functions for common tasks
	virtual void Reset(void);
	virtual void Clear(uint8 red,uint8 green,uint8 blue);
	virtual void Clear(rgb_color col);

	// Settings functions
	virtual void SetScreen(uint32 space);
	virtual int32 GetHeight(void);
	virtual int32 GetWidth(void);
	virtual int GetDepth(void);

	// Drawing functions
	#ifdef PROTO_4
	 virtual void Blit(BPoint loc, ServerBitmap *src, ServerBitmap *dest);
	#else 
	 virtual void Blit(BRect src, BRect dest);
	#endif
	virtual void DrawBitmap(ServerBitmap *bitmap);
	virtual void DrawChar(char c, BPoint point);
	virtual void DrawString(char *string, int length, BPoint point);

	virtual void FillArc(int centerx, int centery, int xradius, int yradius, float angle, float span, uint8 *pattern);
	virtual void FillBezier(BPoint *points, uint8 *pattern);
	virtual void FillEllipse(float centerx, float centery, float x_radius, float y_radius,uint8 *pattern);
	virtual void FillPolygon(int *x, int *y, int numpoints, bool is_closed);
	virtual void FillRect(BRect rect, uint8 *pattern);
	virtual void FillRect(BRect rect, rgb_color col);
	virtual void FillRegion(BRegion *region);
	virtual void FillRoundRect(BRect rect,float xradius, float yradius, uint8 *pattern);
	virtual void FillShape(BShape *shape);
	virtual void FillTriangle(BPoint first, BPoint second, BPoint third, BRect rect, uint8 *pattern);

	virtual void HideCursor(void);
	virtual bool IsCursorHidden(void);
	virtual void MoveCursorTo(float x, float y);
	virtual void MovePenTo(BPoint pt);
	virtual void ObscureCursor(void);
	virtual BPoint PenPosition(void);
	virtual float PenSize(void);
	virtual void SetCursor(int32 value);
	virtual void SetCursor(ServerCursor *cursor);
	virtual void SetHighColor(uint8 r,uint8 g,uint8 b,uint8 a=255);
	virtual void SetLowColor(uint8 r,uint8 g,uint8 b,uint8 a=255);
	virtual void SetPenSize(float size);
	virtual void SetPixel(int x, int y, uint8 *pattern);
	virtual void ShowCursor(void);

	virtual void StrokeArc(int centerx, int centery, int xradius, int yradius, float angle, float span, uint8 *pattern);
	virtual void StrokeBezier(BPoint *points, uint8 *pattern);
	virtual void StrokeEllipse(float centerx, float centery, float x_radius, float y_radius,uint8 *pattern);
	virtual void StrokeLine(BPoint point, uint8 *pattern);
	virtual void StrokeLine(BPoint pt1, BPoint pt2, rgb_color col);
	virtual void StrokePolygon(int *x, int *y, int numpoints, bool is_closed);
	virtual void StrokeRect(BRect rect,uint8 *pattern);
	virtual void StrokeRect(BRect rect,rgb_color col);
	virtual void StrokeRoundRect(BRect rect,float xradius, float yradius, uint8 *pattern);
	virtual void StrokeShape(BShape *shape);
	virtual void StrokeTriangle(BPoint first, BPoint second, BPoint third, BRect rect, uint8 *pattern);
private:
   void HorizLine(int32 x1,int32 x2,int32 y, uint8 *pattern);
   void MidpointEllipse(float centerx, float centery,int32 xrad, int32 yrad,bool fill,uint8 *pattern);
protected:
   int hide_cursor;
   
   int fd;
   image_id image;
   GetAccelerantHook gah;
   display_mode dm;
   
   frame_buffer_config fbc;
   
   uint32 *bits;
   int32 bpr; 
   
   set_cursor_shape scs;
   move_cursor mc;
   show_cursor sc;
   
   sync_token st;
   engine_token *et;
   release_engine re;
   
   screen_to_screen_blit s2sb;
   fill_rectangle fr;
   invert_rectangle ir;
   screen_to_screen_transparent_blit s2stb;
   fill_span fs;
   
   
};

#endif