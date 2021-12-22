/*****************************************************************************/
// ImageView
// Written by Michael Wilber, Haiku Translation Kit Team
//
// ImageView.cpp
//
// BView class for showing images.  Images can be dropped on this view
// from the tracker and images viewed in this view can be dragged
// to the tracker to be saved.
//
//
// Copyright (c) 2003 Haiku Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/


#include "ImageView.h"
#include "Constants.h"
#include "StatusCheck.h"
#include "InspectorApp.h"
#include "TranslatorItem.h"
#include <Application.h>
#include <Catalog.h>
#include <Message.h>
#include <Locale.h>
#include <List.h>
#include <String.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <BitmapStream.h>
#include <Entry.h>
#include <Path.h>
#include <Directory.h>
#include <File.h>
#include <MenuBar.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <Alert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BORDER_WIDTH 16
#define BORDER_HEIGHT 16
#define PEN_SIZE 1.0f

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ImageView"


ImageView::ImageView(BRect rect, const char *name)
	: BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS)
{
	fpbitmap = NULL;
	fdocumentIndex = 1;
	fdocumentCount = 1;

	SetViewColor(192, 192, 192);
	SetHighColor(0, 0, 0);
	SetPenSize(PEN_SIZE);
}


ImageView::~ImageView()
{
	delete fpbitmap;
	fpbitmap = NULL;
}


void
ImageView::AttachedToWindow()
{
	AdjustScrollBars();
}


void
ImageView::Draw(BRect rect)
{
	if (HasImage()) {
		// Draw black rectangle around image
		StrokeRect(
			BRect(BORDER_WIDTH - PEN_SIZE,
				BORDER_HEIGHT - PEN_SIZE,
				fpbitmap->Bounds().Width() + BORDER_WIDTH + PEN_SIZE,
				fpbitmap->Bounds().Height() + BORDER_HEIGHT + PEN_SIZE));

		DrawBitmap(fpbitmap, BPoint(BORDER_WIDTH, BORDER_HEIGHT));
	}
}


void
ImageView::ReDraw()
{
	Draw(Bounds());
}


void
ImageView::FrameResized(float width, float height)
{
	AdjustScrollBars();

	if (!HasImage())
		Invalidate();
}


void
ImageView::MouseDown(BPoint point)
{
	if (!HasImage())
		return;

	// Only accept left button clicks
	BMessage *pmsg = Window()->CurrentMessage();
	int32 button = pmsg->FindInt32("buttons");
	if (button != B_PRIMARY_MOUSE_BUTTON)
		return;

	// Tell BeOS to setup a Drag/Drop operation
	//
	// (When the image is dropped, BeOS sends
	// the following message to ImageWindow,
	// which causes it to call ImageView::SetImage())
	BMessage msg(B_SIMPLE_DATA);
	msg.AddInt32("be:actions", B_COPY_TARGET);
	msg.AddString("be:filetypes", "application/octet-stream");
	msg.AddString("be:types", "application/octet-stream");
	msg.AddString("be:clip_name", "Bitmap");

	DragMessage(&msg, Bounds());
}


void
ImageView::MouseMoved(BPoint point, uint32 state, const BMessage *pmsg)
{
}


void
ImageView::MouseUp(BPoint point)
{
}


void
ImageView::MessageReceived(BMessage *pmsg)
{
	switch (pmsg->what) {
		case B_COPY_TARGET:
			SaveImageAtDropLocation(pmsg);
			break;

		default:
			BView::MessageReceived(pmsg);
			break;
	}
}


void
ImageView::SaveImageAtDropLocation(BMessage *pmsg)
{
	// Find the location and name of the drop and
	// write the image file there
	BBitmapStream stream(fpbitmap);

	StatusCheck chk;
		// throw an exception if this is assigned
		// anything other than B_OK

	try {
		entry_ref dirref;
		chk = pmsg->FindRef("directory", &dirref);
		const char *filename;
		chk = pmsg->FindString("name", &filename);

		BDirectory dir(&dirref);
		BFile file(&dir, filename, B_WRITE_ONLY | B_CREATE_FILE);
		chk = file.InitCheck();

		BTranslatorRoster *proster = BTranslatorRoster::Default();
		chk = proster->Translate(&stream, NULL, NULL, &file, B_TGA_FORMAT);

	} catch (StatusNotOKException) {
		BAlert *palert = new BAlert(NULL,
			B_TRANSLATE("Sorry, unable to write the image file."),
			B_TRANSLATE("OK"));
		palert->Go();
	}

	stream.DetachBitmap(&fpbitmap);
}


