/*
 * Copyright 2007-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "SudokuView.h"

#include "Sudoku.h"
#include "SudokuField.h"
#include "SudokuSolver.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Clipboard.h>
#include <DataIO.h>
#include <Dragger.h>
#include <File.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Picture.h>
#include <String.h>


const uint32 kMsgCheckSolved = 'chks';

const uint32 kStrongLineSize = 2;

extern const char* kSignature;


SudokuView::SudokuView(BRect frame, const char* name,
		const BMessage& settings, uint32 resizingMode)
	: BView(frame, name, resizingMode,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS)
{
	InitObject(&settings);

#if 0
	BRect rect(Bounds());
	rect.top = rect.bottom - 7;
	rect.left = rect.right - 7;
	BDragger* dragger = new BDragger(rect, this);
	AddChild(dragger);
#endif
}


SudokuView::SudokuView(BMessage* archive)
	: BView(archive)
{
	InitObject(archive);
}


SudokuView::~SudokuView()
{
	delete fField;
}


status_t
SudokuView::Archive(BMessage* into, bool deep) const
{
	status_t status;

	status = BView::Archive(into, deep);
	if (status < B_OK)
		return status;

	status = into->AddString("add_on", kSignature);
	if (status < B_OK)
		return status;

	status = into->AddRect("bounds", Bounds());
	if (status < B_OK)
		return status;

	status = SaveState(*into);
	if (status < B_OK)
		return status;
	return B_OK;
}


BArchivable*
SudokuView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "SudokuView"))
		return NULL;
	return new SudokuView(archive);
}


void
SudokuView::InitObject(const BMessage* archive)
{
	fField = NULL;
	fShowHintX = ~0UL;
	fLastHintValue = ~0UL;
	fLastField = ~0UL;
	fKeyboardX = 0;
	fKeyboardY = 0;
	fShowKeyboardFocus = false;
	fEditable = true;

	BMessage field;
	if (archive->FindMessage("field", &field) == B_OK) {
		fField = new SudokuField(&field);
		if (fField->InitCheck() != B_OK) {
			delete fField;
			fField = NULL;
		} else if (fField->IsSolved())
			ClearAll();
	}
	if (fField == NULL)
		fField = new SudokuField(3);

	fBlockSize = fField->BlockSize();

	if (archive->FindInt32("hint flags", (int32*)&fHintFlags) != B_OK)
		fHintFlags = kMarkInvalid;
	if (archive->FindBool("show cursor", &fShowCursor) != B_OK)
		fShowCursor = false;

	SetViewColor(B_TRANSPARENT_COLOR);
		// to avoid flickering
	rgb_color color = { 255, 255, 240 };
	fBackgroundColor = color;
	SetLowColor(color);
	FrameResized(0, 0);
}


status_t
SudokuView::SaveState(BMessage& state) const
{
	BMessage field;
	status_t status = fField->Archive(&field, true);
	if (status == B_OK)
		status = state.AddMessage("field", &field);
	if (status == B_OK)
		status = state.AddInt32("hint flags", fHintFlags);
	if (status == B_OK)
		status = state.AddBool("show cursor", fShowCursor);

	return status;
}


status_t
SudokuView::_FilterString(const char* data, size_t dataLength, char* buffer,
	uint32& out, bool& ignore)
{
	uint32 maxOut = fField->Size() * fField->Size();

	for (uint32 i = 0; data[i] && i < dataLength; i++) {
		if (data[i] == '#')
			ignore = true;
		else if (data[i] == '\n')
			ignore = false;

		if (ignore || isspace(data[i]))
			continue;

		if (!_ValidCharacter(data[i])) {
			return B_BAD_VALUE;
		}

		buffer[out++] = data[i];
		if (out == maxOut)
			break;
	}

	buffer[out] = '\0';
	return B_OK;
}


status_t
SudokuView::SetTo(entry_ref& ref)
{
	BPath path;
	status_t status = path.SetTo(&ref);
	if (status < B_OK)
		return status;

	FILE* file = fopen(path.Path(), "r");
	if (file == NULL)
		return errno;

	uint32 maxOut = fField->Size() * fField->Size();
	char buffer[1024];
	char line[1024];
	bool ignore = false;
	uint32 out = 0;

	while (fgets(line, sizeof(line), file) != NULL
		&& out < maxOut) {
		status = _FilterString(line, sizeof(line), buffer, out, ignore);
		if (status < B_OK) {
			fclose(file);
			return status;
		}
	}

	_PushUndo();
	status = fField->SetTo(_BaseCharacter(), buffer);
	Invalidate();
	fclose(file);
	return status;
}


status_t
SudokuView::SetTo(const char* data)
{
	if (data == NULL)
		return B_BAD_VALUE;

	char buffer[1024];
	bool ignore = false;
	uint32 out = 0;

	status_t status = _FilterString(data, 65536, buffer, out, ignore);
	if (status < B_OK)
		return B_BAD_VALUE;

	_PushUndo();
	status = fField->SetTo(_BaseCharacter(), buffer);
	Invalidate();
	return status;
}


status_t
SudokuView::SetTo(SudokuField* field)
{
	if (field == NULL || field == fField)
		return B_BAD_VALUE;

	_PushUndo();
	delete fField;
	fField = field;

	fBlockSize = fField->BlockSize();
	FrameResized(0, 0);
	Invalidate();
	return B_OK;
}


status_t
SudokuView::SaveTo(entry_ref& ref, uint32 exportAs)
{
	BFile file;
	status_t status = file.SetTo(&ref, B_WRITE_ONLY | B_CREATE_FILE
		| B_ERASE_FILE);
	if (status < B_OK)
		return status;

	return SaveTo(file, exportAs);
}


status_t
SudokuView::SaveTo(BDataIO& stream, uint32 exportAs)
{
	BFile* file = dynamic_cast<BFile*>(&stream);
	uint32 i = 0;
	BNodeInfo nodeInfo;

	if (file)
		nodeInfo.SetTo(file);

	switch (exportAs) {
		case kExportAsText:
		{
			BString text = "# Written by Sudoku\n\n";
			stream.Write(text.String(), text.Length());

			char* line = text.LockBuffer(1024);
			memset(line, 0, 1024);
			for (uint32 y = 0; y < fField->Size(); y++) {
				for (uint32 x = 0; x < fField->Size(); x++) {
					if (x != 0 && x % fBlockSize == 0)
						line[i++] = ' ';
					_SetText(&line[i++], fField->ValueAt(x, y));
				}
				line[i++] = '\n';
			}
			text.UnlockBuffer();

			stream.Write(text.String(), text.Length());
			if (file)
				nodeInfo.SetType("text/plain");
			return B_OK;
		}

		case kExportAsHTML:
		{
			bool netPositiveFriendly = false;
			BString text = "<html>\n<head>\n<!-- Written by Sudoku -->\n"
				"<style type=\"text/css\">\n"
				"table.sudoku { background: #000000; border:0; border-collapse: "
					"collapse; cellpadding: 10px; cellspacing: 10px; width: "
					"300px; height: 300px; }\n"
				"td.sudoku { background: #ffffff; border-color: black; "
					"border-left: none ; border-top: none ; /*border: none;*/ "
					"text-align: center; }\n"
				"td.sudoku_initial {  }\n"
				"td.sudoku_filled { color: blue; }\n"
				"td.sudoku_empty {  }\n";

			// border styles: right bottom (none, small or large)
			const char* kStyles[] = {"none", "small", "large"};
			const char* kCssStyles[] = {"none", "solid 1px black",
				"solid 3px black"};
			enum style_type { kNone = 0, kSmall, kLarge };

			for (int32 right = 0; right < 3; right++) {
				for (int32 bottom = 0; bottom < 3; bottom++) {
					text << "td.sudoku_";
					if (right != bottom)
						text << kStyles[right] << "_" << kStyles[bottom];
					else
						text << kStyles[right];

					text << " { border-right: " << kCssStyles[right]
						<< "; border-bottom: " << kCssStyles[bottom] << "; }\n";
				}
			}

			text << "</style>\n"
				"</head>\n<body>\n\n";
			stream.Write(text.String(), text.Length());

			text = "<table";
			if (netPositiveFriendly)
				text << " border=\"1\"";
			text << " class=\"sudoku\">";
			stream.Write(text.String(), text.Length());

			text = "";
			BString divider;
			divider << (int)(100.0 / fField->Size()) << "%";
			for (uint32 y = 0; y < fField->Size(); y++) {
				text << "<tr height=\"" << divider << "\">\n";
				for (uint32 x = 0; x < fField->Size(); x++) {
					char buff[2];
					_SetText(buff, fField->ValueAt(x, y));

					BString style = "sudoku_";
					style_type right = kSmall;
					style_type bottom = kSmall;
					if ((x + 1) % fField->BlockSize() == 0)
						right = kLarge;
					if ((y + 1) % fField->BlockSize() == 0)
						bottom = kLarge;
					if (x == fField->Size() - 1)
						right = kNone;
					if (y == fField->Size() - 1)
						bottom = kNone;

					if (right != bottom)
						style << kStyles[right] << "_" << kStyles[bottom];
					else
						style << kStyles[right];

					if (fField->ValueAt(x, y) == 0) {
						text << "<td width=\"" << divider << "\" ";
						text << "class=\"sudoku sudoku_empty " << style;
						text << "\">\n&nbsp;";
					} else if (fField->FlagsAt(x, y) & kInitialValue) {
						text << "<td width=\"" << divider << "\" ";
						text << "class=\"sudoku sudoku_initial " << style
							<< "\">\n";
						if (netPositiveFriendly)
							text << "<font color=\"#000000\">";
						text << buff;
						if (netPositiveFriendly)
							text << "</font>";
					} else {
						text << "<td width=\"" << divider << "\" ";
						text << "class=\"sudoku sudoku_filled sudoku_" << style
							<< "\">\n";
						if (netPositiveFriendly)
							text << "<font color=\"#0000ff\">";
						text << buff;
						if (netPositiveFriendly)
							text << "</font>";
					}
					text << "</td>\n";
				}
				text << "</tr>\n";
			}
			text << "</table>\n\n";

			stream.Write(text.String(), text.Length());
			text = "</body></html>\n";
			stream.Write(text.String(), text.Length());
			if (file)
				nodeInfo.SetType("text/html");
			return B_OK;
		}

		case kExportAsBitmap:
		{
			BMallocIO mallocIO;
			status_t status = SaveTo(mallocIO, kExportAsPicture);
			if (status < B_OK)
				return status;

			mallocIO.Seek(0LL, SEEK_SET);
			BPicture picture;
			status = picture.Unflatten(&mallocIO);
			if (status < B_OK)
				return status;

			BBitmap* bitmap = new BBitmap(Bounds(), B_BITMAP_ACCEPTS_VIEWS,
				B_RGB32);
			BView* view = new BView(Bounds(), "bitmap", B_FOLLOW_NONE,
				B_WILL_DRAW);
			bitmap->AddChild(view);

			if (bitmap->Lock()) {
				view->DrawPicture(&picture);
				view->Sync();

				view->RemoveSelf();
				delete view;
					// it should not become part of the archive
				bitmap->Unlock();
			}

			BMessage archive;
			status = bitmap->Archive(&archive);
			if (status >= B_OK)
				status = archive.Flatten(&stream);

			delete bitmap;
			return status;
		}

		case kExportAsPicture:
		{
			BPicture picture;
			BeginPicture(&picture);
			Draw(Bounds());

			status_t status = B_ERROR;
			if (EndPicture())
				status = picture.Flatten(&stream);

			return status;
		}

		default:
			return B_BAD_VALUE;
	}
}


