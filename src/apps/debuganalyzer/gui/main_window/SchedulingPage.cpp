/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "main_window/SchedulingPage.h"

#include <stdio.h>

#include <algorithm>
#include <new>

#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <ScrollView.h>
#include <SplitView.h>

#include <DebugEventStream.h>

#include <thread_defs.h>

#include "Array.h"

#include "chart/NanotimeChartAxisLegendSource.h"
#include "chart/LegendChartAxis.h"
#include "chart/StringChartLegend.h"
#include "HeaderView.h"
#include "Model.h"


static const float kThreadNameMargin = 3.0f;
static const float kViewSeparationMargin = 1.0f;


struct MainWindow::SchedulingPage::SchedulingEvent {
	nanotime_t						time;
	Model::ThreadWaitObjectGroup*	waitObject;
	ThreadState						state;

	SchedulingEvent(nanotime_t time, ThreadState state,
		Model::ThreadWaitObjectGroup* waitObject)
		:
		time(time),
		waitObject(waitObject),
		state(state)
	{
	}
};


class MainWindow::SchedulingPage::SchedulingData {
public:
	SchedulingData()
		:
		fModel(NULL),
		fDataArrays(NULL),
		fRecordingEnabled(false)
	{
	}

	status_t InitCheck() const
	{
		return fDataArrays != NULL ? B_OK : B_NO_MEMORY;
	}

	void SetModel(Model* model)
	{
		delete[] fDataArrays;

		fModel = model;
		fDataArrays = NULL;

		if (fModel != NULL)
			fDataArrays = new(std::nothrow) DataArray[fModel->CountThreads()];
	}

	void SetRecordingEnabled(bool enabled)
	{
		fRecordingEnabled = enabled;
	}

	void Clear()
	{
		if (fDataArrays == NULL)
			return;

		int32 count = fModel->CountThreads();
		for (int32 i = 0; i < count; i++)
			fDataArrays[i].Clear();
	}

	const Array<SchedulingEvent>& EventsForThread(int32 index)
	{
		return fDataArrays[index];
	}

	void AddState(Model::Thread* thread, nanotime_t time, ThreadState state,
		Model::ThreadWaitObjectGroup* waitObject)
	{
		DataArray& array = fDataArrays[thread->Index()];
		if (!array.IsEmpty()) {
			SchedulingEvent& lastEvent = array[array.Size() - 1];
			if (fRecordingEnabled) {
				if (lastEvent.state == state
					&& lastEvent.waitObject == waitObject) {
					return;
				}
			} else {
				// recording not enabled yet -- overwrite the last event
				lastEvent = SchedulingEvent(time, state, waitObject);
				return;
			}
		}

		SchedulingEvent event(time, state, waitObject);
		array.Add(event);
	}

	void AddRun(Model::Thread* thread, nanotime_t time)
	{
		AddState(thread, time, RUNNING, NULL);
	}

	void AddLatency(Model::Thread* thread, nanotime_t time)
	{
		AddState(thread, time, READY, NULL);
	}

	void AddPreemption(Model::Thread* thread, nanotime_t time)
	{
		AddState(thread, time, PREEMPTED, NULL);
	}

	void AddWait(Model::Thread* thread, nanotime_t time,
		Model::ThreadWaitObjectGroup* waitObject)
	{
		AddState(thread, time, WAITING, waitObject);
	}

	void AddUnspecifiedWait(Model::Thread* thread, nanotime_t time)
	{
		AddState(thread, time, WAITING, NULL);
	}

private:
	typedef Array<SchedulingEvent> DataArray;

private:
	Model*		fModel;
	DataArray*	fDataArrays;
	bool		fRecordingEnabled;
};


struct MainWindow::SchedulingPage::TimeRange : BReferenceable {
	nanotime_t	startTime;
	nanotime_t	endTime;

	TimeRange(nanotime_t startTime, nanotime_t endTime)
		:
		startTime(startTime),
		endTime(endTime)
	{
	}
};


