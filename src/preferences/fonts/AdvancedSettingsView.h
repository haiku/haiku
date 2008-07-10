/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrej Spielmann, <andrej.spielmann@seh.ox.ac.uk>
 */
#ifndef ADVANCED_SETTINGS_VIEW_H
#define ADVANCED_SETTINGS_VIEW_H


#include <View.h>

class BBox;
class BMenuField;
class BPopUpMenu;


class AdvancedSettingsView : public BView {
	public:
		AdvancedSettingsView(BRect rect, const char* name);
		virtual ~AdvancedSettingsView();

		virtual void	GetPreferredSize(float *_width, float *_height);
		virtual void	RelayoutIfNeeded();
		virtual void	AttachedToWindow();
		virtual void	MessageReceived(BMessage *msg);

		void			SetDivider(float divider);

		void			SetDefaults();
		void			Revert();
		bool			IsDefaultable();
		bool			IsRevertable();

	private:
		void			_BuildAntialiasingMenu();
		void			_SetCurrentAntialiasing();
		void			_BuildHintingMenu();
		void			_SetCurrentHinting();

	protected:
		float			fDivider;

		BMenuField*		fAntialiasingMenuField;
		BPopUpMenu*		fAntialiasingMenu;
		BMenuField*		fHintingMenuField;
		BPopUpMenu*		fHintingMenu;

		bool			fSavedSubpixelAntialiasing;
		bool			fCurrentSubpixelAntialiasing;
		bool			fSavedHinting;
		bool			fCurrentHinting;
};

#endif	/* ADVANCED_SETTINGS_VIEW_H */
