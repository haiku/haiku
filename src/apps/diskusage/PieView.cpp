/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT/X11 license.
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
#include <Entry.h>
#include <File.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <String.h>
#include <Volume.h>

#include "Common.h"
#include "InfoWindow.h"
#include "MainWindow.h"
#include "Scanner.h"


static const int32 kIdxGetInfo = 0;
static const int32 kIdxOpen = 1;
static const int32 kIdxOpenWith = 2;
static const int32 kIdxRescan = 3;

// TODO: It would be nice to make a common base class for AppMenuItem and
// VolMenuItem (menu items that include an icon).

class AppMenuItem: public BMenuItem {
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


AppMenuItem::AppMenuItem(const char* appSig, int category):
	BMenuItem(kEmptyStr, NULL), fCategory(category), fIcon(NULL), fIsValid(false)
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
	if (_width)
		*_width = fIcon->Bounds().Width() + be_plain_font->StringWidth(Label());

	if (_height) {
		struct font_height fh;
		be_plain_font->GetHeight(&fh);
		float fontHeight = ceilf(fh.ascent) + ceilf(fh.descent)
			+ ceilf(fh.leading);
		*_height = max(fontHeight, fIcon->Bounds().Height());
	}
}


void
AppMenuItem::DrawContent()
{
	Menu()->SetDrawingMode(B_OP_OVER);
	Menu()->MovePenBy(0.0, -1.0);
	Menu()->DrawBitmap(fIcon);
	Menu()->MovePenBy(fIcon->Bounds().Width() + kSmallHMargin, 0.0);
	BMenuItem::DrawContent();
}


// #pragma mark - PieView


PieView::PieView(BRect frame, MainWindow* window)
	: BView(frame, NULL, B_FOLLOW_ALL,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_SUBPIXEL_PRECISE),
	  fWindow(window),
	  fScanners(),
	  fCurrentVolume(NULL),
	  fMouseOverInfo(),
	  fClicked(false),
	  fDragging(false)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(kWindowColor);

	fMouseOverMenu = new BPopUpMenu(kEmptyStr, false, false);
	fMouseOverMenu->AddItem(new BMenuItem(kMenuGetInfo, NULL), kIdxGetInfo);
	fMouseOverMenu->AddItem(new BMenuItem(kMenuOpen, NULL), kIdxOpen);

	fFileUnavailableMenu = new BPopUpMenu(kEmptyStr, false, false);
	BMenuItem* item = new BMenuItem(kStrUnavail, NULL);
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


PieView::~PieView()
{
	delete fMouseOverMenu;
	delete fFileUnavailableMenu;

	while (fScanners.size() != 0) {
		ScannerMap::iterator i = fScanners.begin();
		(*i).second->RequestQuit();
		fScanners.erase(i);
	}
}


