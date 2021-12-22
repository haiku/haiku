#include "PrintTestView.hpp"

PrintTestView::PrintTestView(BRect frame)
	: Inherited(frame, "", B_FOLLOW_ALL, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
}

void PrintTestView::Draw(BRect updateRect)
{
	StrokeRoundRect(Bounds().InsetByCopy(20,20), 10, 10);
	BFont font(be_bold_font);
	font.SetSize(Bounds().Height()/10);
	font.SetShear(Bounds().Height()/10);
	font.SetRotation(Bounds().Width()/10);
	SetFont(&font, B_FONT_ALL);
	DrawString("Haiku", 8, BPoint(Bounds().Width()/2,Bounds().Height()/2));
}

