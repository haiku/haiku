/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ActivityView.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>

#ifdef __HAIKU__
#	include <AbstractLayoutItem.h>
#endif
#include <Application.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <Dragger.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <PopUpMenu.h>
#include <String.h>

#include "ActivityMonitor.h"
#include "ActivityWindow.h"
#include "SystemInfo.h"
#include "SystemInfoHandler.h"


class Scale {
public:
							Scale(scale_type type);

			int64			MinimumValue() const { return fMinimumValue; }
			int64			MaximumValue() const { return fMaximumValue; }

			void			Update(int64 value);

private:
	scale_type				fType;
	int64					fMinimumValue;
	int64					fMaximumValue;
	bool					fInitialized;
};

struct data_item {
	bigtime_t	time;
	int64		value;
};

#ifdef __HAIKU__
class ActivityView::HistoryLayoutItem : public BAbstractLayoutItem {
public:
							HistoryLayoutItem(ActivityView* parent);

	virtual	bool			IsVisible();
	virtual	void			SetVisible(bool visible);

	virtual	BRect			Frame();
	virtual	void			SetFrame(BRect frame);

	virtual	BView*			View();

	virtual	BSize			BasePreferredSize();

private:
	ActivityView*			fParent;
	BRect					fFrame;
};

class ActivityView::LegendLayoutItem : public BAbstractLayoutItem {
public:
							LegendLayoutItem(ActivityView* parent);

	virtual	bool			IsVisible();
	virtual	void			SetVisible(bool visible);

	virtual	BRect			Frame();
	virtual	void			SetFrame(BRect frame);

	virtual	BView*			View();

	virtual	BSize			BaseMinSize();
	virtual	BSize			BaseMaxSize();
	virtual	BSize			BasePreferredSize();
	virtual	BAlignment		BaseAlignment();

private:
	ActivityView*			fParent;
	BRect					fFrame;
};
#endif

const bigtime_t kInitialRefreshInterval = 500000LL;

const uint32 kMsgToggleDataSource = 'tgds';
const uint32 kMsgToggleLegend = 'tglg';

extern const char* kSignature;


Scale::Scale(scale_type type)
	:
	fType(type),
	fMinimumValue(0),
	fMaximumValue(0),
	fInitialized(false)
{
}


void
Scale::Update(int64 value)
{
	if (!fInitialized || fMinimumValue > value)
		fMinimumValue = value;
	if (!fInitialized || fMaximumValue > value)
		fMaximumValue = value;

	fInitialized = true;
}


//	#pragma mark -


DataHistory::DataHistory(bigtime_t memorize, bigtime_t interval)
	:
	fBuffer(10000),
	fMinimumValue(0),
	fMaximumValue(0),
	fRefreshInterval(interval),
	fLastIndex(-1),
	fScale(NULL)
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
	if (fScale != NULL)
		fScale->Update(value);

	data_item item = {time, value};
	fBuffer.AddItem(item);
}


int64
DataHistory::ValueAt(bigtime_t time)
{
	int32 left = 0;
	int32 right = fBuffer.CountItems() - 1;
	data_item* item = NULL;

	while (left <= right) {
		int32 index = (left + right) / 2;
		item = fBuffer.ItemAt(index);

		if (item->time > time) {
			// search in left part
			right = index - 1;
		} else {
			data_item* nextItem = fBuffer.ItemAt(index + 1);
			if (nextItem == NULL)
				return item->value;
			if (nextItem->time > time) {
				// found item
				int64 value = item->value;
				value += int64(double(nextItem->value - value)
					/ (nextItem->time - item->time) * (time - item->time));
				return item->value;
			}

			// search in right part
			left = index + 1;
		}
	}

	return 0;
}


int64
DataHistory::MaximumValue() const
{
	if (fScale != NULL)
		return fScale->MaximumValue();

	return fMaximumValue;
}