class MainWindow::SchedulingPage::TimelineHeaderRenderer
	: public HeaderRenderer {
public:
	TimelineHeaderRenderer()
		:
		fAxis(new NanotimeChartAxisLegendSource, new StringChartLegendRenderer)
	{
		fAxis.SetLocation(CHART_AXIS_TOP);
	}

	virtual float HeaderHeight(BView* view, const Header* header)
	{
		_SetupAxis(view, header);
		return fAxis.PreferredSize(view, view->Frame().Size()).height;
	}

	virtual float PreferredHeaderWidth(BView* view, const Header* header)
	{
		_SetupAxis(view, header);
		return fAxis.PreferredSize(view, view->Frame().Size()).width;
	}

	virtual void DrawHeader(BView* view, BRect frame, BRect updateRect,
		const Header* header, uint32 flags)
	{
		_SetupAxis(view, header);
		fAxis.SetFrame(frame);
		DrawHeaderBackground(view, frame, updateRect, flags);
		fAxis.Render(view, updateRect);
	}

private:
	void _SetupAxis(BView* view, const Header* header)
	{
		BVariant value;
		if (header->GetValue(value)) {
			TimeRange* timeRange = dynamic_cast<TimeRange*>(
				value.ToReferenceable());
			if (timeRange != NULL) {
				ChartDataRange range = ChartDataRange(timeRange->startTime,
					timeRange->endTime);
				if (range != fRange) {
					fAxis.SetRange(range);
					fRange = range;
				}
			}
		}
	}

private:
	LegendChartAxis		fAxis;
	ChartDataRange		fRange;
};


class MainWindow::SchedulingPage::BaseView : public BView {
public:
	BaseView(const char* name, FontInfo& fontInfo, uint32 flags = 0)
		:
		BView(name, flags),
		fModel(NULL),
		fFontInfo(fontInfo)
	{
	}

	virtual void SetModel(Model* model)
	{
		fModel = model;

		InvalidateLayout();
	}

protected:
	int32 CountLines() const
	{
		return fModel != NULL ? fModel->CountThreads() : 0;
	}

	float TotalHeight() const
	{
		return fFontInfo.lineHeight * CountLines();
	}

	int32 LineAt(BPoint point) const
	{
		int32 line = (int32)point.y / (int32)fFontInfo.lineHeight;
		return line < CountLines() ? line : -1;
	}

	void GetLineRange(BRect rect, int32& minLine, int32& maxLine) const
	{
		int32 lineHeight = (int32)fFontInfo.lineHeight;
		minLine = (int32)rect.top / lineHeight;
		maxLine = ((int32)ceilf(rect.bottom) + lineHeight - 1) / lineHeight;
		minLine = std::max(minLine, 0L);
		maxLine = std::min(maxLine, CountLines() - 1);
	}

	BRect LineRect(uint32 line) const
	{
		float y = (float)line * fFontInfo.lineHeight;
		return BRect(0, y, Bounds().right, y + fFontInfo.lineHeight - 1);
	}

protected:
	Model*		fModel;
	FontInfo&	fFontInfo;
};


class MainWindow::SchedulingPage::LineBaseView : public BaseView,
	protected ListSelectionModel::Listener {
public:
	LineBaseView(const char* name, FontInfo& fontInfo,
		ListSelectionModel* selectionModel)
		:
		BaseView(name, fontInfo, B_WILL_DRAW),
		fSelectionModel(selectionModel),
		fTextColor(ui_color(B_DOCUMENT_TEXT_COLOR)),
		fSelectedTextColor(fTextColor),
		fBackgroundColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR)),
		fSelectedBackgroundColor(tint_color(fBackgroundColor, B_DARKEN_2_TINT))
	{
		fSelectionModel->AddListener(this);
	}

	void RemoveListeners()
	{
		fSelectionModel->RemoveListener(this);
	}

	virtual void MouseDown(BPoint where)
	{
		// get buttons and modifiers
		BMessage* message = Looper()->CurrentMessage();
		if (message == NULL)
			return;

		int32 buttons;
		if (message->FindInt32("buttons", &buttons) != B_OK)
			return;

		int32 modifiers;
		if (message->FindInt32("modifiers", &modifiers) != B_OK)
			modifiers = 0;

		int32 line = LineAt(where);
		if (line >= 0) {
			if ((modifiers & B_SHIFT_KEY) != 0) {
				int32 selectedLines = fSelectionModel->CountSelectedItems();
				if (selectedLines > 0) {
					int32 firstLine = fSelectionModel->SelectedItemAt(0);
					int32 lastLine = fSelectionModel->SelectedItemAt(
						selectedLines - 1);
					firstLine = std::min(firstLine, line);
					lastLine = std::max(lastLine, line);
					fSelectionModel->SelectItems(firstLine,
						lastLine - firstLine + 1, false);
				} else
					fSelectionModel->SelectItem(line, true);
			} else {
				if (fSelectionModel->IsItemSelected(line)) {
					if ((modifiers & B_COMMAND_KEY) != 0)
						fSelectionModel->DeselectItem(line);
				} else {
					fSelectionModel->SelectItem(line,
						(modifiers & B_COMMAND_KEY) != 0);
				}
			}
		} else
			fSelectionModel->Clear();
	}

	virtual void ItemsSelected(ListSelectionModel* model, int32 index,
		int32 count)
	{
		InvalidateLines(index, count);
	}

	virtual void ItemsDeselected(ListSelectionModel* model, int32 index,
		int32 count)
	{
		InvalidateLines(index, count);
	}

	void InvalidateLines(int32 index, int32 count)
	{
		float top = (float)index * fFontInfo.lineHeight;
		float bottom = (float)(index + count) * fFontInfo.lineHeight - 1;
		BRect bounds(Bounds());
		Invalidate(BRect(bounds.left, top, bounds.right, bottom));
	}

