/*
 * Copyright 2002-2025, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 *		Alexander von Gluck, kallisti5@unixzen.com
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef APPEARANCE_WINDOW_H
#define APPEARANCE_WINDOW_H


#include <Application.h>
#include <Button.h>
#include <Window.h>
#include <Message.h>
#include <TabView.h>

class ColorsView;
class AntialiasingSettingsView;
class FontView;
class LookAndFeelSettingsView;


class AppearanceWindow : public BWindow {
public:
									AppearanceWindow(BRect frame);
			void					MessageReceived(BMessage *message);

private:
			void					_UpdateButtons();
			bool					_IsDefaultable() const;
			bool					_IsRevertable() const;

		ColorsView*					fColorsView;
		BButton*					fDefaultsButton;
		BButton*					fRevertButton;

		AntialiasingSettingsView* 	fAntialiasingSettings;
		FontView*					fFontSettings;
		LookAndFeelSettingsView*	fLookAndFeelSettings;
};


static const int32 kMsgUpdate = 'updt';


#endif /* APPEARANCE_WINDOW_H */
