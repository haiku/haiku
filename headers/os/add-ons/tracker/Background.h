/*******************************************************************************
/
/	File:			Background.h
/
/   Description:    Defines constants for setting Tracker background images
/
/	Copyright 1998-1999, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_TRACKER_BACKGROUND_H
#define	_TRACKER_BACKGROUND_H

/*----------------------------------------------------------------*/
/*-----  Tracker background attribute name  ----------------------*/

#define B_BACKGROUND_INFO		"be:bgndimginfo"

/*----------------------------------------------------------------*/
/*-----  Tracker background BMessage entries  --------------------*/

#define B_BACKGROUND_IMAGE		"be:bgndimginfopath"		// string path
#define B_BACKGROUND_MODE		"be:bgndimginfomode"		// int32, the enum below
#define B_BACKGROUND_ORIGIN		"be:bgndimginfooffset"		// BPoint
#define B_BACKGROUND_ERASE_TEXT	"be:bgndimginfoerasetext"	// bool
#define B_BACKGROUND_WORKSPACES	"be:bgndimginfoworkspaces"	// uint32

/*----------------------------------------------------------------*/
/*-----  Background mode values  ---------------------------------*/

enum {
	B_BACKGROUND_MODE_USE_ORIGIN,
	B_BACKGROUND_MODE_CENTERED,		// only works on Desktop
	B_BACKGROUND_MODE_SCALED,		// only works on Desktop
	B_BACKGROUND_MODE_TILED
};

/*----------------------------------------------------------------*/
/*----------------------------------------------------------------*/

const int32 B_RESTORE_BACKGROUND_IMAGE = 'Tbgr';	// force a Tracker window to
													// use a new background image

#endif /* _TRACKER_BACKGROUND_H */