status_t
SudokuView::CopyToClipboard()
{
	if (!be_clipboard->Lock())
		return B_ERROR;

	be_clipboard->Clear();

	BMessage* clip = be_clipboard->Data();
	if (clip == NULL) {
		be_clipboard->Unlock();
		return B_ERROR;
	}

	// As BBitmap
	BMallocIO mallocIO;
	status_t status = SaveTo(mallocIO, kExportAsBitmap);
	if (status >= B_OK) {
		mallocIO.Seek(0LL, SEEK_SET);
		// ShowImage, ArtPaint & WonderBrush use that
		status = clip->AddData("image/bitmap", B_MESSAGE_TYPE,
			mallocIO.Buffer(), mallocIO.BufferLength());
		// Becasso uses that ?
		clip->AddData("image/x-be-bitmap", B_MESSAGE_TYPE, mallocIO.Buffer(),
			mallocIO.BufferLength());
		// Gobe Productive uses that...
		// QuickRes as well, with a rect field.
		clip->AddData("image/x-vnd.Be-bitmap", B_MESSAGE_TYPE,
			mallocIO.Buffer(), mallocIO.BufferLength());
	}
	mallocIO.Seek(0LL, SEEK_SET);
	mallocIO.SetSize(0LL);

	// As HTML
	if (status >= B_OK)
		status = SaveTo(mallocIO, kExportAsHTML);
	if (status >= B_OK) {
		status = clip->AddData("text/html", B_MIME_TYPE, mallocIO.Buffer(),
			mallocIO.BufferLength());
	}
	mallocIO.Seek(0LL, SEEK_SET);
	mallocIO.SetSize(0LL);

	// As plain text
	if (status >= B_OK)
		SaveTo(mallocIO, kExportAsText);
	if (status >= B_OK) {
		status = clip->AddData("text/plain", B_MIME_TYPE, mallocIO.Buffer(),
			mallocIO.BufferLength());
	}
	mallocIO.Seek(0LL, SEEK_SET);
	mallocIO.SetSize(0LL);

	// As flattened BPicture, anyone handles that?
	if (status >= B_OK)
		status = SaveTo(mallocIO, kExportAsPicture);
	if (status >= B_OK) {
		status = clip->AddData("image/x-vnd.Be-picture", B_MIME_TYPE,
			mallocIO.Buffer(), mallocIO.BufferLength());
	}
	mallocIO.SetSize(0LL);

	be_clipboard->Commit();
	be_clipboard->Unlock();

	return status;
}