int64
DataHistory::MinimumValue() const
{
	if (fScale != NULL)
		return fScale->MinimumValue();

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


void
DataHistory::SetScale(Scale* scale)
{
	fScale = scale;
}


//	#pragma mark -


#ifdef __HAIKU__
ActivityView::HistoryLayoutItem::HistoryLayoutItem(ActivityView* parent)
	:
	fParent(parent),
	fFrame()
{
}


bool
ActivityView::HistoryLayoutItem::IsVisible()
{
	return !fParent->IsHidden(fParent);
}


void
ActivityView::HistoryLayoutItem::SetVisible(bool visible)
{
	// not allowed
}


BRect
ActivityView::HistoryLayoutItem::Frame()
{
	return fFrame;
}


void
ActivityView::HistoryLayoutItem::SetFrame(BRect frame)
{
	fFrame = frame;
	fParent->_UpdateFrame();
}


BView*
ActivityView::HistoryLayoutItem::View()
{
	return fParent;
}


BSize
ActivityView::HistoryLayoutItem::BasePreferredSize()
{
	BSize size(BaseMaxSize());
	return size;
}


//	#pragma mark -


ActivityView::LegendLayoutItem::LegendLayoutItem(ActivityView* parent)
	:
	fParent(parent),
	fFrame()
{
}


bool
ActivityView::LegendLayoutItem::IsVisible()
{
	return !fParent->IsHidden(fParent);
}


void
ActivityView::LegendLayoutItem::SetVisible(bool visible)
{
	// not allowed
}


BRect
ActivityView::LegendLayoutItem::Frame()
{
	return fFrame;
}


void
ActivityView::LegendLayoutItem::SetFrame(BRect frame)
{
	fFrame = frame;
	fParent->_UpdateFrame();
}


BView*
ActivityView::LegendLayoutItem::View()
{
	return fParent;
}


BSize
ActivityView::LegendLayoutItem::BaseMinSize()
{
	// TODO: Cache the info. Might be too expensive for this call.
	BSize size;
	size.width = 80;
	size.height = fParent->_LegendHeight();

	return size;
}


BSize
ActivityView::LegendLayoutItem::BaseMaxSize()
{
	BSize size(BaseMinSize());
	size.width = B_SIZE_UNLIMITED;
	return size;
}


BSize
ActivityView::LegendLayoutItem::BasePreferredSize()
{
	BSize size(BaseMinSize());
	return size;
}


BAlignment
ActivityView::LegendLayoutItem::BaseAlignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}
#endif


//	#pragma mark -


ActivityView::ActivityView(BRect frame, const char* name,
		const BMessage* settings, uint32 resizingMode)
	: BView(frame, name, resizingMode,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS),
	fSourcesLock("data sources")
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
#ifdef __HAIKU__
	: BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS),
#else
	: BView(BRect(0,0,300,200), name, B_FOLLOW_NONE,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS),
#endif
	fSourcesLock("data sources")
{
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

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
	delete fSystemInfoHandler;
}


