/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>.
 * Copyright (c) 2009 Philippe Saint-Pierre, stpere@gmail.com
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */


#include "PieView.h"

#include <fs_info.h>
#include <math.h>

#include <AppFileInfo.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Entry.h>
#include <File.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <String.h>
#include <StringForSize.h>
#include <Volume.h>

#include <tracker_private.h>

#include "Commands.h"
#include "DiskUsage.h"
#include "InfoWindow.h"
#include "MainWindow.h"
#include "Scanner.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Pie View"

static const int32 kIdxGetInfo = 0;
static const int32 kIdxOpen = 1;
static const int32 kIdxOpenWith = 2;
static const int32 kIdxRescan = 3;


class AppMenuItem : public BMenuItem {
public:
								AppMenuItem(const char* appSig, int category);
	virtual						~AppMenuItem();

	virtual	void				GetContentSize(float* _width, float* _height);
	virtual	void				DrawContent();

			int					Category() const
									{ return fCategory; }
			const entry_ref*	AppRef() const
									{ return &fAppRef; }
			bool				IsValid() const
									{ return fIsValid; }

private:
			int					fCategory;
			BBitmap*			fIcon;
			entry_ref			fAppRef;
			bool				fIsValid;
};


AppMenuItem::AppMenuItem(const char* appSig, int category)
	:
	BMenuItem(kEmptyStr, NULL),
	fCategory(category),
	fIcon(NULL),
	fIsValid(false)
{
	if (be_roster->FindApp(appSig, &fAppRef) == B_NO_ERROR) {
		fIcon = new BBitmap(BRect(0.0, 0.0, 15.0, 15.0), B_RGBA32);
		if (BNodeInfo::GetTrackerIcon(&fAppRef, fIcon, B_MINI_ICON) == B_OK) {
			BEntry appEntry(&fAppRef);
			if (appEntry.InitCheck() == B_OK) {
				char name[B_FILE_NAME_LENGTH];
				appEntry.GetName(name);
				SetLabel(name);
				fIsValid = true;
			}
		}
	}
}


AppMenuItem::~AppMenuItem()
{
	delete fIcon;
}


void
AppMenuItem::GetContentSize(float* _width, float* _height)
{
	BMenuItem::GetContentSize(_width, _height);
	if (_width)
		*_width += fIcon->Bounds().Width();
	if (_height)
		*_height = max_c(*_height, fIcon->Bounds().Height());
}


void
AppMenuItem::DrawContent()
{
	float yOffset, height;
	GetContentSize(NULL, &height);
	yOffset = (height - fIcon->Bounds().Height()) / 2;
	Menu()->SetDrawingMode(B_OP_OVER);
	Menu()->MovePenBy(0.0, yOffset);
	Menu()->DrawBitmap(fIcon);
	Menu()->MovePenBy(fIcon->Bounds().Width() + kSmallHMargin, -yOffset);
	BMenuItem::DrawContent();
}


// #pragma mark - PieView


PieView::PieView(BVolume* volume)
	:
	BView(NULL, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_SUBPIXEL_PRECISE),
	fWindow(NULL),
	fScanner(NULL),
	fVolume(volume),
	fMouseOverInfo(),
	fClicked(false),
	fDragging(false),
	fUpdateFileAt(false)
{
	fMouseOverMenu = new BPopUpMenu(kEmptyStr, false, false);
	fMouseOverMenu->AddItem(new BMenuItem(B_TRANSLATE("Get info"), NULL),
		kIdxGetInfo);
	fMouseOverMenu->AddItem(new BMenuItem(B_TRANSLATE("Open"), NULL),
		kIdxOpen);

	fFileUnavailableMenu = new BPopUpMenu(kEmptyStr, false, false);
	BMenuItem* item = new BMenuItem(B_TRANSLATE("file unavailable"), NULL);
	item->SetEnabled(false);
	fFileUnavailableMenu->AddItem(item);

	BFont font;
	GetFont(&font);
	font.SetSize(ceilf(font.Size() * 1.33));
	font.SetFace(B_BOLD_FACE);
	SetFont(&font);

	struct font_height fh;
	font.GetHeight(&fh);
	fFontHeight = ceilf(fh.ascent) + ceilf(fh.descent) + ceilf(fh.leading);
}


void
PieView::AttachedToWindow()
{
	fWindow = (MainWindow*)Window();
	if (Parent()) {
		SetViewColor(Parent()->ViewColor());
		SetLowColor(Parent()->ViewColor());
	}
}


PieView::~PieView()
{
	delete fMouseOverMenu;
	delete fFileUnavailableMenu;
	if (fScanner != NULL)
		fScanner->RequestQuit();
}


