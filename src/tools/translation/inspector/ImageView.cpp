/*****************************************************************************/
// ImageView
// Written by Michael Wilber, OBOS Translation Kit Team
//
// ImageView.cpp
//
// BView class for showing images.  Images can be dropped on this view
// from the tracker and images viewed in this view can be dragged
// to the tracker to be saved.
//
//
// Copyright (c) 2003 OpenBeOS Project
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
#include <Application.h>
#include <Message.h>
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

ImageView::ImageView(BRect rect, const char *name)
	: BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS)
{
	fpbitmap = NULL;
	
	SetViewColor(255, 255, 255);
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
	if (HasImage())
		DrawBitmap(fpbitmap, BPoint(0, 0));
}

void
ImageView::FrameResized(float width, float height)
{
	AdjustScrollBars();
	
	// If there is a bitmap, draw it.
	// If not, invalidate the entire view so that
	// the advice text will be displayed
	if (HasImage())
		ReDraw();
	else
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
			"Sorry, unable to write the image file.", "OK");
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
		range = rctbitmap.Width() - rctview.Width();
		if (range < 0) range = 0;
		prop = rctview.Width() / rctbitmap.Width();
		if (prop > 1.0f) prop = 1.0f;
		psb->SetRange(0, range);
		psb->SetProportion(prop);
		psb->SetSteps(10, 100);
	}
	
	psb = ScrollBar(B_VERTICAL);
	if (psb) {
		range = rctbitmap.Height() - rctview.Height();
		if (range < 0) range = 0;
		prop = rctview.Height() / rctbitmap.Height();
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

	return "Unknown";
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

// Send information about the currently open image to the
// BApplication object so it can send it to the InfoWindow
void
ImageView::UpdateInfoWindow(const BPath &path, const translator_info &tinfo,
	const char *tranname, const char *traninfo, int32 tranversion)
{
	BMessage msg(M_INFO_WINDOW_TEXT);
	BString bstr;
	
	// Bitmap Info
	bstr << "Image: " << path.Path() << "\n";
	color_space cs = fpbitmap->ColorSpace();
	bstr << "Color Space: " << get_color_space_name(cs) << " (" << 
		hex_format(static_cast<uint32>(cs)) << ")\n";
	bstr << "Dimensions: " << fpbitmap->Bounds().IntegerWidth() + 1 << " x " <<
		fpbitmap->Bounds().IntegerHeight() + 1 << "\n";
	bstr << "Bytes per Row: " << fpbitmap->BytesPerRow() << "\n";
	bstr << "Total Bytes: " << fpbitmap->BitsLength() << "\n";
	
	// Identify Info
	bstr << "\nIdentify Info:\n";
	bstr << "ID String: " << tinfo.name << "\n";
	bstr << "MIME Type: " << tinfo.MIME << "\n";
	bstr << "Type: '" << char_format(tinfo.type) << "' (" <<
		hex_format(tinfo.type) <<")\n";
	bstr << "Translator ID: " << tinfo.translator << "\n";
	bstr << "Group: '" << char_format(tinfo.group) << "' (" <<
		hex_format(tinfo.group) << ")\n";
	bstr << "Quality: " << tinfo.quality << "\n";
	bstr << "Capability: " << tinfo.capability << "\n";
	
	// Translator Info
	bstr << "\nTranslator Used:\n";
	bstr << "Name: " << tranname << "\n";
	bstr << "Info: " << traninfo << "\n";
	bstr << "Version: " << tranversion << "\n";
	
	msg.AddString("text", bstr);
	be_app->PostMessage(&msg);
}

void
ImageView::SetImage(BMessage *pmsg)
{
	// Replace current image with the image
	// specified in the given BMessage
	
	StatusCheck chk;
	
	try {
		entry_ref ref;
		chk = pmsg->FindRef("refs", &ref);

		BFile file(&ref, B_READ_ONLY);
		chk = file.InitCheck();
	
		BTranslatorRoster *proster = BTranslatorRoster::Default();
		if (!proster)
			// throw exception
			chk = B_ERROR;
		
		// determine what type the image is
		translator_info tinfo;
		chk = proster->Identify(&file, NULL, &tinfo, 0, NULL,
			B_TRANSLATOR_BITMAP);
			
		// get the name and info about the translator
		const char *tranname = NULL, *traninfo = NULL;
		int32 tranversion = 0;
		chk = proster->GetTranslatorInfo(tinfo.translator, &tranname,
			&traninfo, &tranversion);
			
		// perform the actual translation
		BBitmapStream outstream;
		chk = proster->Translate(&file, &tinfo, NULL, &outstream,
			B_TRANSLATOR_BITMAP);
		BBitmap *pbitmap = NULL;
		chk = outstream.DetachBitmap(&pbitmap);
		fpbitmap = pbitmap;
		pbitmap = NULL;
		
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
			
		UpdateInfoWindow(path, tinfo, tranname, traninfo, tranversion);
		
		// Resize parent window and set size limits to 
		// reflect the size of the new bitmap
		float width, height;
		BMenuBar *pbar = pwin->KeyMenuBar();
		width = fpbitmap->Bounds().Width() + B_V_SCROLL_BAR_WIDTH;
		height = fpbitmap->Bounds().Height() +
			pbar->Bounds().Height() + B_H_SCROLL_BAR_HEIGHT + 1;
			
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
		
		pwin->Zoom();
			// Perform all of the hard work of resizing the
			// window while taking into account the size of
			// the screen, the tab and borders of the window
			//
			// HACK: Need to fix case where window un-zooms
			// when the window is already the correct size
			// for the current image
		
		// repaint view
		FillRect(Bounds());
		ReDraw();
	} catch (StatusNotOKException) {
		BAlert *palert = new BAlert(NULL,
			"Sorry, unable to load the image.", "OK");
		palert->Go();
	}
}

