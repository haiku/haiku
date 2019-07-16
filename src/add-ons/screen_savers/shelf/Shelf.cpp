/*
 * Copyright 2007-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood
 *		François Revol <revol@free.fr>
 */


#include <Application.h>
#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Screen.h>
#include <ScreenSaver.h>
#include <Shelf.h>
#include <String.h>
#include <StringView.h>
#include <View.h>
#include <Window.h>
#include <Debug.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Shelf Screen Saver"


const rgb_color kMediumBlue = {0, 0, 100};
const rgb_color kWhite = {255, 255, 255};

const char *kInConfigName = "InConfig";
const char *kShelfArchiveName = "Shelf";


// Inspired by the classic BeOS screensaver, of course
class Shelf : public BScreenSaver
{
	public:
					Shelf(BMessage *archive, image_id);
		void		Draw(BView *view, int32 frame);
		void		StartConfig(BView *view);
		void		StopConfig();
		status_t	StartSaver(BView *view, bool preview);
		void		StopSaver();
		status_t	SaveState(BMessage *state) const;

	private:
		BShelf		*fShelf;
		BWindow		*fConfigWindow;
		bool		fInConfig;
		bool		fInEdit;
		BMallocIO	fShelfData;
};


BScreenSaver *instantiate_screen_saver(BMessage *msg, image_id image)
{
	PRINT(("%s()\n", __FUNCTION__));
	return new Shelf(msg, image);
}


Shelf::Shelf(BMessage *archive, image_id id)
 :	BScreenSaver(archive, id)
 ,	fShelf(NULL)
 ,	fConfigWindow(NULL)
 ,	fInConfig(false)
 ,	fInEdit(false)
 ,	fShelfData()
{
	archive->PrintToStream();
	if (archive->FindBool(kInConfigName, &fInConfig) < B_OK)
		fInConfig = false;

	status_t status;
	const void *data;
	ssize_t length;

	status = archive->FindData(kShelfArchiveName, 'shlf', &data, &length);
	if (status == B_OK) {
		fShelfData.WriteAt(0LL, data, length);
		fShelfData.Seek(SEEK_SET, 0LL);
	}


/*
	if (fInConfig) {
		fInEdit = true;
		fInConfig = false;
	}
*/}


void
Shelf::StartConfig(BView *view)
{
	PRINT(("%p:%s()\n", this, __FUNCTION__));
	fInConfig = true;

	BStringView* titleString = new BStringView("Title",
		B_TRANSLATE("Shelf"));
	titleString->SetFont(be_bold_font);

	BStringView* copyrightString = new BStringView("Copyright",
		B_TRANSLATE("© 2012 François Revol."));

	BTextView* helpText = new BTextView("Help Text");
	helpText->MakeEditable(false);
	helpText->SetViewColor(view->ViewColor());
	rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);
	helpText->SetFontAndColor(be_plain_font, B_FONT_ALL, &textColor);
	BString help;
	help << B_TRANSLATE("Drop replicants on the full-screen window "
		"behind the preferences panel.");
	//help << "\n\n";
	//help << B_TRANSLATE("You can also drop colors.");
	helpText->SetText(help.String());

	BLayoutBuilder::Group<>(view, B_VERTICAL, B_USE_HALF_ITEM_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(titleString)
		.Add(copyrightString)
		.AddStrut(roundf(be_control_look->DefaultItemSpacing() / 2))
		.Add(helpText)
		.AddGlue()
	.End();

	BScreen screen;
	fConfigWindow = new BWindow(screen.Frame(), "Shelf Config",
		B_UNTYPED_WINDOW, B_NOT_MOVABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE
		| B_NOT_MINIMIZABLE | B_NOT_RESIZABLE | B_AVOID_FRONT | B_AVOID_FOCUS);

	BView *shelfView = new BView(fConfigWindow->Bounds(), "ShelfView",
		B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS);
	shelfView->SetViewColor(216, 216, 216, 0);

	fConfigWindow->AddChild(shelfView);

	fShelfData.Seek(SEEK_SET, 0LL);
	fShelf = new BShelf(&fShelfData, shelfView);
	fShelf->SetDisplaysZombies(true);
	fShelfData.Seek(SEEK_SET, 0LL);

	// start the Looper
	fConfigWindow->Show();

	fConfigWindow->Lock();
	fConfigWindow->SendBehind(view->Window());
	fConfigWindow->Unlock();

	//"\nDrop replicants on me!"
}


