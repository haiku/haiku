// cortex_ui.h
//
// * PURPOSE
//   Definition of app-wire ui standards, like colors
//
// * HISTORY
//   clenz		26nov99		Begun
//

#ifndef __cortex_ui_h__
#define __cortex_ui_h__

// Interface Kit
#include <InterfaceDefs.h>

// -------------------------------------------------------- //
// colors
// -------------------------------------------------------- //

// the monochromes
const rgb_color M_BLACK_COLOR			= {0, 0, 0, 255};
const rgb_color M_WHITE_COLOR			= {255, 255, 255, 255}; 

// gray
const rgb_color M_GRAY_COLOR			= ui_color(B_PANEL_BACKGROUND_COLOR);
const rgb_color M_LIGHT_GRAY_COLOR		= tint_color(M_GRAY_COLOR, B_LIGHTEN_2_TINT); 
const rgb_color M_MED_GRAY_COLOR		= tint_color(M_GRAY_COLOR, B_DARKEN_2_TINT); 
const rgb_color M_DARK_GRAY_COLOR		= tint_color(M_MED_GRAY_COLOR, B_DARKEN_2_TINT); 

// blue
const rgb_color M_BLUE_COLOR			= {0, 0, 255, 255};
const rgb_color M_LIGHT_BLUE_COLOR		= {165, 182, 198, 0};
const rgb_color M_MED_BLUE_COLOR		= tint_color(M_BLUE_COLOR, B_DARKEN_2_TINT); 
const rgb_color M_DARK_BLUE_COLOR		= {16, 65, 99, 0}; 

// yellow
const rgb_color M_YELLOW_COLOR			= ui_color(B_WINDOW_TAB_COLOR);
const rgb_color M_LIGHT_YELLOW_COLOR	= tint_color(M_YELLOW_COLOR, B_LIGHTEN_2_TINT);
const rgb_color M_MED_YELLOW_COLOR		= tint_color(M_YELLOW_COLOR, B_DARKEN_2_TINT); 
const rgb_color M_DARK_YELLOW_COLOR		= tint_color(M_MED_YELLOW_COLOR, B_DARKEN_2_TINT);

#endif /*__cortex_ui_h__*/