void
ActivityView::_Init(const BMessage* settings)
{
	fHistoryBackgroundColor = (rgb_color){255, 255, 240};
	fLegendBackgroundColor = LowColor();
		// the low color is restored by the BView unarchiving
	fOffscreen = NULL;
#ifdef __HAIKU__
	fHistoryLayoutItem = NULL;
	fLegendLayoutItem = NULL;
#endif
	SetViewColor(B_TRANSPARENT_COLOR);

	fRefreshInterval = kInitialRefreshInterval;
	fDrawInterval = kInitialRefreshInterval * 2;
	fLastRefresh = 0;
	fDrawResolution = 1;
	fZooming = false;

	fSystemInfoHandler = new SystemInfoHandler;

	if (settings == NULL
		|| settings->FindBool("show legend", &fShowLegend) != B_OK)
		fShowLegend = true;

	if (settings == NULL)
		return;

	ssize_t colorLength;
	rgb_color *color;
	if (settings->FindData("history background color", B_RGB_COLOR_TYPE,
			(const void **)&color, &colorLength) == B_OK
		&& colorLength == sizeof(rgb_color))
		fHistoryBackgroundColor = *color;

	const char* name;
	for (int32 i = 0; settings->FindString("source", i, &name) == B_OK; i++) {
		AddDataSource(DataSource::FindSource(name), settings);
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

	status = state.AddData("history background color", B_RGB_COLOR_TYPE,
		&fHistoryBackgroundColor, sizeof(rgb_color));
	if (status != B_OK)
		return status;

	for (int32 i = 0; i < fSources.CountItems(); i++) {
		DataSource* source = fSources.ItemAt(i);

		if (!source->PerCPU() || source->CPU() == 0)
			status = state.AddString("source", source->Name());
		if (status != B_OK)
			return status;

		BString name = source->Name();
		name << " color";
		rgb_color color = source->Color();
		state.AddData(name.String(), B_RGB_COLOR_TYPE, &color,
			sizeof(rgb_color));
	}
	return B_OK;
}


Scale*
ActivityView::_ScaleFor(scale_type type)
{
	if (type == kNoScale)
		return NULL;

	std::map<scale_type, ::Scale*>::iterator iterator = fScales.find(type);
	if (iterator != fScales.end())
		return iterator->second;

	// add new scale
	::Scale* scale = new ::Scale(type);
	fScales[type] = scale;

	return scale;
}


#ifdef __HAIKU__
BLayoutItem*
ActivityView::CreateHistoryLayoutItem()
{
	if (fHistoryLayoutItem == NULL)
		fHistoryLayoutItem = new HistoryLayoutItem(this);

	return fHistoryLayoutItem;
}


BLayoutItem*
ActivityView::CreateLegendLayoutItem()
{
	if (fLegendLayoutItem == NULL)
		fLegendLayoutItem = new LegendLayoutItem(this);

	return fLegendLayoutItem;
}
#endif


DataSource*
ActivityView::FindDataSource(const DataSource* search)
{
	BAutolock _(fSourcesLock);

	for (int32 i = fSources.CountItems(); i-- > 0;) {
		DataSource* source = fSources.ItemAt(i);
		if (!strcmp(source->Name(), search->Name()))
			return source;
	}

	return NULL;
}


status_t
ActivityView::AddDataSource(const DataSource* source, const BMessage* state)
{
	if (source == NULL)
		return B_BAD_VALUE;

	BAutolock _(fSourcesLock);

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

		values->SetScale(_ScaleFor(source->ScaleType()));

		DataSource* copy;
		if (source->PerCPU())
			copy = source->CopyForCPU(i);
		else
			copy = source->Copy();

		BString colorName = source->Name();
		colorName << " color";
		if (state != NULL) {
			const rgb_color* color = NULL;
			ssize_t colorLength;
			if (state->FindData(colorName.String(), B_RGB_COLOR_TYPE, i,
					(const void**)&color, &colorLength) == B_OK
				&& colorLength == sizeof(rgb_color))
				copy->SetColor(*color);
		}

		if (!fSources.AddItem(copy, insert + i)) {
			fValues.RemoveItem(values);
			delete values;
			return B_NO_MEMORY;
		}
	}

#ifdef __HAIKU__
	InvalidateLayout();
#endif
	return B_OK;
}


status_t
ActivityView::RemoveDataSource(const DataSource* remove)
{
	bool removed = false;

	BAutolock _(fSourcesLock);

	while (true) {
		DataSource* source = FindDataSource(remove);
debug_printf("SEARCH %s, found %p\n", remove->Name(), source);
		if (source == NULL) {
			if (removed)
				break;
			return B_ENTRY_NOT_FOUND;
		}

		int32 index = fSources.IndexOf(source);
		if (index < 0)
			return B_ENTRY_NOT_FOUND;

		fSources.RemoveItemAt(index);
		delete source;
		DataHistory* values = fValues.RemoveItemAt(index);
		delete values;
		removed = true;
	}

#ifdef __HAIKU__
	InvalidateLayout();
#endif
	return B_OK;
}


void
ActivityView::RemoveAllDataSources()
{
	BAutolock _(fSourcesLock);

	fSources.MakeEmpty();
	fValues.MakeEmpty();
}


void
ActivityView::AttachedToWindow()
{
	Looper()->AddHandler(fSystemInfoHandler);
	fSystemInfoHandler->StartWatching();

	fRefreshSem = create_sem(0, "refresh sem");
	fRefreshThread = spawn_thread(&_RefreshThread, "source refresh",
		B_NORMAL_PRIORITY, this);
	resume_thread(fRefreshThread);

	FrameResized(Bounds().Width(), Bounds().Height());
}


void
ActivityView::DetachedFromWindow()
{
	fSystemInfoHandler->StopWatching();
	Looper()->RemoveHandler(fSystemInfoHandler);

	delete_sem(fRefreshSem);
	wait_for_thread(fRefreshThread, NULL);
}


#ifdef __HAIKU__
BSize
ActivityView::MinSize()
{
	BSize size(32, 32);
	if (fShowLegend)
		size.height = _LegendHeight();

	return size;
}
#endif


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
	view->SetViewColor(fHistoryBackgroundColor);
	view->SetLowColor(view->ViewColor());
	fOffscreen->AddChild(view);
}


