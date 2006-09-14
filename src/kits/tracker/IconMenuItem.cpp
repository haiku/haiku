/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

// menu items with small icons.

#include "IconCache.h"
#include "IconMenuItem.h"

#include <Debug.h>
#include <Menu.h>
#include <NodeInfo.h>


ModelMenuItem::ModelMenuItem(const Model *model, const char *title,
		BMessage *message, char shortcut, uint32 modifiers,
		bool drawText, bool extraPad)
	: BMenuItem(title, message, shortcut, modifiers),
	fModel(*model),
	fHeightDelta(0),
	fDrawText(drawText),
	fExtraPad(extraPad)
{
	ThrowOnInitCheckError(&fModel);
	// The 'fExtraPad' field is used to when this menu item is added to
	// a menubar instead of a menu. Menus and MenuBars space out items
	// differently (more space around items in a menu). This class wants
	// to be able to space item the same, no matter where they are. The
	// fExtraPad field allows for that.

	if (model->IsRoot())
		SetLabel(model->Name());

	// ModelMenuItem is used in synchronously invoked menus, make sure
	// we invoke with a timeout
	SetTimeout(kSynchMenuInvokeTimeout);
}


ModelMenuItem::ModelMenuItem(const Model *model, BMenu *menu, bool drawText,
	bool extraPad)
	:	BMenuItem(menu),
		fModel(*model),
		fHeightDelta(0),
		fDrawText(drawText),
		fExtraPad(extraPad)
{
	ThrowOnInitCheckError(&fModel);
	// ModelMenuItem is used in synchronously invoked menus, make sure
	// we invoke with a timeout
	SetTimeout(kSynchMenuInvokeTimeout);
}


ModelMenuItem::~ModelMenuItem()
{
}


status_t
ModelMenuItem::SetEntry(const BEntry *entry)
{
	return fModel.SetTo(entry);
}


void
ModelMenuItem::DrawContent()
{
	if (fDrawText) {
		BPoint drawPoint(ContentLocation());
		drawPoint.x += 20 + (fExtraPad ? 6 : 0);
		if (fHeightDelta > 0)
			drawPoint.y += ceil(fHeightDelta / 2);
		Menu()->MovePenTo(drawPoint);
		_inherited::DrawContent();
	}
	DrawIcon();
}


void
ModelMenuItem::Highlight(bool hilited)
{
	_inherited::Highlight(hilited);
	DrawIcon();
}


static void
DimmedIconBlitter(BView *view, BPoint where, BBitmap *bitmap, void *)
{
	if (bitmap->ColorSpace() == B_RGBA32) {
		rgb_color oldHighColor = view->HighColor();
		view->SetHighColor(0, 0, 0, 128);
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		view->DrawBitmap(bitmap, where);
		view->SetHighColor(oldHighColor);
	} else {
		view->SetDrawingMode(B_OP_BLEND);
		view->DrawBitmap(bitmap, where);
	}
	view->SetDrawingMode(B_OP_OVER);
}


void
ModelMenuItem::DrawIcon()
{
	Menu()->PushState();
	
	BPoint where(ContentLocation());
	// center icon with text.
	
	float deltaHeight = fHeightDelta < 0 ? -fHeightDelta : 0;
	where.y += ceil( deltaHeight/2 );

	if (fExtraPad)
		where.x += 6;

	Menu()->SetDrawingMode(B_OP_OVER);
	Menu()->SetLowColor(B_TRANSPARENT_32_BIT);
	
	// draw small icon, synchronously
	if (IsEnabled())
		IconCache::sIconCache->Draw(fModel.ResolveIfLink(), Menu(), where,
			kNormalIcon, B_MINI_ICON);
	else {
		// dimmed, for now use a special blitter; icon cache should
		// know how to blit one eventually
		IconCache::sIconCache->SyncDraw(fModel.ResolveIfLink(), Menu(), where,
			kNormalIcon, B_MINI_ICON, DimmedIconBlitter);
	}
	
	Menu()->PopState();
}


void
ModelMenuItem::GetContentSize(float *width, float *height)
{
	_inherited::GetContentSize(width, height);
	fHeightDelta = 16 - *height;
	if (*height < 16)
		*height = 16;
	*width = *width + 20 + (fExtraPad ? 18 : 0);
}


