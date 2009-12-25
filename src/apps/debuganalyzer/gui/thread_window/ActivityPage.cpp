/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "thread_window/ActivityPage.h"

#include <new>

#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <Message.h>
#include <ScrollView.h>

#include <AutoDeleter.h>

#include "ColorCheckBox.h"
#include "MessageCodes.h"
#include "ThreadModel.h"

#include "chart/NanotimeChartAxisLegendSource.h"
#include "chart/Chart.h"
#include "chart/ChartDataSource.h"
#include "chart/DefaultChartAxisLegendSource.h"
#include "chart/LegendChartAxis.h"
#include "chart/LineChartRenderer.h"
#include "chart/StringChartLegend.h"


enum {
	RUN_TIME			= 0,
	WAIT_TIME			= 1,
	PREEMPTION_TIME		= 2,
	LATENCY_TIME		= 3,
	UNSPECIFIED_TIME	= 4,
	TIME_TYPE_COUNT		= 5
};

static const rgb_color kRunTimeColor		= {0, 0, 0, 255};
static const rgb_color kWaitTimeColor		= {0, 255, 0, 255};
static const rgb_color kPreemptionTimeColor	= {0, 0, 255, 255};
static const rgb_color kLatencyTimeColor	= {255, 0, 0, 255};


class ThreadWindow::ActivityPage::ThreadActivityData
	: public ChartDataSource {
public:
	ThreadActivityData(ThreadModel* model, int32 timeType)
		:
		fModel(model),
		fTimeType(timeType)
	{
	}

	virtual ChartDataRange Domain() const
	{
		return ChartDataRange(0.0, (double)fModel->GetModel()->LastEventTime());
	}

	virtual ChartDataRange Range() const
	{
		return ChartDataRange(0.0, 1.0);
	}

	virtual void GetSamples(double start, double end, double* samples,
		int32 sampleCount)
	{
		thread_id threadID = fModel->GetThread()->ID();

		double sampleLength = (end - start) / (double)sampleCount;

		int32 startIndex = fModel->FindSchedulingEvent((nanotime_t)start);
		nanotime_t baseTime = fModel->GetModel()->BaseTime();

		enum ScheduleState {
			RUNNING,
			STILL_RUNNING,
			PREEMPTED,
			READY,
			WAITING,
			UNKNOWN
		};

		ScheduleState state = UNKNOWN;

		// get the first event and guess the initial state
		const system_profiler_event_header* header
			= fModel->SchedulingEventAt(startIndex);
		if (header == NULL) {
			for (int32 i = 0; i < sampleCount; i++)
				samples[i] = 0;
			return;
		}

		double previousEventTime = start;

		switch (header->event) {
			case B_SYSTEM_PROFILER_THREAD_SCHEDULED:
			{
				system_profiler_thread_scheduled* event
					= (system_profiler_thread_scheduled*)(header + 1);
				if (event->thread == threadID) {
					// thread scheduled -- it must have been ready before
					state = READY;
				} else {
					// thread unscheduled -- it was running earlier, but should
					// now be "still running" or "running", depending on whether
					// it had been added to the run queue before
					const system_profiler_event_header* previousHeader
						= fModel->SchedulingEventAt(startIndex - 1);
					if (previousHeader != NULL
						&& previousHeader->event
							== B_SYSTEM_PROFILER_THREAD_ENQUEUED_IN_RUN_QUEUE) {
						state = STILL_RUNNING;
					} else
						state = RUNNING;
				}

				previousEventTime = event->time;
				break;
			}

			case B_SYSTEM_PROFILER_THREAD_ENQUEUED_IN_RUN_QUEUE:
			{
				system_profiler_thread_enqueued_in_run_queue* event
					= (system_profiler_thread_enqueued_in_run_queue*)
						(header + 1);
				state = WAITING;
				previousEventTime = event->time;
				break;
			}

			case B_SYSTEM_PROFILER_THREAD_REMOVED_FROM_RUN_QUEUE:
			{
				system_profiler_thread_removed_from_run_queue* event
					= (system_profiler_thread_removed_from_run_queue*)
						(header + 1);
				state = READY;
				previousEventTime = event->time;
				break;
			}
		}

		double times[TIME_TYPE_COUNT] = { };
		int32 sampleIndex = -1;
		double nextSampleTime = start;

		for (int32 i = startIndex; ; i++) {
			header = fModel->SchedulingEventAt(i);
			double eventTime = previousEventTime;
			int32 timeType = -1;

			if (header != NULL) {
				switch (header->event) {
					case B_SYSTEM_PROFILER_THREAD_SCHEDULED:
					{
						system_profiler_thread_scheduled* event
							= (system_profiler_thread_scheduled*)(header + 1);
						eventTime = double(event->time - baseTime);

						if (event->thread == threadID) {
							// thread scheduled
							if (state == READY) {
								// thread scheduled after having been woken up
								timeType = LATENCY_TIME;
							} else if (state == PREEMPTED) {
								// thread scheduled after having been preempted
								// before
								timeType = PREEMPTION_TIME;
							} else if (state == STILL_RUNNING) {
								// Thread was running and continues to run.
								timeType = RUN_TIME;
							} else {
								// Can only happen, if we're missing context.
								// Impossible to guess what the thread was doing
								// before.
								timeType = UNSPECIFIED_TIME;
							}

							state = RUNNING;
						} else {
							// thread unscheduled
							if (state == STILL_RUNNING) {
								// thread preempted
								state = PREEMPTED;
							} else if (state == RUNNING) {
								// thread starts waiting (it hadn't been added
								// to the run queue before being unscheduled)
								state = WAITING;
							} else {
								// Can only happen, if we're missing context.
								// Obviously the thread was running, but we
								// can't guess the new thread state.
							}

							timeType = RUN_TIME;
						}

						break;
					}

					case B_SYSTEM_PROFILER_THREAD_ENQUEUED_IN_RUN_QUEUE:
					{
						system_profiler_thread_enqueued_in_run_queue* event
							= (system_profiler_thread_enqueued_in_run_queue*)
								(header + 1);
						eventTime = double(event->time - baseTime);

						if (state == RUNNING || state == STILL_RUNNING) {
							// Thread was running and is reentered into the run
							// queue. This is done by the scheduler, if the
							// thread remains ready.
							state = STILL_RUNNING;
							timeType = RUN_TIME;
						} else if (state == READY || state == PREEMPTED) {
							// Happens only after having been removed from the
							// run queue for altering the priority.
							timeType = state == READY
								? LATENCY_TIME : PREEMPTION_TIME;
						} else {
							// Thread was waiting and is ready now.
							state = READY;
							timeType = WAIT_TIME;
						}

						break;
					}

					case B_SYSTEM_PROFILER_THREAD_REMOVED_FROM_RUN_QUEUE:
					{
						system_profiler_thread_removed_from_run_queue* event
							= (system_profiler_thread_removed_from_run_queue*)
								(header + 1);
						eventTime = double(event->time - baseTime);

						// This really only happens when the thread priority is
						// changed while the thread is ready.

						if (state == RUNNING) {
							// This should never happen.
							state = READY;
							timeType = RUN_TIME;
						} else if (state == READY) {
							// Thread was ready after having been woken up.
							timeType = LATENCY_TIME;
						} else if (state == PREEMPTED) {
							// Thread was ready after having been preempted.
							timeType = PREEMPTION_TIME;
						} else
							state = READY;

						break;
					}
				}
			} else {
				// no more events for this thread -- assume things go on like
				// this until the death of the thread
				switch (state) {
					case RUNNING:
					case STILL_RUNNING:
						timeType = RUN_TIME;
						break;
					case PREEMPTED:
						timeType = PREEMPTION_TIME;
						break;
					case READY:
						timeType = LATENCY_TIME;
						break;
					case WAITING:
						timeType = WAIT_TIME;
						break;
					case UNKNOWN:
						timeType = UNSPECIFIED_TIME;
						break;
				}

				if (fModel->GetThread()->DeletionTime() >= 0) {
					eventTime = double(fModel->GetThread()->DeletionTime()
						- baseTime);
					if (eventTime <= previousEventTime) {
						// thread is dead
						eventTime = end + 1;
						timeType = UNSPECIFIED_TIME;
					}
				} else
					eventTime = end + 1;
			}

			if (timeType < 0)
				continue;

			while (eventTime >= nextSampleTime) {
				// We've reached the next sample. Partially add this time to the
				// current sample.
				times[timeType] += nextSampleTime - previousEventTime;
				previousEventTime = nextSampleTime;

				// write the sample value
				if (sampleIndex >= 0) {
					samples[sampleIndex] = times[fTimeType] / sampleLength;
					if (samples[sampleIndex] > 1.0)
						samples[sampleIndex] = 1.0;
				}

				for (int32 k = 0; k < TIME_TYPE_COUNT; k++)
					times[k] = 0;

				// compute the time of the next sample
				if (++sampleIndex >= sampleCount)
					return;
				nextSampleTime = start
					+ (end - start)
						* (double)(sampleIndex + 1) / (double)sampleCount;
			}

			// next sample not yet reached -- just add the time
			times[timeType] += double(eventTime - previousEventTime);
			previousEventTime = eventTime;
		}
	}

private:
	ThreadModel*	fModel;
	int32			fTimeType;
};


