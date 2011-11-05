/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


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
