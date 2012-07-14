/* PoorManView.cpp
 *
 *	Philip Harrison
 *	Started: 5/14/2004
 *	Version: 0.1
 */
 

#include "StatusSlider.h"

#include <stdio.h>


StatusSlider::StatusSlider(const char* name, const char* label,
	char* statusPrefix, BMessage* message, int32 minValue, int32 maxValue)
	:
	BSlider(name, label, message, minValue, maxValue, B_HORIZONTAL), 
	fStatusPrefix(statusPrefix)
{
	fTemp = fStr; 
}
				
const char*
StatusSlider::UpdateText() const
{
	sprintf(fTemp, "%ld %s", Value(), fStatusPrefix);

	return fTemp;
}
