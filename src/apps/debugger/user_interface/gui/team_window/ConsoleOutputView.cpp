/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
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


enum {
	MSG_CLEAR_OUTPUT	= 'clou'
};


// #pragma mark - ConsoleOutputView


ConsoleOutputView::ConsoleOutputView()
	:
	BGroupView(B_VERTICAL, 0.0f),
	fStdoutEnabled(NULL),
	fStderrEnabled(NULL),
	fConsoleOutput(NULL),
	fClearButton(NULL)
{
	SetName("ConsoleOutput");
}


ConsoleOutputView::~ConsoleOutputView()
{
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

	text_run_array run;
	run.count = 1;
	run.runs[0].font = be_fixed_font;
	run.runs[0].offset = 0;
	run.runs[0].color.red = fd == 1 ? 0 : 192;
	run.runs[0].color.green = 0;
	run.runs[0].color.blue = 0;
	run.runs[0].color.alpha = 255;

	bool autoScroll = false;
	BScrollBar* scroller = fConsoleOutput->ScrollBar(B_VERTICAL);
	float min, max;
	scroller->GetRange(&min, &max);
	if (min == max || scroller->Value() == max)
		autoScroll = true;

	fConsoleOutput->Insert(fConsoleOutput->TextLength(), output.String(),
		output.Length(), &run);
	if (autoScroll) {
		scroller->GetRange(&min, &max);
		fConsoleOutput->ScrollTo(0.0, max);
	}
}


void
ConsoleOutputView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_CLEAR_OUTPUT:
		{
			fConsoleOutput->SetText("");
			break;
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
