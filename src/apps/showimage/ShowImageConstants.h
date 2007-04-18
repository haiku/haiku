/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 *		Michael Pfeiffer
 *		Ryan Leavengood
 */
#ifndef SHOW_IMAGE_CONSTANTS_H
#define SHOW_IMAGE_CONSTANTS_H


#include <SupportDefs.h>


enum {
	MSG_CAPTURE_MOUSE			= 'mCPM',
	MSG_CHANGE_FOCUS			= 'mCFS',
	MSG_FILE_OPEN				= 'mFOP',
	MSG_WINDOW_QUIT				= 'mWQT',
	MSG_OUTPUT_TYPE				= 'BTMN',
	MSG_SAVE_PANEL				= 'mFSP',
	MSG_CLEAR_SELECT			= 'mCSL',
	MSG_SELECT_ALL				= 'mSAL',
	MSG_CLIPBOARD_CHANGED		= 'mCLP',
	MSG_DITHER_IMAGE			= 'mDIM',
	MSG_MODIFIED				= 'mMOD',
	MSG_UPDATE_STATUS			= 'mUPS',
	MSG_UPDATE_STATUS_TEXT		= 'mUPT',
	MSG_UNDO_STATE				= 'mUNS',
	MSG_SELECTION				= 'mSEL',
	MSG_SELECTION_BITMAP		= 'mSBT',
	MSG_PAGE_FIRST				= 'mPGF',
	MSG_PAGE_LAST				= 'mPGL',
	MSG_PAGE_NEXT				= 'mPGN',
	MSG_PAGE_PREV				= 'mPGP',
	MSG_GOTO_PAGE				= 'mGTP',
	MSG_FILE_NEXT				= 'mFLN',
	MSG_FILE_PREV				= 'mFLP',
	MSG_SHRINK_TO_WINDOW		= 'mSTW',
	MSG_ZOOM_TO_WINDOW			= 'mZTW',
	MSG_ROTATE_90				= 'mR90',
	MSG_ROTATE_270				= 'mR27',
	MSG_FLIP_LEFT_TO_RIGHT		= 'mFLR',
	MSG_FLIP_TOP_TO_BOTTOM		= 'mFTB',
	MSG_INVERT					= 'mINV',
	MSG_SLIDE_SHOW				= 'mSSW',
	MSG_SLIDE_SHOW_DELAY		= 'mSSD',
	MSG_FULL_SCREEN				= 'mFSC',
	MSG_EXIT_FULL_SCREEN		= 'mEFS',
	MSG_SHOW_CAPTION			= 'mSCP',
	MSG_PAGE_SETUP				= 'mPSU',
	MSG_PREPARE_PRINT			= 'mPPT',
	MSG_PRINT					= 'mPRT',
	MSG_ZOOM_IN					= 'mZIN',
	MSG_ZOOM_OUT				= 'mZOU',
	MSG_ORIGINAL_SIZE			= 'mOSZ',
	MSG_INVALIDATE				= 'mIVD',
	MSG_SCALE_BILINEAR			= 'mSBL',
	MSG_DESKTOP_BACKGROUND		= 'mDBG',
	MSG_OPEN_RESIZER_WINDOW		= 'mORS',
	MSG_RESIZER_WINDOW_QUIT		= 'mRSQ',
	MSG_RESIZE					= 'mRSZ',
	kMsgProgressStatusUpdate	= 'SIup'
};

#endif	// SHOW_IMAGE_CONSTANTS_H
