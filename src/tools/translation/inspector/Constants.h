/*****************************************************************************/
// Constants
// Written by Michael Wilber, Haiku Translation Kit Team
//
// Constants.h
//
// Global include file containing BMessage 'what' constants and other constants
//
//
// Copyright (c) 2003 Haiku Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef MESSAGES_H
#define MESSAGES_H

// BMessage 'what' values
#define M_OPEN_IMAGE						'opim'
#define M_SAVE_IMAGE						'saim'
#define M_OPEN_FILE_PANEL					'ofpl'
#define M_ACTIVE_TRANSLATORS_WINDOW			'atrn'
#define M_ACTIVE_TRANSLATORS_WINDOW_QUIT	'atrq'
#define M_INFO_WINDOW						'infw'
#define M_INFO_WINDOW_QUIT					'infq'
#define M_INFO_WINDOW_TEXT					'inft'
#define M_VIEW_FIRST_PAGE					'vwfp'
#define M_VIEW_LAST_PAGE					'vwlp'
#define M_VIEW_NEXT_PAGE					'vwnp'
#define M_VIEW_PREV_PAGE					'vwpp'

// String constants
#define APP_SIG				"application/x-vnd.OBOS-Inspector"
#define IMAGEWINDOW_TITLE	"Inspector"


#endif // #ifndef MESSAGES_H