void
ImageView::AdjustScrollBars()
{
	BRect rctview = Bounds(), rctbitmap(0, 0, 0, 0);
	if (HasImage())
		rctbitmap = fpbitmap->Bounds();

	float prop, range;
	BScrollBar *psb = ScrollBar(B_HORIZONTAL);
	if (psb) {
		range = rctbitmap.Width() + (BORDER_WIDTH * 2) - rctview.Width();
		if (range < 0) range = 0;
		prop = rctview.Width() / (rctbitmap.Width() + (BORDER_WIDTH * 2));
		if (prop > 1.0f) prop = 1.0f;
		psb->SetRange(0, range);
		psb->SetProportion(prop);
		psb->SetSteps(10, 100);
	}

	psb = ScrollBar(B_VERTICAL);
	if (psb) {
		range = rctbitmap.Height() + (BORDER_HEIGHT * 2) - rctview.Height();
		if (range < 0) range = 0;
		prop = rctview.Height() / (rctbitmap.Height() + (BORDER_HEIGHT * 2));
		if (prop > 1.0f) prop = 1.0f;
		psb->SetRange(0, range);
		psb->SetProportion(prop);
		psb->SetSteps(10, 100);
	}
}


struct ColorSpaceName {
	color_space id;
	const char *name;
};
#define COLORSPACENAME(id) {id, #id}


// convert colorspace numerical value to
// a string value
const char *
get_color_space_name(color_space colors)
{
	// print out colorspace if it matches an item in the list
	const ColorSpaceName kcolorspaces[] = {
		COLORSPACENAME(B_NO_COLOR_SPACE),
		COLORSPACENAME(B_RGB32),
		COLORSPACENAME(B_RGBA32),
		COLORSPACENAME(B_RGB24),
		COLORSPACENAME(B_RGB16),
		COLORSPACENAME(B_RGB15),
		COLORSPACENAME(B_RGBA15),
		COLORSPACENAME(B_CMAP8),
		COLORSPACENAME(B_GRAY8),
		COLORSPACENAME(B_GRAY1),
		COLORSPACENAME(B_RGB32_BIG),
		COLORSPACENAME(B_RGBA32_BIG),
		COLORSPACENAME(B_RGB24_BIG),
		COLORSPACENAME(B_RGB16_BIG),
		COLORSPACENAME(B_RGB15_BIG),
		COLORSPACENAME(B_RGBA15_BIG),
		COLORSPACENAME(B_YCbCr422),
		COLORSPACENAME(B_YCbCr411),
		COLORSPACENAME(B_YCbCr444),
		COLORSPACENAME(B_YCbCr420),
		COLORSPACENAME(B_YUV422),
		COLORSPACENAME(B_YUV411),
		COLORSPACENAME(B_YUV444),
		COLORSPACENAME(B_YUV420),
		COLORSPACENAME(B_YUV9),
		COLORSPACENAME(B_YUV12),
		COLORSPACENAME(B_UVL24),
		COLORSPACENAME(B_UVL32),
		COLORSPACENAME(B_UVLA32),
		COLORSPACENAME(B_LAB24),
		COLORSPACENAME(B_LAB32),
		COLORSPACENAME(B_LABA32),
		COLORSPACENAME(B_HSI24),
		COLORSPACENAME(B_HSI32),
		COLORSPACENAME(B_HSIA32),
		COLORSPACENAME(B_HSV24),
		COLORSPACENAME(B_HSV32),
		COLORSPACENAME(B_HSVA32),
		COLORSPACENAME(B_HLS24),
		COLORSPACENAME(B_HLS32),
		COLORSPACENAME(B_HLSA32),
		COLORSPACENAME(B_CMY24),
		COLORSPACENAME(B_CMY32),
		COLORSPACENAME(B_CMYA32),
		COLORSPACENAME(B_CMYK32)
	};
	const int32 kncolorspaces =  sizeof(kcolorspaces) /
		sizeof(ColorSpaceName);
	for (int32 i = 0; i < kncolorspaces; i++) {
		if (colors == kcolorspaces[i].id)
			return kcolorspaces[i].name;
	}

	return B_TRANSLATE("Unknown");
}


