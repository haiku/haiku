#ifndef _APPSERVER_PROTOCOL_
#define _APPSERVER_PROTOCOL_

#define CREATE_APP 'drca'
#define DELETE_APP 'drda'
#define QUIT_APP 'srqa'

#define SET_SERVER_PORT 'srsp'

#define CREATE_WINDOW 'drcw'
#define DELETE_WINDOW 'drdw'
#define SHOW_WINDOW 'drsw'
#define HIDE_WINDOW 'drhw'
#define QUIT_WINDOW 'srqw'

#define SERVER_PORT_NAME "OBappserver"
#define SERVER_INPUT_PORT "OBinputport"


// This will be modified. Currently a kludge for the input server until
// BScreens are implemented by the IK Taeam
#define GET_SCREEN_MODE 'gsmd'

#define SET_UI_COLORS 'suic'
#define GET_UI_COLOR 'guic'
#define SET_DECORATOR 'sdec'
#define GET_DECORATOR 'gdec'

#define SET_CURSOR_DATA 'sscd'
#define SET_CURSOR_BCURSOR 'sscb'
#define SET_CURSOR_BBITMAP 'sscB'
#define SHOW_CURSOR 'srsc'
#define HIDE_CURSOR 'srhc'
#define OBSCURE_CURSOR 'sroc'

#define GFX_COUNT_WORKSPACES 'gcws'
#define GFX_SET_WORKSPACE_COUNT 'ggwc'
#define GFX_CURRENT_WORKSPACE 'ggcw'
#define GFX_ACTIVATE_WORKSPACE 'gaws'
#define GFX_GET_SYSTEM_COLORS 'gsyc'
#define GFX_SET_SYSTEM_COLORS 'gsyc'
#define GFX_SET_SCREEN_MODE 'gssm'
#define GFX_GET_SCROLLBAR_INFO 'ggsi'
#define GFX_SET_SCROLLBAR_INFO 'gssi'
#define GFX_IDLE_TIME 'gidt'
#define GFX_SELECT_PRINTER_PANEL 'gspp'
#define GFX_ADD_PRINTER_PANEL 'gapp'
#define GFX_RUN_BE_ABOUT 'grba'
#define GFX_SET_FOCUS_FOLLOWS_MOUSE 'gsfm'
#define GFX_FOCUS_FOLLOWS_MOUSE 'gffm'

#define GFX_SET_HIGH_COLOR 'gshc'
#define GFX_SET_LOW_COLOR 'gslc'
#define GFX_SET_VIEW_COLOR 'gsvc'

#define GFX_STROKE_ARC 'gsar'
#define GFX_STROKE_BEZIER 'gsbz'
#define GFX_STROKE_ELLIPSE 'gsel'
#define GFX_STROKE_LINE 'gsln'
#define GFX_STROKE_POLYGON 'gspy'
#define GFX_STROKE_RECT 'gsrc'
#define GFX_STROKE_ROUNDRECT 'gsrr'
#define GFX_STROKE_SHAPE 'gssh'
#define GFX_STROKE_TRIANGLE 'gstr'

#define GFX_FILL_ARC 'gfar'
#define GFX_FILL_BEZIER 'gfbz'
#define GFX_FILL_ELLIPSE 'gfel'
#define GFX_FILL_POLYGON 'gfpy'
#define GFX_FILL_RECT 'gfrc'
#define GFX_FILL_REGION 'gfrg'
#define GFX_FILL_ROUNDRECT 'gfrr'
#define GFX_FILL_SHAPE 'gfsh'
#define GFX_FILL_TRIANGLE 'gftr'

#define GFX_MOVEPENBY 'gmpb'
#define GFX_MOVEPENTO 'gmpt'
#define GFX_SETPENSIZE 'gsps'

#define GFX_DRAW_STRING 'gdst'
#define GFX_SET_FONT 'gsft'
#define GFX_SET_FONT_SIZE 'gsfs'

#define GFX_FLUSH 'gfsh'
#define GFX_SYNC 'gsyn'

#define LAYER_CREATE 'lycr'
#define LAYER_DELETE 'lydl'
#define LAYER_ADD_CHILD 'lyac'
#define LAYER_REMOVE_CHILD 'lyrc'
#define LAYER_REMOVE_SELF 'lyrs'
#define LAYER_SHOW 'lysh'
#define LAYER_HIDE 'lyhd'
#define LAYER_MOVE 'lymv'
#define LAYER_RESIZE 'lyre'
#define LAYER_INVALIDATE 'lyin'
#define LAYER_DRAW 'lydr'

#define BEGIN_RECT_TRACKING 'rtbg'
#define END_RECT_TRACKING 'rten'

#define VIEW_GET_TOKEN 'vgtk'
#define VIEW_ADD 'vadd'
#define VIEW_REMOVE 'vrem'
#endif