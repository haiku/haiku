/*! \file CacheView.cpp
 *  \brief The file that contains code for the CacheView.
 */
#ifndef CACHE_VIEW_H

	#include "CacheView.h"

#endif

/**
 * Constructor
 * @param rect The size of the view.
 * @param minVal The minimum value of the cache sliders.
 * @param maxVal The maximum value of the cache sliders.
 * @param printCurrVal The starting value of the print slider.
 * @param screenCurrVal The starting value of the screen slider.
 */
CacheView::CacheView(BRect rect, int minVal, int maxVal, int32 printCurrVal, int32 screenCurrVal)
	   	   : BView(rect, "CacheView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	
	float x;
	float y;
	BRect viewSize = Bounds();
	char sliderMinLabel[10];
	char sliderMaxLabel[10];
	char msg[100];
	
	origPrintVal = printCurrVal;
	origScreenVal = screenCurrVal;
	SetViewColor(216, 216, 216, 0);
	
	rgb_color fillColor;
	fillColor.red = 0;
	fillColor.blue = 152;
	fillColor.green = 102;
	
	viewSize.InsetBy(15, 10);
	
	sprintf(msg, "Screen font cache size : %d kB", screenCurrVal);
	
	screenFCS = new BSlider(*(new BRect(viewSize.left, viewSize.top, viewSize.right, viewSize.top + 25.0)),
							"screenFontCache",
							msg,
							new BMessage(SCREEN_FCS_UPDATE_MSG),
							minVal,
							maxVal,
							B_TRIANGLE_THUMB
						   );
	screenFCS->SetModificationMessage(new BMessage(SCREEN_FCS_MODIFICATION_MSG));
	
	sprintf(sliderMinLabel, "%d kB", minVal);
	sprintf(sliderMaxLabel, "%d kB", maxVal);
	screenFCS->SetLimitLabels(sliderMinLabel, sliderMaxLabel);
	screenFCS->UseFillColor(TRUE, &fillColor);
	screenFCS->SetValue(screenCurrVal);
	
	viewSize.top = viewSize.top + 65.0;
	
	sprintf(msg, "Printing font cache size : %d kB", printCurrVal);
	
	printFCS = new BSlider(*(new BRect(viewSize.left, viewSize.top, viewSize.right, viewSize.top + 25.0)),
							"printFontCache",
							msg,
							new BMessage(PRINT_FCS_UPDATE_MSG),
							minVal,
							maxVal,
							B_TRIANGLE_THUMB
						   );
	printFCS->SetModificationMessage(new BMessage(PRINT_FCS_MODIFICATION_MSG));
	
	printFCS->SetLimitLabels(sliderMinLabel, sliderMaxLabel);
	printFCS->UseFillColor(TRUE, &fillColor);
	printFCS->SetValue(printCurrVal);
	
	viewSize.top = viewSize.top + 70.0;
	
	BButton *saveCache = new BButton(*(new BRect(viewSize.left, viewSize.top, 100.0, 20.0)),
							   "saveCache",
							   "Save Cache",
							   NULL,
							   B_FOLLOW_LEFT,
							   B_WILL_DRAW
							  );
	
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

	char msg[100];
	
	screenFCS->SetValue(origScreenVal);
	sprintf(msg, "Screen font cache size : %d kB", getScreenFCSValue());
	updateScreenFCS(msg);
	
	printFCS->SetValue(origPrintVal);
	sprintf(msg, "Printing font cache size : %d kB", getPrintFCSValue());
	updatePrintFCS(msg);

}//revertToOriginal

/**
 * Sets the sliders to their default values.
 */
void CacheView::resetToDefaults(){

	char msg[100];
	
	screenFCS->SetValue((int32) 256);
	sprintf(msg, "Screen font cache size : %d kB", getScreenFCSValue());
	updateScreenFCS(msg);
	
	printFCS->SetValue((int32) 256);
	sprintf(msg, "Printing font cache size : %d kB", getPrintFCSValue());
	updatePrintFCS(msg);

}//revertToOriginal

