/* PoorManView.cpp
 *
 *	Philip Harrison
 *	Started: 5/14/2004
 *	Version: 0.1
 */
 

#include "StatusSlider.h"

#include <stdio.h>


StatusSlider::StatusSlider   (BRect frame,
				const char *name,
				const char *label,
				char *statusPrefix,
				BMessage *message,
				int32 minValue,
				int32 maxValue)
			
			:	BSlider(frame,
						name,
						label,
						message,
						minValue,
						maxValue
						), 
				StatusPrefix(statusPrefix)
{
	temp = str; 
}
				
const char*
StatusSlider::UpdateText() const
{
	
	sprintf(temp, "%ld %s", Value(), StatusPrefix);
	
	return temp;
}