void
PieView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kBtnCancel:
			if (fScanner != NULL)
				fScanner->Cancel();
			break;

		case kBtnRescan:
			if (fVolume != NULL) {
				if (fScanner != NULL)
					fScanner->Refresh();
				else
					_ShowVolume(fVolume);
				fWindow->EnableCancel();
				Invalidate();
			}
			break;

		case kScanDone:
			fWindow->EnableRescan();
			// fall-through
		case kScanProgress:
			Invalidate();
			break;

		default:
			BView::MessageReceived(message);
	}
}


void
PieView::MouseDown(BPoint where)
{
	BMessage* current = Window()->CurrentMessage();

	uint32 buttons;
	if (current->FindInt32("buttons", (int32*)&buttons) != B_OK)
		buttons = B_PRIMARY_MOUSE_BUTTON;

	FileInfo* info = _FileAt(where);
	if (info == NULL || info->pseudo)
		return;

	if ((buttons & B_PRIMARY_MOUSE_BUTTON) != 0) {
		fClicked = true;
		fDragStart = where;
		fClickedFile = info;
		SetMouseEventMask(B_POINTER_EVENTS);
	} else if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		where = ConvertToScreen(where);
		_ShowContextMenu(info, where);
	}
}


void
PieView::MouseUp(BPoint where)
{
	if (fClicked && !fDragging) {
		// The primary mouse button was released and there's no dragging happening.
		FileInfo* info = _FileAt(where);
		if (info != NULL) {
			BMessage* current = Window()->CurrentMessage();

			uint32 modifiers;
			if (current->FindInt32("modifiers", (int32*)&modifiers) != B_OK)
				modifiers = 0;

			if ((modifiers & B_COMMAND_KEY) != 0) {
				// launch the app on command-click
				_Launch(info);
			} else {
				// zoom in or out
				if (info == fScanner->CurrentDir()) {
					fScanner->ChangeDir(info->parent);
					fLastWhere = where;
					fUpdateFileAt = true;
					Invalidate();
				} else if (info->children.size() > 0) {
					fScanner->ChangeDir(info);
					fLastWhere = where;
					fUpdateFileAt = true;
					Invalidate();
				}
			}
		}
	}

	fClicked = false;
	fDragging = false;
}


void
PieView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if (fClicked) {
		// Primary mouse button is down.
		if (fDragging)
			return;
		// If the mouse has moved far enough, initiate dragging.
		BPoint diff = where - fDragStart;
		float distance = sqrtf(diff.x * diff.x + diff.y * diff.x);
		if (distance > kDragThreshold) {
			fDragging = true;

			BBitmap* icon = new BBitmap(BRect(0.0, 0.0, 31.0, 31.0), B_RGBA32);
			if (BNodeInfo::GetTrackerIcon(&fClickedFile->ref, icon,
					B_LARGE_ICON) == B_OK) {
				BMessage msg(B_SIMPLE_DATA);
				msg.AddRef("refs", &fClickedFile->ref);
				DragMessage(&msg, icon, B_OP_BLEND, BPoint(15.0, 15.0));
			} else
				delete icon;
		}
	} else {
		// Mouse button is not down, display file info.
		if (transit == B_EXITED_VIEW) {
			// Clear status view
			fWindow->ShowInfo(NULL);
		} else {
			// Display file information.
			fWindow->ShowInfo(_FileAt(where));
		}
	}
}


void
PieView::Draw(BRect updateRect)
{
	if (fScanner != NULL) {
		// There is a current volume.
		if (fScanner->IsBusy()) {
			// Show progress of scanning.
			_DrawProgressBar(updateRect);
		} else if (fScanner->Snapshot() != NULL) {
			_DrawPieChart(updateRect);
			if (fUpdateFileAt) {
				fWindow->ShowInfo(_FileAt(fLastWhere));
				fUpdateFileAt = false;
			}
		}
	}
}


void
PieView::SetPath(BPath path)
{
	if (fScanner == NULL)
		_ShowVolume(fVolume);

	if (fScanner != NULL) {
		string desiredPath(path.Path());
		fScanner->SetDesiredPath(desiredPath);
		Invalidate();
	}
}


// #pragma mark - private


void
PieView::_ShowVolume(BVolume* volume)
{
	if (volume != NULL) {
		if (fScanner == NULL)
			fScanner = new Scanner(volume, this);

		if (fScanner->Snapshot() == NULL)
			fScanner->Refresh();
	}

	Invalidate();
}


