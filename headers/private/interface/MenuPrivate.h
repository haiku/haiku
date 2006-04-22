#ifndef __MENU_PRIVATE_H
#define __MENU_PRIVATE_H


enum menu_states {
	MENU_STATE_TRACKING = 0,
	MENU_STATE_TRACKING_SUBMENU = 1,
	MENU_STATE_CLOSED = 5
};


extern const char *kEmptyMenuLabel;


#endif // __MENU_PRIVATE_H
