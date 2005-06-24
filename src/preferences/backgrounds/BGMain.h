/*

"Backgrounds" by Jerome Duval (jerome.duval@free.fr)

(C)2002 OpenBeOS under MIT license

*/

#ifndef BG_WORLD_H
#define BG_WORLD_H

#include <Application.h>
#include "BGWindow.h"

#define APP_SIGNATURE		"application/x-vnd.haiku.Backgrounds"

class BGApplication : public BApplication {
	public:
		BGApplication();
		BGWindow *fBGWin;
};

#endif