void
SudokuView::ClearChanged()
{
	_PushUndo();

	for (uint32 y = 0; y < fField->Size(); y++) {
		for (uint32 x = 0; x < fField->Size(); x++) {
			if ((fField->FlagsAt(x, y) & kInitialValue) == 0)
				fField->SetValueAt(x, y, 0);
		}
	}

	Invalidate();
}


void
SudokuView::ClearAll()
{
	_PushUndo();
	fField->Reset();
	Invalidate();
}


void
SudokuView::SetHintFlags(uint32 flags)
{
	if (flags == fHintFlags)
		return;

	if ((flags & kMarkInvalid) ^ (fHintFlags & kMarkInvalid))
		Invalidate();

	fHintFlags = flags;
}


void
SudokuView::SetEditable(bool editable)
{
	fEditable = editable;
}


void
SudokuView::AttachedToWindow()
{
	MakeFocus(true);
}


void
SudokuView::_FitFont(BFont& font, float fieldWidth, float fieldHeight)
{
	font.SetSize(100);

	font_height fontHeight;
	font.GetHeight(&fontHeight);

	float width = font.StringWidth("W");
	float height = ceilf(fontHeight.ascent) + ceilf(fontHeight.descent);

	float factor = fieldWidth != fHintWidth ? 4.f / 5.f : 1.f;
	float widthFactor = fieldWidth / (width / factor);
	float heightFactor = fieldHeight / (height / factor);
	font.SetSize(100 * min_c(widthFactor, heightFactor));
}


