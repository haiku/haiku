#include <StringView.h>
#include <Window.h>
#include <Locker.h>
#include <Slider.h>

#include <cstdlib>
#include <cstring>
#include <cstdio>

#include "RefreshSlider.h"
#include "Constants.h"

RefreshSlider::RefreshSlider(BRect frame)
	: BSlider(frame, "Screen", "Refresh Rate:", new BMessage(SLIDER_INVOKE_MSG), 450, gMaxRefresh * 10),
	fStatus(new char[32])
{	
}


RefreshSlider::~RefreshSlider()
{
	delete[] fStatus;
}


void
RefreshSlider::DrawFocusMark()
{
	if (IsFocus())
	{
		rgb_color blueColor = { 0, 0, 229, 255 };
		
		BRect rect(ThumbFrame());		
		BView *view = OffscreenView();
				
		rect.InsetBy(2.0, 2.0);
		rect.right--;
		rect.bottom--;
		
		view->SetHighColor(blueColor);
		view->StrokeRect(rect);
	}
}


void
RefreshSlider::KeyDown(const char *bytes, int32 numBytes)
{
	switch ( *bytes )
	{
		case B_LEFT_ARROW:
		{
			SetValue(Value() - 1);
			
			Invoke();
			
			break;
		}
		
		case B_RIGHT_ARROW:
		{
			SetValue(Value() + 1);
			
			Invoke();
			
			break;
		}
			
		default:
			break;
	}
}


char*
RefreshSlider::UpdateText() const
{
	if (fStatus && Window()->Lock())
	{
		sprintf(fStatus, "%.1f Hz", (float)Value() / 10);
		return fStatus;
	}
	else
	{
		return NULL;
	}
}

