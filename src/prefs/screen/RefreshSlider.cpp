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
	: BSlider(frame, "Screen", "Refresh Rate:", new BMessage(SLIDER_INVOKE_MSG), 450, 845)
{
	fStatus = (char*)malloc(64);
}

RefreshSlider::~RefreshSlider()
{
	if (fStatus)
		free(fStatus);
}

void RefreshSlider::DrawFocusMark()
{
	if (IsFocus())
	{
		rgb_color blueColor = {0, 0, 229, 255};
		
		BRect Rect;
		
		BView *View;
		
		Rect = ThumbFrame();
		
		View = OffscreenView();
		
		Rect.InsetBy(2.0, 2.0);
		Rect.right = Rect.right - 1;
		Rect.bottom = Rect.bottom - 1;
		
		View->SetHighColor(blueColor);
		View->StrokeRect(Rect);
	}
}

void RefreshSlider::KeyDown(const char *bytes, int32 numBytes)
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

char* RefreshSlider::UpdateText() const
{
	if (fStatus && Window()->Lock())
	{
		BString String;
	
		String << Value();
	
		int32 Len = String.Length()+1;
		strcpy(fStatus, String.LockBuffer(Len));
			
		char Last = fStatus[2];
		
		String.UnlockBuffer(Len);
			
		String.Truncate(2);
			
		String << "." << Last << " Hz";
	
		strcpy(fStatus, String.LockBuffer(Len));
	
		String.UnlockBuffer(Len);
	
		return(fStatus);
	}
	else
	{
		return NULL;
	}
}