// return a string of the passed number formated
// as a hexadecimal number in lowercase with a leading "0x"
const char *
hex_format(uint32 num)
{
	static char str[11] = { 0 };
	sprintf(str, "0x%.8lx", num);

	return str;
}


// convert passed number to a string of 4 characters
// and return that string
const char *
char_format(uint32 num)
{
	static char str[5] = { 0 };
	uint32 bnum = B_HOST_TO_BENDIAN_INT32(num);
	memcpy(str, &bnum, 4);

	return str;
}


void
dump_translation_formats(BString &bstr, const translation_format *pfmts,
	int32 nfmts)
{
	BString *str1 = NULL;
	for (int i = 0; i < nfmts; i++) {
		BString string = B_TRANSLATE("\nType: '%1' (%2)\n"
			"Group: '%3' (%4)\n"
			"Quality: %5\n"
			"Capability: %6\n"
			"MIME Type: %7\n"
			"Name: %8\n");
		string.ReplaceFirst("%1", char_format(pfmts[i].type));
		string.ReplaceFirst("%2", hex_format(pfmts[i].type));
		string.ReplaceFirst("%3", char_format(pfmts[i].group));
		string.ReplaceFirst("%4", hex_format(pfmts[i].group));
		char str2[127] = { 0 };
		sprintf(str2, "%f", pfmts[i].quality);
		string.ReplaceFirst("%5", str2 );
		str2[0] = '\0';
		sprintf(str2, "%f", pfmts[i].capability);
		string.ReplaceFirst("%6",  str2 );
		string.ReplaceFirst("%7", pfmts[i].MIME);
		string.ReplaceFirst("%8", pfmts[i].name);
		if (i == 0)
			str1 = new BString(string);
		else
			str1->Append(string);
	}
	bstr = str1->String();
}


