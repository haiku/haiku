#include <StringView.h>
#include <Window.h>
#include <Locker.h>
#include <Slider.h>
#include <String.h>

#include <cstdlib>
#include <cstring>

#include "RefreshSlider.h"
#include "Constants.h"

RefreshSlider::RefreshSlider(BRect frame)
	: BSlider(frame, "Screen", "Refresh Rate:", new BMessage(SLIDER_INVOKE_MSG), 450, 900),
	fStatus(new char[64])
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
		BString String;
	
		String << Value();
		String.CopyInto(fStatus, 0, String.Length() + 1);
		
		char Last = fStatus[2];
				
		String.Truncate(2);		
		String << "." << Last << " Hz";
		
		String.CopyInto(fStatus, 0, String.Length() + 1);
		
		return fStatus;
	}
	else
	{
		return NULL;
	}
}