ThreadWindow::ActivityPage::ActivityPage()
	:
	BGroupView(B_VERTICAL),
	fThreadModel(NULL),
	fActivityChart(NULL),
	fActivityChartRenderer(NULL),
	fRunTimeData(NULL),
	fWaitTimeData(NULL),
	fPreemptionTimeData(NULL),
	fLatencyTimeData(NULL)
{
	SetName("Activity");

	GroupLayout()->SetInsets(10, 10, 10, 10);

	fActivityChartRenderer = new LineChartRenderer;
	ObjectDeleter<ChartRenderer> rendererDeleter(fActivityChartRenderer);

	fActivityChart = new Chart(fActivityChartRenderer);
	fActivityChart->SetDomainZoomLimit(1000);
		// maximal zoom: 1 ms for the whole displayed domain

	BGroupLayoutBuilder(this)
		.Add(new BScrollView("activity scroll", fActivityChart, 0, true, false))
		.AddStrut(20)
		.Add(BGridLayoutBuilder()
			.Add(fRunTimeCheckBox = new ColorCheckBox("Run time", kRunTimeColor,
					new BMessage(MSG_CHECK_BOX_RUN_TIME)),
				0, 0)
			.Add(fWaitTimeCheckBox = new ColorCheckBox("Wait time",
					kWaitTimeColor, new BMessage(MSG_CHECK_BOX_WAIT_TIME)),
				1, 0)
			.Add(fPreemptionTimeCheckBox = new ColorCheckBox("Preemption time",
					kPreemptionTimeColor,
					new BMessage(MSG_CHECK_BOX_PREEMPTION_TIME)),
				0, 1)
			.Add(fLatencyTimeCheckBox = new ColorCheckBox("Latency time",
					kLatencyTimeColor,
					new BMessage(MSG_CHECK_BOX_LATENCY_TIME)),
				1, 1)
		)
	;

	rendererDeleter.Detach();

	// enable the run time chart data
	fRunTimeCheckBox->CheckBox()->SetValue(B_CONTROL_ON);

// TODO: Allocation management...
	LegendChartAxis* axis = new LegendChartAxis(
		new NanotimeChartAxisLegendSource, new StringChartLegendRenderer);
	fActivityChart->SetAxis(CHART_AXIS_BOTTOM, axis);

	axis = new LegendChartAxis(
		new NanotimeChartAxisLegendSource, new StringChartLegendRenderer);
	fActivityChart->SetAxis(CHART_AXIS_TOP, axis);

	axis = new LegendChartAxis(
		new DefaultChartAxisLegendSource, new StringChartLegendRenderer);
	fActivityChart->SetAxis(CHART_AXIS_LEFT, axis);

	axis = new LegendChartAxis(
		new DefaultChartAxisLegendSource, new StringChartLegendRenderer);
	fActivityChart->SetAxis(CHART_AXIS_RIGHT, axis);
}


