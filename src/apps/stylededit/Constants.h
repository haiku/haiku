/*StyledEdit: constants*/
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <GraphicsDefs.h>
#include <SupportDefs.h>

//See if this takes care of some problems with closing the application
//by clicking the windowtab
//#define APP_SIGNATURE  "application/x-vnd.obos-stylededit"
//application signature consistent with with the one in StyledEdit.rdef
//seems to work, 021021
#define APP_SIGNATURE  "application/x-vnd.obos.styled-edit"

const float TEXT_INSET= 3.0;

/*Messages for window registry with application*/
const uint32 WINDOW_REGISTRY_ADD= 'WRad';
const uint32 WINDOW_REGISTRY_SUB='WRsb';
const uint32 WINDOW_REGISTRY_ADDED='WRdd';
/*Messages for menu commands
file menu*/
const uint32 MENU_NEW					='MFnw';
const uint32 MENU_OPEN					='MFop';
const uint32 MENU_SAVE					='MSav';
const uint32 MENU_SAVEAS				='MEsa';
const uint32 MENU_REVERT				='MFre'; 
const uint32 MENU_CLOSE					='MFcl';
const uint32 MENU_PAGESETUP				='MFps';
const uint32 MENU_PRINT					='MFpr';
const uint32 MENU_QUIT					='MFqu';
//edit menu
const uint32 MENU_CLEAR					='MEcl';
const uint32 MENU_FIND					='MEfi';
const uint32 MENU_FIND_AGAIN			='MEfa';
const uint32 MENU_FIND_SELECTION		='MEfs';
const uint32 MENU_REPLACE				='MEre';
const uint32 MENU_REPLACE_SAME			='MErs';

const uint32 MSG_SEARCH					='msea';
const uint32 MSG_REPLACE				='msre';
const uint32 MSG_REPLACE_ALL			='mrea';
//"Font"-menu
const uint32 FONT_SIZE					='FMsi';
const uint32 FONT_STYLE					='FSch';
const uint32 FONT_COLOR					='Fcol';

//fontcolors
//red, green, blue, alphachannel
const rgb_color	BLACK 					= {0, 0, 0, 255};
const rgb_color	RED 					= {255, 0, 0, 255};
const rgb_color	GREEN			 		= {0, 255, 0, 255};
const rgb_color	BLUE			 		= {0, 0, 255, 255};
const rgb_color	CYAN			 		= {0, 255, 255, 255};
const rgb_color	MAGENTA				 	= {255, 0, 255, 255};
const rgb_color	YELLOW 					= {255, 255, 0, 255};

//"Document"-menu
const uint32 ALIGN_LEFT					='ALle';
const uint32 ALIGN_CENTER				='ALce';
const uint32 ALIGN_RIGHT				='ALri';
const uint32 WRAP_LINES					='MDwr';

//enables "edit" menuitems
const uint32 ENABLE_ITEMS				='ENit';
const uint32 DISABLE_ITEMS				='DIit';
const uint32 CHANGE_WINDOW				='CHwi'; 
const uint32 TEXT_CHANGED				='TEch';

#endif // CONSTANTS_H