protected:
	ListSelectionModel*		fSelectionModel;
	rgb_color				fTextColor;
	rgb_color				fSelectedTextColor;
	rgb_color				fBackgroundColor;
	rgb_color				fSelectedBackgroundColor;
};


class MainWindow::SchedulingPage::ThreadsView : public LineBaseView {
public:
	ThreadsView(FontInfo& fontInfo, ListSelectionModel* selectionModel)
		:
		LineBaseView("threads", fontInfo, selectionModel)
	{
		SetViewColor(B_TRANSPARENT_32_BIT);
	}

	~ThreadsView()
	{
	}

	virtual void Draw(BRect updateRect)
	{
		if (fModel == NULL)
			return;

		// get the lines intersecting with the update rect
		int32 minLine, maxLine;
		GetLineRange(updateRect, minLine, maxLine);

		for (int32 i = minLine; i <= maxLine; i++) {
			// draw the line background
			BRect lineRect = LineRect(i);
			if (fSelectionModel->IsItemSelected(i)) {
				SetLowColor(fSelectedBackgroundColor);
				SetHighColor(fTextColor);
			} else {
				SetLowColor(fBackgroundColor);
				SetHighColor(fSelectedTextColor);
			}
			FillRect(lineRect, B_SOLID_LOW);

			// draw the string
			float y = lineRect.bottom - fFontInfo.fontHeight.descent + 1;
			DrawString(fModel->ThreadAt(i)->Name(),
				BPoint(kThreadNameMargin, y));
		}

		BRect bounds(Bounds());
		BRect lowerRect(0, (float)CountLines() * fFontInfo.lineHeight,
			bounds.Width(), bounds.Height());
		if (lowerRect.Intersects(updateRect)) {
			SetHighColor(fBackgroundColor);
			FillRect(lowerRect);
		}
	}

	virtual BSize MinSize()
	{
		return BSize(100, TotalHeight());
	}

	virtual BSize MaxSize()
	{
		return BSize(1000, B_SIZE_UNLIMITED);
	}
};


class MainWindow::SchedulingPage::SchedulingView : public LineBaseView {
public:
	struct Listener {
		virtual ~Listener()
		{
		}

		virtual void DataWidthChanged() = 0;
		virtual void DataRangeChanged() = 0;
	};

private:
	enum {
		COLOR_RUNNING = 0,
		COLOR_PREEMPTED,
		COLOR_READY,
		ACTIVITY_COLOR_COUNT
	};

public:
	SchedulingView(FontInfo& fontInfo, ListSelectionModel* selectionModel)
		:
		LineBaseView("scheduling", fontInfo, selectionModel),
		fStartTime(0),
		fEndTime(0),
		fNSecsPerPixel(1000000),
		fLastMousePos(-1, -1),
		fListener(NULL)
	{
		fActivityColors[COLOR_RUNNING].set_to(0, 255, 0);
		fActivityColors[COLOR_PREEMPTED].set_to(255, 127, 0);
		fActivityColors[COLOR_READY].set_to(255, 0, 0);
		fActivitySelectedColors[COLOR_RUNNING] = tint_color(
			fActivityColors[COLOR_RUNNING], B_DARKEN_2_TINT);
		fActivitySelectedColors[COLOR_PREEMPTED] = tint_color(
			fActivityColors[COLOR_PREEMPTED], B_DARKEN_2_TINT);
		fActivitySelectedColors[COLOR_READY] = tint_color(
			fActivityColors[COLOR_READY], B_DARKEN_2_TINT);

	}

	~SchedulingView()
	{
	}

	virtual void SetModel(Model* model)
	{
		LineBaseView::SetModel(model);
		fSchedulingData.SetModel(model);
		fStartTime = 0;
		fEndTime = 0;

		if (fListener != NULL) {
			fListener->DataWidthChanged();
			fListener->DataRangeChanged();
		}

	}

	void SetListener(Listener* listener)
	{
		fListener = listener;
	}

