#include <Application.h>

#include "MediaPrefsWindow.h"

class MediaPrefsApp : public BApplication {

	public:
		MediaPrefsApp();

	private:
		MediaPrefsWindow *window;
};
