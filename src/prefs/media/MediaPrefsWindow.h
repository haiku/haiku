#include <Window.h>

#include "MediaPrefsView.h"
#include "MediaPrefsSlider.h"


class MediaPrefsWindow : public BWindow {

	public:
		MediaPrefsWindow( BRect wRect );
		virtual bool QuitRequested();
		virtual void MessageReceived( BMessage *msg );

	private:
		MediaPrefsView	 *view;
		MediaPrefsSlider *slider;
};