void
SudokuView::FrameResized(float /*width*/, float /*height*/)
{
	// font for numbers

	uint32 size = fField->Size();
	fWidth = (Bounds().Width() - kStrongLineSize * (fBlockSize - 1)) / size;
	fHeight = (Bounds().Height() - kStrongLineSize * (fBlockSize - 1)) / size;
	_FitFont(fFieldFont, fWidth - 2, fHeight - 2);

	font_height fontHeight;
	fFieldFont.GetHeight(&fontHeight);
	fBaseline = ceilf(fontHeight.ascent) / 2
		+ (fHeight - ceilf(fontHeight.descent)) / 2;

	// font for hint

	fHintWidth = (fWidth - 2) / fBlockSize;
	fHintHeight = (fHeight - 2) / fBlockSize;
	_FitFont(fHintFont, fHintWidth, fHintHeight);

	fHintFont.GetHeight(&fontHeight);
	fHintBaseline = ceilf(fontHeight.ascent) / 2
		+ (fHintHeight - ceilf(fontHeight.descent)) / 2;

	// fix the dragger's position
	BView *dragger = FindView("_dragger_");
	if (dragger)
		dragger->MoveTo(Bounds().right - 7, Bounds().bottom - 7);
}


BPoint
SudokuView::_LeftTop(uint32 x, uint32 y)
{
	return BPoint(x * fWidth + x / fBlockSize * kStrongLineSize + 1,
		y * fHeight + y / fBlockSize * kStrongLineSize + 1);
}


