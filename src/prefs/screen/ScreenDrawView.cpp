#include <InterfaceDefs.h>
#include <Message.h>
#include <Screen.h>
#include <Roster.h>
#include <View.h>

#include <cstring>

#include "ScreenDrawView.h"
#include "Constants.h"

ScreenDrawView::ScreenDrawView(BRect rect, char *name)
	: BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	fScreen = new BScreen(B_MAIN_SCREEN_ID);

	desktopColor = fScreen->DesktopColor(current_workspace());

	display_mode Mode;
	
	fScreen->GetMode(&Mode);

	fResolution = Mode.virtual_width;
}

void ScreenDrawView::AttachedToWindow()
{
	rgb_color greyColor = {216, 216, 216, 255};
	SetViewColor(greyColor);
}

void ScreenDrawView::MouseDown(BPoint point)
{
	be_roster->Launch("application/x-vnd.Be-BACK");
}

void ScreenDrawView::Draw(BRect updateRect)
{
	if (fResolution == 1600)
	{
		rgb_color darkColor = {160, 160, 160, 255};
		rgb_color blackColor = {0, 0, 0, 255};
		rgb_color redColor = {228, 0, 0, 255};
		
		SetHighColor(darkColor);
	
		FillRoundRect(Bounds(), 3.0, 3.0);
		
		SetHighColor(blackColor);
		
		StrokeRoundRect(Bounds(), 3.0, 3.0);
		
		SetHighColor(desktopColor);
		
		FillRoundRect(BRect(4.0, 4.0, 98.0, 73.0), 2.0, 2.0);
		
		SetHighColor(blackColor);
		
		StrokeRoundRect(BRect(4.0, 4.0, 98.0, 73.0), 2.0, 2.0);
		
		SetHighColor(redColor);
		
		StrokeLine(BPoint(4.0, 75.0), BPoint(5.0, 75.0));
	}
	else if(fResolution == 1280)
	{
		rgb_color darkColor = {160, 160, 160, 255};
		rgb_color blackColor = {0, 0, 0, 255};
		rgb_color redColor = {228, 0, 0, 255};
		
		SetHighColor(darkColor);
	
		FillRoundRect(BRect(10.0, 6.0, 92.0, 72.0), 3.0, 3.0);
		
		SetHighColor(blackColor);
		
		StrokeRoundRect(BRect(10.0, 6.0, 92.0, 72.0), 3.0, 3.0);
		
		SetHighColor(desktopColor);
		
		FillRoundRect(BRect(14.0, 10.0, 88.0, 68.0), 2.0, 2.0);
		
		SetHighColor(blackColor);
		
		StrokeRoundRect(BRect(14.0, 10.0, 88.0, 68.0), 2.0, 2.0);
		
		SetHighColor(redColor);
		
		StrokeLine(BPoint(15.0, 70.0), BPoint(16.0, 70.0));
	}
	else if(fResolution == 1152)
	{
		rgb_color darkColor = {160, 160, 160, 255};
		rgb_color blackColor = {0, 0, 0, 255};
		rgb_color redColor = {228, 0, 0, 255};
		
		SetHighColor(darkColor);
	
		FillRoundRect(BRect(15.0, 9.0, 89.0, 68.0), 3.0, 3.0);
		
		SetHighColor(blackColor);
		
		StrokeRoundRect(BRect(15.0, 9.0, 89.0, 68.0), 3.0, 3.0);
		
		SetHighColor(desktopColor);
		
		FillRoundRect(BRect(19.0, 13.0, 85.0, 64.0), 2.0, 2.0);
		
		SetHighColor(blackColor);
		
		StrokeRoundRect(BRect(19.0, 13.0, 85.0, 64.0), 2.0, 2.0);
		
		SetHighColor(redColor);
		
		StrokeLine(BPoint(19.0, 66.0), BPoint(20.0, 66.0));
	}
	else if(fResolution == 1024)
	{
		rgb_color darkColor = {160, 160, 160, 255};
		rgb_color blackColor = {0, 0, 0, 255};
		rgb_color redColor = {228, 0, 0, 255};
		
		SetHighColor(darkColor);
	
		FillRoundRect(BRect(18.0, 14.0, 84.0, 64.0), 3.0, 3.0);
		
		SetHighColor(blackColor);
		
		StrokeRoundRect(BRect(18.0, 14.0, 84.0, 64.0), 3.0, 3.0);
		
		SetHighColor(desktopColor);
		
		FillRoundRect(BRect(22.0, 18.0, 80.0, 60.0), 2.0, 2.0);
		
		SetHighColor(blackColor);
		
		StrokeRoundRect(BRect(22.0, 18.0, 80.0, 60.0), 2.0, 2.0);
		
		SetHighColor(redColor);
		
		StrokeLine(BPoint(22.0, 62.0), BPoint(23.0, 62.0));
	}
	else if(fResolution == 800)
	{
		rgb_color darkColor = {160, 160, 160, 255};
		rgb_color blackColor = {0, 0, 0, 255};
		rgb_color redColor = {228, 0, 0, 255};
		
		SetHighColor(darkColor);
	
		FillRoundRect(BRect(25.0, 19.0, 77.0, 58.0), 3.0, 3.0);
		
		SetHighColor(blackColor);
		
		StrokeRoundRect(BRect(25.0, 19.0, 77.0, 58.0), 3.0, 3.0);
		
		SetHighColor(desktopColor);
		
		FillRoundRect(BRect(29.0, 23.0, 73.0, 54.0), 2.0, 2.0);
		
		SetHighColor(blackColor);
		
		StrokeRoundRect(BRect(29.0, 23.0, 73.0, 54.0), 2.0, 2.0);
		
		SetHighColor(redColor);
		
		StrokeLine(BPoint(29.0, 56.0), BPoint(30.0, 56.0));
	}
	else if(fResolution == 640)
	{
		rgb_color darkColor = {160, 160, 160, 255};
		rgb_color blackColor = {0, 0, 0, 255};
		rgb_color redColor = {228, 0, 0, 255};
		
		SetHighColor(darkColor);
	
		FillRoundRect(BRect(31.0, 23.0, 73.0, 55.0), 3.0, 3.0);
		
		SetHighColor(blackColor);
		
		StrokeRoundRect(BRect(31.0, 23.0, 73.0, 55.0), 3.0, 3.0);
		
		SetHighColor(desktopColor);
		
		FillRoundRect(BRect(35.0, 27.0, 69.0, 51.0), 2.0, 2.0);
		
		SetHighColor(blackColor);
		
		StrokeRoundRect(BRect(35.0, 27.0, 69.0, 51.0), 2.0, 2.0);
		
		SetHighColor(redColor);
		
		StrokeLine(BPoint(35.0, 53.0), BPoint(36.0, 53.0));
	}
}

void ScreenDrawView::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		case UPDATE_DESKTOP_MSG:
		{
			const char *Resolution;
			int32 NextfResolution;
			
			message->FindString("resolution", &Resolution);
			
			if (strcmp(Resolution, "640 x 480") == 0)
				NextfResolution = 640;
			else if (strcmp(Resolution, "800 x 600") == 0)
				NextfResolution = 800;
			else if (strcmp(Resolution, "1024 x 768") == 0)
				NextfResolution = 1024;
			else if (strcmp(Resolution, "1152 x 864") == 0)
				NextfResolution = 1152;
			else if (strcmp(Resolution, "1280 x 1024") == 0)
				NextfResolution = 1280;
			else if (strcmp(Resolution, "1600 x 1200") == 0)
				NextfResolution = 1600;
			else
				NextfResolution = 640;
			
			if (fResolution != NextfResolution)
			{
				fResolution = NextfResolution;
				
				Invalidate();
			}
		}
	
		case UPDATE_DESKTOP_COLOR_MSG:
		{
			desktopColor = fScreen->DesktopColor(current_workspace());
			
			Invalidate();
		}
		
		default:
			BView::MessageReceived(message);
			
			break;
	}
}
