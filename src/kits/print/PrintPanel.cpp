/*
 * Copyright 2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Julun, <host.haiku@gmx.de
 */

#include <PrintPanel.h>

#include <Button.h>
#include <GroupLayoutBuilder.h>
#include <GroupView.h>
#include <Screen.h>


namespace BPrivate {
	namespace Print {


// #pragma mark -- _BPrintPanelFilter_


BPrintPanel::_BPrintPanelFilter_::_BPrintPanelFilter_(BPrintPanel* panel)
	: BMessageFilter(B_KEY_DOWN)
	, fPrintPanel(panel)
{
}


filter_result
BPrintPanel::_BPrintPanelFilter_::Filter(BMessage* msg, BHandler** target)
{
	int32 key;
	filter_result result = B_DISPATCH_MESSAGE;
	if (msg->FindInt32("key", &key) == B_OK && key == 1) {
		fPrintPanel->PostMessage(B_QUIT_REQUESTED);
		result = B_SKIP_MESSAGE;
	}
	return result;
}


// #pragma mark -- BPrintPanel


BPrintPanel::BPrintPanel(const BString& title)
	: BWindow(BRect(0, 0, 640, 480), title.String(), B_TITLED_WINDOW_LOOK,
		B_MODAL_APP_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE |
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE)
	, fPanel(new BGroupView)
	, fPrintPanelSem(-1)
	, fPrintPanelResult(B_CANCEL)
{
	BButton* ok = new BButton("OK", new BMessage('_ok_'));
	BButton* cancel = new BButton("Cancel", new BMessage('_cl_'));

	BGroupLayout *layout = new BGroupLayout(B_HORIZONTAL);
	SetLayout(layout);

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10.0)
			.Add(fPanel)
			.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10.0)
				.AddGlue()
				.Add(cancel)
				.Add(ok)
				.SetInsets(0.0, 0.0, 0.0, 0.0))
			.SetInsets(10.0, 10.0, 10.0, 10.0)
		);

	ok->MakeDefault(true);
	AddCommonFilter(new _BPrintPanelFilter_(this));
}


BPrintPanel::~BPrintPanel()
{
	if (fPrintPanelSem > 0)
		delete_sem(fPrintPanelSem);
}


BPrintPanel::BPrintPanel(BMessage* data)
	: BWindow(data)
{
	// TODO: implement
}


BArchivable*
BPrintPanel::Instantiate(BMessage* data)
{
	// TODO: implement
	return NULL;
}


status_t
BPrintPanel::Archive(BMessage* data, bool deep) const
{
	// TODO: implement
	return B_ERROR;
}


BView*
BPrintPanel::Panel() const
{
	return fPanel->ChildAt(0);
}


void
BPrintPanel::AddPanel(BView* panel)
{
	BView* child = Panel();
	if (child) {
		RemovePanel(child);
		delete child;
	}

	fPanel->AddChild(panel);

	BSize size = GetLayout()->PreferredSize();
	ResizeTo(size.Width(), size.Height());
}


bool
BPrintPanel::RemovePanel(BView* child)
{
	BView* panel = Panel();
	if (child == panel)
		return fPanel->RemoveChild(child);

	return false;
}


void
BPrintPanel::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case '_ok_': {
			fPrintPanelResult = B_OK;

		// fall through
		case '_cl_':
			delete_sem(fPrintPanelSem);
			fPrintPanelSem = -1;
		}	break;

		default:
			BWindow::MessageReceived(message);
	}
}


void
BPrintPanel::FrameResized(float newWidth, float newHeight)
{
	BWindow::FrameResized(newWidth, newHeight);
}


BHandler*
BPrintPanel::ResolveSpecifier(BMessage* message, int32 index, BMessage* specifier,
	int32 form, const char* property)
{
	return BWindow::ResolveSpecifier(message, index, specifier, form, property);
}


status_t
BPrintPanel::GetSupportedSuites(BMessage* data)
{
	return BWindow::GetSupportedSuites(data);
}


status_t
BPrintPanel::Perform(perform_code d, void* arg)
{
	return BWindow::Perform(d, arg);
}


void
BPrintPanel::Quit()
{
	BWindow::Quit();
}


bool
BPrintPanel::QuitRequested()
{
	return BWindow::QuitRequested();
}


void
BPrintPanel::DispatchMessage(BMessage* message, BHandler* handler)
{
	BWindow::DispatchMessage(message, handler);
}


status_t
BPrintPanel::ShowPanel()
{
	fPrintPanelSem = create_sem(0, "PrintPanel");
	if (fPrintPanelSem < 0) {
		Quit();
		return B_CANCEL;
	}

	BWindow* window = dynamic_cast<BWindow*> (BLooper::LooperForThread(find_thread(NULL)));

	{
		BRect bounds(Bounds());
		BRect frame(BScreen(B_MAIN_SCREEN_ID).Frame());
		MoveTo((frame.Width() - bounds.Width()) / 2.0,
			(frame.Height() - bounds.Height()) / 2.0);
	}

	Show();

	if (window) {
		status_t err;
		while (true) {
			do {
				err = acquire_sem_etc(fPrintPanelSem, 1, B_RELATIVE_TIMEOUT, 50000);
			} while (err == B_INTERRUPTED);

			if (err == B_BAD_SEM_ID)
				break;
			window->UpdateIfNeeded();
		}
	} else {
		while (acquire_sem(fPrintPanelSem) == B_INTERRUPTED) {}
	}

	return fPrintPanelResult;
}


void
BPrintPanel::AddChild(BView* child, BView* before)
{
	BWindow::AddChild(child, before);
}


bool
BPrintPanel::RemoveChild(BView* child)
{
	return BWindow::RemoveChild(child);
}


BView*
BPrintPanel::ChildAt(int32 index) const
{
	return BWindow::ChildAt(index);
}


void BPrintPanel::_ReservedBPrintPanel1() {}
void BPrintPanel::_ReservedBPrintPanel2() {}
void BPrintPanel::_ReservedBPrintPanel3() {}
void BPrintPanel::_ReservedBPrintPanel4() {}
void BPrintPanel::_ReservedBPrintPanel5() {}


	}	// namespace Print
}	// namespace BPrivate