status_t 
ModelMenuItem::Invoke(BMessage *message)
{
	if (!Menu())
		return B_ERROR;
	
	if (!IsEnabled())
		return B_ERROR;

	if (!message)
		message = Message();

	if (!message)
		return B_BAD_VALUE;

	BMessage clone(*message);
	clone.AddInt32("index", Menu()->IndexOf(this));
	clone.AddInt64("when", system_time());
	clone.AddPointer("source", this);

	if ((modifiers() & B_OPTION_KEY) == 0) {
		// if option not held, remove refs to close to prevent closing
		// parent window
		clone.RemoveData("nodeRefsToClose");
	}

	return BInvoker::Invoke(&clone);
}


//	#pragma mark -


SpecialModelMenuItem::SpecialModelMenuItem(const Model *model,BMenu *menu)
	: ModelMenuItem(model,menu)
{
}


void 
SpecialModelMenuItem::DrawContent()
{
	Menu()->PushState();

	BFont font;
	Menu()->GetFont(&font);
	font.SetFace(B_ITALIC_FACE);
	Menu()->SetFont(&font);

	_inherited::DrawContent();
	Menu()->PopState();
}


//	#pragma mark -


IconMenuItem::IconMenuItem(const char *label, BMessage *message, BBitmap *icon)
	: PositionPassingMenuItem(label, message),
	fDeviceIcon(icon)
{
	// IconMenuItem is used in synchronously invoked menus, make sure
	// we invoke with a timeout
	SetTimeout(kSynchMenuInvokeTimeout);
}


IconMenuItem::IconMenuItem(const char *label, BMessage *message,
		const BNodeInfo *nodeInfo, icon_size which)
	: PositionPassingMenuItem(label, message),
	fDeviceIcon(NULL)
{
	if (nodeInfo) {
		fDeviceIcon = new BBitmap(BRect(0, 0, which - 1, which - 1), B_CMAP8);
		if (nodeInfo->GetTrackerIcon(fDeviceIcon, B_MINI_ICON)) {
			delete fDeviceIcon;
			fDeviceIcon = NULL;
		}
	}

	// IconMenuItem is used in synchronously invoked menus, make sure
	// we invoke with a timeout
	SetTimeout(kSynchMenuInvokeTimeout);
}


IconMenuItem::IconMenuItem(const char *label, BMessage *message,
		const char *iconType, icon_size which)
	: PositionPassingMenuItem(label, message),
	fDeviceIcon(NULL)
{
	BMimeType mime(iconType);
	fDeviceIcon = new BBitmap(BRect(0, 0, which - 1, which - 1), B_CMAP8);

	if (mime.GetIcon(fDeviceIcon, which) != B_OK) {
		delete fDeviceIcon;
		fDeviceIcon = NULL;
	}

	// IconMenuItem is used in synchronously invoked menus, make sure
	// we invoke with a timeout
	SetTimeout(kSynchMenuInvokeTimeout);
}


IconMenuItem::IconMenuItem(BMenu *submenu, BMessage *message,
		const char *iconType, icon_size which)
	: PositionPassingMenuItem(submenu, message),
	fDeviceIcon(NULL)
{
	BMimeType mime(iconType);
	fDeviceIcon = new BBitmap(BRect(0, 0, which - 1, which - 1), B_CMAP8);

	if (mime.GetIcon(fDeviceIcon, which) != B_OK) {
		delete fDeviceIcon;
		fDeviceIcon = NULL;
	}

	// IconMenuItem is used in synchronously invoked menus, make sure
	// we invoke with a timeout
	SetTimeout(kSynchMenuInvokeTimeout);
}


IconMenuItem::~IconMenuItem()
{
	delete fDeviceIcon;
}


void
IconMenuItem::GetContentSize(float *width, float *height)
{
	_inherited::GetContentSize(width, height);
	*width += 20;
	*height += 3;
}


void
IconMenuItem::DrawContent()
{
	BPoint drawPoint(ContentLocation());
	drawPoint.x += 20;
	Menu()->MovePenTo(drawPoint);
	_inherited::DrawContent();
	
	BPoint where(ContentLocation());
	where.y = Frame().top;
	
	if (fDeviceIcon) {
		if (IsEnabled())
			Menu()->SetDrawingMode(B_OP_OVER);
		else
			Menu()->SetDrawingMode(B_OP_BLEND);	
		
		Menu()->DrawBitmapAsync(fDeviceIcon, where);
	}
}

