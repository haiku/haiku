#ifndef CONSTANTS_H
#define CONSTANTS_H

//Messages
static const int32 WORKSPACE_CHECK_MSG = 'wchk';
static const int32 BUTTON_DEFAULTS_MSG = 'bdef';
static const int32 BUTTON_REVERT_MSG = 'brev';
static const int32 BUTTON_APPLY_MSG = 'bapl';
static const int32 BUTTON_DONE_MSG = 'bdon';
static const int32 BUTTON_CANCEL_MSG = 'bcnc';
static const int32 BUTTON_KEEP_MSG = 'bkep';
static const int32 POP_WORKSPACE_CHANGED_MSG = 'pwsc';
static const int32 POP_RESOLUTION_MSG = 'pres';
static const int32 POP_COLORS_MSG = 'pclr';
static const int32 POP_REFRESH_MSG = 'prfr';
static const int32 POP_OTHER_REFRESH_MSG = 'porf';
static const int32 UPDATE_DESKTOP_COLOR_MSG = 'udsc';
static const int32 UPDATE_DESKTOP_MSG = 'udsk';
static const int32 SLIDER_MODIFICATION_MSG = 'sldm';
static const int32 SLIDER_INVOKE_MSG = 'sldi';
static const int32 SET_INITIAL_MODE_MSG = 'sinm';
static const int32 SET_CUSTOM_REFRESH_MSG = 'scrf';
static const int32 DIM_COUNT_MSG = 'scrf';
static const int32 MAKE_INITIAL_MSG = 'mkin';

//Constants
static const char kAppSignature[] = "application/x-vnd.Be-SCRN";
static const int32 gMaxRefresh = 120; //This is the maximum selectable refresh

#endif //CONSTANTS_H
