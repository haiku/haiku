// BMPView.cpp

#include <stdio.h>
#include <string.h>
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
	float xbold, ybold;
	xbold = fh.descent + 1;
	ybold = fh.ascent + fh.descent * 2 + fh.leading;
	
	char title[] = "OpenBeOS BMP Image Translator";
	DrawString(title, BPoint(xbold, ybold));
	
	SetFont(be_plain_font);
	font_height plainh;
	GetFontHeight(&plainh);
	float yplain;
	yplain = plainh.ascent + plainh.descent * 2 + plainh.leading;
	
	char detail[100];
	sprintf(detail, "Version %d.%d.%d %s",
		BMP_TRANSLATOR_VERSION / 100, (BMP_TRANSLATOR_VERSION / 10) % 10,
		BMP_TRANSLATOR_VERSION % 10, __DATE__);
	DrawString(detail, BPoint(xbold, yplain + ybold));
/*	char copyright[] = "© 2002 OpenBeOS Project";
	DrawString(copyright, BPoint(xbold, yplain * 2 + ybold));
	
	char becopyright[] = "Portions Copyright 1991-1999, Be Incorporated.";
	DrawString(becopyright, BPoint(xbold, yplain * 4 + ybold));
	char allrights[] = "All rights reserved.";
	DrawString(allrights, BPoint(xbold, yplain * 5 + ybold));
*/	
	char writtenby[] = "Written by the OBOS Translation Kit Team";
	DrawString(writtenby, BPoint(xbold, yplain * 7 + ybold));
}