/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CONTROL_THEME_H
#define _CONTROL_THEME_H


#include <Entry.h>
#include <MediaDefs.h>

class BBitmap;
class BControl;
class BParameter;
class BParameterWeb;
class BRect;
class BView;


class BMediaTheme {
	public:
		virtual	~BMediaTheme();

		const char* Name();
		const char* Info();
		int32 ID();
		bool GetRef(entry_ref* ref);

		static BView* ViewFor(BParameterWeb* web, const BRect* hintRect = NULL,
			BMediaTheme* usingTheme = NULL);

		static status_t SetPreferredTheme(BMediaTheme* defaultTheme = NULL);
		static BMediaTheme* PreferredTheme();

		virtual	BControl* MakeControlFor(BParameter* control) = 0;

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

		virtual	BBitmap* BackgroundBitmapFor(bg_kind bg = B_GENERAL_BG);
		virtual	rgb_color BackgroundColorFor(bg_kind bg = B_GENERAL_BG);
		virtual	rgb_color ForegroundColorFor(fg_kind fg = B_GENERAL_FG);

	protected:
		BMediaTheme(const char* name, const char* info,
			const entry_ref* addOn = NULL, int32 themeID = 0);

		virtual	BView* MakeViewFor(BParameterWeb* web,
			const BRect* hintRect = NULL) = 0;

		static BControl* MakeFallbackViewFor(BParameter* control);

	private:
		BMediaTheme();		/* private unimplemented */
		BMediaTheme(const BMediaTheme& other);
		BMediaTheme& operator=(const BMediaTheme& other);

		virtual status_t _Reserved_ControlTheme_0(void *);
		virtual status_t _Reserved_ControlTheme_1(void *);
		virtual status_t _Reserved_ControlTheme_2(void *);
		virtual status_t _Reserved_ControlTheme_3(void *);
		virtual status_t _Reserved_ControlTheme_4(void *);
		virtual status_t _Reserved_ControlTheme_5(void *);
		virtual status_t _Reserved_ControlTheme_6(void *);
		virtual status_t _Reserved_ControlTheme_7(void *);

		char*		fName;
		char*		fInfo;
		int32		fID;
		bool		fIsAddOn;
		entry_ref	fAddOnRef;

		uint32 _reserved[8];

		static BMediaTheme* sDefaultTheme;
};


// Theme add-ons should export these functions:
#if defined(_BUILDING_THEME_ADDON)
extern "C" BMediaTheme* make_theme(int32 id, image_id you);
extern "C" status_t get_theme_at(int32 index, const char** _name,
	const char** _info, int32* _id);
#endif	// _BUILDING_THEME_ADDON

#endif	// _CONTROL_THEME_H
