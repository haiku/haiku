/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 */
#include "CacheView.h"

#include <String.h>
#include <stdio.h>
#include "MainWindow.h"
#include "Pref_Utils.h"

#define PRINT_FCS_UPDATE_MSG 'pfum'
#define SCREEN_FCS_UPDATE_MSG 'sfum'
#define PRINT_FCS_MODIFICATION_MSG 'pfmm'
#define SCREEN_FCS_MODIFICATION_MSG 'sfmm'


CacheView::CacheView(const BRect &frame, const int32 &sliderMin, 
						const int32 &sliderMax, const int32 &printVal,
						const int32 &screenVal)
	
	: BView(frame, "Cache", B_FOLLOW_ALL, B_WILL_DRAW),
	fSavedPrintValue(printVal),
	fSavedScreenValue(screenVal)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	rgb_color fillColor = { 102, 152, 203, 255 };
	
	BString minLabel(B_EMPTY_STRING);
	BString maxLabel(B_EMPTY_STRING);
	
	float fontheight = FontHeight(true);
	
	BRect rect( Bounds().InsetByCopy(5.0,3.0) );
	rect.top += fontheight;
	rect.bottom = rect.top +(fontheight *2.0);
	
	BString labels(B_EMPTY_STRING);
	labels << "Screen font cache size: " << screenVal << " kB";
	
	fScreenSlider = new BSlider(rect, "screenslider", labels.String(),
								new BMessage(SCREEN_FCS_UPDATE_MSG),
								sliderMin, sliderMax,	B_TRIANGLE_THUMB);
			
	fScreenSlider->SetModificationMessage(new BMessage(SCREEN_FCS_MODIFICATION_MSG));
	AddChild(fScreenSlider);

	minLabel << sliderMin << " kB";
	maxLabel << sliderMax << " kB";
	
	fScreenSlider->SetLimitLabels(minLabel.String(), maxLabel.String());
	fScreenSlider->UseFillColor(TRUE, &fillColor);
	fScreenSlider->SetValue(screenVal);
	
	rect.OffsetTo(rect.left, rect.bottom +(fontheight *2.5));
	
	labels = "";
	labels << "Printing font cache size: " << printVal << " kB";
	
	fPrintSlider = new BSlider(rect, "printFontCache", labels.String(),
								new BMessage(PRINT_FCS_UPDATE_MSG),
								sliderMin, sliderMax, B_TRIANGLE_THUMB);
	AddChild(fPrintSlider);
				
	fPrintSlider->SetModificationMessage(new BMessage(PRINT_FCS_MODIFICATION_MSG));
	
	fPrintSlider->SetLimitLabels(minLabel.String(), maxLabel.String());
	fPrintSlider->UseFillColor(TRUE, &fillColor);
	fPrintSlider->SetValue(printVal);
	
	rect.OffsetTo(rect.left -1.0, rect.bottom +(fontheight *2.0));
	rect.right = rect.left +86.0;
	rect.bottom = rect.top +24.0;
	
	// TODO: figure out what to do with 'Save Cache' on R5. According to the 
	// BeOS Bible, it allocates a block of memory to disk, which is loaded on
	// next boot. FreeType does better than this.
	fSaveCache = new BButton(rect, "saveCache", "Save Cache",
										NULL, B_FOLLOW_LEFT, B_WILL_DRAW);
	AddChild(fSaveCache);
	fSaveCache->SetEnabled(false);
}

void
CacheView::AttachedToWindow(void)
{
	fScreenSlider->SetTarget(this);
	fPrintSlider->SetTarget(this);
	fSaveCache->SetTarget(this);
}

void
CacheView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case PRINT_FCS_MODIFICATION_MSG: 
		{
			UpdatePrintSettings(fPrintSlider->Value());
			
			Window()->PostMessage(M_ENABLE_REVERT);
			break;
		}
		case SCREEN_FCS_MODIFICATION_MSG:
		{
			UpdateScreenSettings(fScreenSlider->Value());
			Window()->PostMessage(M_ENABLE_REVERT);
			break;
		}
		default:
		{
			BView::MessageReceived(msg);
			break;
		}
	}
}

void CacheView::Revert(void)
{
	fPrintSlider->SetValue(fSavedPrintValue);
	fScreenSlider->SetValue(fSavedScreenValue);
	UpdatePrintSettings(fSavedPrintValue);
	UpdateScreenSettings(fSavedScreenValue);
}

void CacheView::SetDefaults(void)
{
	fPrintSlider->SetValue(256);
	fScreenSlider->SetValue(256);
	UpdatePrintSettings(256);
	UpdateScreenSettings(256);
}

void
CacheView::UpdatePrintSettings(int32 value)
{
	struct font_cache_info fontCacheInfo;
	
	get_font_cache_info(B_PRINTING_FONT_CACHE|B_DEFAULT_CACHE_SETTING, 
						&fontCacheInfo);
	fontCacheInfo.cache_size = value << 10;
	set_font_cache_info(B_PRINTING_FONT_CACHE|B_DEFAULT_CACHE_SETTING, 
						&fontCacheInfo);
	
	BString str("Printing font cache size : ");
	str << value << " kB";
	fPrintSlider->SetLabel(str.String());
}

void
CacheView::UpdateScreenSettings(int32 value)
{
	struct font_cache_info fontCacheInfo;
	
	get_font_cache_info(B_SCREEN_FONT_CACHE|B_DEFAULT_CACHE_SETTING, 
						&fontCacheInfo);
	fontCacheInfo.cache_size = value << 10;
	set_font_cache_info(B_SCREEN_FONT_CACHE|B_DEFAULT_CACHE_SETTING, 
						&fontCacheInfo);
	
	BString str("Screen font cache size : ");
	str << value << " kB";
	fScreenSlider->SetLabel(str.String());
}
