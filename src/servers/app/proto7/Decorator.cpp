#include "Decorator.h"
#include "DisplayDriver.h"

Decorator::Decorator(BRect rect, const char *title, int32 wlook, int32 wfeel, int32 wflags,
	DisplayDriver *ddriver)
{
	close_state=false;
	minimize_state=false;
	zoom_state=false;
	title_string=new BString(title);
	driver=ddriver;

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
	// lots of subclasses will depend on the driver for text support, so call
	// _DoLayout() after this
	driver=d;
	_DoLayout();
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
	title_string->SetTo(string);
}

const char *Decorator::GetTitle(void)
{
	return title_string->String();
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
int32 Decorator::_ClipTitle(float width)
{
	if(driver)
	{
		int32 strlength=title_string->CountChars();
		float pixwidth=driver->StringWidth(title_string->String(),strlength,&layerdata);
	
		while(strlength>=0)
		{
			if(pixwidth<width)
				break;
			strlength--;
			pixwidth=driver->StringWidth(title_string->String(),strlength,&layerdata);
		}
		
		return strlength;
	}
	return 0;
}

//-------------------------------------------------------------------------
//						Virtual Methods
//-------------------------------------------------------------------------
void Decorator::MoveBy(float x, float y)
{
}

void Decorator::MoveBy(BPoint pt)
{
}

void Decorator::ResizeBy(float x, float y)
{
}

void Decorator::ResizeBy(BPoint pt)
{
}

void Decorator::Draw(BRect r)
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

void Decorator::_DrawClose(BRect r)
{
}

void Decorator::_DrawFrame(BRect r)
{
}

void Decorator::_DrawMinimize(BRect r)
{
}

void Decorator::_DrawTab(BRect r)
{
}

void Decorator::_DrawTitle(BRect r)
{
}

void Decorator::_DrawZoom(BRect r)
{
}

/*
SRegion Decorator::GetFootprint(void)
{
}
*/
click_type Decorator::Clicked(BPoint pt, int32 buttons, int32 modifiers)
{
	return CLICK_NONE;
}

