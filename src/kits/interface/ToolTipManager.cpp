/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <ToolTipManager.h>
#include <ToolTipWindow.h>

#include <pthread.h>

#include <Autolock.h>
#include <LayoutBuilder.h>
#include <MessageRunner.h>
#include <Screen.h>

#include <WindowPrivate.h>
#include <ToolTip.h>


static pthread_once_t sManagerInitOnce = PTHREAD_ONCE_INIT;
BToolTipManager* BToolTipManager::sDefaultInstance;

static const uint32 kMsgHideToolTip = 'hide';
static const uint32 kMsgShowToolTip = 'show';
static const uint32 kMsgCurrentToolTip = 'curr';
static const uint32 kMsgCloseToolTip = 'clos';


namespace BPrivate {

class ToolTipView : public BView {
public:
	ToolTipView(BToolTip* tip)
		:
		BView("tool tip", B_WILL_DRAW),
		fToolTip(tip),
		fHidden(false)
	{
		fToolTip->AcquireReference();
		SetViewColor(ui_color(B_TOOL_TIP_BACKGROUND_COLOR));

		BGroupLayout* layout = new BGroupLayout(B_VERTICAL);
		layout->SetInsets(5, 5, 5, 5);
		SetLayout(layout);

		AddChild(fToolTip->View());
	}

	virtual ~ToolTipView()
	{
		fToolTip->ReleaseReference();
	}

	virtual void AttachedToWindow()
	{
		SetEventMask(B_POINTER_EVENTS, 0);
		fToolTip->AttachedToWindow();
	}

	virtual void DetachedFromWindow()
	{
		BToolTipManager* manager = BToolTipManager::Manager();
		manager->Lock();

		RemoveChild(fToolTip->View());
			// don't delete this one!
		fToolTip->DetachedFromWindow();

		manager->Unlock();
	}

	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* dragMessage)
	{
		if (fToolTip->IsSticky()) {
			// TODO: move window with mouse!
			Window()->MoveTo(
				ConvertToScreen(where) + fToolTip->MouseRelativeLocation());
		} else if (transit == B_ENTERED_VIEW) {
			// close instantly if the user managed to enter
			Window()->Quit();
		} else {
			// close with the preferred delay in case the mouse just moved
			HideTip();
		}
	}

	void HideTip()
	{
		if (fHidden)
			return;

		BMessage quit(kMsgCloseToolTip);
		BMessageRunner::StartSending(Window(), &quit,
			BToolTipManager::Manager()->HideDelay(), 1);
		fHidden = true;
	}

	void ShowTip()
	{
		fHidden = false;
	}

	BToolTip* Tip() const { return fToolTip; }
	bool IsTipHidden() const { return fHidden; }

private:
	BToolTip*	fToolTip;
	bool		fHidden;
};


ToolTipWindow::ToolTipWindow(BToolTip* tip, BPoint where)
	:
	BWindow(BRect(0, 0, 250, 10), "tool tip", B_BORDERED_WINDOW_LOOK,
		kMenuWindowFeel, B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE
			| B_AUTO_UPDATE_SIZE_LIMITS | B_AVOID_FRONT | B_AVOID_FOCUS)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	BToolTipManager* manager = BToolTipManager::Manager();
	manager->Lock();
	AddChild(new ToolTipView(tip));
	manager->Unlock();

	BSize size = ChildAt(0)->PreferredSize();
	ResizeTo(size.width, size.height);
	//AddChild(BLayoutBuilder::Group<>(B_VERTICAL).Add(new ToolTipView(tip)));

	// figure out location
	// TODO: take alignment into account!
	where += tip->MouseRelativeLocation();

	BScreen screen(this);
	if (screen.IsValid()) {
		BRect screenFrame = screen.Frame().InsetBySelf(5, 5);
		BRect frame = Frame().OffsetToSelf(where);
		if (!screenFrame.Contains(frame)) {
			if (screenFrame.top > frame.top)
				where.y -= frame.top - screenFrame.top;
			else if (screenFrame.bottom < frame.bottom)
				where.y -= frame.bottom - screenFrame.bottom;
			if (screenFrame.left > frame.left)
				where.x -= frame.left - screenFrame.left;
			else if (screenFrame.right < frame.right)
				where.x -= frame.right - screenFrame.right;
		}
	}

	MoveTo(where);
}


void
ToolTipWindow::MessageReceived(BMessage* message)
{
	ToolTipView* view = static_cast<ToolTipView*>(ChildAt(0));

	switch (message->what) {
		case kMsgHideToolTip:
			view->HideTip();
			break;

		case kMsgCurrentToolTip:
		{
			BToolTip* tip = view->Tip();

			BMessage reply(B_REPLY);
			reply.AddPointer("current", tip);

			if (message->SendReply(&reply) == B_OK)
				tip->AcquireReference();
			break;
		}

		case kMsgShowToolTip:
			view->ShowTip();
			break;

		case kMsgCloseToolTip:
			if (view->IsTipHidden())
				Quit();
			break;

		default:
			BWindow::MessageReceived(message);
	}
}

}	// namespace BPrivate


//	#pragma mark -


/*static*/ BToolTipManager*
BToolTipManager::Manager()
{
	// Note: The check is not necessary; it's just faster than always calling
	// pthread_once(). It requires reading/writing of pointers to be atomic
	// on the architecture.
	if (sDefaultInstance == NULL)
		pthread_once(&sManagerInitOnce, &_InitSingleton);

	return sDefaultInstance;
}


/*static*/ void
BToolTipManager::_InitSingleton()
{
	sDefaultInstance = new BToolTipManager();
}


void
BToolTipManager::ShowTip(BToolTip* tip, BPoint point)
{
	BToolTip* current = NULL;
	BMessage reply;
	if (fWindow.SendMessage(kMsgCurrentToolTip, &reply) == B_OK)
		reply.FindPointer("current", (void**)&current);

	if (current != NULL)
		current->ReleaseReference();

	if (current == tip) {
		fWindow.SendMessage(kMsgShowToolTip);
		return;
	}

	fWindow.SendMessage(kMsgHideToolTip);

	if (current != NULL)
		current->ReleaseReference();

	if (tip != NULL) {
		BWindow* window = new BPrivate::ToolTipWindow(tip, point);
		window->Show();

		fWindow = BMessenger(window);
	}
}


void
BToolTipManager::HideTip()
{
	fWindow.SendMessage(kMsgHideToolTip);
}


void
BToolTipManager::SetShowDelay(bigtime_t time)
{
	// between 10ms and 3s
	if (time < 10000)
		time = 10000;
	else if (time > 3000000)
		time = 3000000;

	fShowDelay = time;
}


bigtime_t
BToolTipManager::ShowDelay() const
{
	return fShowDelay;
}


void
BToolTipManager::SetHideDelay(bigtime_t time)
{
	// between 0 and 0.5s
	if (time < 0)
		time = 0;
	else if (time > 500000)
		time = 500000;

	fHideDelay = time;
}


bigtime_t
BToolTipManager::HideDelay() const
{
	return fHideDelay;
}


BToolTipManager::BToolTipManager()
	:
	fLock("tool tip manager"),
	fShowDelay(750000),
	fHideDelay(50000)
{
}


BToolTipManager::~BToolTipManager()
{
}
