#include "MediaPrefsSlider.h"
#include "VolumeControl.h"


MediaPrefsSlider::MediaPrefsSlider( BRect r, const char *name, const char *label, BMessage *model,
									int32 channels, uint32 resize, uint32 flags )
	: BChannelSlider( r, name, label, model, B_VERTICAL, channels, resize, flags ) 
{
}
