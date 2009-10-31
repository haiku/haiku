/*
 * Copyright (c) 2008-2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 */


#include <Application.h>
#include <Window.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Screen.h>

#include "GradientsView.h"

#define MSG_LINEAR			'gtli'
#define MSG_RADIAL			'gtra'
#define MSG_RADIAL_FOCUS	'gtrf'
#define MSG_DIAMOND			'gtdi'
#define MSG_CONIC			'gtco'

class GradientsWindow : public BWindow {
public:
							GradientsWindow(void);

			bool			QuitRequested(void);
	virtual	void			MessageReceived(BMessage* msg);

private:
			BPopUpMenu*		fGradientsMenu;
			BMenuItem*		fLinearItem;
			BMenuItem*		fRadialItem;
			BMenuItem*		fRadialFocusItem;
			BMenuItem*		fDiamondItem;
			BMenuItem*		fConicItem;
			BMenuField*		fGradientsTypeField;
			GradientsView*	fGradientsView;
};
