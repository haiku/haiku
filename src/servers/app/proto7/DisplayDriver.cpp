/*
	DisplayDriver.cpp
		Modular class to allow the server to not care what the ultimate output is
		for graphics calls.
*/
#include "DisplayDriver.h"
#include "LayerData.h"
#include "ServerCursor.h"

DisplayDriver::DisplayDriver(void)
{
	lock_sem=create_sem(1,"displaydriver_lock");
	buffer_depth=0;
	buffer_width=0;
	buffer_height=0;
	buffer_mode=-1;
}

DisplayDriver::~DisplayDriver(void)
{
	delete_sem(lock_sem);
}

bool DisplayDriver::Initialize(void)
{
	return false;
}

uint8 DisplayDriver::GetDepth(void)
{
	return buffer_depth;
}

uint16 DisplayDriver::GetHeight(void)
{
	return buffer_height;
}

uint16 DisplayDriver::GetWidth(void)
{
	return buffer_width;
}

int32 DisplayDriver::GetMode(void)
{
	return buffer_mode;
}

bool DisplayDriver::DumpToFile(const char *path)
{
	return false;
}

bool DisplayDriver::IsCursorHidden(void)
{
	return (is_cursor_hidden>0)?true:false;
}

void DisplayDriver::HideCursor(void)
{
	SetCursorHidden(true);
}

void DisplayDriver::ShowCursor(void)
{
	SetCursorHidden(false);
}

void DisplayDriver::ObscureCursor(void)
{
	SetCursorObscured(true);
}

void DisplayDriver::SetCursor(ServerCursor *cursor)
{
}

//---------------------------------------------------------
//					Private Methods
//---------------------------------------------------------

void DisplayDriver::Lock(void)
{
	acquire_sem(lock_sem);
}

void DisplayDriver::Unlock(void)
{
	release_sem(lock_sem);
}

void DisplayDriver::SetDepthInternal(uint8 d)
{
	buffer_depth=d;
}

void DisplayDriver::SetHeightInternal(uint16 h)
{
	buffer_height=h;
}

void DisplayDriver::SetWidthInternal(uint16 w)
{
	buffer_width=w;
}

void DisplayDriver::SetModeInternal(int32 m)
{
	buffer_mode=m;
}

void DisplayDriver::SetCursorHidden(bool state)
{
	if(state)
	{
		is_cursor_hidden++;
		cursor_state_changed=(is_cursor_hidden==1)?true:false;
	}
	else
	{
		if(is_cursor_hidden>0)
		{
			is_cursor_hidden--;
			cursor_state_changed=(is_cursor_hidden==0)?true:false;
		}
	}
}

bool DisplayDriver::CursorStateChanged(void)
{
	return cursor_state_changed;
}
bool DisplayDriver::IsCursorObscured(bool state)
{
	return is_cursor_obscured;
}

void DisplayDriver::SetCursorObscured(bool state)
{
	is_cursor_obscured=state;
}

//---------------------------------------------------------
//					Empty Methods
//---------------------------------------------------------

void DisplayDriver::Shutdown(void) {}
void DisplayDriver::CopyBits(BRect src, BRect dest) {}
void DisplayDriver::DrawBitmap(ServerBitmap *bmp, BRect src, BRect dest) {}
void DisplayDriver::DrawChar(char c, BPoint pt) {}
//void DisplayDriver::DrawPicture(SPicture *pic, BPoint pt) {}
void DisplayDriver::DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *delta=NULL) {}

void DisplayDriver::FillArc(BRect r, float angle, float span, LayerData *d, int8 *pat) {}
void DisplayDriver::FillBezier(BPoint *pts, LayerData *d, int8 *pat) {}
void DisplayDriver::FillEllipse(BRect r, LayerData *d, int8 *pat) {}
void DisplayDriver::FillPolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat) {}
void DisplayDriver::FillRect(BRect r, LayerData *d, int8 *pat) {}
void DisplayDriver::FillRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat) {}
//void DisplayDriver::FillShape(SShape *sh, LayerData *d, int8 *pat) {}
void DisplayDriver::FillTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat) {}

void DisplayDriver::MoveCursorTo(float x, float y) {}
void DisplayDriver::InvertRect(BRect r) {}

void DisplayDriver::StrokeArc(BRect r, float angle, float span, LayerData *d, int8 *pat) {}
void DisplayDriver::StrokeBezier(BPoint *pts, LayerData *d, int8 *pat) {}
void DisplayDriver::StrokeEllipse(BRect r, LayerData *d, int8 *pat) {}
void DisplayDriver::StrokeLine(BPoint start, BPoint end, LayerData *d, int8 *pat) {}
void DisplayDriver::StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat, bool is_closed=true) {}
void DisplayDriver::StrokeRect(BRect r, LayerData *d, int8 *pat) {}
void DisplayDriver::StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat) {}
//void DisplayDriver::StrokeShape(SShape *sh, LayerData *d, int8 *pat) {}
void DisplayDriver::StrokeTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat) {}
void DisplayDriver::StrokeLineArray(BPoint *pts, int32 numlines, RGBColor *colors, LayerData *d) {}
void DisplayDriver::SetMode(int32 mode) {}