void
PieView::_DrawProgressBar(BRect updateRect)
{
	// Show the progress of the scanning operation.

	fMouseOverInfo.clear();

	// Draw the progress bar.
	BRect b = Bounds();
	float bx = floorf((b.left + b.Width() - kProgBarWidth) / 2.0);
	float by = floorf((b.top + b.Height() - kProgBarHeight) / 2.0);
	float ex = bx + kProgBarWidth;
	float ey = by + kProgBarHeight;
	float mx = bx + floorf((kProgBarWidth - 2.0) * fScanner->Progress() + 0.5);

	const rgb_color kBarColor = {50, 150, 255, 255};
	BRect barFrame(bx, by, ex, ey);
	be_control_look->DrawStatusBar(this, barFrame, updateRect,
		ui_color(B_PANEL_BACKGROUND_COLOR), kBarColor, mx);

	// Tell what we are doing.
	const char* task = fScanner->Task();
	float strWidth = StringWidth(task);
	bx = (b.left + b.Width() - strWidth) / 2.0;
	by -= fFontHeight + 2.0 * kSmallVMargin;
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
	DrawString(task, BPoint(bx, by));
}


void
PieView::_DrawPieChart(BRect updateRect)
{
	BRect pieRect = Bounds();
	if (!updateRect.Intersects(pieRect))
		return;

	pieRect.InsetBy(kPieOuterMargin, kPieOuterMargin);

	// constraint proportions
	if (pieRect.Width() > pieRect.Height()) {
		float moveBy = (pieRect.Width() - pieRect.Height()) / 2;
		pieRect.left += moveBy;
		pieRect.right -= moveBy;
	} else {
		float moveBy = (pieRect.Height() - pieRect.Width()) / 2;
		pieRect.top -= moveBy;
		pieRect.bottom += moveBy;
	}
	int colorIdx = 0;
	FileInfo* currentDir = fScanner->CurrentDir();
	FileInfo* parent = currentDir;
	while (parent != NULL) {
		parent = parent->parent;
		colorIdx++;
	}
	_DrawDirectory(pieRect, currentDir, 0.0, 0.0,
		colorIdx % kBasePieColorCount, 0);
}


