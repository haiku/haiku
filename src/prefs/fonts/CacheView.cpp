/*! \file CacheView.cpp
 *  \brief The file that contains code for the CacheView.
 */
#ifndef CACHE_VIEW_H

	#include "CacheView.h"

#endif
#include <String.h>
#include "Pref_Utils.h"

const char *kLabel = "font cache size: ";

/**
 * Constructor
 * @param rect The size of the view.
 * @param minVal The minimum value of the cache sliders.
 * @param maxVal The maximum value of the cache sliders.
 * @param printCurrVal The starting value of the print slider.
 * @param screenCurrVal The starting value of the screen slider.
 */
CacheView::CacheView(BRect rect, int minVal, int maxVal, int32 printCurrVal, int32 screenCurrVal)
	:BView(rect, "CacheView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(216, 216, 216, 0);
	
	origPrintVal = printCurrVal;
	origScreenVal = screenCurrVal;
	
	rgb_color fillColor;
	fillColor.red = 102;
	fillColor.blue = 203;
	fillColor.green = 152;
	
	float fontheight = FontHeight(true);
	BRect rect(Bounds());
	rect.InsetBy(5.0, 3.0);
	rect.top += fontheight;
	rect.bottom = rect.top +(fontheight *2.0);
	BString labels(B_EMPTY_STRING);
	labels << "Screen" << kLabel << screenCurrVal << " kB";
	screenFCS = new BSlider(rect,
			"screenFontCache", labels.String(),
			new BMessage(SCREEN_FCS_UPDATE_MSG),
			minVal, maxVal,	B_TRIANGLE_THUMB);
			
	screenFCS->SetModificationMessage(new BMessage(SCREEN_FCS_MODIFICATION_MSG));

	BString minLabel(B_EMPTY_STRING);
	BString maxLabel(B_EMPTY_STRING);
	minLabel << minVal << " kB";
	maxLabel << maxVal << " kB";
	screenFCS->SetLimitLabels(minLabel.String(), maxLabel.String());
	screenFCS->UseFillColor(TRUE, &fillColor);
	screenFCS->SetValue(screenCurrVal);
	
	rect.OffsetTo(rect.left, rect.bottom +(fontheight *2.5));
	
	labels = ""; 
	labels << "Printing " << kLabel << printCurrVal << " kB";
	
	printFCS = new BSlider(rect, "printFontCache", labels.String(),
				new BMessage(PRINT_FCS_UPDATE_MSG),
				minVal, maxVal, B_TRIANGLE_THUMB);
				
	printFCS->SetModificationMessage(new BMessage(PRINT_FCS_MODIFICATION_MSG));
	
	printFCS->SetLimitLabels(minLabel.String(), maxLabel.String());
	printFCS->UseFillColor(TRUE, &fillColor);
	printFCS->SetValue(printCurrVal);
	
	rect.OffsetTo(rect.left -1.0, rect.bottom +(fontheight *2.0));
	rect.right = rect.left +86.0;
	rect.bottom = rect.top +24.0;
	
	BButton *saveCache = new BButton(rect,
				"saveCache", "Save Cache",
				NULL, B_FOLLOW_LEFT, B_WILL_DRAW);
	
	AddChild(screenFCS);
	AddChild(printFCS);
	AddChild(saveCache);  	
}

/**
 * Updates the label on the print slider.
 * @param txt The text to update the slider to.
 */
void CacheView::updatePrintFCS(const char* txt){

	printFCS->SetLabel(txt);

}//updatePrintFCS

/**
 * Updates the label on the screen slider.
 * @param txt The text to update the slider to.
 */
void CacheView::updateScreenFCS(const char* txt){

	screenFCS->SetLabel(txt);

}//updatePrintFCS

/**
 * Gets the current value of the print slider.
 */
int CacheView::getPrintFCSValue(){

	return int(printFCS->Value());

}//getPrintValue

/**
 * Gets the current value of the screen slider.
 */
int CacheView::getScreenFCSValue(){

	return int(screenFCS->Value());

}//getPrintValue

/**
 * Sets the sliders to their original values.
 */
void CacheView::revertToOriginal(){
	
	BString label;
	label << "Screen " << kLabel << getScreenFCSValue();
	updateScreenFCS(label.String());

	label = "Printing ";
	label << kLabel << getPrintFCSValue();
	updatePrintFCS(label.String());	

}//revertToOriginal

/**
 * Sets the sliders to their default values.
 */
void CacheView::resetToDefaults(){
	BString label;
	label << "Screen " << kLabel << getScreenFCSValue() << " kB";
	updateScreenFCS(label.String());
	
	label = "Printing ";
	label << kLabel << getPrintFCSValue() << " kB";
	updatePrintFCS(label.String());

}//revertToOriginal

