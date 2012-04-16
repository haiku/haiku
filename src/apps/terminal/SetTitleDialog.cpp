/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "SetTitleDialog.h"

#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <TextControl.h>


static const uint32 kMessageOK				= 'okok';
static const uint32 kMessageDefault			= 'dflt';
static const uint32 kMessageTitleChanged	= 'chng';


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Terminal SetTitleWindow"


// #pragma mark - SetTitleDialog


SetTitleDialog::SetTitleDialog(const char* dialogTitle, const char* label,
	const char* toolTip)
	:
	BWindow(BRect(0, 0, 0, 0), dialogTitle, B_BORDERED_WINDOW_LOOK,
		B_FLOATING_APP_WINDOW_FEEL,
		B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fListener(NULL),
	fTitle(),
	fTitleUserDefined(false)
{
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(10, 10, 10, 10)
		.Add(fTitleTextControl = new BTextControl("title", label, "", NULL))
		.AddGroup(B_HORIZONTAL)
			.Add(fDefaultButton = new BButton("defaultButton",
				B_TRANSLATE("Use default"), new BMessage(kMessageDefault)))
			.AddGlue()
			.Add(fCancelButton = new BButton("cancelButton",
				B_TRANSLATE("Cancel"), new BMessage(B_QUIT_REQUESTED)))
			.Add(fOKButton = new BButton("okButton", B_TRANSLATE("OK"),
				new BMessage(kMessageOK)));

	fTitleTextControl->SetToolTip(toolTip);

	fOKButton->MakeDefault(true);

	UpdateSizeLimits();
		// as a courtesy to our creator, who might want to place us
}


SetTitleDialog::~SetTitleDialog()
{
	if (fListener != NULL) {
		// reset to old title
		fListener->TitleChanged(this, fOldTitle, fOldTitleUserDefined);

		Listener* listener = fListener;
		fListener = NULL;
		listener->SetTitleDialogDone(this);
	}
}


void
SetTitleDialog::Go(const BString& title, bool titleUserDefined,
	Listener* listener)
{
	fTitle = fOldTitle = title;
	fTitleUserDefined = fOldTitleUserDefined = titleUserDefined;

	fDefaultButton->SetEnabled(titleUserDefined);

	fTitleTextControl->SetText(fTitle);
	fTitleTextControl->SetModificationMessage(
		new BMessage(kMessageTitleChanged));
	fTitleTextControl->MakeFocus(true);

	fListener = listener;

	Show();
}


void
SetTitleDialog::Finish()
{
	if (Listener* listener = fListener) {
		fListener = NULL;
		listener->SetTitleDialogDone(this);
	}

	PostMessage(B_QUIT_REQUESTED);
}


void
SetTitleDialog::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMessageDefault:
			if (fListener != NULL)
				fListener->TitleChanged(this, BString(), false);
			// We're done now, fall through...

		case kMessageOK:
			Finish();
			break;

		case kMessageTitleChanged:
			fTitle = fTitleTextControl->Text();
			fTitleUserDefined = true;

			fDefaultButton->SetEnabled(true);

			if (fListener != NULL)
				fListener->TitleChanged(this, fTitle, fTitleUserDefined);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}

}


// #pragma mark - SetTitleDialog


SetTitleDialog::Listener::~Listener()
{
}