	void UpdateScrollBar()
	{
		float width = Frame().Width();
		float dataWidth = std::max(width, MinSize().width);

		if (BScrollBar* scrollBar = ScrollBar(B_HORIZONTAL)) {
			float range = dataWidth - width;
			if (range > 0) {
				scrollBar->SetRange(0, range);
				scrollBar->SetProportion((width + 1) / (dataWidth + 1));
				scrollBar->SetSteps(fFontInfo.lineHeight, width + 1);
			} else {
				scrollBar->SetRange(0, 0);
				scrollBar->SetProportion(1);
			}
		}
	}

	void GetDataRange(nanotime_t& _startTime, nanotime_t& _endTime)
	{
		_GetEventTimeRange(_startTime, _endTime);
	}

	virtual BSize MinSize()
	{
		nanotime_t timeSpan = fModel != NULL ? fModel->LastEventTime() : 0;
		float width = std::max(float(timeSpan / fNSecsPerPixel), 100.0f);
		return BSize(width, TotalHeight());
	}

	virtual BSize MaxSize()
	{
		return BSize(MinSize().width, B_SIZE_UNLIMITED);
	}

	virtual void ScrollTo(BPoint where)
	{
		LineBaseView::ScrollTo(where);
		fStartTime = 0;
		fEndTime = 0;

		if (fListener != NULL)
			fListener->DataRangeChanged();
	}

	void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case B_MOUSE_WHEEL_CHANGED:
			{
				// We're only interested in Shift + vertical wheel.
				float deltaY;
				if ((modifiers() & B_SHIFT_KEY) == 0
					|| message->FindFloat("be:wheel_delta_y", &deltaY)
						!= B_OK) {
					break;
				}

				_Zoom(fLastMousePos.x, deltaY);

				return;
			}
		}

		BView::MessageReceived(message);
	}

	void MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
	{
		fLastMousePos = where - LeftTop();

//		if (fDraggingStartPos.x < 0)
//			return;
//
//		ScrollBar(B_HORIZONTAL)->SetValue(fDraggingStartScrollValue
//			+ fDraggingStartPos.x - where.x);
	}

	virtual void Draw(BRect updateRect)
	{
		if (fModel == NULL || fSchedulingData.InitCheck() != B_OK)
			return;

		_UpdateData();

		// draw the events
// TODO: Draw only the threads currently visible.
		int32 threadCount = fModel->CountThreads();
		for (int32 i = 0; i < threadCount; i++) {
			// draw the background
			const rgb_color* activityColors;
			if (fSelectionModel->IsItemSelected(i)) {
				activityColors = fActivitySelectedColors;
				SetLowColor(fSelectedBackgroundColor);
			} else {
				activityColors = fActivityColors;
				SetLowColor(fBackgroundColor);
			}
			FillRect(LineRect(i), B_SOLID_LOW);

			Model::Thread* thread = fModel->ThreadAt(i);
			const Array<SchedulingEvent>& events
				= fSchedulingData.EventsForThread(thread->Index());

			int32 eventCount = events.Size();
//printf("drawing events for thread %ld: %ld events\n", thread->Index(), eventCount);
			for (int32 k = 0; k < eventCount; k++) {
				const SchedulingEvent& event = events[k];
				nanotime_t startTime = std::max(event.time, fStartTime);
				nanotime_t endTime = k + 1 < eventCount
					? std::min(events[k + 1].time, fEndTime) : fEndTime;

				switch (event.state) {
					case RUNNING:
					case STILL_RUNNING:
						SetHighColor(activityColors[COLOR_RUNNING]);
						break;
					case PREEMPTED:
						SetHighColor(activityColors[COLOR_PREEMPTED]);
						break;
					case READY:
						SetHighColor(activityColors[COLOR_READY]);
						break;
					case WAITING:
					case UNKNOWN:
					default:
						continue;
				}

				BRect rect = LineRect(i);
				rect.left = startTime / fNSecsPerPixel;
				rect.right = endTime / fNSecsPerPixel - 1;
				FillRect(rect);
			}
		}
	}

private:
	// shorthands for the longish structure names
	typedef system_profiler_thread_enqueued_in_run_queue
		thread_enqueued_in_run_queue;
	typedef system_profiler_thread_removed_from_run_queue
		thread_removed_from_run_queue;

