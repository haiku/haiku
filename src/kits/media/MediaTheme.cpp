/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: MediaTheme.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaTheme.h>
#include "debug.h"

/*************************************************************
 * public BMediaTheme
 *************************************************************/

BMediaTheme::~BMediaTheme()
{
	UNIMPLEMENTED();
}


const char *
BMediaTheme::Name()
{
	UNIMPLEMENTED();
	return "";
}


const char *
BMediaTheme::Info()
{
	UNIMPLEMENTED();
	return "";
}


int32
BMediaTheme::ID()
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


bool
BMediaTheme::GetRef(entry_ref *out_ref)
{
	UNIMPLEMENTED();
	bool dummy;

	return dummy;
}


BView *
BMediaTheme::ViewFor(BParameterWeb *web,
					 const BRect *hintRect,
					 BMediaTheme *using_theme)
{
	UNIMPLEMENTED();
	return NULL;
}


status_t
BMediaTheme::SetPreferredTheme(BMediaTheme *default_theme)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


BMediaTheme *
BMediaTheme::PreferredTheme()
{
	UNIMPLEMENTED();
	return NULL;
}


BBitmap *
BMediaTheme::BackgroundBitmapFor(bg_kind bg)
{
	UNIMPLEMENTED();
	return NULL;
}


rgb_color
BMediaTheme::BackgroundColorFor(bg_kind bg)
{
	UNIMPLEMENTED();
	rgb_color dummy;

	return dummy;
}


rgb_color
BMediaTheme::ForegroundColorFor(fg_kind fg)
{
	UNIMPLEMENTED();
	rgb_color dummy;

	return dummy;
}

/*************************************************************
 * protected BMediaTheme
 *************************************************************/

BMediaTheme::BMediaTheme(const char *name,
						 const char *info,
						 const entry_ref *add_on,
						 int32 theme_id)
{
	UNIMPLEMENTED();
}


BControl *
BMediaTheme::MakeFallbackViewFor(BParameter *control)
{
	UNIMPLEMENTED();
	return NULL;
}

/*************************************************************
 * private BMediaTheme
 *************************************************************/

/*
private unimplemented
BMediaTheme::BMediaTheme()
BMediaTheme::BMediaTheme(const BMediaTheme &clone)
BMediaTheme & BMediaTheme::operator=(const BMediaTheme &clone)
*/

status_t BMediaTheme::_Reserved_ControlTheme_0(void *) { return B_ERROR; }
status_t BMediaTheme::_Reserved_ControlTheme_1(void *) { return B_ERROR; }
status_t BMediaTheme::_Reserved_ControlTheme_2(void *) { return B_ERROR; }
status_t BMediaTheme::_Reserved_ControlTheme_3(void *) { return B_ERROR; }
status_t BMediaTheme::_Reserved_ControlTheme_4(void *) { return B_ERROR; }
status_t BMediaTheme::_Reserved_ControlTheme_5(void *) { return B_ERROR; }
status_t BMediaTheme::_Reserved_ControlTheme_6(void *) { return B_ERROR; }
status_t BMediaTheme::_Reserved_ControlTheme_7(void *) { return B_ERROR; }

/*************************************************************
 * static BMediaTheme variables
 *************************************************************/

BMediaTheme * BMediaTheme::_mDefaultTheme;