void
Shelf::StopConfig()
{
	fInConfig = false;
	PRINT(("%p:%s()\n", this, __FUNCTION__));
	fConfigWindow->Lock();
	fConfigWindow->Quit();
	fShelf = NULL;

	BScreenSaver::StopConfig();
}


status_t
Shelf::StartSaver(BView *view, bool preview)
{
	PRINT(("%p:%s(, %d)\n", this, __FUNCTION__, preview));
	if (!preview) {
		view->SetViewColor(216, 216, 216, 0);
		fShelfData.Seek(SEEK_SET, 0LL);
		fShelf = new BShelf(&fShelfData, view);

	}
	BString s;
	s << "preview: " << preview << " ";
	s << "BView:Name: " << view->Name() << " ";
	s << "BApp:Name: " << be_app->Name();

	PRINT(("%p:%s:%s\n", this, __FUNCTION__, s.String()));
	//BAlert *a = new BAlert("debug", s.String(), "OK");
	//a->Go();
	return B_ERROR;
#if 0
	float width = view->Bounds().Width();
	float height = view->Bounds().Height();

	BFont font;
	view->GetFont(&font);
	font.SetSize(height / 2.5);
	view->SetFont(&font);

	BRect rect;
	escapement_delta delta;
	delta.nonspace = 0;
	delta.space = 0;
	// If anyone has suggestions for how to clean this up, speak up
	font.GetBoundingBoxesForStrings(&fLine1, 1, B_SCREEN_METRIC, &delta, &rect);
	float y = ((height - (rect.Height() * 2 + height / 10)) / 2) + rect.Height();
	fLine1Start.Set((width - rect.Width()) / 2, y);
	font.GetBoundingBoxesForStrings(&fLine2, 1, B_SCREEN_METRIC, &delta, &rect);
	fLine2Start.Set((width - rect.Width()) / 2, y + rect.Height() + height / 10);

#endif
	return B_OK;
}


void
Shelf::StopSaver()
{
	PRINT(("%p:%s()\n", this, __FUNCTION__));
	//if (fShelf)
		//delete fShelf;
	//fShelf = NULL;
}


status_t
Shelf::SaveState(BMessage *state) const
{
	status_t status;

	PRINT(("%p:%s()\n", this, __FUNCTION__));
	state->PrintToStream();

	if (fInConfig)
		state->AddBool(kInConfigName, fInConfig);
	if (!fInConfig)
		state->RemoveData(kInConfigName);
	if (fInConfig && fShelf) {
		status = state->AddBool("got it", true);

		fShelf->LockLooper();
		status = fShelf->Save();
		fShelf->UnlockLooper();
		if (status < B_OK)
			return status;
		status = state->AddData(kShelfArchiveName, 'shlf', fShelfData.Buffer(),
			fShelfData.BufferLength());

//		return B_OK;
//fShelfData.SetSize(0LL);
#if 0
		BMallocIO mio;
		status = fShelf->SetSaveLocation(&mio);
		if (status < B_OK)
			return status;
		status = fShelf->Save();
		fShelf->SetSaveLocation((BDataIO *)NULL);
		if (status < B_OK)
			return status;
		status = state->AddData(kShelfArchiveName, 'shlf', mio.Buffer(),
			mio.BufferLength());
#endif
		if (status < B_OK)
			return status;
	}
	return B_OK;
}


void
Shelf::Draw(BView *view, int32 frame)
{
	PRINT(("%p:%s(, %" B_PRId32 ")\n", this, __FUNCTION__, frame));
	BScreenSaver::Draw(view, frame);
#if 0
	if (frame == 0) {
		// fill with blue on first frame
		view->SetLowColor(kMediumBlue);
		view->FillRect(view->Bounds(), B_SOLID_LOW);

		// Set tick size to 500,000 microseconds = 0.5 second
		SetTickSize(500000);
	} else {
		// Drawing the background color every other frame to make the text blink
		if (frame % 2 == 1)
			view->SetHighColor(kWhite);
		else
			view->SetHighColor(kMediumBlue);

		view->DrawString(fLine1, fLine1Start);
		view->DrawString(fLine2, fLine2Start);
	}
#endif
}


