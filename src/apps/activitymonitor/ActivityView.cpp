/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ActivityView.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <Bitmap.h>
#include <Dragger.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <PopUpMenu.h>
#include <String.h>

#include "ActivityMonitor.h"
#include "DataSource.h"
#include "SystemInfo.h"


struct data_item {
	bigtime_t	time;
	int64		value;
};

const bigtime_t kInitialRefreshInterval = 500000LL;

const uint32 kMsgRefresh = 'refr';
const uint32 kMsgToggleDataSource = 'tgds';
const uint32 kMsgToggleLegend = 'tglg';

extern const char* kSignature;


DataHistory::DataHistory(bigtime_t memorize, bigtime_t interval)
	:
	fBuffer(10000),
	fMinimumValue(0),
	fMaximumValue(0),
	fRefreshInterval(interval),
	fLastIndex(-1)
{
}


DataHistory::~DataHistory()
{
}


void
DataHistory::AddValue(bigtime_t time, int64 value)
{
	if (fBuffer.IsEmpty() || fMaximumValue < value)
		fMaximumValue = value;
	if (fBuffer.IsEmpty() || fMinimumValue > value)
		fMinimumValue = value;

	data_item item = {time, value};
	fBuffer.AddItem(item);
}


int64
DataHistory::ValueAt(bigtime_t time)
{
	// TODO: if the refresh rate changes, this computation won't work anymore!
	int32 index = (time - Start()) / fRefreshInterval;
	data_item* item = fBuffer.ItemAt(index);
	if (item != NULL)
		return item->value;

	return 0;
}


int64
DataHistory::MaximumValue() const
{
	return fMaximumValue;
}


int64
DataHistory::MinimumValue() const
{
	return fMinimumValue;
}


bigtime_t
DataHistory::Start() const
{
	if (fBuffer.CountItems() == 0)
		return 0;

	return fBuffer.ItemAt(0)->time;
}


bigtime_t
DataHistory::End() const
{
	if (fBuffer.CountItems() == 0)
		return 0;

	return fBuffer.ItemAt(fBuffer.CountItems() - 1)->time;
}


void
DataHistory::SetRefreshInterval(bigtime_t interval)
{
	// TODO: adjust buffer size
}


//	#pragma mark -


ActivityView::ActivityView(BRect frame, const char* name,
		const BMessage* settings, uint32 resizingMode)
	: BView(frame, name, resizingMode,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS)
{
	_Init(settings);

	BRect rect(Bounds());
	rect.top = rect.bottom - 7;
	rect.left = rect.right - 7;
	BDragger* dragger = new BDragger(rect, this,
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(dragger);
}


ActivityView::ActivityView(const char* name, const BMessage* settings)
	: BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS)
{
	_Init(settings);

	BRect rect(Bounds());
	rect.top = rect.bottom - 7;
	rect.left = rect.right - 7;
	BDragger* dragger = new BDragger(rect, this,
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(dragger);
}


ActivityView::ActivityView(BMessage* archive)
	: BView(archive)
{
	_Init(archive);
}


ActivityView::~ActivityView()
{
	delete fOffscreen;
}


void
ActivityView::_Init(const BMessage* settings)
{
	fBackgroundColor = (rgb_color){255, 255, 240};
	fOffscreen = NULL;
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fRefreshInterval = kInitialRefreshInterval;
	fDrawInterval = kInitialRefreshInterval * 2;
	fLastRefresh = 0;
	fDrawResolution = 1;

	if (settings == NULL
		|| settings->FindBool("show legend", &fShowLegend) != B_OK)
		fShowLegend = true;

	if (settings == NULL) {
		AddDataSource(new UsedMemoryDataSource());
		AddDataSource(new CachedMemoryDataSource());
		return;
	}

	const char* name;
	for (int32 i = 0; settings->FindString("source", i, &name) == B_OK; i++) {
		AddDataSource(DataSource::FindSource(name));
	}
}


status_t
ActivityView::Archive(BMessage* into, bool deep) const
{
	status_t status;

	status = BView::Archive(into, deep);
	if (status < B_OK)
		return status;

	status = into->AddString("add_on", kSignature);
	if (status < B_OK)
		return status;

	status = SaveState(*into);
	if (status < B_OK)
		return status;

	return B_OK;
}


BArchivable*
ActivityView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "ActivityView"))
		return NULL;

	return new ActivityView(archive);
}


