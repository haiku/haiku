/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "SavePanel.h"

#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <Locale.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <ScrollBar.h>
#include <TextControl.h>
#include <TranslationKit.h>
#include <View.h>
#include <Window.h>

#include "Exporter.h"
#include "IconEditorApp.h"
#include "Panel.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-SavePanel"


enum {
	MSG_FORMAT		= 'sfpf',
	MSG_SETTINGS	= 'sfps',
};

// SaveItem class
SaveItem::SaveItem(const char* name,
				   BMessage* message,
				   uint32 exportMode)
	: BMenuItem(name, message),
	  fExportMode(exportMode)
{
}

// #pragma mark -

// SavePanel class
SavePanel::SavePanel(const char* name,
					 BMessenger* target,
					 entry_ref* startDirectory,
					 uint32 nodeFlavors,
					 bool allowMultipleSelection,
					 BMessage* message,
					 BRefFilter* filter,
					 bool modal,
					 bool hideWhenDone)
	: BFilePanel(B_SAVE_PANEL, target, startDirectory,
				 nodeFlavors, allowMultipleSelection,
				 message, filter, modal, hideWhenDone),
	  BHandler(name),
	  fConfigWindow(NULL),
	  fFormatM(NULL),
	  fExportMode(EXPORT_MODE_ICON_RDEF)
{
	BWindow* window = Window();
	if (!window || !window->Lock())
		return;

	// add this instance as BHandler to the window's looper
	window->AddHandler(this);

	// find a couple of important views and mess with their layout
	BView* background = Window()->ChildAt(0);
	if (background == NULL) {
		printf("SavePanel::SavePanel() - couldn't find necessary controls.\n");
		return;
	}
	BButton* cancel = dynamic_cast<BButton*>(
		background->FindView("cancel button"));
	BView* textview = background->FindView("text view");

	if (!cancel || !textview) {
		printf("SavePanel::SavePanel() - couldn't find necessary controls.\n");
		return;
	}

	_BuildMenu();

	BRect rect = textview->Frame();
	rect.top = cancel->Frame().top;
	font_height fh;
	be_plain_font->GetHeight(&fh);
	rect.bottom = rect.top + fh.ascent + fh.descent + 5.0;

	fFormatMF = new BMenuField(rect, "format popup", B_TRANSLATE("Format"),
								fFormatM, true,
								B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
								B_WILL_DRAW | B_NAVIGABLE);
	fFormatMF->SetDivider(be_plain_font->StringWidth(
		B_TRANSLATE("Format")) + 7);
	fFormatMF->MenuBar()->ResizeToPreferred();
	fFormatMF->ResizeToPreferred();

	float height = fFormatMF->Bounds().Height() + 8.0;

	// find all the views that are in the way and
	// move up them up the height of the menu field
	BView *poseview = background->FindView("PoseView");
	if (poseview) poseview->ResizeBy(0, -height);
	BView *countvw = (BView *)background->FindView("CountVw");
	if (countvw) countvw->MoveBy(0, -height);
	textview->MoveBy(0, -height);

#if HAIKU_TARGET_PLATFORM_DANO
	fFormatMF->MoveTo(textview->Frame().left, fFormatMF->Frame().top + 2);
#else
	fFormatMF->MoveTo(textview->Frame().left, fFormatMF->Frame().top);
#endif

	background->AddChild(fFormatMF);

	// Build the "Settings" button relative to the format menu
	rect = cancel->Frame();
	rect.OffsetTo(fFormatMF->Frame().right + 5.0, rect.top);
	fSettingsB = new BButton(rect, "settings",
							 B_TRANSLATE("Settings" B_UTF8_ELLIPSIS),
							 new BMessage(MSG_SETTINGS),
							 B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
							 B_WILL_DRAW | B_NAVIGABLE);
	fSettingsB->ResizeToPreferred();
	background->AddChild(fSettingsB);
	fSettingsB->SetTarget(this);

	textview->ResizeTo(fSettingsB->Frame().right - fFormatMF->Frame().left,
					   textview->Frame().Height());

	BButton *insert = (BButton *)background->FindView("default button");

	// Make sure the smallest window won't draw the "Settings" button over
	// anything else
	float minWindowWidth = textview->Bounds().Width()
							+ cancel->Bounds().Width()
							+ (insert ? insert->Bounds().Width() : 0.0)
							+ 90;
	Window()->SetSizeLimits(minWindowWidth, 10000, 250, 10000);
	if (Window()->Bounds().IntegerWidth() + 1 < minWindowWidth)
		Window()->ResizeTo(minWindowWidth, Window()->Bounds().Height());


	// Init window title
	SetExportMode(true);

	window->Unlock();
}

// destructor
SavePanel::~SavePanel()
{
}