float
PieView::_DrawDirectory(BRect b, FileInfo* info, float parentSpan,
	float beginAngle, int colorIdx, int level)
{
	if (b.Width() < 2.0 * (kPieCenterSize + level * kPieRingSize
		+ kPieOuterMargin + kPieInnerMargin)) {
		return 0.0;
	}

	if (info != NULL && info->color >= 0 && level == 0)
		colorIdx = info->color % kBasePieColorCount;
	else if (info != NULL)
		info->color = colorIdx;

	VolumeSnapshot* snapshot = fScanner->Snapshot();

	float cx = floorf(b.left + b.Width() / 2.0 + 0.5);
	float cy = floorf(b.top + b.Height() / 2.0 + 0.5);

	float mySpan;

	if (level == 0) {
		// Make room for mouse over info.
		fMouseOverInfo.clear();
		fMouseOverInfo[0] = SegmentList();

		// Draw the center circle.
		const char* displayName;
		if (info == NULL) {
			// NULL represents the entire volume.  Show used and free space in
			// the center circle, with the used segment representing the
			// volume's root directory.
			off_t volCapacity = snapshot->capacity;
			mySpan = 360.0 * (volCapacity - snapshot->freeBytes) / volCapacity;

			SetHighColor(kEmptySpcColor);
			FillEllipse(BPoint(cx, cy), kPieCenterSize, kPieCenterSize);

			SetHighColor(kBasePieColor[0]);
			FillArc(BPoint(cx, cy), kPieCenterSize, kPieCenterSize, 0.0,
				mySpan);

			// Show total volume capacity.
			char label[B_PATH_NAME_LENGTH];
			string_for_size(volCapacity, label, sizeof(label));
			SetHighColor(kPieBGColor);
			SetDrawingMode(B_OP_OVER);
			DrawString(label, BPoint(cx - StringWidth(label) / 2.0,
				cy + fFontHeight + kSmallVMargin));
			SetDrawingMode(B_OP_COPY);

			displayName = snapshot->name.c_str();

			// Record in-use space and free space for use during MouseMoved().
			info = snapshot->rootDir;
			info->color = colorIdx;
			fMouseOverInfo[0].push_back(Segment(0.0, mySpan, info));
			if (mySpan < 360.0 - kMinSegmentSpan) {
				fMouseOverInfo[0].push_back(Segment(mySpan, 360.0,
					snapshot->freeSpace));
			}
		} else {
			// Show a normal directory.
			SetHighColor(kBasePieColor[colorIdx]);
			FillEllipse(BRect(cx - kPieCenterSize, cy - kPieCenterSize,
				cx + kPieCenterSize + 0.5, cy + kPieCenterSize + 0.5));
			displayName = info->ref.name;
			mySpan = 360.0;

			// Record the segment for use during MouseMoved().
			fMouseOverInfo[0].push_back(Segment(0.0, mySpan, info));
		}

		SetPenSize(1.0);
		SetHighColor(kOutlineColor);
		StrokeEllipse(BPoint(cx, cy), kPieCenterSize + 0.5,
			kPieCenterSize + 0.5);

		// Show the name of the volume or directory.
		BString label(displayName);
		BFont font;
		GetFont(&font);
		font.TruncateString(&label, B_TRUNCATE_END,
			2.0 * (kPieCenterSize - kSmallHMargin));
		float labelWidth = font.StringWidth(label.String());

		SetHighColor(kPieBGColor);
		SetDrawingMode(B_OP_OVER);
		DrawString(label.String(), BPoint(cx - labelWidth / 2.0, cy));
		SetDrawingMode(B_OP_COPY);
		beginAngle = 0.0;
	} else {
		// Draw an exterior segment.
		float parentSize;
		if (info->parent == NULL)
			parentSize = (float)snapshot->capacity;
		else
			parentSize = (float)info->parent->size;

		mySpan = parentSpan * (float)info->size / parentSize;
		if (mySpan >= kMinSegmentSpan) {
			const float tint = 1.4f - level * 0.08f;
			float radius = kPieCenterSize + level * kPieRingSize
				- kPieRingSize / 2.0;

			// Draw the grey border
			SetHighColor(tint_color(kOutlineColor, tint));
			SetPenSize(kPieRingSize + 1.5f);
			StrokeArc(BPoint(cx, cy), radius, radius,
				beginAngle - 0.001f * radius, mySpan  + 0.002f * radius);

			// Draw the colored area
			rgb_color color = tint_color(kBasePieColor[colorIdx], tint);
			SetHighColor(color);
			SetPenSize(kPieRingSize);
			StrokeArc(BPoint(cx, cy), radius, radius, beginAngle, mySpan);

			// Record the segment for use during MouseMoved().
			if (fMouseOverInfo.find(level) == fMouseOverInfo.end())
				fMouseOverInfo[level] = SegmentList();

			fMouseOverInfo[level].push_back(
				Segment(beginAngle, beginAngle + mySpan, info));
		}
	}

	// Draw children.
	vector<FileInfo*>::iterator i = info->children.begin();
	while (i != info->children.end()) {
		float childSpan
			= _DrawDirectory(b, *i, mySpan, beginAngle, colorIdx, level + 1);
		if (childSpan >= kMinSegmentSpan) {
			beginAngle += childSpan;
			colorIdx = (colorIdx + 1) % kBasePieColorCount;
		}
		i++;
	}

	return mySpan;
}


FileInfo*
PieView::_FileAt(const BPoint& where)
{
	BRect b = Bounds();
	float cx = b.left + b.Width() / 2.0;
	float cy = b.top + b.Height() / 2.0;
	float dx = where.x - cx;
	float dy = where.y - cy;
	float dist = sqrt(dx * dx + dy * dy);

	int level;
	if (dist < kPieCenterSize)
		level = 0;
	else
		level = 1 + (int)((dist - kPieCenterSize) / kPieRingSize);

	float angle = rad2deg(atan(dy / dx));
	angle = ((dx < 0.0) ? 180.0 : (dy < 0.0) ? 0.0 : 360.0) - angle;

	if (fMouseOverInfo.find(level) == fMouseOverInfo.end()) {
		// No files in this level (ring) of the pie.
		return NULL;
	}

	SegmentList s = fMouseOverInfo[level];
	SegmentList::iterator i = s.begin();
	while (i != s.end() && (angle < (*i).begin || (*i).end < angle))
		i++;
	if (i == s.end()) {
		// Nothing at this angle.
		return NULL;
	}

	return (*i).info;
}


void
PieView::_AddAppToList(vector<AppMenuItem*>& list, const char* appSig,
	int category)
{
	// skip self.
	if (strcmp(appSig, kAppSignature) == 0)
		return;

	AppMenuItem* item = new AppMenuItem(appSig, category);
	if (item->IsValid()) {
		vector<AppMenuItem*>::iterator i = list.begin();
		while (i != list.end()) {
			if (*item->AppRef() == *(*i)->AppRef()) {
				// Skip duplicates.
				delete item;
				return;
			}
			i++;
		}
		list.push_back(item);
	} else {
		// Skip items that weren't constructed successfully.
		delete item;
	}
}


