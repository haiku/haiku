/*
 * Copyright 2003-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 *		Michael Pfeiffer
 */
#ifndef SHOW_IMAGE_CONSTANTS_H
#define SHOW_IMAGE_CONSTANTS_H


#include <SupportDefs.h>


const uint32 MSG_CAPTURE_MOUSE		= 'mCPM';
const uint32 MSG_CHANGE_FOCUS		= 'mCFS';
const uint32 MSG_FILE_OPEN			= 'mFOP';
const uint32 MSG_WINDOW_QUIT		= 'mWQT';
const uint32 MSG_OUTPUT_TYPE		= 'BTMN';
const uint32 MSG_SAVE_PANEL			= 'mFSP';
const uint32 MSG_CLEAR_SELECT		= 'mCSL';
const uint32 MSG_SELECT_ALL			= 'mSAL';
const uint32 MSG_CLIPBOARD_CHANGED	= 'mCLP';
const uint32 MSG_DITHER_IMAGE		= 'mDIM';
const uint32 MSG_MODIFIED			= 'mMOD';
const uint32 MSG_UPDATE_STATUS		= 'mUPS';
const uint32 MSG_UNDO_STATE			= 'mUNS';
const uint32 MSG_SELECTION			= 'mSEL';
const uint32 MSG_SELECTION_BITMAP	= 'mSBT';
const uint32 MSG_PAGE_FIRST			= 'mPGF';
const uint32 MSG_PAGE_LAST			= 'mPGL';
const uint32 MSG_PAGE_NEXT			= 'mPGN';
const uint32 MSG_PAGE_PREV			= 'mPGP';
const uint32 MSG_GOTO_PAGE			= 'mGTP';
const uint32 MSG_FILE_NEXT          = 'mFLN';
const uint32 MSG_FILE_PREV          = 'mFLP';
const uint32 MSG_SHRINK_TO_WINDOW   = 'mSTW';
const uint32 MSG_ZOOM_TO_WINDOW     = 'mZTW';
const uint32 MSG_ROTATE_90			= 'mR90';
const uint32 MSG_ROTATE_270			= 'mR27';
const uint32 MSG_MIRROR_VERTICAL    = 'mMIV';
const uint32 MSG_MIRROR_HORIZONTAL  = 'mMIH';
const uint32 MSG_INVERT             = 'mINV';
const uint32 MSG_SLIDE_SHOW         = 'mSSW';
const uint32 MSG_SLIDE_SHOW_DELAY   = 'mSSD';
const uint32 MSG_FULL_SCREEN        = 'mFSC';
const uint32 MSG_EXIT_FULL_SCREEN   = 'mEFS';
const uint32 MSG_SHOW_CAPTION       = 'mSCP';
const uint32 MSG_PAGE_SETUP         = 'mPSU';
const uint32 MSG_PREPARE_PRINT      = 'mPPT';
const uint32 MSG_PRINT              = 'mPRT';
const uint32 MSG_ZOOM_IN            = 'mZIN';
const uint32 MSG_ZOOM_OUT           = 'mZOU';
const uint32 MSG_ORIGINAL_SIZE      = 'mOSZ';
const uint32 MSG_INVALIDATE         = 'mIVD';
const uint32 MSG_SCALE_BILINEAR     = 'mSBL';
const uint32 MSG_DESKTOP_BACKGROUND = 'mDBG';

#endif	// SHOW_IMAGE_CONSTANTS_H
