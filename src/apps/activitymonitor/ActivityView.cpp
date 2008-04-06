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
		const BMessage& settings, uint32 resizingMode)
	: BView(frame, name, resizingMode,
		B_WILL_DRAW | B_SUBPIXEL_PRECISE | B_FULL_UPDATE_ON_RESIZE
		| B_FRAME_EVENTS)
{
	_Init(&settings);

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
}


void
ActivityView::_Init(const BMessage* settings)
{
	fBackgroundColor = (rgb_color){255, 255, 240};
	SetLowColor(fBackgroundColor);

	fRefreshInterval = kInitialRefreshInterval;
	fDrawInterval = kInitialRefreshInterval * 2;
	fLastRefresh = 0;
	fDrawResolution = 1;

	AddDataSource(new UsedMemoryDataSource());
	AddDataSource(new CachedMemoryDataSource());
	AddDataSource(new ThreadsDataSource());
	AddDataSource(new CpuUsageDataSource());
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
	return B_ERROR;
}


DataSource*
ActivityView::FindDataSource(const char* name)
{
	for (int32 i = fSources.CountItems(); i-- > 0;) {
		DataSource* source = fSources.ItemAt(i);
		if (!strcmp(source->Label(), name))
			return source;
	}

	return NULL;
}


status_t
ActivityView::AddDataSource(DataSource* source)
{
	DataHistory* values = new(std::nothrow) DataHistory(10 * 60000000LL,
		fRefreshInterval);
	if (values == NULL)
		return B_NO_MEMORY;
	if (!fValues.AddItem(values)) {
		delete values;
		return B_NO_MEMORY;
	}

	if (!fSources.AddItem(source)) {
		fValues.RemoveItem(values);
		delete values;
		return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
ActivityView::RemoveDataSource(DataSource* source)
{
	int32 index = fSources.IndexOf(source);
	if (index < 0)
		return B_ENTRY_NOT_FOUND;

	fSources.RemoveItemAt(index);
	delete source;
	DataHistory* values = fValues.RemoveItemAt(index);
	delete values;

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
}


void
ActivityView::DetachedFromWindow()
{
	delete fRunner;
}


void
ActivityView::FrameResized(float /*width*/, float /*height*/)
{
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
	BMenuItem* item;

	for (int32 i = 0; i < DataSource::CountSources(); i++) {
		const DataSource* source = DataSource::SourceAt(i);

		BMessage* message = new BMessage(kMsgToggleDataSource);
		message->AddInt32("index", i);

		item = new BMenuItem(source->Label(), message);
		if (FindDataSource(source->Label()))
			item->SetMarked(true);

		menu->AddItem(item);
	}

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

			DataSource* source = FindDataSource(baseSource->Label());
			if (source == NULL)
				AddDataSource(baseSource->Copy());
			else
				RemoveDataSource(source);

			Invalidate();
			break;
		}

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

	float height = Bounds().Height();
	return height - (value - min) * height / (max - min);
}


void
ActivityView::Draw(BRect /*updateRect*/)
{
	uint32 step = 2;
	uint32 resolution = fDrawResolution;
	if (fDrawResolution > 1) {
		step = 1;
		resolution--;
	}

	uint32 width = Bounds().IntegerWidth() - 10;
	uint32 steps = width / step;
	bigtime_t timeStep = fRefreshInterval * resolution;
	bigtime_t now = system_time();

	SetPenSize(2);

	for (uint32 i = fSources.CountItems(); i-- > 0;) {
		DataSource* source = fSources.ItemAt(i);
		DataHistory* values = fValues.ItemAt(i);
		bigtime_t time = now - steps * timeStep;
			// for now steps pixels per second

		BeginLineArray(steps);
		SetHighColor(source->Color());

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
			if (lastY != FLT_MIN)
				AddLine(BPoint(lastX, lastY), BPoint(x, y), source->Color());

			lastX = x;
			lastY = y;
		}

		EndLineArray();
	}

	// TODO: add marks when an app started or quit
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

