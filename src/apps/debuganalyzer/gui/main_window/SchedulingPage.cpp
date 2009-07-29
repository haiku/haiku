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

#include "HeaderView.h"
#include "Model.h"


static const float kThreadNameMargin = 3.0f;
static const float kViewSeparationMargin = 5.0f;


struct MainWindow::SchedulingPage::SchedulingEvent {
	bigtime_t						time;
	Model::ThreadWaitObjectGroup*	waitObject;
	ThreadState						state;

	SchedulingEvent(bigtime_t time, ThreadState state,
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

	void AddState(Model::Thread* thread, bigtime_t time, ThreadState state,
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

	void AddRun(Model::Thread* thread, bigtime_t time)
	{
		AddState(thread, time, RUNNING, NULL);
	}

	void AddLatency(Model::Thread* thread, bigtime_t time)
	{
		AddState(thread, time, READY, NULL);
	}

	void AddPreemption(Model::Thread* thread, bigtime_t time)
	{
		AddState(thread, time, PREEMPTED, NULL);
	}

	void AddWait(Model::Thread* thread, bigtime_t time,
		Model::ThreadWaitObjectGroup* waitObject)
	{
		AddState(thread, time, WAITING, waitObject);
	}

	void AddUnspecifiedWait(Model::Thread* thread, bigtime_t time)
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


class MainWindow::SchedulingPage::BaseView : public BView {
public:
	BaseView(const char* name, FontInfo& fontInfo)
		:
		BView(name, B_WILL_DRAW),
		fFontInfo(fontInfo)
	{
	}

	virtual void SetModel(Model* model)
	{
		fModel = model;

		InvalidateLayout();
	}

protected:
	int32 _CountLines() const
	{
		return fModel != NULL ? fModel->CountThreads() : 0;
	}

	float TotalHeight() const
	{
		return fFontInfo.lineHeight * _CountLines();
	}

	void GetLineRange(BRect rect, int32& minLine, int32& maxLine) const
	{
		int32 lineHeight = (int32)fFontInfo.lineHeight;
		minLine = (int32)rect.top / lineHeight;
		maxLine = ((int32)ceilf(rect.bottom) + lineHeight - 1) / lineHeight;
		minLine = std::max(minLine, 0L);
		maxLine = std::min(maxLine, _CountLines() - 1);
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


class MainWindow::SchedulingPage::ThreadsView : public BaseView {
public:
	ThreadsView(FontInfo& fontInfo)
		:
		BaseView("threads", fontInfo)
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
			float y = (float)(i + 1) * fFontInfo.lineHeight
				- fFontInfo.fontHeight.descent;
			DrawString(fModel->ThreadAt(i)->Name(),
				BPoint(kThreadNameMargin, y));
		}
	}

	virtual BSize MinSize()
	{
		return BSize(100, TotalHeight());
	}

	virtual BSize MaxSize()
	{
		return BSize(MinSize().width, B_SIZE_UNLIMITED);
	}
};


class MainWindow::SchedulingPage::SchedulingView : public BaseView {
public:
	SchedulingView(FontInfo& fontInfo)
		:
		BaseView("scheduling", fontInfo),
		fStartTime(0),
		fEndTime(0),
		fUSecsPerPixel(1000),
		fLastMousePos(-1, -1)
	{
	}

	virtual void SetModel(Model* model)
	{
		BaseView::SetModel(model);
		fSchedulingData.SetModel(model);
		fStartTime = 0;
		fEndTime = 0;
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

	virtual BSize MinSize()
	{
		bigtime_t timeSpan = fModel != NULL ? fModel->LastEventTime() : 0;
		float width = std::max(float(timeSpan / fUSecsPerPixel), 100.0f);
		return BSize(width, TotalHeight());
	}

	virtual BSize MaxSize()
	{
		return BSize(MinSize().width, B_SIZE_UNLIMITED);
	}

	virtual void ScrollTo(BPoint where)
	{
		BaseView::ScrollTo(where);
		fStartTime = 0;
		fEndTime = 0;
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
			Model::Thread* thread = fModel->ThreadAt(i);
			const Array<SchedulingEvent>& events
				= fSchedulingData.EventsForThread(thread->Index());

			int32 eventCount = events.Size();
//printf("drawing events for thread %ld: %ld events\n", thread->Index(), eventCount);
			for (int32 k = 0; k < eventCount; k++) {
				const SchedulingEvent& event = events[k];
				bigtime_t startTime = std::max(event.time, fStartTime);
				bigtime_t endTime = k + 1 < eventCount
					? std::min(events[k + 1].time, fEndTime) : fEndTime;

				rgb_color color;
				switch (event.state) {
					case RUNNING:
					case STILL_RUNNING:
						color.set_to(0, 255, 0);
						break;
					case PREEMPTED:
						color.set_to(255, 127, 0);
						break;
					case READY:
						color.set_to(255, 0, 0);
						break;
					case WAITING:
					case UNKNOWN:
					default:
						continue;
				}

				SetHighColor(color);
				BRect rect = LineRect(i);
				rect.left = startTime / fUSecsPerPixel;
				rect.right = endTime / fUSecsPerPixel - 1;
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
		bigtime_t startTime;
		bigtime_t endTime;
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

	void _GetEventTimeRange(bigtime_t& _startTime, bigtime_t& _endTime)
	{
		if (fModel != NULL) {
			float scrollOffset = _ScrollOffset();
			_startTime = (bigtime_t)scrollOffset * fUSecsPerPixel;
			_endTime = (bigtime_t)(scrollOffset + Bounds().Width() + 1)
				* fUSecsPerPixel;
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
		double timeForX = (scrollOffset + x) * fUSecsPerPixel;

		double factor = 4;
		if (steps < 0) {
			steps = -steps;
			factor = 1;
		}

		uint32 oldUsecPerPixel = fUSecsPerPixel;
		for (; steps > 0; steps--)
			fUSecsPerPixel = fUSecsPerPixel * factor / 2;

		if (fUSecsPerPixel < 1)
			fUSecsPerPixel = 1;
		else if (fUSecsPerPixel > 1000)
			fUSecsPerPixel = 1000;

		if (fUSecsPerPixel == oldUsecPerPixel)
			return;

		Invalidate();

		UpdateScrollBar();
		if (BScrollBar* scrollBar = ScrollBar(B_HORIZONTAL))
			scrollBar->SetValue(timeForX / fUSecsPerPixel - x);
	}

	inline void _UpdateLastEventTime(bigtime_t time)
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
			bigtime_t diffTime = fState.LastEventTime() - thread->lastTime;
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
	bigtime_t				fStartTime;
	bigtime_t				fEndTime;
	uint32					fUSecsPerPixel;
	BPoint					fLastMousePos;
};


class MainWindow::SchedulingPage::ViewPort : public BaseView {
public:
	ViewPort(ThreadsView* threadsView, SchedulingView* schedulingView,
		FontInfo& fontInfo)
		:
		BaseView("viewport", fontInfo),
		fHeaderView(NULL),
		fThreadsView(threadsView),
		fSchedulingView(schedulingView)
	{
		AddChild(fHeaderView = new HeaderView);
		AddChild(threadsView);
		AddChild(schedulingView);

		Header* header = new Header(100, 100, 10000, 200);
		header->SetValue("Thread");
		fHeaderView->Model()->AddHeader(header);

		header = new Header(100, 100, 10000, 200);
		header->SetValue("Activity");
		fHeaderView->Model()->AddHeader(header);
			// TODO: Set header width correctly!
	}

	void TargetedByScrollView(BScrollView* scrollView)
	{
		_UpdateScrollBars();
	}

	virtual BSize MinSize()
	{
		return BSize(10, fHeaderView->MinSize().Height());
	}

	virtual BSize MaxSize()
	{
		return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
	}

	BSize PreferredSize()
	{
		BSize headerViewSize(fHeaderView->PreferredSize());
		BSize threadsViewSize(fThreadsView->PreferredSize());
		BSize schedulingViewSize(fSchedulingView->PreferredSize());
		BSize size(BLayoutUtils::AddDistances(
				threadsViewSize.width + kViewSeparationMargin,
				schedulingViewSize.width),
			std::max(threadsViewSize.height, schedulingViewSize.height));

		return BSize(std::max(size.width, headerViewSize.width),
			BLayoutUtils::AddDistances(size.height, headerViewSize.height));
	}

	void DoLayout()
	{
		float headerHeight = fHeaderView->MinSize().Height();
		float width = Bounds().Width();
		float height = fThreadsView->MinSize().Height();
		float threadsViewWidth = fThreadsView->MinSize().width;
		float schedulingViewLeft = threadsViewWidth + 1 + kViewSeparationMargin;

		fHeaderView->MoveTo(0, 0);
		fHeaderView->ResizeTo(width, headerHeight);

		fThreadsView->MoveTo(0, headerHeight + 1);
		fThreadsView->ResizeTo(threadsViewWidth, height);

		fSchedulingView->MoveTo(schedulingViewLeft, headerHeight + 1);
		fSchedulingView->ResizeTo(width - schedulingViewLeft, height);

		_UpdateScrollBars();
	}

private:
	void _UpdateScrollBars()
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
	fSchedulingView(NULL)
{
	SetName("Scheduling");

	be_plain_font->GetHeight(&fFontInfo.fontHeight);
	fFontInfo.lineHeight = ceilf(fFontInfo.fontHeight.ascent)
		+ ceilf(fFontInfo.fontHeight.descent);

	fViewPort = new ViewPort(fThreadsView = new ThreadsView(fFontInfo),
		fSchedulingView = new SchedulingView(fFontInfo), fFontInfo);

	AddChild(fScrollView = new BScrollView("scroll", fViewPort, 0, true, true));

	fScrollView->ScrollBar(B_HORIZONTAL)->SetTarget(fSchedulingView);
}


MainWindow::SchedulingPage::~SchedulingPage()
{
}


void
MainWindow::SchedulingPage::SetModel(Model* model)
{
	if (model == fModel)
		return;

	if (fModel != NULL) {
	}

	fModel = model;

	if (fModel != NULL) {
	}

	fViewPort->SetModel(fModel);
	fThreadsView->SetModel(fModel);
	fSchedulingView->SetModel(fModel);
}