BRect
SudokuView::_Frame(uint32 x, uint32 y)
{
	BPoint leftTop = _LeftTop(x, y);
	BPoint rightBottom = leftTop + BPoint(fWidth - 2, fHeight - 2);

	return BRect(leftTop, rightBottom);
}


void
SudokuView::_InvalidateHintField(uint32 x, uint32 y, uint32 hintX,
	uint32 hintY)
{
	BPoint leftTop = _LeftTop(x, y);
	leftTop.x += hintX * fHintWidth;
	leftTop.y += hintY * fHintHeight;
	BPoint rightBottom = leftTop;
	rightBottom.x += fHintWidth;
	rightBottom.y += fHintHeight;

	Invalidate(BRect(leftTop, rightBottom));
}


void
SudokuView::_InvalidateField(uint32 x, uint32 y)
{
	Invalidate(_Frame(x, y));
}


void
SudokuView::_InvalidateKeyboardFocus(uint32 x, uint32 y)
{
	BRect frame = _Frame(x, y);
	frame.InsetBy(-1, -1);
	Invalidate(frame);
}


bool
SudokuView::_GetHintFieldFor(BPoint where, uint32 x, uint32 y,
	uint32& hintX, uint32& hintY)
{
	BPoint leftTop = _LeftTop(x, y);
	hintX = (uint32)floor((where.x - leftTop.x) / fHintWidth);
	hintY = (uint32)floor((where.y - leftTop.y) / fHintHeight);

	if (hintX >= fBlockSize || hintY >= fBlockSize)
		return false;

	return true;
}


bool
SudokuView::_GetFieldFor(BPoint where, uint32& x, uint32& y)
{
	float block = fWidth * fBlockSize + kStrongLineSize;
	x = (uint32)floor(where.x / block);
	uint32 offsetX = (uint32)floor((where.x - x * block) / fWidth);
	x = x * fBlockSize + offsetX;

	block = fHeight * fBlockSize + kStrongLineSize;
	y = (uint32)floor(where.y / block);
	uint32 offsetY = (uint32)floor((where.y - y * block) / fHeight);
	y = y * fBlockSize + offsetY;

	if (offsetX >= fBlockSize || offsetY >= fBlockSize
		|| x >= fField->Size() || y >= fField->Size())
		return false;

	return true;
}


void
SudokuView::_RemoveHint()
{
	if (fShowHintX == ~0UL)
		return;

	uint32 x = fShowHintX;
	uint32 y = fShowHintY;
	fShowHintX = ~0;
	fShowHintY = ~0;

	_InvalidateField(x, y);
}


void
SudokuView::_UndoRedo(BObjectList<BMessage>& undos,
	BObjectList<BMessage>& redos)
{
	if (undos.IsEmpty())
		return;

	BMessage* undo = undos.RemoveItemAt(undos.CountItems() - 1);

	BMessage* redo = new BMessage;
	if (fField->Archive(redo, true) == B_OK)
		redos.AddItem(redo);

	SudokuField field(undo);
	delete undo;

	fField->SetTo(&field);

	SendNotices(kUndoRedoChanged);
	Invalidate();
}


void
SudokuView::Undo()
{
	_UndoRedo(fUndos, fRedos);
}


void
SudokuView::Redo()
{
	_UndoRedo(fRedos, fUndos);
}


void
SudokuView::_PushUndo()
{
	fRedos.MakeEmpty();

	BMessage* undo = new BMessage;
	if (fField->Archive(undo, true) == B_OK
		&& fUndos.AddItem(undo))
		SendNotices(kUndoRedoChanged);
}