// SendMessage
void
SavePanel::SendMessage(const BMessenger* messenger, BMessage* message)
{
	// add the current format information to the message,
	// bot only if we are indeed in export mode
	if (message && fFormatM->IsEnabled())
		message->AddInt32("export mode", ExportMode());
	// let the original file panel code handle the rest
	BFilePanel::SendMessage(messenger, message);
}

// MessageReceived
void
SavePanel::MessageReceived(BMessage* message)
{
	// Handle messages from controls we've added
	switch (message->what) {
		case MSG_FORMAT:
			fExportMode = ExportMode();
			AdjustExtension();
				// TODO: make this behaviour a setting
			_EnableSettings();
			break;
		case MSG_SETTINGS:
			_ExportSettings();
			break;
		default:
			BHandler::MessageReceived(message);
			break;
	}
}

// SetExportMode
void
SavePanel::SetExportMode(bool exportMode)
{
	BWindow* window = Window();
	if (!window || !window->Lock())
		return;

	// adjust window title and enable format menu
	BString title("Icon-O-Matic: ");
	if (exportMode) {
		fFormatMF->SetEnabled(true);
		SetExportMode(fExportMode);
		_EnableSettings();
		title << B_TRANSLATE_CONTEXT("Export icon", "Dialog title");
	} else {
		fExportMode = ExportMode();
			// does not overwrite fExportMode in case we already were
			// in native save mode
		fNativeMI->SetMarked(true);

		fFormatMF->SetEnabled(false);
		fSettingsB->SetEnabled(false);
		title << B_TRANSLATE_CONTEXT("Save icon", "Dialog title");
	}

	window->SetTitle(title);
	window->Unlock();
}

// SetExportMode
void
SavePanel::SetExportMode(int32 mode)
{
	BWindow* window = Window();
	if (!window || !window->Lock())
		return;

	switch (mode) {
		case EXPORT_MODE_MESSAGE:
			fNativeMI->SetMarked(true);
			break;
		case EXPORT_MODE_FLAT_ICON:
			fHVIFMI->SetMarked(true);
			break;
		case EXPORT_MODE_SVG:
			fSVGMI->SetMarked(true);
			break;
		case EXPORT_MODE_BITMAP_16:
			fBitmap16MI->SetMarked(true);
			break;
		case EXPORT_MODE_BITMAP_32:
			fBitmap32MI->SetMarked(true);
			break;
		case EXPORT_MODE_BITMAP_64:
			fBitmap64MI->SetMarked(true);
			break;
		case EXPORT_MODE_BITMAP_SET:
			fBitmapSetMI->SetMarked(true);
			break;
		case EXPORT_MODE_ICON_ATTR:
			fIconAttrMI->SetMarked(true);
			break;
		case EXPORT_MODE_ICON_MIME_ATTR:
			fIconMimeAttrMI->SetMarked(true);
			break;
		case EXPORT_MODE_ICON_RDEF:
			fRDefMI->SetMarked(true);
			break;
		case EXPORT_MODE_ICON_SOURCE:
			fSourceMI->SetMarked(true);
			break;
	}

	if (mode != EXPORT_MODE_MESSAGE)
		fExportMode = mode;

	fFormatMF->SetEnabled(mode != EXPORT_MODE_MESSAGE);
	_EnableSettings();

	window->Unlock();
}

// ExportMode
int32
SavePanel::ExportMode() const
{
	int32 mode = fExportMode;
	BWindow* window = Window();
	if (!window || !window->Lock())
		return mode;

	if (fFormatMF->IsEnabled()) {
		// means we are actually in export mode
		SaveItem* item = _GetCurrentMenuItem();
		mode = item->ExportMode();
	}
	window->Unlock();

	return mode;
}

// AdjustExtension
void
SavePanel::AdjustExtension()
{
//	if (!Window()->Lock())
//		return;
//
//	BView* background = Window()->ChildAt(0);
//	BTextControl* textview = dynamic_cast<BTextControl*>(
//		background->FindView("text view"));
//
//	if (textview) {
//
//		translator_id id = 0;
//		uint32 format = 0;
//		int32 mode = ExportMode();
//		SaveItem* exportItem = dynamic_cast<SaveItem*>(_GetCurrentMenuItem());
//		if (mode == EXPORT_TRANSLATOR && exportItem) {
//			id = exportItem->id;
//			format = exportItem->format;
//		}
//
//		Exporter* exporter = Exporter::ExporterFor(mode, id, format);
//
//		if (exporter) {
//			BString name(textview->Text());
//
//			// adjust the name extension
//			const char* extension = exporter->Extension();
//			if (strlen(extension) > 0) {
//				int32 cutPos = name.FindLast('.');
//				int32 cutCount = name.Length() - cutPos;
//				if (cutCount > 0 && cutCount <= 4) {
//					name.Remove(cutPos, cutCount);
//				}
//				name << "." << extension;
//			}
//
//			SetSaveText(name.String());
//		}
//
//		delete exporter;
//	}
//	Window()->Unlock();
}

