#include "Pref_Utils.h"

float
FontHeight(bool full, BView* target)
{
	font_height finfo;	
	if (target != NULL)	
		target->GetFontHeight(&finfo);
	else
		be_plain_font->GetHeight(&finfo);
	
	float height = ceil(finfo.ascent) + ceil(finfo.descent);

	if (full)
		height += ceil(finfo.leading);
	
	return height;
}

color_map*
ColorMap()
{
	color_map* cmap;
	
	BScreen screen(B_MAIN_SCREEN_ID);
	cmap = (color_map*)screen.ColorMap();
	
	return cmap;
}

