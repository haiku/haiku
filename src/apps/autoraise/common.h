#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

#include <AppKit.h>
#include <InterfaceKit.h>
#include <String.h>
#include <StorageKit.h>


// R5.1 has a very helpful API...
#if (B_BEOS_VERSION == 0x0510)
#define USE_DANO_HACK
#endif

/***************************************************
	common.h
	Constants used by app
***************************************************/

// used to check the image to use to get the resources
#define APP_NAME "AutoRaise"
#define APP_SIG "application/x-vnd.mmu.AutoRaise" 
#define SETTINGS_FILE "x-vnd.mmu.AutoRaise_settings"

//names of data segments in settings file
//also used in messages

#define DEFAULT_DELAY 500000LL

// float: delay before raise
#define AR_DELAY "ar:delay"
// bool: last state
#define AR_ACTIVE "ar:active"

#define AR_MODE "ar:mode"
enum {
	Mode_All,
	Mode_DeskbarOver,
	Mode_DeskbarTouch
};

#define AR_BEHAVIOUR "ar:behaviour"

// resources
#define ACTIVE_ICON "AR:ON"
#define INACTIVE_ICON "AR:OFF"

// messages

#define ADD_TO_TRAY 'zATT'
#define REMOVE_FROM_TRAY 'zRFT'

#define OPEN_SETTINGS 'zOPS'


#define MSG_DELAY_POPUP 'arDP'

#define MSG_TOOGLE_ACTIVE 'arTA'
#define MSG_SET_ACTIVE 'arSA'
#define MSG_SET_INACTIVE 'arSI'
#define MSG_SET_DELAY 'arSD'
#define MSG_SET_MODE 'arSM'
#define MSG_SET_BEHAVIOUR 'arSB'

/* from OpenTracker deskbar/WindowMenuItem.h */

// from interface_defs.h 
struct window_info { 
        team_id         team; 
    int32       id;                       /* window's token */ 

        int32           thread; 
        int32           client_token; 
        int32           client_port; 
        uint32          workspaces; 

        int32           layer; 
    uint32              w_type;           /* B_TITLED_WINDOW, etc. */ 
    uint32      flags;            /* B_WILL_FLOAT, etc. */ 
        int32           window_left; 
        int32           window_top; 
        int32           window_right; 
        int32           window_bottom; 
        int32           show_hide_level; 
        bool            is_mini; 
        char                name[1]; 
}; 

// from interface_misc.h 
enum window_action { 
        B_MINIMIZE_WINDOW, 
        B_BRING_TO_FRONT 
}; 

// from interface_misc.h 
void                do_window_action(int32 window_id, int32 action, 
                                                         BRect zoomRect, bool zoom); 
window_info     *get_window_info(int32 a_token); 
int32           *get_token_list(team_id app, int32 *count); 
void do_minimize_team(BRect zoomRect, team_id team, bool zoom); 
void do_bring_to_front_team(BRect zoomRect, team_id app, bool zoom); 


#endif