void
SudokuView::MouseDown(BPoint where)
{
	uint32 x, y;
	if (!fEditable || !_GetFieldFor(where, x, y))
		return;

	int32 buttons = B_PRIMARY_MOUSE_BUTTON;
	int32 clicks = 1;
	if (Looper() != NULL && Looper()->CurrentMessage() != NULL) {
		Looper()->CurrentMessage()->FindInt32("buttons", &buttons);
		Looper()->CurrentMessage()->FindInt32("clicks", &clicks);
	}

	uint32 hintX, hintY;
	if (!_GetHintFieldFor(where, x, y, hintX, hintY))
		return;

	uint32 value = hintX + hintY * fBlockSize;
	uint32 field = x + y * fField->Size();
	_PushUndo();

	if ((clicks == 2 && fLastHintValue == value && fLastField == field)
		|| (buttons & (B_SECONDARY_MOUSE_BUTTON
				| B_TERTIARY_MOUSE_BUTTON)) != 0) {
		// double click or other buttons set a value
		if ((fField->FlagsAt(x, y) & kInitialValue) == 0) {
			bool wasCompleted;
			if (fField->ValueAt(x, y) > 0) {
				value = fField->ValueAt(x, y) - 1;
				wasCompleted = fField->IsValueCompleted(value + 1);

				fField->SetValueAt(x, y, 0);
				fShowHintX = x;
				fShowHintY = y;
			} else {
				wasCompleted = fField->IsValueCompleted(value + 1);

				fField->SetValueAt(x, y, value + 1);
				BMessenger(this).SendMessage(kMsgCheckSolved);

				// allow dragging to remove the hint from other fields
				fLastHintValueSet = false;
				fLastHintValue = value;
				fLastField = field;
			}

			if (wasCompleted != fField->IsValueCompleted(value + 1))
				Invalidate();
			else
				_InvalidateField(x, y);
		}
		return;
	}

	uint32 hintMask = fField->HintMaskAt(x, y);
	uint32 valueMask = 1UL << value;
	fLastHintValueSet = (hintMask & valueMask) == 0;

	if (fLastHintValueSet)
		hintMask |= valueMask;
	else
		hintMask &= ~valueMask;

	fField->SetHintMaskAt(x, y, hintMask);
	_InvalidateHintField(x, y, hintX, hintY);

	fLastHintValue = value;
	fLastField = field;
}


void
SudokuView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	if (transit == B_EXITED_VIEW || dragMessage != NULL) {
		_RemoveHint();
		return;
	}

	if (fShowKeyboardFocus) {
		fShowKeyboardFocus = false;
		_InvalidateKeyboardFocus(fKeyboardX, fKeyboardY);
	}

	uint32 x, y;
	bool isField = _GetFieldFor(where, x, y);
	if (isField) {
		fKeyboardX = x;
		fKeyboardY = y;
	}
	if (!isField
		|| (fField->FlagsAt(x, y) & kInitialValue) != 0
		|| (!fShowCursor && fField->ValueAt(x, y) != 0)) {
		_RemoveHint();
		return;
	}

	if (fShowHintX == x && fShowHintY == y)
		return;

	int32 buttons = 0;
	if (Looper() != NULL && Looper()->CurrentMessage() != NULL)
		Looper()->CurrentMessage()->FindInt32("buttons", &buttons);

	uint32 field = x + y * fField->Size();

	if (buttons != 0 && field != fLastField) {
		// if a button is pressed, we drag the last hint selection
		// (either set or removal) to the field under the mouse
		uint32 hintMask = fField->HintMaskAt(x, y);
		uint32 valueMask = 1UL << fLastHintValue;
		if (fLastHintValueSet)
			hintMask |= valueMask;
		else
			hintMask &= ~valueMask;

		fField->SetHintMaskAt(x, y, hintMask);
	}

	_RemoveHint();
	fShowHintX = x;
	fShowHintY = y;
	_InvalidateField(x, y);
}


void
SudokuView::_InsertKey(char rawKey, int32 modifiers)
{
	if (!fEditable || !_ValidCharacter(rawKey)
		|| (fField->FlagsAt(fKeyboardX, fKeyboardY) & kInitialValue) != 0)
		return;

	uint32 value = rawKey - _BaseCharacter();

	if (modifiers & (B_SHIFT_KEY | B_OPTION_KEY)) {
		// set or remove hint
		if (value == 0)
			return;

		_PushUndo();
		uint32 hintMask = fField->HintMaskAt(fKeyboardX, fKeyboardY);
		uint32 valueMask = 1UL << (value - 1);
		if (modifiers & B_OPTION_KEY)
			hintMask &= ~valueMask;
		else
			hintMask |= valueMask;

		fField->SetValueAt(fKeyboardX, fKeyboardY, 0);
		fField->SetHintMaskAt(fKeyboardX, fKeyboardY, hintMask);
	} else {
		_PushUndo();
		fField->SetValueAt(fKeyboardX, fKeyboardY, value);
		if (value)
			BMessenger(this).SendMessage(kMsgCheckSolved);
	}
}


