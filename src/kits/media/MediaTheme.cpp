/* Author: Marcus Overhagen
**         Axel Dörfler, axeld@pinc-software.de
**
** This file may be used under the terms of the OpenBeOS License.
*/

#include <MediaTheme.h>
#include <StringView.h>
#include <Locker.h>
#include <Autolock.h>

#include <string.h>

#include "DefaultMediaTheme.h"
#include "debug.h"


static BLocker sLock;

/*************************************************************
 * static BMediaTheme variables
 *************************************************************/

BMediaTheme * BMediaTheme::_mDefaultTheme;


/*************************************************************
 * public BMediaTheme
 *************************************************************/

BMediaTheme::~BMediaTheme()
{
	CALLED();

	free(_mName);
	free(_mInfo);
}


const char *
BMediaTheme::Name()
{
	return _mName;
}


const char *
BMediaTheme::Info()
{
	return _mInfo;
}


int32
BMediaTheme::ID()
{
	return _mID;
}


bool
BMediaTheme::GetRef(entry_ref *out_ref)
{
	UNIMPLEMENTED();

	return false;
}


BView *
BMediaTheme::ViewFor(BParameterWeb *web, const BRect *hintRect, BMediaTheme *usingTheme)
{
	CALLED();

	// use default theme if none was specified

	if (usingTheme == NULL) {
		BAutolock locker(sLock);

		if (_mDefaultTheme == NULL)
			_mDefaultTheme = new BPrivate::DefaultMediaTheme();

		//usingTheme = _mDefaultTheme;
	}

	if (usingTheme == NULL)
		return new BStringView(BRect(0, 0, 200, 30), "", "No BMediaTheme available, sorry!");

	return usingTheme->MakeViewFor(web, hintRect);
}


status_t
BMediaTheme::SetPreferredTheme(BMediaTheme *defaultTheme)
{
	CALLED();

	// ToDo: this method should probably set some global settings file
	//	to make the new preferred theme available to all applications

	BAutolock locker(sLock);

	if (defaultTheme == NULL) {
		// if the current preferred theme is not the default media theme,
		// delete it, and set it back to the default
		if (dynamic_cast<BPrivate::DefaultMediaTheme *>(_mDefaultTheme) == NULL)
			_mDefaultTheme = new BPrivate::DefaultMediaTheme();

		return B_OK;
	}

	// this method takes possession of the BMediaTheme passed, even
	// if it fails, so it has to delete it
	if (defaultTheme != _mDefaultTheme)
		delete _mDefaultTheme;

	_mDefaultTheme = defaultTheme;

	return B_OK;
}


BMediaTheme *
BMediaTheme::PreferredTheme()
{
	CALLED();

	BAutolock locker(sLock);

	// ToDo: should look in the global prefs file for the preferred
	//	add-on and load this from disk - in the meantime, just use
	//	the default theme

	if (_mDefaultTheme == NULL)
		_mDefaultTheme = new BPrivate::DefaultMediaTheme();

	return _mDefaultTheme;
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
	rgb_color dummy = {0, 0, 0};

	return dummy;
}


rgb_color
BMediaTheme::ForegroundColorFor(fg_kind fg)
{
	UNIMPLEMENTED();
	rgb_color dummy = {255, 255, 255};

	return dummy;
}


/*************************************************************
 * protected BMediaTheme
 *************************************************************/


BMediaTheme::BMediaTheme(const char *name, const char *info, const entry_ref *ref, int32 id)
	:
	_mID(id)
{
	_mName = strdup(name);
	_mInfo = strdup(info);

	// ToDo: is there something else here, which has to be done?

	if (ref) {
		_mAddOn = true;
		_mAddOnRef = *ref;
	} else
		_mAddOn = false;
}


BControl *
BMediaTheme::MakeFallbackViewFor(BParameter *parameter)
{
	if (parameter == NULL)
		return NULL;

	return BPrivate::DefaultMediaTheme::MakeViewFor(parameter);
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