// Send information about the currently open image to the
// BApplication object so it can send it to the InfoWindow
void
ImageView::UpdateInfoWindow(const BPath &path, BMessage &ioExtension,
	const translator_info &tinfo, BTranslatorRoster *proster)
{
	BMessage msg(M_INFO_WINDOW_TEXT);
	BString bstr;

	bstr = B_TRANSLATE("Image: %1\n"
		"Color Space: %2 (%3)\n"
		"Dimensions: %4 x %5\n"
		"Bytes per Row: %6\n"
		"Total Bytes: %7\n"
		"\nIdentify Info:\n"
		"ID String: %8\n"
		"MIME Type: %9\n"
		"Type: '%10' (%11)\n"
		"Translator ID: %12\n"
		"Group: '%13' (%14)\n"
		"Quality: %15\n"
		"Capability: %16\n"
		"\nExtension Info:\n");
	bstr.ReplaceFirst("%1", path.Path());
	color_space cs = fpbitmap->ColorSpace();
	bstr.ReplaceFirst("%2", get_color_space_name(cs));
	bstr.ReplaceFirst("%3", hex_format(static_cast<uint32>(cs)));
	char str2[127] = { 0 };
	sprintf(str2, "%ld", fpbitmap->Bounds().IntegerWidth() + 1);
	bstr.ReplaceFirst("%4", str2);
	str2[0] = '\0';
	sprintf(str2, "%ld", fpbitmap->Bounds().IntegerHeight() + 1);
	bstr.ReplaceFirst("%5", str2);
	str2[0] = '\0';
	sprintf(str2, "%ld", fpbitmap->BytesPerRow());
	bstr.ReplaceFirst("%6", str2);
	str2[0] = '\0';
	sprintf(str2, "%ld", fpbitmap->BitsLength());
	bstr.ReplaceFirst("%7", str2);
	bstr.ReplaceFirst("%8", tinfo.name);
	bstr.ReplaceFirst("%9", tinfo.MIME);
	bstr.ReplaceFirst("%10", char_format(tinfo.type));
	bstr.ReplaceFirst("%11", hex_format(tinfo.type));
	str2[0] = '\0';
	sprintf(str2, "%ld", tinfo.translator);
	bstr.ReplaceFirst("%12", str2);
	bstr.ReplaceFirst("%13", char_format(tinfo.group));
	bstr.ReplaceFirst("%14", hex_format(tinfo.group));
	str2[0] = '\0';
	sprintf(str2, "%f", tinfo.quality);
	bstr.ReplaceFirst("%15", str2);
	str2[0] = '\0';
	sprintf(str2, "%f", tinfo.capability);
	bstr.ReplaceFirst("%16", str2);

	int32 document_count = 0, document_index = 0;
	// Translator Info
	const char *tranname = NULL, *traninfo = NULL;
	int32 tranversion = 0;

	if (ioExtension.FindInt32("/documentCount", &document_count) == B_OK) {
		BString str = B_TRANSLATE("Number of Documents: %1\n"
			"\nTranslator Used:\n"
			"Name: %2\n"
			"Info: %3\n"
			"Version: %4\n");
		char str2[127] = { 0 };
		sprintf(str2, "%ld", document_count);
		str.ReplaceFirst("%1", str2);
		str.ReplaceFirst("%2", tranname);
		str.ReplaceFirst("%3", traninfo);
		str2[0] = '\0';
		sprintf(str2, "%d", (int)tranversion);
		str.ReplaceFirst("%4", str2);
		bstr.Append(str.String());
	}
	else
		if (ioExtension.FindInt32("/documentIndex", &document_index) == B_OK) {
			BString str = B_TRANSLATE("Selected Document: %1\n"
				"\nTranslator Used:\n"
				"Name: %2\n"
				"Info: %3\n"
				"Version: %4\n");
			char str2[127] = { 0 };
			sprintf(str2, "%ld", document_index);
			str.ReplaceFirst("%1", str2);
			str.ReplaceFirst("%2", tranname);
			str.ReplaceFirst("%3", traninfo);
			str2[0] = '\0';
			sprintf(str2, "%d", (int)tranversion);
			str.ReplaceFirst("%4", str2);
			bstr.Append(str.String());
		}
		else
			if (proster->GetTranslatorInfo(tinfo.translator, &tranname,
				&traninfo, &tranversion) == B_OK) {
					BString str = B_TRANSLATE("\nTranslator Used:\n"
						"Name: %1\n"
						"Info: %2\n"
						"Version: %3\n");
					str.ReplaceFirst("%1", tranname);
					str.ReplaceFirst("%2", traninfo);
					char str2[127] = { 0 };
					sprintf(str2, "%d", (int)tranversion);
					str.ReplaceFirst("%3", str2);
					bstr.Append(str.String());
			}

	// Translator Input / Output Formats
	int32 nins = 0, nouts = 0;
	const translation_format *pins = NULL, *pouts = NULL;
	if (proster->GetInputFormats(tinfo.translator, &pins, &nins) == B_OK) {
		bstr << B_TRANSLATE("\nInput Formats:");
		dump_translation_formats(bstr, pins, nins);
		pins = NULL;
	}
	if (proster->GetOutputFormats(tinfo.translator, &pouts, &nouts) == B_OK) {
		bstr << B_TRANSLATE("\nOutput Formats:");
		dump_translation_formats(bstr, pouts, nouts);
		pouts = NULL;
	}
	msg.AddString("text", bstr);
	be_app->PostMessage(&msg);
}


BTranslatorRoster *
ImageView::SelectTranslatorRoster(BTranslatorRoster &roster)
{
	bool bNoneSelected = true;

	InspectorApp *papp;
	papp = static_cast<InspectorApp *>(be_app);
	if (papp) {
		BList *plist = papp->GetTranslatorsList();
		if (plist) {
			for (int32 i = 0; i < plist->CountItems(); i++) {
				BTranslatorItem *pitem =
					static_cast<BTranslatorItem *>(plist->ItemAt(i));
				if (pitem->IsSelected()) {
					bNoneSelected = false;
					roster.AddTranslators(pitem->Path());
				}
			}
		}
	}

	if (bNoneSelected)
		return BTranslatorRoster::Default();
	else
		return &roster;
}


