/*
    OBOS ShowImage 0.1 - 17/02/2002 - 22:22 - Fernando Francisco de Oliveira
*/

#include "ShowImageStatusView.h"

ShowImageStatusView::ShowImageStatusView(BRect r, const char* name, uint32 resizingMode, uint32 flags)
	: BView(r, name, resizingMode, flags)
{
}

void ShowImageStatusView::Draw(BRect updateRect)
{
	DrawString( fstrText.String(), BPoint( 3, 11) );
}

void ShowImageStatusView::SetText(BString &strText)
{
	fstrText = strText;
	Invalidate();
}