private:
	void _UpdateData()
	{
		// get the interesting event time range
		nanotime_t startTime;
		nanotime_t endTime;
		_GetEventTimeRange(startTime, endTime);

		if (startTime == fStartTime && endTime == fEndTime)
			return;
		fStartTime = startTime;
		fEndTime = endTime;

		fSchedulingData.Clear();

//printf("MainWindow::SchedulingPage::SchedulingView::_UpdateData()\n");
//printf("  time range: %lld - %lld\n", startTime, endTime);

		// get a scheduling state close to our start time
		const Model::CompactSchedulingState* compactState
			= fModel->ClosestSchedulingState(startTime);
//printf("  compactState: %p\n", compactState);
		fState.Clear();
		status_t error = fState.Init(compactState);
		if (error != B_OK)
			return;

		// init the event stream
		BDebugEventInputStream input;
		error = input.SetTo((uint8*)fModel->EventData(),
			fModel->EventDataSize(), false);
		if (error == B_OK && compactState != NULL)
			error = input.Seek(compactState->EventOffset());
//printf("  event offset: %lld, input init error: %s\n", compactState != NULL ? compactState->EventOffset() : 0, strerror(error));
		if (error != B_OK)
			return;

		fSchedulingData.SetRecordingEnabled(
			fState.LastEventTime() >= startTime);

		// add the initial thread states to the scheduling data
		if (compactState != NULL) {
			int32 threadStateCount = compactState->CountThreadsStates();
			for (int32 i = 0; i < threadStateCount; i++) {
				const Model::CompactThreadSchedulingState* threadState
					= compactState->ThreadStateAt(i);
				switch (threadState->state) {
					case RUNNING:
					case STILL_RUNNING:
						fSchedulingData.AddRun(threadState->thread,
							threadState->lastTime);
						break;
					case PREEMPTED:
						fSchedulingData.AddPreemption(threadState->thread,
							threadState->lastTime);
						break;
					case READY:
						fSchedulingData.AddLatency(threadState->thread,
							threadState->lastTime);
						break;
					case WAITING:
					{
						Model::ThreadWaitObjectGroup* group = NULL;
						if (threadState->waitObject != NULL) {
							group = fModel->ThreadWaitObjectGroupFor(
								threadState->ID(),
								threadState->waitObject->Type(),
								threadState->waitObject->Object());
						}
						fSchedulingData.AddWait(threadState->thread,
							threadState->lastTime, group);
						break;
					}
					case UNKNOWN:
					default:
						break;
				}
			}
		}

		// process the events
		while (true) {
			// get next event
			uint32 event;
			uint32 cpu;
			const void* buffer;
			off_t offset;
			ssize_t bufferSize = input.ReadNextEvent(&event, &cpu, &buffer,
				&offset);
			if (bufferSize < 0)
{
printf("failed to read event!\n");
				return;
}
			if (buffer == NULL)
				break;

			// process the event
			error = _ProcessEvent(event, cpu, buffer, bufferSize);
			if (error != B_OK)
				return;

			if (fState.LastEventTime() >= startTime)
				fSchedulingData.SetRecordingEnabled(true);
			if (fState.LastEventTime() >= endTime)
				break;
		}
	}

	void _GetEventTimeRange(nanotime_t& _startTime, nanotime_t& _endTime)
	{
		if (fModel != NULL) {
			float scrollOffset = _ScrollOffset();
			_startTime = (nanotime_t)scrollOffset * fNSecsPerPixel;
			_endTime = (nanotime_t)(scrollOffset + Bounds().Width() + 1)
				* fNSecsPerPixel;
		} else {
			_startTime = 0;
			_endTime = 1;
		}
	}

	float _ScrollOffset() const
	{
		if (BScrollBar* scrollBar = ScrollBar(B_HORIZONTAL))
			return scrollBar->Value();
		return 0;
	}

	void _Zoom(float x, float steps)
	{
		if (steps == 0 || fModel == NULL)
			return;

		// compute the domain point where to zoom in
		float scrollOffset = _ScrollOffset();
		double timeForX = (scrollOffset + x) * fNSecsPerPixel;

		uint32 factor = 4;
		if (steps < 0) {
			steps = -steps;
			factor = 1;
		}

		uint32 oldNsecPerPixel = fNSecsPerPixel;
		for (; steps > 0; steps--)
			fNSecsPerPixel = fNSecsPerPixel * factor / 2;

		if (fNSecsPerPixel < 1)
			fNSecsPerPixel = 1;
		else if (fNSecsPerPixel > 1000000)
			fNSecsPerPixel = 1000000;

		if (fNSecsPerPixel == oldNsecPerPixel)
			return;

		Invalidate();

		UpdateScrollBar();
		if (BScrollBar* scrollBar = ScrollBar(B_HORIZONTAL))
			scrollBar->SetValue(timeForX / fNSecsPerPixel - x);

		if (fListener != NULL) {
			fListener->DataWidthChanged();
			fListener->DataRangeChanged();
		}
	}

	inline void _UpdateLastEventTime(nanotime_t time)
	{
		fState.SetLastEventTime(time - fModel->BaseTime());
	}

	status_t _ProcessEvent(uint32 event, uint32 cpu, const void* buffer,
		size_t size)
	{
		switch (event) {
			case B_SYSTEM_PROFILER_TEAM_ADDED:
			case B_SYSTEM_PROFILER_TEAM_REMOVED:
			case B_SYSTEM_PROFILER_TEAM_EXEC:
				break;

			case B_SYSTEM_PROFILER_THREAD_ADDED:
				_HandleThreadAdded((system_profiler_thread_added*)buffer);
				break;

			case B_SYSTEM_PROFILER_THREAD_REMOVED:
				_HandleThreadRemoved((system_profiler_thread_removed*)buffer);
				break;

			case B_SYSTEM_PROFILER_THREAD_SCHEDULED:
				_HandleThreadScheduled(
					(system_profiler_thread_scheduled*)buffer);
				break;

			case B_SYSTEM_PROFILER_THREAD_ENQUEUED_IN_RUN_QUEUE:
				_HandleThreadEnqueuedInRunQueue(
					(thread_enqueued_in_run_queue*)buffer);
				break;

			case B_SYSTEM_PROFILER_THREAD_REMOVED_FROM_RUN_QUEUE:
				_HandleThreadRemovedFromRunQueue(
					(thread_removed_from_run_queue*)buffer);
				break;

			case B_SYSTEM_PROFILER_WAIT_OBJECT_INFO:
				break;

			default:
printf("unsupported event type %lu, size: %lu\n", event, size);
return B_BAD_DATA;
				break;
		}

		return B_OK;
	}

	void _HandleThreadAdded(system_profiler_thread_added* event)
	{
//printf("  thread added: %ld\n", event->thread);
		// do we know the thread already?
		Model::ThreadSchedulingState* info = fState.LookupThread(event->thread);
		if (info != NULL) {
			// TODO: ?
			return;
		}

		Model::Thread* thread = fModel->ThreadByID(event->thread);
		if (thread == NULL)
			return;

		// create and add a ThreadSchedulingState
		info = new(std::nothrow) Model::ThreadSchedulingState(thread);
		if (info == NULL)
			return;

		fState.InsertThread(info);
	}

	void _HandleThreadRemoved(system_profiler_thread_removed* event)
	{
//printf("  thread removed: %ld\n", event->thread);
//		Model::ThreadSchedulingState* thread = fState.LookupThread(
//			event->thread);
//		if (thread != NULL) {
//			fState.RemoveThread(thread);
//			delete thread;
//// TODO: The thread will be unscheduled in a moment and cause a warning! So
//// maybe keep it around in a separate hash table a bit longer?
//		}
	}

	void _HandleThreadScheduled(system_profiler_thread_scheduled* event)
	{
		_UpdateLastEventTime(event->time);

		Model::ThreadSchedulingState* thread = fState.LookupThread(
			event->thread);
		if (thread == NULL) {
			printf("Schedule event for unknown thread: %ld\n", event->thread);
			return;
		}

		thread->lastTime = fState.LastEventTime();
		thread->state = RUNNING;
		fSchedulingData.AddRun(thread->thread, fState.LastEventTime());

		// unscheduled thread

		if (event->thread == event->previous_thread)
			return;

		thread = fState.LookupThread(event->previous_thread);
		if (thread == NULL) {
			printf("Schedule event for unknown previous thread: %ld\n",
				event->previous_thread);
			return;
		}

		if (thread->state == STILL_RUNNING) {
			// thread preempted
			fSchedulingData.AddPreemption(thread->thread,
				fState.LastEventTime());

			thread->lastTime = fState.LastEventTime();
			thread->state = PREEMPTED;
		} else if (thread->state == RUNNING) {
			// thread starts waiting (it hadn't been added to the run
			// queue before being unscheduled)
			if (event->previous_thread_state == B_THREAD_WAITING) {
				addr_t waitObject = event->previous_thread_wait_object;
				switch (event->previous_thread_wait_object_type) {
					case THREAD_BLOCK_TYPE_SNOOZE:
					case THREAD_BLOCK_TYPE_SIGNAL:
						waitObject = 0;
						break;
					case THREAD_BLOCK_TYPE_SEMAPHORE:
					case THREAD_BLOCK_TYPE_CONDITION_VARIABLE:
					case THREAD_BLOCK_TYPE_MUTEX:
					case THREAD_BLOCK_TYPE_RW_LOCK:
					case THREAD_BLOCK_TYPE_OTHER:
					default:
						break;
				}

				fSchedulingData.AddWait(thread->thread, fState.LastEventTime(),
					fModel->ThreadWaitObjectGroupFor(thread->ID(),
						event->previous_thread_wait_object_type, waitObject));
			} else {
				fSchedulingData.AddUnspecifiedWait(thread->thread,
					fState.LastEventTime());
			}

			thread->lastTime = fState.LastEventTime();
			thread->state = WAITING;
		} else if (thread->state == UNKNOWN) {
			uint32 threadState = event->previous_thread_state;
			if (threadState == B_THREAD_WAITING
				|| threadState == B_THREAD_SUSPENDED) {
				fSchedulingData.AddWait(thread->thread, fState.LastEventTime(),
					NULL);
				thread->lastTime = fState.LastEventTime();
				thread->state = WAITING;
			} else if (threadState == B_THREAD_READY) {
				thread->lastTime = fState.LastEventTime();
				thread->state = PREEMPTED;
				fSchedulingData.AddPreemption(thread->thread,
					fState.LastEventTime());
			}
		}
	}

	void _HandleThreadEnqueuedInRunQueue(thread_enqueued_in_run_queue* event)
	{
		_UpdateLastEventTime(event->time);

		Model::ThreadSchedulingState* thread = fState.LookupThread(
			event->thread);
		if (thread == NULL) {
			printf("Enqueued in run queue event for unknown thread: %ld\n",
				event->thread);
			return;
		}

		if (thread->state == RUNNING || thread->state == STILL_RUNNING) {
			// Thread was running and is reentered into the run queue. This
			// is done by the scheduler, if the thread remains ready.
			thread->state = STILL_RUNNING;
		} else {
			// Thread was waiting and is ready now.
			nanotime_t diffTime = fState.LastEventTime() - thread->lastTime;
			if (thread->waitObject != NULL) {
				thread->waitObject->AddWait(diffTime);
				thread->waitObject = NULL;
			}

			fSchedulingData.AddLatency(thread->thread, fState.LastEventTime());
			thread->lastTime = fState.LastEventTime();
			thread->state = READY;
		}
	}

	void _HandleThreadRemovedFromRunQueue(thread_removed_from_run_queue* event)
	{
		_UpdateLastEventTime(event->time);

		Model::ThreadSchedulingState* thread = fState.LookupThread(
			event->thread);
		if (thread == NULL) {
			printf("Removed from run queue event for unknown thread: %ld\n",
				event->thread);
			return;
		}

		// This really only happens when the thread priority is changed
		// while the thread is ready.
		fSchedulingData.AddUnspecifiedWait(thread->thread,
			fState.LastEventTime());

		thread->lastTime = fState.LastEventTime();
		thread->state = WAITING;
	}