void
ImageView::SetImage(BMessage *pmsg)
{
	// Replace current image with the image
	// specified in the given BMessage

	entry_ref ref;
	if (!pmsg)
		ref = fcurrentRef;
	else if (pmsg->FindRef("refs", &ref) != B_OK)
		// If refs not found, just ignore the message
		return;

	StatusCheck chk;

	try {
		BFile file(&ref, B_READ_ONLY);
		chk = file.InitCheck();

		BTranslatorRoster roster, *proster;
		proster = SelectTranslatorRoster(roster);
		if (!proster)
			// throw exception
			chk = B_ERROR;
		// determine what type the image is
		translator_info tinfo;
		BMessage ioExtension;
		if (ref != fcurrentRef)
			// if new image, reset to first document
			fdocumentIndex = 1;
		chk = ioExtension.AddInt32("/documentIndex", fdocumentIndex);
		chk = proster->Identify(&file, &ioExtension, &tinfo, 0, NULL,
			B_TRANSLATOR_BITMAP);

		// perform the actual translation
		BBitmapStream outstream;
		chk = proster->Translate(&file, &tinfo, &ioExtension, &outstream,
			B_TRANSLATOR_BITMAP);
		BBitmap *pbitmap = NULL;
		chk = outstream.DetachBitmap(&pbitmap);
		delete fpbitmap;
		fpbitmap = pbitmap;
		pbitmap = NULL;
		fcurrentRef = ref;
			// need to keep the ref around if user wants to switch pages
		int32 documentCount = 0;
		if (ioExtension.FindInt32("/documentCount", &documentCount) == B_OK &&
			documentCount > 0)
			fdocumentCount = documentCount;
		else
			fdocumentCount = 1;

		// Set the name of the Window to reflect the file name
		BWindow *pwin = Window();
		BEntry entry(&ref);
		BPath path;
		if (entry.InitCheck() == B_OK) {
			if (path.SetTo(&entry) == B_OK)
				pwin->SetTitle(path.Leaf());
			else
				pwin->SetTitle(IMAGEWINDOW_TITLE);
		} else
			pwin->SetTitle(IMAGEWINDOW_TITLE);
		UpdateInfoWindow(path, ioExtension, tinfo, proster);

		// Resize parent window and set size limits to
		// reflect the size of the new bitmap
		float width, height;
		BMenuBar *pbar = pwin->KeyMenuBar();
		width = fpbitmap->Bounds().Width() + B_V_SCROLL_BAR_WIDTH + (BORDER_WIDTH * 2);
		height = fpbitmap->Bounds().Height() +
			pbar->Bounds().Height() + B_H_SCROLL_BAR_HEIGHT + (BORDER_HEIGHT * 2) + 1;
		BScreen *pscreen = new BScreen(pwin);
		BRect rctscreen = pscreen->Frame();
		if (width > rctscreen.Width())
			width = rctscreen.Width();
		if (height > rctscreen.Height())
			height = rctscreen.Height();
		pwin->SetSizeLimits(B_V_SCROLL_BAR_WIDTH * 4, width,
			pbar->Bounds().Height() + (B_H_SCROLL_BAR_HEIGHT * 4) + 1, height);
		pwin->SetZoomLimits(width, height);
		AdjustScrollBars();

		//pwin->Zoom();
			// Perform all of the hard work of resizing the
			// window while taking into account the size of
			// the screen, the tab and borders of the window
			//
			// HACK: Need to fix case where window un-zooms
			// when the window is already the correct size
			// for the current image

		// repaint view
		Invalidate();

	} catch (StatusNotOKException) {
		BAlert *palert = new BAlert(NULL,
			B_TRANSLATE("Sorry, unable to load the image."),
			B_TRANSLATE("OK"));
		palert->Go();
	}
}


void
ImageView::FirstPage()
{
	if (fdocumentIndex != 1) {
		fdocumentIndex = 1;
		SetImage(NULL);
	}
}


void
ImageView::LastPage()
{
	if (fdocumentIndex != fdocumentCount) {
		fdocumentIndex = fdocumentCount;
		SetImage(NULL);
	}
}


void
ImageView::NextPage()
{
	if (fdocumentIndex < fdocumentCount) {
		fdocumentIndex++;
		SetImage(NULL);
	}
}


void
ImageView::PrevPage()
{
	if (fdocumentIndex > 1) {
		fdocumentIndex--;
		SetImage(NULL);
	}
}