void
SudokuView::KeyDown(const char *bytes, int32 /*numBytes*/)
{
	be_app->ObscureCursor();

	uint32 x = fKeyboardX, y = fKeyboardY;

	switch (bytes[0]) {
		case B_UP_ARROW:
			if (fKeyboardY == 0)
				fKeyboardY = fField->Size() - 1;
			else
				fKeyboardY--;
			break;
		case B_DOWN_ARROW:
			if (fKeyboardY == fField->Size() - 1)
				fKeyboardY = 0;
			else
				fKeyboardY++;
			break;

		case B_LEFT_ARROW:
			if (fKeyboardX == 0)
				fKeyboardX = fField->Size() - 1;
			else
				fKeyboardX--;
			break;
		case B_RIGHT_ARROW:
			if (fKeyboardX == fField->Size() - 1)
				fKeyboardX = 0;
			else
				fKeyboardX++;
			break;

		case B_BACKSPACE:
		case B_DELETE:
		case B_SPACE:
			// clear value
			_InsertKey(_BaseCharacter(), 0);
			break;

		default:
			int32 rawKey = bytes[0];
			int32 modifiers = 0;
			if (Looper() != NULL && Looper()->CurrentMessage() != NULL) {
				Looper()->CurrentMessage()->FindInt32("raw_char", &rawKey);
				Looper()->CurrentMessage()->FindInt32("modifiers", &modifiers);
			}

			_InsertKey(rawKey, modifiers);
			break;
	}

	if (!fShowKeyboardFocus && fShowHintX != ~0UL) {
		// always start at last mouse position, if any
		fKeyboardX = fShowHintX;
		fKeyboardY = fShowHintY;
	}

	_RemoveHint();

	// remove old focus, if any
	if (fShowKeyboardFocus && (x != fKeyboardX || y != fKeyboardY))
		_InvalidateKeyboardFocus(x, y);

	fShowKeyboardFocus = true;
	_InvalidateKeyboardFocus(fKeyboardX, fKeyboardY);
}