ThreadWindow::ActivityPage::~ActivityPage()
{
	delete fRunTimeData;
	delete fWaitTimeData;
	delete fLatencyTimeData;
	delete fPreemptionTimeData;
	delete fActivityChartRenderer;
}


void
ThreadWindow::ActivityPage::SetModel(ThreadModel* model)
{
	if (model == fThreadModel)
		return;

	if (fThreadModel != NULL) {
		fActivityChart->RemoveAllDataSources();
		delete fRunTimeData;
		delete fWaitTimeData;
		delete fLatencyTimeData;
		delete fPreemptionTimeData;
		fRunTimeData = NULL;
		fWaitTimeData = NULL;
		fLatencyTimeData = NULL;
		fPreemptionTimeData = NULL;
	}

	fThreadModel = model;

	if (fThreadModel != NULL) {
		_UpdateChartDataEnabled(RUN_TIME);
		_UpdateChartDataEnabled(WAIT_TIME);
		_UpdateChartDataEnabled(PREEMPTION_TIME);
		_UpdateChartDataEnabled(LATENCY_TIME);
	}
}


void
ThreadWindow::ActivityPage::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_CHECK_BOX_RUN_TIME:
			_UpdateChartDataEnabled(RUN_TIME);
			break;
		case MSG_CHECK_BOX_WAIT_TIME:
			_UpdateChartDataEnabled(WAIT_TIME);
			break;
		case MSG_CHECK_BOX_PREEMPTION_TIME:
			_UpdateChartDataEnabled(PREEMPTION_TIME);
			break;
		case MSG_CHECK_BOX_LATENCY_TIME:
			_UpdateChartDataEnabled(LATENCY_TIME);
			break;
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
ThreadWindow::ActivityPage::AttachedToWindow()
{
	fRunTimeCheckBox->SetTarget(this);
	fWaitTimeCheckBox->SetTarget(this);
	fPreemptionTimeCheckBox->SetTarget(this);
	fLatencyTimeCheckBox->SetTarget(this);
}