status_t
ActivityView::SaveState(BMessage& state) const
{
	status_t status = state.AddBool("show legend", fShowLegend);
	if (status != B_OK)
		return status;

	for (int32 i = 0; i < fSources.CountItems(); i++) {
		DataSource* source = fSources.ItemAt(i);

		if (!source->PerCPU() || source->CPU() == 0)
			status = state.AddString("source", source->Name());
		if (status != B_OK)
			return status;

		// TODO: save and restore color as well
	}
	return B_OK;
}


DataSource*
ActivityView::FindDataSource(const DataSource* search)
{
	for (int32 i = fSources.CountItems(); i-- > 0;) {
		DataSource* source = fSources.ItemAt(i);
		if (!strcmp(source->Name(), search->Name()))
			return source;
	}

	return NULL;
}


status_t
ActivityView::AddDataSource(const DataSource* source)
{
	if (source == NULL)
		return B_BAD_VALUE;

	int32 insert = DataSource::IndexOf(source);
	for (int32 i = 0; i < fSources.CountItems() && i < insert; i++) {
		DataSource* before = fSources.ItemAt(i);
		if (DataSource::IndexOf(before) > insert) {
			insert = i;
			break;
		}
	}
	if (insert > fSources.CountItems())
		insert = fSources.CountItems();

	uint32 count = 1;
	if (source->PerCPU()) {
		SystemInfo info;
		count = info.CPUCount();
	}

	for (uint32 i = 0; i < count; i++) {
		DataHistory* values = new(std::nothrow) DataHistory(10 * 60000000LL,
			fRefreshInterval);
		if (values == NULL)
			return B_NO_MEMORY;

		if (!fValues.AddItem(values, insert + i)) {
			delete values;
			return B_NO_MEMORY;
		}

		DataSource* copy;
		if (source->PerCPU())
			copy = source->CopyForCPU(i);
		else
			copy = source->Copy();

		if (!fSources.AddItem(copy, insert + i)) {
			fValues.RemoveItem(values);
			delete values;
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


status_t
ActivityView::RemoveDataSource(const DataSource* remove)
{
	while (true) {
		DataSource* source = FindDataSource(remove);
		if (source == NULL)
			return B_OK;

		int32 index = fSources.IndexOf(source);
		if (index < 0)
			return B_ENTRY_NOT_FOUND;

		fSources.RemoveItemAt(index);
		delete source;
		DataHistory* values = fValues.RemoveItemAt(index);
		delete values;
	}

	return B_OK;
}


void
ActivityView::RemoveAllDataSources()
{
	fSources.MakeEmpty();
	fValues.MakeEmpty();
}


void
ActivityView::AttachedToWindow()
{
	BMessage refresh(kMsgRefresh);
	fRunner = new BMessageRunner(this, &refresh, fRefreshInterval);

	FrameResized(Bounds().Width(), Bounds().Height());
}


void
ActivityView::DetachedFromWindow()
{
	delete fRunner;
}


BSize
ActivityView::MinSize()
{
	BSize size(32, 32);
	if (fShowLegend)
		size.height = _LegendFrame().Height();

	return size;
}


void
ActivityView::FrameResized(float /*width*/, float /*height*/)
{
	_UpdateOffscreenBitmap();
}


void
ActivityView::_UpdateOffscreenBitmap()
{
	BRect frame = _HistoryFrame();
	if (fOffscreen != NULL && frame == fOffscreen->Bounds())
		return;

	delete fOffscreen;

	// create offscreen bitmap

	fOffscreen = new(std::nothrow) BBitmap(frame, B_BITMAP_ACCEPTS_VIEWS,
		B_RGB32);
	if (fOffscreen == NULL || fOffscreen->InitCheck() != B_OK) {
		delete fOffscreen;
		fOffscreen = NULL;
		return;
	}

	BView* view = new BView(frame, NULL, B_FOLLOW_NONE, B_SUBPIXEL_PRECISE);
	view->SetViewColor(fBackgroundColor);
	view->SetLowColor(view->ViewColor());
	fOffscreen->AddChild(view);
}


void
ActivityView::MouseDown(BPoint where)
{
#if 0
	int32 buttons = B_PRIMARY_MOUSE_BUTTON;
	int32 clicks = 1;
	if (Looper() != NULL && Looper()->CurrentMessage() != NULL) {
		Looper()->CurrentMessage()->FindInt32("buttons", &buttons);
		Looper()->CurrentMessage()->FindInt32("clicks", &clicks);
	}
#endif

	BPopUpMenu *menu = new BPopUpMenu(B_EMPTY_STRING, false, false);
	menu->SetFont(be_plain_font);

	SystemInfo info;
	BMenuItem* item;

	for (int32 i = 0; i < DataSource::CountSources(); i++) {
		const DataSource* source = DataSource::SourceAt(i);

		if (source->MultiCPUOnly() && info.CPUCount() == 1)
			continue;

		BMessage* message = new BMessage(kMsgToggleDataSource);
		message->AddInt32("index", i);

		item = new BMenuItem(source->Name(), message);
		if (FindDataSource(source))
			item->SetMarked(true);

		menu->AddItem(item);
	}

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(fShowLegend ? "Hide Legend" : "Show Legend",
		new BMessage(kMsgToggleLegend)));

	menu->SetTargetForItems(this);

	ConvertToScreen(&where);
	menu->Go(where, true, false, true);

}


void
ActivityView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
}


void
ActivityView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_ABOUT_REQUESTED:
			ActivityMonitor::ShowAbout();
			break;

		case kMsgRefresh:
			_Refresh();
			break;

		case kMsgToggleDataSource:
		{
			int32 index;
			if (message->FindInt32("index", &index) != B_OK)
				break;

			const DataSource* baseSource = DataSource::SourceAt(index);
			if (baseSource == NULL)
				break;

			DataSource* source = FindDataSource(baseSource);
			if (source == NULL)
				AddDataSource(baseSource);
			else
				RemoveDataSource(source);

			Invalidate();
			break;
		}

		case kMsgToggleLegend:
			fShowLegend = !fShowLegend;
			Invalidate();
			break;

		case B_MOUSE_WHEEL_CHANGED:
		{
			float deltaY = 0.0f;
			if (message->FindFloat("be:wheel_delta_y", &deltaY) != B_OK
				|| deltaY == 0.0f)
				break;

			if (deltaY > 0)
				fDrawResolution *= 2;
			else
				fDrawResolution /= 2;

			if (fDrawResolution < 1)
				fDrawResolution = 1;
			if (fDrawResolution > 128)
				fDrawResolution = 128;

			Invalidate();
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


BRect
ActivityView::_HistoryFrame() const
{
	if (!fShowLegend)
		return Bounds();

	BRect frame = Bounds();
	BRect legendFrame = _LegendFrame();

	frame.bottom = legendFrame.top - 1;

	return frame;
}


BRect
ActivityView::_LegendFrame() const
{
	BRect frame = Bounds();
	font_height fontHeight;
	GetFontHeight(&fontHeight);

	int32 rows = (fSources.CountItems() + 1) / 2;
	frame.top = frame.bottom - rows * (4 + ceilf(fontHeight.ascent)
		+ ceilf(fontHeight.descent) + ceilf(fontHeight.leading));

	return frame;
}


BRect
ActivityView::_LegendFrameAt(BRect frame, int32 index) const
{
	int32 column = index & 1;
	int32 row = index / 2;
	if (column == 0)
		frame.right = frame.left + floorf(frame.Width() / 2) - 5;
	else
		frame.left = frame.right - floorf(frame.Width() / 2) + 5;

	int32 rows = (fSources.CountItems() + 1) / 2;
	float height = floorf((frame.Height() - 5) / rows);

	frame.top = frame.top + 5 + row * height;
	frame.bottom = frame.top + height - 1;

	return frame;
}


float
ActivityView::_PositionForValue(DataSource* source, DataHistory* values,
	int64 value)
{
	int64 min = source->Minimum();
	int64 max = source->Maximum();
	if (source->AdaptiveScale()) {
		int64 adaptiveMax = int64(values->MaximumValue() * 1.2);
		if (adaptiveMax < max)
			max = adaptiveMax;
	}

	if (value > max)
		value = max;
	if (value < min)
		value = min;

	float height = _HistoryFrame().Height();
	return height - (value - min) * height / (max - min);
}


void
ActivityView::_DrawHistory()
{
	_UpdateOffscreenBitmap();

	BView* view = this;
	if (fOffscreen != NULL) {
		fOffscreen->Lock();
		view = fOffscreen->ChildAt(0);
	}

	BRect frame = _HistoryFrame();
	view->FillRect(frame, B_SOLID_LOW);

	uint32 step = 2;
	uint32 resolution = fDrawResolution;
	if (fDrawResolution > 1) {
		step = 1;
		resolution--;
	}

	uint32 width = frame.IntegerWidth() - 10;
	uint32 steps = width / step;
	bigtime_t timeStep = fRefreshInterval * resolution;
	bigtime_t now = system_time();

	view->SetPenSize(1);

	view->SetHighColor(tint_color(view->ViewColor(), B_DARKEN_2_TINT));
	view->StrokeRect(frame);
	view->StrokeLine(BPoint(frame.left, frame.top + frame.Height() / 2),
		BPoint(frame.right, frame.top + frame.Height() / 2));

	view->SetPenSize(2);

	for (uint32 i = fSources.CountItems(); i-- > 0;) {
		DataSource* source = fSources.ItemAt(i);
		DataHistory* values = fValues.ItemAt(i);
		bigtime_t time = now - steps * timeStep;
			// for now steps pixels per second

		view->BeginLineArray(steps);
		view->SetHighColor(source->Color());

		float lastY = FLT_MIN;
		uint32 lastX = 0;

		for (uint32 x = 0; x < width; x += step, time += timeStep) {
			// TODO: compute starting point instead!
			if (values->Start() > time || values->End() < time)
				continue;

			int64 value = values->ValueAt(time);
			if (timeStep > fRefreshInterval) {
				// TODO: always start with the same index, so that it always
				// uses the same values for computation (currently it jumps)
				uint32 count = 1;
				for (bigtime_t offset = fRefreshInterval; offset < timeStep;
						offset += fRefreshInterval) {
					// TODO: handle int64 overflow correctly!
					value += values->ValueAt(time + offset);
					count++;
				}
				value /= count;
			}

			float y = _PositionForValue(source, values, value);
			if (lastY != FLT_MIN) {
				view->AddLine(BPoint(lastX, lastY), BPoint(x, y),
					source->Color());
			}

			lastX = x;
			lastY = y;
		}

		view->EndLineArray();
	}

	// TODO: add marks when an app started or quit
	view->Sync();
	if (fOffscreen != NULL) {
		fOffscreen->Unlock();
		DrawBitmap(fOffscreen);
	}
}


void
ActivityView::Draw(BRect /*updateRect*/)
{
	_DrawHistory();

	if (!fShowLegend)
		return;

	// draw legend

	BRect legendFrame = _LegendFrame();
	FillRect(legendFrame, B_SOLID_LOW);

	font_height fontHeight;
	GetFontHeight(&fontHeight);

	for (int32 i = 0; i < fSources.CountItems(); i++) {
		DataSource* source = fSources.ItemAt(i);
		DataHistory* values = fValues.ItemAt(i);
		BRect frame = _LegendFrameAt(legendFrame, i);

		// draw color box
		BRect colorBox = frame.InsetByCopy(2, 2);
		colorBox.right = colorBox.left + colorBox.Height();
		SetHighColor(tint_color(source->Color(), B_DARKEN_1_TINT));
		StrokeRect(colorBox);
		SetHighColor(source->Color());
		colorBox.InsetBy(1, 1);
		FillRect(colorBox);

		// show current value and label
		float y = frame.top + ceilf(fontHeight.ascent);
		int64 value = values->ValueAt(values->End());
		BString text;
		source->Print(text, value);
		float width = StringWidth(text.String());

		BString label = source->Label();
		TruncateString(&label, B_TRUNCATE_MIDDLE,
			frame.right - colorBox.right - 12 - width);

		SetHighColor(ui_color(B_CONTROL_TEXT_COLOR));
		DrawString(label.String(), BPoint(6 + colorBox.right, y));
		DrawString(text.String(), BPoint(frame.right - width, y));
	}
}


void
ActivityView::_Refresh()
{
	SystemInfo info;

	// TODO: run refresh in another thread to decouple it from the UI!

	for (uint32 i = fSources.CountItems(); i-- > 0;) {
		DataSource* source = fSources.ItemAt(i);
		DataHistory* values = fValues.ItemAt(i);

		int64 value = source->NextValue(info);
		values->AddValue(info.Time(), value);
	}

	bigtime_t now = info.Time();
	if (fLastRefresh + fDrawInterval <= now) {
		Invalidate();
		fLastRefresh = now;
	}
}

