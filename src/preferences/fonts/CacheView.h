/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 */
#ifndef CACHE_VIEW_H
#define CACHE_VIEW_H
	
#include <View.h>
#include <Box.h>
#include <Slider.h>
#include <Button.h>

class CacheView : public BView
{
public:
			CacheView(const BRect &frame, const int32 &sliderMin, 
						const int32 &sliderMax, const int32 &printVal,
						const int32 &screenVal);
	void	AttachedToWindow(void);
	void	MessageReceived(BMessage *msg);
	
	void	Revert(void);
	void	SetDefaults(void);

private:
	void		UpdatePrintSettings(int32 value);
	void		UpdateScreenSettings(int32 value);
	
	BSlider		*fScreenSlider;
	BSlider		*fPrintSlider;
	BButton		*fSaveCache;
			
	int32		fSavedPrintValue;
	int32 		fSavedScreenValue;
};
	
#endif
