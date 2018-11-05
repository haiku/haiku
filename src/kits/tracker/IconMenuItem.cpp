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

//! Menu items with small icons.


#include "IconMenuItem.h"

#include <ControlLook.h>
#include <Debug.h>
#include <Menu.h>
#include <MenuField.h>
#include <NodeInfo.h>

#include "IconCache.h"


static void
DimmedIconBlitter(BView* view, BPoint where, BBitmap* bitmap, void*)
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


//	#pragma mark - ModelMenuItem


ModelMenuItem::ModelMenuItem(const Model* model, const char* title,
	BMessage* message, char shortcut, uint32 modifiers,
	bool drawText, bool extraPad)
	:
	BMenuItem(title, message, shortcut, modifiers),
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


ModelMenuItem::ModelMenuItem(const Model* model, BMenu* menu, bool drawText,
	bool extraPad)
	:
	BMenuItem(menu),
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
ModelMenuItem::SetEntry(const BEntry* entry)
{
	return fModel.SetTo(entry);
}


void
ModelMenuItem::DrawContent()
{
	if (fDrawText) {
		BPoint drawPoint(ContentLocation());
		drawPoint.x += ListIconSize() + ListIconSize() / 4
			+ (fExtraPad ? 6 : 0);
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


void
ModelMenuItem::DrawIcon()
{
	Menu()->PushState();

	BPoint where(ContentLocation());
	// center icon with text.

	float deltaHeight = fHeightDelta < 0 ? -fHeightDelta : 0;
	where.y += ceil(deltaHeight / 2);

	if (fExtraPad)
		where.x += 6;

	Menu()->SetDrawingMode(B_OP_OVER);
	Menu()->SetLowColor(B_TRANSPARENT_32_BIT);

	// draw small icon, synchronously
	if (IsEnabled()) {
		IconCache::sIconCache->Draw(fModel.ResolveIfLink(), Menu(), where,
			kNormalIcon, (icon_size)ListIconSize());
	} else {
		// dimmed, for now use a special blitter; icon cache should
		// know how to blit one eventually
		IconCache::sIconCache->SyncDraw(fModel.ResolveIfLink(), Menu(), where,
			kNormalIcon, (icon_size)ListIconSize(), DimmedIconBlitter);
	}

	Menu()->PopState();
}


void
ModelMenuItem::GetContentSize(float* width, float* height)
{
	_inherited::GetContentSize(width, height);
	float iconSize = ListIconSize();
	fHeightDelta = iconSize - *height;
	if (*height < iconSize)
		*height = iconSize;
	*width = *width + iconSize / 4 + iconSize + (fExtraPad ? 18 : 0);
}


status_t
ModelMenuItem::Invoke(BMessage* message)
{
	if (Menu() == NULL)
		return B_ERROR;

	if (!IsEnabled())
		return B_ERROR;

	if (message == NULL)
		message = Message();

	if (message == NULL)
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


//	#pragma mark - SpecialModelMenuItem


/*!	A ModelMenuItem subclass that draws its label in italics.

	It's used for example in the "Copy To" menu to indicate some special
	folders like the parent folder.
*/
SpecialModelMenuItem::SpecialModelMenuItem(const Model* model, BMenu* menu)
	: ModelMenuItem(model, menu)
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


//	#pragma mark - IconMenuItem


/*!	A menu item that draws an icon alongside the label.

	It's currently used in the mount and new file template menus.
*/
IconMenuItem::IconMenuItem(const char* label, BMessage* message, BBitmap* icon,
	icon_size which)
	:
	PositionPassingMenuItem(label, message),
	fDeviceIcon(NULL),
	fHeightDelta(0),
	fWhich(which)
{
	SetIcon(icon);

	// IconMenuItem is used in synchronously invoked menus, make sure
	// we invoke with a timeout
	SetTimeout(kSynchMenuInvokeTimeout);
}


IconMenuItem::IconMenuItem(const char* label, BMessage* message,
	const BNodeInfo* nodeInfo, icon_size which)
	:
	PositionPassingMenuItem(label, message),
	fDeviceIcon(NULL),
	fHeightDelta(0),
	fWhich(which)
{
	if (nodeInfo != NULL) {
		fDeviceIcon = new BBitmap(BRect(0, 0, which - 1, which - 1),
			kDefaultIconDepth);

		if (nodeInfo->GetTrackerIcon(fDeviceIcon, B_MINI_ICON) != B_OK) {
			delete fDeviceIcon;
			fDeviceIcon = NULL;
		}
	}

	// IconMenuItem is used in synchronously invoked menus, make sure
	// we invoke with a timeout
	SetTimeout(kSynchMenuInvokeTimeout);
}


IconMenuItem::IconMenuItem(const char* label, BMessage* message,
	const char* iconType, icon_size which)
	:
	PositionPassingMenuItem(label, message),
	fDeviceIcon(NULL),
	fHeightDelta(0),
	fWhich(which)
{
	BMimeType mime(iconType);
	fDeviceIcon = new BBitmap(BRect(0, 0, which - 1, which - 1),
		kDefaultIconDepth);

	if (mime.GetIcon(fDeviceIcon, which) != B_OK) {
		BMimeType super;
		mime.GetSupertype(&super);
		if (super.GetIcon(fDeviceIcon, which) != B_OK) {
			delete fDeviceIcon;
			fDeviceIcon = NULL;
		}
	}

	// IconMenuItem is used in synchronously invoked menus, make sure
	// we invoke with a timeout
	SetTimeout(kSynchMenuInvokeTimeout);
}


IconMenuItem::IconMenuItem(BMenu* submenu, BMessage* message,
	const char* iconType, icon_size which)
	:
	PositionPassingMenuItem(submenu, message),
	fDeviceIcon(NULL),
	fHeightDelta(0),
	fWhich(which)
{
	BMimeType mime(iconType);
	fDeviceIcon = new BBitmap(BRect(0, 0, which - 1, which - 1),
		kDefaultIconDepth);

	if (mime.GetIcon(fDeviceIcon, which) != B_OK) {
		BMimeType super;
		mime.GetSupertype(&super);
		if (super.GetIcon(fDeviceIcon, which) != B_OK) {
			delete fDeviceIcon;
			fDeviceIcon = NULL;
		}
	}

	// IconMenuItem is used in synchronously invoked menus, make sure
	// we invoke with a timeout
	SetTimeout(kSynchMenuInvokeTimeout);
}


IconMenuItem::IconMenuItem(BMessage* data)
	:
	PositionPassingMenuItem(data),
	fDeviceIcon(NULL),
	fHeightDelta(0),
	fWhich(B_MINI_ICON)
{
	if (data != NULL) {
		fWhich = (icon_size)data->GetInt32("_which", B_MINI_ICON);

		fDeviceIcon = new BBitmap(BRect(0, 0, fWhich - 1, fWhich - 1),
			kDefaultIconDepth);

		if (data->HasData("_deviceIconBits", B_RAW_TYPE)) {
			ssize_t numBytes;
			const void* bits;
			if (data->FindData("_deviceIconBits", B_RAW_TYPE, &bits, &numBytes)
					== B_OK) {
				fDeviceIcon->SetBits(bits, numBytes, (int32)0,
					kDefaultIconDepth);
			}
		}
	}

	// IconMenuItem is used in synchronously invoked menus, make sure
	// we invoke with a timeout
	SetTimeout(kSynchMenuInvokeTimeout);
}


BArchivable*
IconMenuItem::Instantiate(BMessage* data)
{
	//if (validate_instantiation(data, "IconMenuItem"))
		return new IconMenuItem(data);

	return NULL;
}


status_t
IconMenuItem::Archive(BMessage* data, bool deep) const
{
	status_t result = PositionPassingMenuItem::Archive(data, deep);

	if (result == B_OK)
		result = data->AddInt32("_which", (int32)fWhich);

	if (result == B_OK && fDeviceIcon != NULL) {
		result = data->AddData("_deviceIconBits", B_RAW_TYPE,
			fDeviceIcon->Bits(), fDeviceIcon->BitsLength());
	}

	return result;
}


IconMenuItem::~IconMenuItem()
{
	delete fDeviceIcon;
}


void
IconMenuItem::GetContentSize(float* width, float* height)
{
	_inherited::GetContentSize(width, height);

	fHeightDelta = 16 - *height;
	if (*height < 16)
		*height = 16;

	*width += 20;
}


void
IconMenuItem::DrawContent()
{
	BPoint drawPoint(ContentLocation());
	if (fDeviceIcon != NULL)
		drawPoint.x += (float)fWhich + 4.0f;

	if (fHeightDelta > 0)
		drawPoint.y += ceilf(fHeightDelta / 2);

	Menu()->MovePenTo(drawPoint);
	_inherited::DrawContent();

	Menu()->PushState();

	BPoint where(ContentLocation());
	float deltaHeight = fHeightDelta < 0 ? -fHeightDelta : 0;
	where.y += ceilf(deltaHeight / 2);

	if (fDeviceIcon != NULL) {
		if (IsEnabled())
			Menu()->SetDrawingMode(B_OP_ALPHA);
		else {
			Menu()->SetDrawingMode(B_OP_ALPHA);
			Menu()->SetHighColor(0, 0, 0, 64);
			Menu()->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		}
		Menu()->DrawBitmapAsync(fDeviceIcon, where);
	}

	Menu()->PopState();
}


void
IconMenuItem::SetMarked(bool mark)
{
	_inherited::SetMarked(mark);

	if (!mark)
		return;

	// we are marking the item

	BMenu* menu = Menu();
	if (menu == NULL)
		return;

	// we have a parent menu

	BMenu* _menu = menu;
	while ((_menu = _menu->Supermenu()) != NULL)
		menu = _menu;

	// went up the hierarchy to found the topmost menu

	if (menu == NULL || menu->Parent() == NULL)
		return;

	// our topmost menu has a parent

	if (dynamic_cast<BMenuField*>(menu->Parent()) == NULL)
		return;

	// our topmost menu's parent is a BMenuField

	BMenuItem* topLevelItem = menu->ItemAt((int32)0);

	if (topLevelItem == NULL)
		return;

	// our topmost menu has a menu item

	IconMenuItem* topLevelIconMenuItem
		= dynamic_cast<IconMenuItem*>(topLevelItem);
	if (topLevelIconMenuItem == NULL)
		return;

	// our topmost menu's item is an IconMenuItem

	// update the icon
	topLevelIconMenuItem->SetIcon(fDeviceIcon);
	menu->Invalidate();
}


void
IconMenuItem::SetIcon(BBitmap* icon)
{
	if (icon != NULL) {
		if (fDeviceIcon != NULL)
			delete fDeviceIcon;

		fDeviceIcon = new BBitmap(BRect(0, 0, fWhich - 1, fWhich - 1),
			icon->ColorSpace());
		fDeviceIcon->SetBits(icon->Bits(), icon->BitsLength(), 0,
			icon->ColorSpace());
	} else {
		delete fDeviceIcon;
		fDeviceIcon = NULL;
	}
}
