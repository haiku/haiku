/*****************************************************************************/
// ShowImageConstants
// Written by Fernando Francisco de Oliveira, Michael Wilber, Michael Pfeiffer
//
// ShowImageConstants.h
//
//
// Copyright (c) 2003 OpenBeOS Project
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

#ifndef _ShowImageConstants_
#define _ShowImageConstants_

#include <SupportDefs.h>

const uint32 MSG_CAPTURE_MOUSE		= 'mCPM';
const uint32 MSG_CHANGE_FOCUS		= 'mCFS';
const uint32 MSG_FILE_OPEN			= 'mFOP';
const uint32 MSG_CLOSE				= 'mCLS';
const uint32 MSG_WINDOW_QUIT		= 'mWQT';
const uint32 MSG_OUTPUT_TYPE		= 'BTMN';
const uint32 MSG_SAVE_PANEL			= 'mFSP';
const uint32 MSG_CLEAR_SELECT		= 'mCSL';
const uint32 MSG_SELECT_ALL			= 'mSAL';
const uint32 MSG_DITHER_IMAGE		= 'mDIM';
const uint32 MSG_UPDATE_STATUS		= 'mUPS';
const uint32 MSG_SELECTION			= 'mSEL';
const uint32 MSG_PAGE_FIRST			= 'mPGF';
const uint32 MSG_PAGE_LAST			= 'mPGL';
const uint32 MSG_PAGE_NEXT			= 'mPGN';
const uint32 MSG_PAGE_PREV			= 'mPGP';
const uint32 MSG_GOTO_PAGE			= 'mGTP';
const uint32 MSG_FILE_NEXT          = 'mFLN';
const uint32 MSG_FILE_PREV          = 'mFLP';
const uint32 MSG_FIT_TO_WINDOW_SIZE = 'mFWS';
const uint32 MSG_ROTATE_90			= 'mR90';
const uint32 MSG_ROTATE_270			= 'mR27';
const uint32 MSG_MIRROR_VERTICAL    = 'mMIV';
const uint32 MSG_MIRROR_HORIZONTAL  = 'mMIH';
const uint32 MSG_INVERT             = 'mINV';
const uint32 MSG_SLIDE_SHOW         = 'mSSW';
const uint32 MSG_SLIDE_SHOW_DELAY   = 'mSSD';
const uint32 MSG_FULL_SCREEN        = 'mFSC';
const uint32 MSG_UPDATE_RECENT_DOCUMENTS = 'mURD';
const uint32 MSG_SHOW_CAPTION       = 'mSCP';
const uint32 MSG_PAGE_SETUP         = 'mPSU';
const uint32 MSG_PREPARE_PRINT      = 'mPPT';
const uint32 MSG_PRINT              = 'mPRT';
const uint32 MSG_ZOOM_IN            = 'mZIN';
const uint32 MSG_ZOOM_OUT           = 'mZOU';
const uint32 MSG_ORIGINAL_SIZE      = 'mOSZ';
const uint32 MSG_INVALIDATE         = 'mIVD';
const uint32 MSG_SCALE_BILINEAR     = 'mSBL';

extern const char *APP_SIG;


#endif /* _ShowImageConstants_ */
