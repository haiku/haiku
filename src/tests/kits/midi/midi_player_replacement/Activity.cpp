/*
Author:	Jerome LEVEQUE
Email:	jerl1@caramail.com
*/
#include "Activity.h"

#include <Window.h>
#include <StringView.h>

//----------------------------------------------------------

Activity::Activity(BRect rect)
			: BView(rect, "Midi Activity",
				B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
{
	for (int i = 0; i < 16; i++)
		fActivity[i] = 0;
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

//----------------------------------------------------------

void Activity::AttachedToWindow(void)
{
	Window()->SetPulseRate(200000);

	AddChild(new BStringView(BRect( 65, 45,  80, 55), NULL, "1"));
	AddChild(new BStringView(BRect(125, 45, 140, 55), NULL, "2"));
	AddChild(new BStringView(BRect(185, 45, 200, 55), NULL, "3"));
	AddChild(new BStringView(BRect(245, 45, 260, 55), NULL, "4"));

	AddChild(new BStringView(BRect( 65, 105,  80, 115), NULL, "5"));
	AddChild(new BStringView(BRect(125, 105, 140, 115), NULL, "6"));
	AddChild(new BStringView(BRect(185, 105, 200, 115), NULL, "7"));
	AddChild(new BStringView(BRect(245, 105, 260, 115), NULL, "8"));

	AddChild(new BStringView(BRect( 65, 165,  80, 175), NULL, "9"));
	AddChild(new BStringView(BRect(123, 165, 140, 175), NULL, "10"));
	AddChild(new BStringView(BRect(183, 165, 200, 175), NULL, "11"));
	AddChild(new BStringView(BRect(243, 165, 260, 175), NULL, "12"));

	AddChild(new BStringView(BRect( 63, 220,  80, 235), NULL, "13"));
	AddChild(new BStringView(BRect(123, 220, 140, 235), NULL, "14"));
	AddChild(new BStringView(BRect(183, 220, 200, 235), NULL, "15"));
	AddChild(new BStringView(BRect(243, 220, 260, 235), NULL, "16"));
}

//----------------------------------------------------------

void Activity::Draw(BRect rect)
{
bigtime_t present = system_time() - 600000;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
		{
		BPoint aPoint(i * 60 + 70, j * 60 + 70);
			SetHighColor(0, 0, 0);
			StrokeEllipse(aPoint, 5, 5);
			SetHighColor(0, 255, 0);
			if (present < fActivity[j * 4 + i])
				FillEllipse(aPoint, 4, 4);
		}
}

//----------------------------------------------------------

void Activity::Pulse(void)
{
	Invalidate(BRect(60, 60, 300, 300));
}

//----------------------------------------------------------

void Activity::NoteOn(uchar channel, uchar note,
	uchar velocity, uint32 time = B_NOW)
{//This function just update the variable
	fActivity[channel] = system_time();
	SprayNoteOn(channel, note, velocity, time);
}

//----------------------------------------------------------