private:
	Model::SchedulingState	fState;
	SchedulingData			fSchedulingData;
	nanotime_t				fStartTime;
	nanotime_t				fEndTime;
	uint32					fNSecsPerPixel;
	BPoint					fLastMousePos;
	Listener*				fListener;

	rgb_color				fActivityColors[ACTIVITY_COLOR_COUNT];
	rgb_color				fActivitySelectedColors[ACTIVITY_COLOR_COUNT];
};


class MainWindow::SchedulingPage::ViewPort : public BaseView,
	private HeaderListener, private SchedulingView::Listener {
public:
	ViewPort(HeaderView* headerView, ThreadsView* threadsView,
		SchedulingView* schedulingView, FontInfo& fontInfo)
		:
		BaseView("viewport", fontInfo),
		fHeaderView(headerView),
		fThreadsView(threadsView),
		fSchedulingView(schedulingView)
	{
		AddChild(threadsView);
		AddChild(schedulingView);

		fSchedulingView->SetListener(this);

		HeaderModel* headerModel = fHeaderView->Model();

		Header* header = new Header(100, 100, 10000, 200, 0);
		header->SetValue("Thread");
		headerModel->AddHeader(header);
		header->AddListener(this);

		header = new Header(100, 100, 10000, 200, 1);
			// TODO: Set header width correctly!
		header->SetValue("Activity");
		header->SetHeaderRenderer(new TimelineHeaderRenderer);
		headerModel->AddHeader(header);
//		header->AddListener(this);
	}

	~ViewPort()
	{
	}

	void RemoveListeners()
	{
		fHeaderView->Model()->HeaderAt(0)->RemoveListener(this);
	}

	void UpdateScrollBars()
	{
		float height = Frame().Height();
		float dataHeight = std::max(height, fSchedulingView->MinSize().height);

		fSchedulingView->UpdateScrollBar();

		if (BScrollBar* scrollBar = ScrollBar(B_VERTICAL)) {
			float range = dataHeight - height;
			if (range > 0) {
				scrollBar->SetRange(0, range);
				scrollBar->SetProportion(
					(height + 1) / (dataHeight + 1));
				scrollBar->SetSteps(fFontInfo.lineHeight, height + 1);
			} else {
				scrollBar->SetRange(0, 0);
				scrollBar->SetProportion(1);
			}
		}
	}

	virtual BSize MinSize()
	{
		return BSize(10, 10);
	}

	virtual BSize MaxSize()
	{
		return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
	}

	BSize PreferredSize()
	{
		BSize threadsViewSize(fThreadsView->PreferredSize());
		BSize schedulingViewSize(fSchedulingView->PreferredSize());
		return BSize(BLayoutUtils::AddDistances(
				threadsViewSize.width + kViewSeparationMargin,
				schedulingViewSize.width),
			std::max(threadsViewSize.height, schedulingViewSize.height));
	}

	void DoLayout()
	{
		float width = Bounds().Width();
		float height = fThreadsView->MinSize().Height();

		float threadsViewWidth = 0;
		if (fHeaderView->Model()->HeaderAt(0) != NULL)
			threadsViewWidth = fHeaderView->HeaderFrame(0).Width();
		threadsViewWidth = std::max(threadsViewWidth,
			fThreadsView->MinSize().width);
		threadsViewWidth = std::min(threadsViewWidth,
			fThreadsView->MaxSize().width);

		float schedulingViewLeft = threadsViewWidth + 1 + kViewSeparationMargin;
		float schedulingViewWidth = width - schedulingViewLeft;

		fThreadsView->MoveTo(0, 0);
		fThreadsView->ResizeTo(threadsViewWidth, height);

		fSchedulingView->MoveTo(schedulingViewLeft, 0);
		fSchedulingView->ResizeTo(schedulingViewWidth, height);

		if (Header* header = fHeaderView->Model()->HeaderAt(1)) {
			float headerWidth = schedulingViewWidth + 1 + kViewSeparationMargin;
			header->SetMinWidth(headerWidth);
			header->SetMaxWidth(headerWidth);
			header->SetPreferredWidth(headerWidth);
			header->SetWidth(headerWidth);
		}

		UpdateScrollBars();
	}

private:
	virtual void HeaderWidthChanged(Header* header)
	{
		if (header->ModelIndex() == 0)
			InvalidateLayout();
	}

	virtual void DataWidthChanged()
	{
	}

	virtual void DataRangeChanged()
	{
		Header* header = fHeaderView->Model()->HeaderAt(1);
		if (header == NULL)
			return;

		nanotime_t startTime;
		nanotime_t endTime;
		fSchedulingView->GetDataRange(startTime, endTime);
		TimeRange* range = new(std::nothrow) TimeRange(startTime, endTime);
		if (range != NULL) {
			header->SetValue(BVariant(range, 'time'));
			range->ReleaseReference();
		}
	}

private:
	HeaderView*		fHeaderView;
	ThreadsView*	fThreadsView;
	SchedulingView*	fSchedulingView;
};


