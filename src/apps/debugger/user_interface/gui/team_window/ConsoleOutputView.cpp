/*
 * Copyright 2013-2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "ConsoleOutputView.h"

#include <new>

#include <Button.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <String.h>
#include <TextView.h>

#include <AutoDeleter.h>


enum {
	MSG_CLEAR_OUTPUT	= 'clou',
	MSG_POST_OUTPUT		= 'poou'
};


static const bigtime_t kOutputWaitInterval = 10000;


// #pragma mark - ConsoleOutputView::OutputInfo


struct ConsoleOutputView::OutputInfo {
	int32 	fd;
	BString	text;

	OutputInfo(int32 fd, const BString& text)
		:
		fd(fd),
		text(text)
	{
	}
};


// #pragma mark - ConsoleOutputView


ConsoleOutputView::ConsoleOutputView()
	:
	BGroupView(B_VERTICAL, 0.0f),
	fStdoutEnabled(NULL),
	fStderrEnabled(NULL),
	fConsoleOutput(NULL),
	fClearButton(NULL),
	fPendingOutput(NULL),
	fWorkToDoSem(-1),
	fOutputWorker(-1)
{
	SetName("ConsoleOutput");
}


ConsoleOutputView::~ConsoleOutputView()
{
	if (fWorkToDoSem > 0)
		delete_sem(fWorkToDoSem);

	if (fOutputWorker > 0)
		wait_for_thread(fOutputWorker, NULL);

	delete fPendingOutput;
}


/*static*/ ConsoleOutputView*
ConsoleOutputView::Create()
{
	ConsoleOutputView* self = new ConsoleOutputView();

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
ConsoleOutputView::ConsoleOutputReceived(int32 fd, const BString& output)
{
	if (fd == 1 && fStdoutEnabled->Value() != B_CONTROL_ON)
		return;
	else if (fd == 2 && fStderrEnabled->Value() != B_CONTROL_ON)
		return;

	OutputInfo* info = new(std::nothrow) OutputInfo(fd, output);
	if (info == NULL)
		return;

	ObjectDeleter<OutputInfo> infoDeleter(info);
	if (fPendingOutput->AddItem(info)) {
		infoDeleter.Detach();
		release_sem(fWorkToDoSem);
	}
}


void
ConsoleOutputView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_CLEAR_OUTPUT:
		{
			fConsoleOutput->SetText("");
			fPendingOutput->MakeEmpty();
			break;
		}
		case MSG_POST_OUTPUT:
		{
			OutputInfo* info = fPendingOutput->RemoveItemAt(0);
			if (info == NULL)
				break;

			ObjectDeleter<OutputInfo> infoDeleter(info);
			_HandleConsoleOutput(info);
		}
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
ConsoleOutputView::AttachedToWindow()
{
	BGroupView::AttachedToWindow();

	fStdoutEnabled->SetValue(B_CONTROL_ON);
	fStderrEnabled->SetValue(B_CONTROL_ON);
	fClearButton->SetTarget(this);
}


void
ConsoleOutputView::LoadSettings(const BMessage& settings)
{
	fStdoutEnabled->SetValue(settings.GetBool("showStdout", true)
			? B_CONTROL_ON : B_CONTROL_OFF);
	fStderrEnabled->SetValue(settings.GetBool("showStderr", true)
			? B_CONTROL_ON : B_CONTROL_OFF);
}


status_t
ConsoleOutputView::SaveSettings(BMessage& settings)
{
	bool value = fStdoutEnabled->Value() == B_CONTROL_ON;
	if (settings.AddBool("showStdout", value) != B_OK)
		return B_NO_MEMORY;

	value = fStderrEnabled->Value() == B_CONTROL_ON;
	if (settings.AddBool("showStderr", value) != B_OK)
		return B_NO_MEMORY;

	return B_OK;
}


void
ConsoleOutputView::_Init()
{
	fPendingOutput = new OutputInfoList(10, true);

	fWorkToDoSem = create_sem(0, "output_work_available");
	if (fWorkToDoSem < 0)
		throw std::bad_alloc();

	fOutputWorker = spawn_thread(_OutputWorker, "output worker", B_LOW_PRIORITY, this);
	if (fOutputWorker < 0)
		throw std::bad_alloc();

	resume_thread(fOutputWorker);

	BScrollView* consoleScrollView;

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0.0f)
		.Add(consoleScrollView = new BScrollView("console scroll", NULL, 0,
			true, true), 3.0f)
		.AddGroup(B_VERTICAL, 0.0f)
			.SetInsets(B_USE_SMALL_SPACING)
			.Add(fStdoutEnabled = new BCheckBox("Stdout"))
			.Add(fStderrEnabled = new BCheckBox("Stderr"))
			.Add(fClearButton = new BButton("Clear"))
			.AddGlue()
		.End()
	.End();

	consoleScrollView->SetTarget(fConsoleOutput = new BTextView("Console"));

	fClearButton->SetMessage(new BMessage(MSG_CLEAR_OUTPUT));
	fConsoleOutput->MakeEditable(false);
	fConsoleOutput->SetStylable(true);
	fConsoleOutput->SetDoesUndo(false);
}


int32
ConsoleOutputView::_OutputWorker(void* arg)
{
	ConsoleOutputView* view = (ConsoleOutputView*)arg;

	for (;;) {
		status_t error = acquire_sem(view->fWorkToDoSem);
		if (error == B_INTERRUPTED)
			continue;
		else if (error != B_OK)
			break;

		BMessenger(view).SendMessage(MSG_POST_OUTPUT);
		snooze(kOutputWaitInterval);
	}

	return B_OK;
}


void
ConsoleOutputView::_HandleConsoleOutput(OutputInfo* info)
{
	if (info->fd == 1 && fStdoutEnabled->Value() != B_CONTROL_ON)
		return;
	else if (info->fd == 2 && fStderrEnabled->Value() != B_CONTROL_ON)
		return;

	text_run_array run;
	run.count = 1;
	run.runs[0].font = be_fixed_font;
	run.runs[0].offset = 0;
	run.runs[0].color.red = info->fd == 1 ? 0 : 192;
	run.runs[0].color.green = 0;
	run.runs[0].color.blue = 0;
	run.runs[0].color.alpha = 255;

	bool autoScroll = false;
	BScrollBar* scroller = fConsoleOutput->ScrollBar(B_VERTICAL);
	float min, max;
	scroller->GetRange(&min, &max);
	if (min == max || scroller->Value() == max)
		autoScroll = true;

	fConsoleOutput->Insert(fConsoleOutput->TextLength(), info->text,
		info->text.Length(), &run);
	if (autoScroll) {
		scroller->GetRange(&min, &max);
		fConsoleOutput->ScrollTo(0.0, max);
	}
}