void
PieView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
		{
			entry_ref ref;
			for (int i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
				BEntry entry(&ref, true);
				_HandleArg(&entry);
			}
			break;
		}

		case kBtnRescan:
			if (fCurrentVolume != NULL) {
				fScanners[fCurrentVolume]->Refresh();
				Invalidate();
			}
			break;

		case kMenuSelectVol:
		{
			BVolume* volume;
			if (message->FindPointer(kNameVolPtr, (void**)&volume) == B_OK)
				_ShowVolume(volume);
			break;
		}

		case kScanProgress:
		case kScanDone:
		{
			BVolume* volume;
			if (message->FindPointer(kNameVolPtr, (void**)&volume) == B_OK
				&& volume == fCurrentVolume) {
				Invalidate();
			}
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
PieView::MouseDown(BPoint where)
{
	uint32 buttons;
	BMessage* current = Window()->CurrentMessage();
	if (current->FindInt32("buttons", (int32*)&buttons) != B_OK)
		buttons = B_PRIMARY_MOUSE_BUTTON;

	FileInfo* info = _FileAt(where);
	if (info == NULL || info->pseudo)
		return;

	if (buttons & B_PRIMARY_MOUSE_BUTTON) {
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
	// If the primary button was released and there was no dragging happening,
	// just zoom in or out.
	if (fClicked && !fDragging) {
		Scanner* scanner = fScanners[fCurrentVolume];
		FileInfo* info = _FileAt(where);
		if (info != NULL) {
			if (info == scanner->CurrentDir()) {
				scanner->ChangeDir(info->parent);
				Invalidate();
			} else if (info->children.size() > 0) {
				scanner->ChangeDir(info);
				Invalidate();
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
	if (fScanners.find(fCurrentVolume) != fScanners.end()) {
		// There is a current volume.
		Scanner* scanner = fScanners[fCurrentVolume];
		if (scanner->IsBusy()) {
			// Show progress of scanning.
			_DrawProgressBar(updateRect);
			fWindow->SetRescanEnabled(false);
		} else {
			_DrawPieChart(updateRect);
			fWindow->SetRescanEnabled(true);
		}
	} else {
		// No volume has been selected, so display a prompt for one.
		FillRect(updateRect, B_SOLID_LOW);
		BRect b = Bounds();
		float strWidth = StringWidth(kVolPrompt);
		float bx = (b.left + b.Width() - strWidth) / 2.0;
		float by = (b.top + b.Height()) / 2.0;
		SetHighColor(0x00, 0x00, 0x00);
		DrawString(kVolPrompt, BPoint(bx, by));
		fWindow->SetRescanEnabled(false);
	}
}


// #pragma mark - private


void
PieView::_HandleArg(const BEntry* entry)
{
	// Going through these seemingly pointless gyrations (instead of getting
	// the dev_t directly from the BEntry) allows us to process volumes
	// properly.  It looks like a dropped volume refers to dev_t 1 (why?), but
	// getting the dev_t from the path allows us to get at the dev_t where the
	// root directory actually lives.
	BPath path;
	entry->GetPath(&path);
	dev_t device = dev_for_path(path.Path());

	// Set the desired path in the scanner.
	Scanner* scanner = NULL;
	ScannerMap::iterator i = fScanners.begin();
	while (i != fScanners.end()) {
		if ((*i).second->Device() == device) {
			scanner = (*i).second;
			break;
		}
		i++;
	}

	if (scanner == NULL) {
		BVolume* volume = fWindow->FindDeviceFor(device);
		if (volume != NULL)
			scanner = fScanners[volume] = new Scanner(volume, this);
	}
	if (scanner != NULL) {
		string desiredPath(path.Path());
		scanner->SetDesiredPath(desiredPath);

		// Select the volume on the menu.
		fWindow->FindDeviceFor(device, true);
	}
}


void
PieView::_ShowVolume(BVolume* volume)
{
	if (volume != NULL) {
		if (fScanners.find(volume) == fScanners.end())
			fScanners[volume] = new Scanner(volume, this);

		Scanner* scanner = fScanners[volume];
		if (scanner->Snapshot() == NULL)
			scanner->Refresh();
	}

	fCurrentVolume = volume;
	Invalidate();
}


void
PieView::_DrawProgressBar(BRect updateRect)
{
	// Show the progress of the scanning operation.

	fMouseOverInfo.clear();

	FillRect(updateRect, B_SOLID_LOW);

	// Draw the progress bar.
	BRect b = Bounds();
	float bx = floorf((b.left + b.Width() - kProgBarWidth) / 2.0);
	float by = floorf((b.top + b.Height() - kProgBarHeight) / 2.0);
	float ex = bx + kProgBarWidth;
	float ey = by + kProgBarHeight;
	SetPenSize(1.0);
	SetHighColor(tint_color(kWindowColor, B_LIGHTEN_2_TINT));
	StrokeLine(BPoint(bx, ey), BPoint(ex, ey));
	StrokeLine(BPoint(ex, by), BPoint(ex, ey));
	SetHighColor(tint_color(kWindowColor, B_DARKEN_2_TINT));
	StrokeLine(BPoint(bx, by), BPoint(ex, by));
	StrokeLine(BPoint(bx, by), BPoint(bx, ey));

	bx += 1.0; by += 1.0;
	ex -= 1.0; ey -= 1.0;
	float mx = bx + floorf((kProgBarWidth - 2.0)
		* fScanners[fCurrentVolume]->Progress() / 100.0 + 0.5);
	SetHighColor(tint_color(kWindowColor, B_DARKEN_1_TINT));
	FillRect(BRect(mx, by, ex, ey));

	SetHighColor(0x00, 0x20, 0x90);
	StrokeLine(BPoint(bx, ey), BPoint(mx, ey));
	StrokeLine(BPoint(mx, by), BPoint(mx, ey));
	SetHighColor(0x00, 0xc0, 0xff);
	StrokeLine(BPoint(bx, by), BPoint(mx, by));
	StrokeLine(BPoint(bx, by), BPoint(bx, ey));

	bx += 1.0; by += 1.0;
	mx -= 1.0; ey -= 1.0;
	SetHighColor(0x00, 0x80, 0xf0);
	FillRect(BRect(bx, by, mx, ey));

	// Tell what we are doing.
	const char* task = fScanners[fCurrentVolume]->Task();
	float strWidth = StringWidth(task);
	bx = (b.left + b.Width() - strWidth) / 2.0;
	by -= fFontHeight + 2.0 * kSmallVMargin;
	SetHighColor(0, 0, 0);
	DrawString(task, BPoint(bx, by));
}


void
PieView::_DrawPieChart(BRect updateRect)
{
	BRect pieRect = Bounds();
	pieRect.InsetBy(kPieOuterMargin, kPieOuterMargin);
	if (!updateRect.Intersects(pieRect))
		return;

	SetHighColor(kPieBGColor);
	FillRect(updateRect);

	int colorIdx = 0;
	FileInfo* currentDir = fScanners[fCurrentVolume]->CurrentDir();
	FileInfo* parent = currentDir;
	while (parent != NULL) {
		parent = parent->parent;
		colorIdx++;
	}
	_DrawDirectory(currentDir, 0.0, 0.0, colorIdx % kBasePieColorCount, 0);

	// This is just for the case when the mouse hovers over the view
	// while the scanning process is running and then does not move
	// until after the results are shown. In that case the info will
	// not automatically update. (TODO: fix this and put into MessageReceived()
	// kScanDone - currently, fileAt() returns NULL (mouseoverinfo...))
	BPoint where;
	uint32 ignore;
	GetMouse(&where, &ignore, false);
	fWindow->ShowInfo(_FileAt(where));
}


float
PieView::_DrawDirectory(FileInfo* info, float parentSpan, float beginAngle,
	int colorIdx, int level)
{
	BRect b = Bounds();
	if (b.Width() < 2.0 * (kPieCenterSize + level * kPieRingSize
		+ kPieOuterMargin + kPieInnerMargin)) {
		return 0.0;
	}

	VolumeSnapshot* snapshot = fScanners[fCurrentVolume]->Snapshot();

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
			FillArc(BPoint(cx, cy), kPieCenterSize, kPieCenterSize, 0.0, mySpan);

			// Show total volume capacity.
			char label[B_PATH_NAME_LENGTH];
			size_to_string(volCapacity, label);
			SetHighColor(kPieBGColor);
			SetDrawingMode(B_OP_OVER);
			DrawString(label, BPoint(cx - StringWidth(label) / 2.0,
				cy + fFontHeight + kSmallVMargin));
			SetDrawingMode(B_OP_COPY);

			displayName = snapshot->name.c_str();

			// Record in-use space and free space for use during MouseMoved().
			info = snapshot->rootDir;
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
		StrokeEllipse(BPoint(cx, cy), kPieCenterSize + 0.5, kPieCenterSize + 0.5);

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
			rgb_color color = kBasePieColor[colorIdx];
			color.red += kLightenFactor * level;
			color.green += kLightenFactor * level;
			color.blue += kLightenFactor * level;
			SetHighColor(color);
			SetPenSize(kPieRingSize);
			float radius = kPieCenterSize + level * kPieRingSize
				- kPieRingSize / 2.0;
			StrokeArc(BPoint(cx, cy), radius, radius, beginAngle, mySpan);

			SetHighColor(kOutlineColor);
			SetPenSize(0.0);
			float segBeginRadius = kPieCenterSize + (level - 1)
				* kPieRingSize + 0.5;
			float segLength = kPieRingSize - 0.5;
			float rad = deg2rad(beginAngle);
			float bx = cx + segBeginRadius * cos(rad);
			float by = cy - segBeginRadius * sin(rad);
			float ex = bx + segLength * cos(rad);
			float ey = by - segLength * sin(rad);
			StrokeLine(BPoint(bx, by), BPoint(ex, ey));

			rad = deg2rad(beginAngle + mySpan);
			bx = cx + segBeginRadius * cos(rad);
			by = cy - segBeginRadius * sin(rad);
			ex = bx + segLength * cos(rad);
			ey = by - segLength * sin(rad);
			StrokeLine(BPoint(bx, by), BPoint(ex, ey));

			SetPenSize(0.0);
			SetHighColor(kOutlineColor);
			radius += kPieRingSize / 2.0;
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
			= _DrawDirectory(*i, mySpan, beginAngle, colorIdx, level + 1);
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
	float dist = sqrt(dx*dx + dy*dy);

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

	BMenu* openWith = new BMenu(kMenuOpenWith);

	if (appList.size() == 0) {
		BMenuItem* item = new BMenuItem(kMenuNoApps, NULL);
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
			rescan = new BMenuItem(kStrRescan, NULL);
			fMouseOverMenu->AddItem(rescan, kIdxRescan);
		}

		BMenuItem* item = fMouseOverMenu->Go(p, false, true, openRect);
		if (item != NULL) {
			switch (fMouseOverMenu->IndexOf(item)) {
				case kIdxGetInfo:
					new InfoWin(p, info, Window());
					break;
				case kIdxOpen:
					_Launch(info);
					break;
				case kIdxRescan:
					fScanners[fCurrentVolume]->Refresh(info);
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
	}
	else {
		// The file is no longer available.
		fFileUnavailableMenu->Go(p, false, true, openRect);
	}
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


