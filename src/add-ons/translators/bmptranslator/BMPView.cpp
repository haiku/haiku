// BMPView.cpp

#include <stdio.h>
#include "BMPView.h"
#include "BMPTranslator.h"

BMPView::BMPView(const BRect &frame, const char *name, uint32 resize, uint32 flags) :
	BView(frame, name, resize, flags)
{
	SetViewColor(220,220,220,0);
}
			
BMPView::~BMPView()
{
}

void BMPView::Draw(BRect area)
{
	SetFont(be_bold_font);
	font_height fh;
	GetFontHeight(&fh);
	char str[100];
	sprintf(str, "BMPTranslator %.2f", (float) BMP_TRANSLATOR_VERSION / 100.0);
	DrawString(str, BPoint(fh.descent + 1, fh.ascent + fh.descent * 2 + fh.leading));
}