#include <ChannelSlider.h>

#define vI2F (float)(9.999 / 100)

class MediaPrefsSlider : public BChannelSlider {

	public:
		MediaPrefsSlider( BRect r, const char *name, const char *label, BMessage *model,
							int32 channels, uint32 resize, uint32 flags );
};