void
SudokuView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgCheckSolved:
			if (fField->IsSolved()) {
				// notify window
				Looper()->PostMessage(kMsgSudokuSolved);
			}
			break;

		case B_UNDO:
			Undo();
			break;

		case B_REDO:
			Redo();
			break;

		case kMsgSolveSudoku:
		{
			SudokuSolver solver;
			solver.SetTo(fField);
			bigtime_t start = system_time();
			solver.ComputeSolutions();
			printf("found %ld solutions in %g msecs\n",
				solver.CountSolutions(), (system_time() - start) / 1000.0);
			if (solver.CountSolutions() > 0) {
				_PushUndo();
				fField->SetTo(solver.SolutionAt(0));
				Invalidate();
			} else
				beep();
			break;
		}

		case kMsgSolveSingle:
		{
			if (fField->IsSolved()) {
				beep();
				break;
			}

			SudokuSolver solver;
			solver.SetTo(fField);
			bigtime_t start = system_time();
			solver.ComputeSolutions();
			printf("found %ld solutions in %g msecs\n",
				solver.CountSolutions(), (system_time() - start) / 1000.0);
			if (solver.CountSolutions() > 0) {
				_PushUndo();

				// find free spot
				uint32 x, y;
				do {
					x = rand() % fField->Size();
					y = rand() % fField->Size();
				} while (fField->ValueAt(x, y));

				fField->SetValueAt(x, y,
					solver.SolutionAt(0)->ValueAt(x, y));
				_InvalidateField(x, y);
			} else
				beep();
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


char
SudokuView::_BaseCharacter()
{
	return fField->Size() > 9 ? '@' : '0';
}


bool
SudokuView::_ValidCharacter(char c)
{
	char min = _BaseCharacter();
	char max = min + fField->Size();
	return c >= min && c <= max;
}


void
SudokuView::_SetText(char* text, uint32 value)
{
	text[0] = value + _BaseCharacter();
	text[1] = '\0';
}


void
SudokuView::_DrawKeyboardFocus()
{
	BRect frame = _Frame(fKeyboardX, fKeyboardY);
	SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
	StrokeRect(frame);
	frame.InsetBy(-1, -1);
	StrokeRect(frame);
	frame.InsetBy(2, 2);
	StrokeRect(frame);
}


void
SudokuView::_DrawHints(uint32 x, uint32 y)
{
	bool showAll = fShowHintX == x && fShowHintY == y;
	uint32 hintMask = fField->HintMaskAt(x, y);
	if (hintMask == 0 && !showAll)
		return;

	uint32 validMask = fField->ValidMaskAt(x, y);
	BPoint leftTop = _LeftTop(x, y);
	SetFont(&fHintFont);

	for (uint32 j = 0; j < fBlockSize; j++) {
		for (uint32 i = 0; i < fBlockSize; i++) {
			uint32 value = j * fBlockSize + i;
			if (hintMask & (1UL << value))
				SetHighColor(200, 0, 0);
			else {
				if (!showAll)
					continue;

				if ((fHintFlags & kMarkValidHints) == 0
					|| validMask & (1UL << value))
					SetHighColor(110, 110, 80);
				else
					SetHighColor(180, 180, 120);
			}

			char text[2];
			_SetText(text, value + 1);
			DrawString(text, leftTop + BPoint((i + 0.5f) * fHintWidth
				- StringWidth(text) / 2, floorf(j * fHintHeight)
					+ fHintBaseline));
		}
	}
}


void
SudokuView::Draw(BRect /*updateRect*/)
{
	// draw one pixel border otherwise not covered
	// by lines and fields
	SetLowColor(fBackgroundColor);
	StrokeRect(Bounds(), B_SOLID_LOW);

	// draw lines

	uint32 size = fField->Size();

	SetHighColor(0, 0, 0);

	float width = fWidth;
	for (uint32 x = 1; x < size; x++) {
		if (x % fBlockSize == 0) {
			FillRect(BRect(width, 0, width + kStrongLineSize,
				Bounds().Height()));
			width += kStrongLineSize;
		} else {
			StrokeLine(BPoint(width, 0), BPoint(width, Bounds().Height()));
		}
		width += fWidth;
	}

	float height = fHeight;
	for (uint32 y = 1; y < size; y++) {
		if (y % fBlockSize == 0) {
			FillRect(BRect(0, height, Bounds().Width(),
				height + kStrongLineSize));
			height += kStrongLineSize;
		} else {
			StrokeLine(BPoint(0, height), BPoint(Bounds().Width(), height));
		}
		height += fHeight;
	}

	// draw text

	for (uint32 y = 0; y < size; y++) {
		for (uint32 x = 0; x < size; x++) {
			if (((fShowCursor && x == fShowHintX && y == fShowHintY)
					|| (fShowKeyboardFocus && x == fKeyboardX
						&& y == fKeyboardY))
				&& (fField->FlagsAt(x, y) & kInitialValue) == 0) {
				// TODO: make color more intense
				SetLowColor(tint_color(fBackgroundColor, B_DARKEN_2_TINT));
				FillRect(_Frame(x, y), B_SOLID_LOW);
			} else {
				SetLowColor(fBackgroundColor);
				FillRect(_Frame(x, y), B_SOLID_LOW);
			}

			if (fShowKeyboardFocus && x == fKeyboardX && y == fKeyboardY)
				_DrawKeyboardFocus();

			uint32 value = fField->ValueAt(x, y);
			if (value == 0) {
				_DrawHints(x, y);
				continue;
			}

			SetFont(&fFieldFont);
			if ((fField->FlagsAt(x, y) & kInitialValue) != 0)
				SetHighColor(0, 0, 0);
			else {
				if ((fHintFlags & kMarkInvalid) == 0
					|| fField->ValidMaskAt(x, y) & (1UL << (value - 1))) {
					if (fField->IsValueCompleted(value))
						SetHighColor(60, 60, 150);
					else
						SetHighColor(0, 0, 220);
				} else
					SetHighColor(200, 0, 0);
			}

			char text[2];
			_SetText(text, value);
			DrawString(text, _LeftTop(x, y)
				+ BPoint((fWidth - StringWidth(text)) / 2, fBaseline));
		}
	}
}