BView*
ActivityView::_OffscreenView()
{
	if (fOffscreen == NULL)
		return NULL;

	return fOffscreen->ChildAt(0);
}


void
ActivityView::MouseDown(BPoint where)
{
	int32 buttons = B_SECONDARY_MOUSE_BUTTON;
	if (Looper() != NULL && Looper()->CurrentMessage() != NULL)
		Looper()->CurrentMessage()->FindInt32("buttons", &buttons);

	if (buttons == B_PRIMARY_MOUSE_BUTTON) {
		fZoomPoint = where;
		fOriginalResolution = fDrawResolution;
		fZooming = true;
		return;
	}

	BPopUpMenu *menu = new BPopUpMenu(B_EMPTY_STRING, false, false);
	menu->SetFont(be_plain_font);

	BMenu* additionalMenu = new BMenu("Additional Items");
	additionalMenu->SetFont(be_plain_font);

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

		if (source->Primary())
			menu->AddItem(item);
		else
			additionalMenu->AddItem(item);
	}

	menu->AddItem(new BMenuItem(additionalMenu));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(fShowLegend ? "Hide Legend" : "Show Legend",
		new BMessage(kMsgToggleLegend)));

	menu->SetTargetForItems(this);
	additionalMenu->SetTargetForItems(this);

	ActivityWindow* window = dynamic_cast<ActivityWindow*>(Window());
	if (window != NULL && window->ActivityViewCount() > 1) {
		menu->AddSeparatorItem();
		BMessage* message = new BMessage(kMsgRemoveView);
		message->AddPointer("view", this);
		menu->AddItem(item = new BMenuItem("Remove View", message));
		item->SetTarget(window);
	}

	ConvertToScreen(&where);
	menu->Go(where, true, false, true);

}


void
ActivityView::MouseUp(BPoint where)
{
	fZooming = false;
}


void
ActivityView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	if (!fZooming)
		return;

	int32 previousResolution = fDrawResolution;

	int32 shift = int32(where.x - fZoomPoint.x) / 25;
	if (shift > 0)
		fDrawResolution = fOriginalResolution << shift;
	else
		fDrawResolution = fOriginalResolution >> -shift;

	if (fDrawResolution < 1)
		fDrawResolution = 1;
	if (fDrawResolution > 128)
		fDrawResolution = 128;

	if (previousResolution != fDrawResolution)
		Invalidate();
}


