/*******************************************************************************
/
/	File:			ControlTheme.h
/
/   Description:  A BMediaTheme is something which can live in an add-on and is 
/	responsible for creating BViews or BControls from a BParameterWeb and its 
/	set of BMControls. This way, different "looks" for Media Kit control panels 
/	can be achieved.
/
/	Copyright 1997-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#if !defined(_CONTROL_THEME_H)
#define _CONTROL_THEME_H


#include <Entry.h>
#include <MediaDefs.h>

class BParameterWeb;


class BMediaTheme
{
public:
virtual	~BMediaTheme();

		const char * Name();
		const char * Info();
		int32 ID();
		bool GetRef(
				entry_ref * out_ref);

static	BView * ViewFor(
				BParameterWeb * web,
				const BRect * hintRect = NULL,
				BMediaTheme * using_theme = NULL);

		/* This function takes possession of default_theme, even it if returns error */
static	status_t SetPreferredTheme(
				BMediaTheme * default_theme = NULL);

static	BMediaTheme * PreferredTheme();

virtual	BControl * MakeControlFor(
				BParameter * control) = 0;

		enum bg_kind {
			B_GENERAL_BG = 0,
			B_SETTINGS_BG,
			B_PRESENTATION_BG,
			B_EDIT_BG,
			B_CONTROL_BG,
			B_HILITE_BG
		};
		enum fg_kind {
			B_GENERAL_FG = 0,
			B_SETTINGS_FG,
			B_PRESENTATION_FG,
			B_EDIT_FG,
			B_CONTROL_FG,
			B_HILITE_FG
		};

virtual	BBitmap * BackgroundBitmapFor(
				bg_kind bg = B_GENERAL_BG);
virtual	rgb_color BackgroundColorFor(
				bg_kind bg = B_GENERAL_BG);
virtual	rgb_color ForegroundColorFor(
				fg_kind fg = B_GENERAL_FG);

protected:
		BMediaTheme(
				const char * name,
				const char * info,
				const entry_ref * add_on = 0,
				int32 theme_id = 0);

virtual	BView * MakeViewFor(
				BParameterWeb * web,
				const BRect * hintRect = NULL) = 0;

static	BControl * MakeFallbackViewFor(
				BParameter * control);

private:

		BMediaTheme();		/* private unimplemented */
		BMediaTheme(
				const BMediaTheme & clone);
		BMediaTheme & operator=(
				const BMediaTheme & clone);

		/* Mmmh, stuffing! */
virtual		status_t _Reserved_ControlTheme_0(void *);
virtual		status_t _Reserved_ControlTheme_1(void *);
virtual		status_t _Reserved_ControlTheme_2(void *);
virtual		status_t _Reserved_ControlTheme_3(void *);
virtual		status_t _Reserved_ControlTheme_4(void *);
virtual		status_t _Reserved_ControlTheme_5(void *);
virtual		status_t _Reserved_ControlTheme_6(void *);
virtual		status_t _Reserved_ControlTheme_7(void *);

		char * _mName;
		char * _mInfo;
		int32 _mID;
		bool _mAddOn;
		entry_ref _mAddOnRef;
		uint32 _reserved_control_theme_[8];

static	BMediaTheme * _mDefaultTheme;

};

/* Theme add-ons should export the functions: */
#if defined(_BUILDING_THEME_ADDON)
extern "C" _EXPORT BMediaTheme * make_theme(int32 id, image_id you);
extern "C" _EXPORT status_t get_theme_at(int32 n, const char ** out_name, const char ** out_info, int32 * out_id);
#endif	/* _BUILDING_THEME_ADDON */

#endif	/* _CONTROL_THEME_H */
