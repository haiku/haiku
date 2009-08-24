/* StatusSlider.h
 *
 *	Philip Harrison
 *	Started: 5/14/2004
 *	Version: 0.1
 */
 
#ifndef STATUS_SLIDER
#define STATUS_SLIDER

//#define BEOS_R5_COMPATIBLE

#include <Slider.h>


class StatusSlider: public BSlider
{
public:
		StatusSlider(BRect frame, 
					const char *name, 
					const char *label,
					char *statusPrefix, 
					BMessage *message,
					int32 minValue,
					int32 maxValue);
virtual const char*	UpdateText() const;
private:
	char *	StatusPrefix;
	char *	temp;
	char 	str[128];
};

#endif