MainWindow::SchedulingPage::SchedulingPage(MainWindow* parent)
	:
	BGroupView(B_VERTICAL),
	fParent(parent),
	fModel(NULL),
	fScrollView(NULL),
	fViewPort(NULL),
	fThreadsView(NULL),
	fSchedulingView(NULL),
	fSelectionModel()
{
	SetName("Scheduling");

	be_plain_font->GetHeight(&fFontInfo.fontHeight);
	fFontInfo.lineHeight = ceilf(fFontInfo.fontHeight.ascent)
		+ ceilf(fFontInfo.fontHeight.descent);

	HeaderView* headerView = new HeaderView;
	BView* scrollChild = BLayoutBuilder::Group<>(B_VERTICAL)
		.Add(headerView)
		.Add(fViewPort = new ViewPort(headerView,
			fThreadsView = new ThreadsView(fFontInfo, &fSelectionModel),
			fSchedulingView = new SchedulingView(fFontInfo, &fSelectionModel),
			fFontInfo))
	;

	AddChild(fScrollView = new BScrollView("scroll", scrollChild, 0, true,
		true));

	fScrollView->ScrollBar(B_HORIZONTAL)->SetTarget(fSchedulingView);
	fScrollView->ScrollBar(B_VERTICAL)->SetTarget(fViewPort);
	fViewPort->UpdateScrollBars();
}


MainWindow::SchedulingPage::~SchedulingPage()
{
	fViewPort->RemoveListeners();
	fThreadsView->RemoveListeners();
	fSchedulingView->RemoveListeners();
}


void
MainWindow::SchedulingPage::SetModel(Model* model)
{
	if (model == fModel)
		return;

	fSelectionModel.Clear();

	if (fModel != NULL) {
	}

	fModel = model;

	if (fModel != NULL) {
	}

	fViewPort->SetModel(fModel);
	fThreadsView->SetModel(fModel);
	fSchedulingView->SetModel(fModel);
}
