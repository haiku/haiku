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
	fFormat(statusPrefix)
{
}


const char*
StatusSlider::UpdateText() const
{
	fStr.Truncate(0);
	fFormat.Format(fStr, Value());
	strlcpy(fPattern, fStr.String(), sizeof(fPattern));
	return fPattern;
}
