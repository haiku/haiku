/* PoorManView.cpp
 *
 *	Philip Harrison
 *	Started: 5/14/2004
 *	Version: 0.1
 */
 

#include "StatusSlider.h"

#include <MessageFormat.h>


StatusSlider::StatusSlider(const char* name, const char* label,
	const char* statusPrefix, BMessage* message, int32 minValue, int32 maxValue)
	:
	BSlider(name, label, message, minValue, maxValue, B_HORIZONTAL),
	fStatusPrefix(statusPrefix)
{
}


const char*
StatusSlider::UpdateText() const
{
	BMessageFormat().Format(fStr, fStatusPrefix, Value());
	return fStr.String();
}