BMenu*
PieView::_BuildOpenWithMenu(FileInfo* info)
{
	vector<AppMenuItem*> appList;

	// Get preferred app.
	BMimeType* type = info->Type();
	char appSignature[B_MIME_TYPE_LENGTH];
	if (type->GetPreferredApp(appSignature) == B_OK)
		_AddAppToList(appList, appSignature, 1);

	// Get apps that handle this subtype and supertype.
	BMessage msg;
	if (type->GetSupportingApps(&msg) == B_OK) {
		int32 subs, supers, i;
		msg.FindInt32("be:sub", &subs);
		msg.FindInt32("be:super", &supers);

		const char* appSig;
		for (i = 0; i < subs; i++) {
			msg.FindString("applications", i, &appSig);
			_AddAppToList(appList, appSig, 2);
		}
		int hold = i;
		for (i = 0; i < supers; i++) {
			msg.FindString("applications", i + hold, &appSig);
			_AddAppToList(appList, appSig, 3);
		}
	}

	// Get apps that handle any type.
	if (BMimeType::GetWildcardApps(&msg) == B_OK) {
		const char* appSig;
		for (int32 i = 0; true; i++) {
			if (msg.FindString("applications", i, &appSig) == B_OK)
				_AddAppToList(appList, appSig, 4);
			else
				break;
		}
	}

	delete type;

	BMenu* openWith = new BMenu(B_TRANSLATE("Open with"));

	if (appList.size() == 0) {
		BMenuItem* item = new BMenuItem(B_TRANSLATE("no supporting apps"),
			NULL);
		item->SetEnabled(false);
		openWith->AddItem(item);
	} else {
		vector<AppMenuItem*>::iterator i = appList.begin();
		int category = (*i)->Category();
		while (i != appList.end()) {
			if (category != (*i)->Category()) {
				openWith->AddSeparatorItem();
				category = (*i)->Category();
			}
			openWith->AddItem(*i);
			i++;
		}
	}

	return openWith;
}


void
PieView::_ShowContextMenu(FileInfo* info, BPoint p)
{
	BRect openRect(p.x - 5.0, p.y - 5.0, p.x + 5.0, p.y + 5.0);

	// Display the open-with menu only if the file is still available.
	BNode node(&info->ref);
	if (node.InitCheck() == B_OK) {
		// Add "Open With" submenu.
		BMenu* openWith = _BuildOpenWithMenu(info);
		fMouseOverMenu->AddItem(openWith, kIdxOpenWith);

		// Add a "Rescan" option for folders.
		BMenuItem* rescan = NULL;
		if (info->children.size() > 0) {
			rescan = new BMenuItem(B_TRANSLATE("Rescan"), NULL);
			fMouseOverMenu->AddItem(rescan, kIdxRescan);
		}

		BMenuItem* item = fMouseOverMenu->Go(p, false, true, openRect);
		if (item != NULL) {
			switch (fMouseOverMenu->IndexOf(item)) {
				case kIdxGetInfo:
					_OpenInfo(info, p);
					break;
				case kIdxOpen:
					_Launch(info);
					break;
				case kIdxRescan:
					fScanner->Refresh(info);
					fWindow->EnableCancel();
					Invalidate();
					break;
				default: // must be "Open With" submenu
					_Launch(info, ((AppMenuItem*)item)->AppRef());
					break;
			}
		}

		if (rescan != NULL) {
			fMouseOverMenu->RemoveItem(rescan);
			delete rescan;
		}

		fMouseOverMenu->RemoveItem(openWith);
		delete openWith;
	} else
		// The file is no longer available.
		fFileUnavailableMenu->Go(p, false, true, openRect);
}


void
PieView::_Launch(FileInfo* info, const entry_ref* appRef)
{
	BMessage msg(B_REFS_RECEIVED);
	msg.AddRef("refs", &info->ref);

	if (appRef == NULL) {
		// Let the registrar pick an app based on the file's MIME type.
		BMimeType* type = info->Type();
		be_roster->Launch(type->Type(), &msg);
		delete type;
	} else {
		// Launch a designated app to handle this file.
		be_roster->Launch(appRef, &msg);
	}
}


void
PieView::_OpenInfo(FileInfo* info, BPoint p)
{
	BMessenger tracker(kTrackerSignature);
	if (!tracker.IsValid()) {
		new InfoWin(p, info, Window());
	} else {
		BMessage message(kGetInfo);
		message.AddRef("refs", &info->ref);
		tracker.SendMessage(&message);
	}
}
