#include "View.h"
#include "Mime.h"
#include "TypeConstants.h"
#include "Application.h"
#include "InterfaceDefs.h"
#include "Bitmap.h"
#include "TranslationUtils.h"
#include "Alert.h"
#include "Button.h"
#include "Errors.h"

#include "ConsoleHeaderView.h"

// ----- ConsoleHeaderView ---------------------------------------------------------------

ConsoleHeaderView::ConsoleHeaderView(BRect rect)
	: BView(rect, "ConsoleHeaderView", B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW)
{
	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetViewColor(gray);

	BRect bmpRect(0.0, 0.0, 31.0, 31.0);
	icon = new BBitmap(bmpRect, B_CMAP8);
	BMimeType mime("application/x-vnd.BeServed-DomainManager");
	mime.GetIcon(icon, B_LARGE_ICON);
}

ConsoleHeaderView::~ConsoleHeaderView()
{
}

void ConsoleHeaderView::Draw(BRect rect)
{
	BRect r = Bounds();
	BRect iconRect(13.0, 5.0, 45.0, 37.0);
	rgb_color black = { 0, 0, 0, 255 };
	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);

	SetViewColor(gray);
	SetLowColor(gray);
	FillRect(r, B_SOLID_LOW);

	SetHighColor(black);
	SetFont(be_bold_font);
	SetFontSize(11);
	MovePenTo(55, 15);
	DrawString("BeServed Domain Management Console");

	SetFont(be_plain_font);
	SetFontSize(10);
	MovePenTo(55, 28);
	DrawString("Version 1.2.6");
	MovePenTo(55, 40);
	DrawString("");

	SetDrawingMode(B_OP_ALPHA); 
	SetHighColor(0, 0, 0, 180);       
	SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
	DrawBitmap(icon, iconRect);
}