// _GetCurrentMenuItem
SaveItem*
SavePanel::_GetCurrentMenuItem() const
{
	SaveItem* item = dynamic_cast<SaveItem*>(fFormatM->FindMarked());
	if (!item)
		return fNativeMI;
	return item;
}

// _ExportSettings
void
SavePanel::_ExportSettings()
{
//	SaveItem *item = dynamic_cast<SaveItem*>(_GetCurrentMenuItem());
//	if (item == NULL)
//		return;
//
//	BTranslatorRoster *roster = BTranslatorRoster::Default();
//	BView *view;
//	BRect rect(0, 0, 239, 239);
//
//	// Build a window around this translator's configuration view
//	status_t err = roster->MakeConfigurationView(item->id, NULL, &view, &rect);
//	if (err < B_OK || view == NULL) {
//		BAlert *alert = new BAlert(NULL, strerror(err), B_TRANSLATE("OK"));
//		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
//		alert->Go();
//	} else {
//		if (fConfigWindow != NULL) {
//			if (fConfigWindow->Lock())
//				fConfigWindow->Quit();
//		}
//		fConfigWindow = new Panel(rect, "Translator Settings",
//								  B_TITLED_WINDOW_LOOK,
//								  B_NORMAL_WINDOW_FEEL,
//								  B_NOT_ZOOMABLE | B_NOT_RESIZABLE);
//		fConfigWindow->AddChild(view);
//		// Just to make sure
//		view->MoveTo(0, 0);
//		view->ResizeTo(rect.Width(), rect.Height());
//		view->ResizeToPreferred();
//		fConfigWindow->MoveTo(100, 100);
//		fConfigWindow->Show();
//	}
}

// _BuildMenu
void
SavePanel::_BuildMenu()
{
	fFormatM = new BPopUpMenu(B_TRANSLATE("Format"));

	fNativeMI = new SaveItem("Icon-O-Matic",
		new BMessage(MSG_FORMAT), EXPORT_MODE_MESSAGE);
	fFormatM->AddItem(fNativeMI);
	fNativeMI->SetEnabled(false);

	fFormatM->AddSeparatorItem();

	fHVIFMI = new SaveItem("HVIF",
		new BMessage(MSG_FORMAT), EXPORT_MODE_FLAT_ICON);
	fFormatM->AddItem(fHVIFMI);

	fRDefMI = new SaveItem("HVIF RDef",
		new BMessage(MSG_FORMAT), EXPORT_MODE_ICON_RDEF);
	fFormatM->AddItem(fRDefMI);

	fSourceMI = new SaveItem(B_TRANSLATE("HVIF Source Code"),
		new BMessage(MSG_FORMAT), EXPORT_MODE_ICON_SOURCE);
	fFormatM->AddItem(fSourceMI);

	fFormatM->AddSeparatorItem();

	fSVGMI = new SaveItem("SVG",
		new BMessage(MSG_FORMAT), EXPORT_MODE_SVG);

	fFormatM->AddItem(fSVGMI);

	fFormatM->AddSeparatorItem();

	fBitmap16MI = new SaveItem("PNG 16x16",
		new BMessage(MSG_FORMAT), EXPORT_MODE_BITMAP_16);
	fFormatM->AddItem(fBitmap16MI);

	fBitmap32MI = new SaveItem("PNG 32x32",
		new BMessage(MSG_FORMAT), EXPORT_MODE_BITMAP_32);
	fFormatM->AddItem(fBitmap32MI);

	fBitmap64MI = new SaveItem("PNG 64x64",
		new BMessage(MSG_FORMAT), EXPORT_MODE_BITMAP_64);
	fFormatM->AddItem(fBitmap64MI);

	fBitmapSetMI = new SaveItem(B_TRANSLATE("PNG Set"),
		new BMessage(MSG_FORMAT), EXPORT_MODE_BITMAP_SET);
	fFormatM->AddItem(fBitmapSetMI);

	fFormatM->AddSeparatorItem();

	fIconAttrMI = new SaveItem(B_TRANSLATE("BEOS:ICON Attribute"),
		new BMessage(MSG_FORMAT), EXPORT_MODE_ICON_ATTR);
	fFormatM->AddItem(fIconAttrMI);

	fIconMimeAttrMI = new SaveItem(B_TRANSLATE("META:ICON Attribute"),
		new BMessage(MSG_FORMAT), EXPORT_MODE_ICON_MIME_ATTR);

	fFormatM->AddItem(fIconMimeAttrMI);


	fFormatM->SetTargetForItems(this);

	// pick the RDef item in the list
	fRDefMI->SetMarked(true);
}

// _EnableSettings
void
SavePanel::_EnableSettings() const
{
	// no settings currently necessary
	fSettingsB->SetEnabled(false);
}

