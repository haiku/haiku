/*
 * Copyright 2021, Pascal R. G. Abresch, nep@packageloss.eu.
 * Distributed under the terms of the MIT License.
 */

#include <ControlLook.h>
#include <View.h>


namespace BPrivate {


void
AdoptScrollBarFontSize(BView* view)
{
	float maxSize = be_control_look->GetScrollBarWidth();
	BFont testFont = be_plain_font;
	float currentSize;
	font_height fontHeight;

	float minFontSize = 0.0f;
	float maxFontSize = 48.0f;

	while (maxFontSize - minFontSize > 1.0f) {
		float midFontSize = (maxFontSize + minFontSize) / 2.0f;

		testFont.SetSize(midFontSize);
		testFont.GetHeight(&fontHeight);
		currentSize = fontHeight.ascent + fontHeight.descent;

		if (currentSize > maxSize)
			maxFontSize = midFontSize;
		else
			minFontSize = midFontSize;
	}

	view->SetFontSize(minFontSize);
}


} // namespace BPrivate
