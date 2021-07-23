/*
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "ShuttingDownWindow.h"

#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Locker.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ShuttingDownWindow"

#define WINDOW_FRAME BRect(0, 0, 240, 120)


ShuttingDownWindow::ShuttingDownWindow(BWindow* parent)
	:
	BWindow(WINDOW_FRAME, B_TRANSLATE("Shutting Down"),
		B_FLOATING_WINDOW_LOOK, B_MODAL_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
			| B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_CLOSABLE )
{
	AddToSubset(parent);

	BTextView* textView = new BTextView("shutting down message");
	textView->AdoptSystemColors();
	textView->MakeEditable(false);
	textView->MakeSelectable(false);
	textView->SetText(B_TRANSLATE("Haiku Depot is stopping or completing "
		"running operations before shutting down."));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
			.SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
				B_USE_WINDOW_SPACING, B_USE_DEFAULT_SPACING)
			.Add(textView)
		.End();

	CenterOnScreen();
}


ShuttingDownWindow::~ShuttingDownWindow()
{
}

