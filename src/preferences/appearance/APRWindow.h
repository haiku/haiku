/*
 * Copyright 2002-2012, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 *		Alexander von Gluck, kallisti5@unixzen.com
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef APR_WINDOW_H
#define APR_WINDOW_H


#include <Application.h>
#include <Button.h>
#include <Window.h>
#include <Message.h>
#include <TabView.h>

class APRView;
class AntialiasingSettingsView;
class FontView;
class DecorSettingsView;


class APRWindow : public BWindow {
public:
							APRWindow(BRect frame);
			void			MessageReceived(BMessage *message);

private:
			void			_UpdateButtons();
			bool			_IsDefaultable() const;
			bool			_IsRevertable() const;

		APRView*			fColorsView;
		BButton*			fDefaultsButton;
		BButton*			fRevertButton;

		AntialiasingSettingsView* fAntialiasingSettings;
		FontView*			fFontSettings;
		DecorSettingsView*	fDecorSettings;
};


static const int32 kMsgUpdate = 'updt';


#endif