void
ActivityView::MessageReceived(BMessage* message)
{
	// if a color is dropped, use it as background
	if (message->WasDropped()) {
		rgb_color* color;
		ssize_t size;
		if (message->FindData("RGBColor", B_RGB_COLOR_TYPE, 0,
				(const void**)&color, &size) == B_OK
			&& size == sizeof(rgb_color)) {
			BPoint dropPoint = message->DropPoint();
			ConvertFromScreen(&dropPoint);

			if (_HistoryFrame().Contains(dropPoint)) {
				fHistoryBackgroundColor = *color;
				Invalidate(_HistoryFrame());
			} else {
				// check each legend color box
				BAutolock _(fSourcesLock);

				BRect legendFrame = _LegendFrame();
				for (int32 i = 0; i < fSources.CountItems(); i++) {
					BRect frame = _LegendColorFrameAt(legendFrame, i);
					if (frame.Contains(dropPoint)) {
						fSources.ItemAt(i)->SetColor(*color);
						Invalidate(_HistoryFrame());
						Invalidate(frame);
						return;
					}
				}

				if (dynamic_cast<ActivityMonitor*>(be_app) == NULL) {
					// allow background color change in the replicant only
					fLegendBackgroundColor = *color;
					SetLowColor(fLegendBackgroundColor);
					Invalidate(legendFrame);
				}
			}
			return;
		}
	}

	switch (message->what) {
		case B_ABOUT_REQUESTED:
			ActivityMonitor::ShowAbout();
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


void
ActivityView::_UpdateFrame()
{
#ifdef __HAIKU__
	if (fLegendLayoutItem == NULL || fHistoryLayoutItem == NULL)
		return;

	BRect historyFrame = fHistoryLayoutItem->Frame();
	BRect legendFrame = fLegendLayoutItem->Frame();
#else
	BRect historyFrame = Bounds();
	BRect legendFrame = Bounds();
	historyFrame.bottom -= 2 * Bounds().Height() / 3;
	legendFrame.top += Bounds().Height() / 3;
#endif
	MoveTo(historyFrame.left, historyFrame.top);
	ResizeTo(legendFrame.left + legendFrame.Width() - historyFrame.left,
		legendFrame.top + legendFrame.Height() - historyFrame.top);
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


float
ActivityView::_LegendHeight() const
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);

	BAutolock _(fSourcesLock);

	int32 rows = (fSources.CountItems() + 1) / 2;
	return rows * (4 + ceilf(fontHeight.ascent)
		+ ceilf(fontHeight.descent) + ceilf(fontHeight.leading));
}


BRect
ActivityView::_LegendFrame() const
{
	float height;
#ifdef __HAIKU__
	if (fLegendLayoutItem != NULL)
		height = fLegendLayoutItem->Frame().Height();
	else
#endif
		height = _LegendHeight();

	BRect frame = Bounds();
	frame.top = frame.bottom - height;

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

	BAutolock _(fSourcesLock);

	int32 rows = (fSources.CountItems() + 1) / 2;
	float height = floorf((frame.Height() - 5) / rows);

	frame.top = frame.top + 5 + row * height;
	frame.bottom = frame.top + height - 1;

	return frame;
}


BRect
ActivityView::_LegendColorFrameAt(BRect frame, int32 index) const
{
	frame = _LegendFrameAt(frame, index);
	frame.InsetBy(1, 1);
	frame.right = frame.left + frame.Height();

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
		view = _OffscreenView();
	}

	BRect frame = _HistoryFrame();
	view->SetLowColor(fHistoryBackgroundColor);
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

	// Draw scale
	// TODO: add second markers?

	view->SetPenSize(1);

	rgb_color scaleColor = view->LowColor();
	uint32 average = (scaleColor.red + scaleColor.green + scaleColor.blue) / 3;
	if (average < 96)
		scaleColor = tint_color(scaleColor, B_LIGHTEN_2_TINT);
	else
		scaleColor = tint_color(scaleColor, B_DARKEN_2_TINT);

	view->SetHighColor(scaleColor);
	view->StrokeRect(frame);
	view->StrokeLine(BPoint(frame.left, frame.top + frame.Height() / 2),
		BPoint(frame.right, frame.top + frame.Height() / 2));

	// Draw values

	view->SetPenSize(2);
	BAutolock _(fSourcesLock);

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

	BAutolock _(fSourcesLock);
	BRect legendFrame = _LegendFrame();

	SetLowColor(fLegendBackgroundColor);
	FillRect(legendFrame, B_SOLID_LOW);

	font_height fontHeight;
	GetFontHeight(&fontHeight);

	for (int32 i = 0; i < fSources.CountItems(); i++) {
		DataSource* source = fSources.ItemAt(i);
		DataHistory* values = fValues.ItemAt(i);
		BRect frame = _LegendFrameAt(legendFrame, i);

		// draw color box
		BRect colorBox = _LegendColorFrameAt(legendFrame, i);
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
	bigtime_t lastTimeout = system_time() - fRefreshInterval;

	while (true) {
		status_t status = acquire_sem_etc(fRefreshSem, 1, B_ABSOLUTE_TIMEOUT,
			lastTimeout + fRefreshInterval);
		if (status == B_OK || status == B_BAD_SEM_ID)
			break;
		if (status == B_INTERRUPTED)
			continue;

		SystemInfo info(fSystemInfoHandler);
		lastTimeout += fRefreshInterval;

		fSourcesLock.Lock();

		for (uint32 i = fSources.CountItems(); i-- > 0;) {
			DataSource* source = fSources.ItemAt(i);
			DataHistory* values = fValues.ItemAt(i);

			int64 value = source->NextValue(info);
			values->AddValue(info.Time(), value);
		}

		fSourcesLock.Unlock();

		bigtime_t now = info.Time();
		if (fLastRefresh + fDrawInterval <= now) {
			if (LockLooper()) {
				Invalidate();
				UnlockLooper();
				fLastRefresh = now;
			}
		}
	}
}


/*static*/ status_t
ActivityView::_RefreshThread(void* self)
{
	((ActivityView*)self)->_Refresh();
	return B_OK;
}
