/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marcus Overhagen
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "DefaultMediaTheme.h"
#include "MediaDebug.h"

#include <MediaTheme.h>
#include <StringView.h>
#include <Locker.h>
#include <Autolock.h>

#include <string.h>


static BLocker sLock("media theme lock");

BMediaTheme* BMediaTheme::sDefaultTheme;


BMediaTheme::~BMediaTheme()
{
	CALLED();

	free(fName);
	free(fInfo);
}


const char *
BMediaTheme::Name()
{
	return fName;
}


const char *
BMediaTheme::Info()
{
	return fInfo;
}


int32
BMediaTheme::ID()
{
	return fID;
}


bool
BMediaTheme::GetRef(entry_ref* ref)
{
	if (!fIsAddOn || ref == NULL)
		return false;

	*ref = fAddOnRef;
	return true;
}


BView *
BMediaTheme::ViewFor(BParameterWeb* web, const BRect* hintRect,
	BMediaTheme* usingTheme)
{
	CALLED();

	// use default theme if none was specified
	if (usingTheme == NULL)
		usingTheme = PreferredTheme();

	if (usingTheme == NULL) {
		BStringView* view = new BStringView(BRect(0, 0, 200, 30), "",
			"No BMediaTheme available, sorry!");
		view->ResizeToPreferred();
		return view;
	}

	return usingTheme->MakeViewFor(web, hintRect);
}


status_t
BMediaTheme::SetPreferredTheme(BMediaTheme* defaultTheme)
{
	CALLED();

	// ToDo: this method should probably set some global settings file
	//	to make the new preferred theme available to all applications

	BAutolock locker(sLock);

	if (defaultTheme == NULL) {
		// if the current preferred theme is not the default media theme,
		// delete it, and set it back to the default
		if (dynamic_cast<BPrivate::DefaultMediaTheme *>(sDefaultTheme) == NULL)
			sDefaultTheme = new BPrivate::DefaultMediaTheme();

		return B_OK;
	}

	// this method takes possession of the BMediaTheme passed, even
	// if it fails, so it has to delete it
	if (defaultTheme != sDefaultTheme)
		delete sDefaultTheme;

	sDefaultTheme = defaultTheme;

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

	if (sDefaultTheme == NULL)
		sDefaultTheme = new BPrivate::DefaultMediaTheme();

	return sDefaultTheme;
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
	return ui_color(B_PANEL_BACKGROUND_COLOR);
}


rgb_color
BMediaTheme::ForegroundColorFor(fg_kind fg)
{
	UNIMPLEMENTED();
	rgb_color dummy = {255, 255, 255};

	return dummy;
}


//! protected BMediaTheme
BMediaTheme::BMediaTheme(const char* name, const char* info,
	const entry_ref* ref, int32 id)
	:
	fID(id)
{
	fName = strdup(name);
	fInfo = strdup(info);

	// ToDo: is there something else here, which has to be done?

	if (ref) {
		fIsAddOn = true;
		fAddOnRef = *ref;
	} else
		fIsAddOn = false;
}


BControl *
BMediaTheme::MakeFallbackViewFor(BParameter *parameter)
{
	if (parameter == NULL)
		return NULL;

	return BPrivate::DefaultMediaTheme::MakeViewFor(parameter);
}


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