void
ThreadWindow::ActivityPage::_UpdateChartDataEnabled(int timeType)
{
	ThreadActivityData** data;
	ColorCheckBox* checkBox;
	rgb_color color;

	switch (timeType) {
		case RUN_TIME:
			data = &fRunTimeData;
			checkBox = fRunTimeCheckBox;
			color = kRunTimeColor;
			break;
		case WAIT_TIME:
			data = &fWaitTimeData;
			checkBox = fWaitTimeCheckBox;
			color = kWaitTimeColor;
			break;
		case PREEMPTION_TIME:
			data = &fPreemptionTimeData;
			checkBox = fPreemptionTimeCheckBox;
			color = kPreemptionTimeColor;
			break;
		case LATENCY_TIME:
			data = &fLatencyTimeData;
			checkBox = fLatencyTimeCheckBox;
			color = kLatencyTimeColor;
			break;
		default:
			return;
	}

	bool enabled = checkBox->CheckBox()->Value() == B_CONTROL_ON;

	if ((*data != NULL) == enabled)
		return;

	if (enabled) {
		LineChartRendererDataSourceConfig config(color);

		*data = new(std::nothrow) ThreadActivityData(fThreadModel, timeType);
		if (*data == NULL)
			return;

		fActivityChart->AddDataSource(*data, &config);
	} else {
		fActivityChart->RemoveDataSource(*data);
		delete *data;
		*data = NULL;
	}
}
