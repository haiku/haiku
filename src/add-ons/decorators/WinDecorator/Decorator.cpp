#include "Decorator.h"
#include <string.h>

Decorator::Decorator(SRect rect, int32 wlook, int32 wfeel, int32 wflags)
{
	close_state=false;
	minimize_state=false;
	zoom_state=false;
	title_string=NULL;
	driver=NULL;

	closerect.Set(0,0,1,1);
	zoomrect.Set(0,0,1,1);
	minimizerect.Set(0,0,1,1);
	resizerect.Set(0,0,1,1);
	frame=rect;
	tabrect.Set(rect.left,rect.top,rect.right, rect.top+((rect.bottom-rect.top)/4));

	look=wlook;
	feel=wfeel;
	flags=wflags;
	
	colors=new ColorSet();
}

Decorator::~Decorator(void)
{
	if(colors!=NULL)
	{
		delete colors;
		colors=NULL;
	}
	if(title_string)
		delete title_string;
}

void Decorator::SetColors(ColorSet cset)
{
	colors->SetColors(cset);
}

void Decorator::SetDriver(DisplayDriver *d)
{
	driver=d;
}

void Decorator::SetClose(bool is_down)
{
	close_state=is_down;
}

void Decorator::SetMinimize(bool is_down)
{
	zoom_state=is_down;
}

void Decorator::SetZoom(bool is_down)
{
	minimize_state=is_down;
}

void Decorator::SetFlags(int32 wflags)
{
	flags=wflags;
}

void Decorator::SetFeel(int32 wfeel)
{
	feel=wfeel;
}

void Decorator::SetLook(int32 wlook)
{
	look=wlook;
}

bool Decorator::GetClose(void)
{
	return close_state;
}

bool Decorator::GetMinimize(void)
{
	return minimize_state;
}

bool Decorator::GetZoom(void)
{
	return zoom_state;
}

int32 Decorator::GetLook(void)
{
	return look;
}

int32 Decorator::GetFeel(void)
{
	return feel;
}

int32 Decorator::GetFlags(void)
{
	return flags;
}

void Decorator::SetTitle(const char *string)
{
	if(string)
	{
		size_t size=strlen(string);
		if(!(size>0))
			return;
		delete title_string;
		title_string=new char[size];
		strcpy(title_string,string);
	}
	else
	{
		delete title_string;
		title_string=NULL;
	}
}

const char *Decorator::GetTitle(void)
{
	return title_string;
}

void Decorator::SetFocus(bool is_active)
{
	has_focus=is_active;
	_SetFocus();
}

/*
void Decorator::SetFont(SFont *sf)
{
}
*/
void Decorator::_ClipTitle(void)
{
}

//-------------------------------------------------------------------------
//						Virtual Methods
//-------------------------------------------------------------------------
void Decorator::MoveBy(float x, float y)
{
}

void Decorator::MoveBy(SPoint pt)
{
}

void Decorator::ResizeBy(float x, float y)
{
}

void Decorator::ResizeBy(SPoint pt)
{
}

void Decorator::Draw(SRect r)
{
	_DrawTab(r & tabrect);
	_DrawFrame(r & frame);
}

void Decorator::Draw(void)
{
	_DrawTab(tabrect);
	_DrawFrame(frame);
}

void Decorator::DrawClose(void)
{
	_DrawClose(closerect);
}

void Decorator::DrawFrame(void)
{
	_DrawFrame(frame);
}

void Decorator::DrawMinimize(void)
{
	_DrawTab(minimizerect);
}

void Decorator::DrawTab(void)
{
	_DrawTab(tabrect);
	_DrawZoom(zoomrect);
	_DrawMinimize(minimizerect);
	_DrawTitle(tabrect);
	_DrawClose(closerect);
}

void Decorator::DrawTitle(void)
{
	_DrawTitle(tabrect);
}

void Decorator::DrawZoom(void)
{
	_DrawZoom(zoomrect);
}

void Decorator::_DrawClose(SRect r)
{
}

void Decorator::_DrawFrame(SRect r)
{
}

void Decorator::_DrawMinimize(SRect r)
{
}

void Decorator::_DrawTab(SRect r)
{
}

void Decorator::_DrawTitle(SRect r)
{
}

void Decorator::_DrawZoom(SRect r)
{
}
/*
SRegion Decorator::GetFootprint(void)
{
}
*/
click_type Decorator::Clicked(SPoint pt, int32 buttons, int32 modifiers)
{
	return CLICK_NONE;
}

