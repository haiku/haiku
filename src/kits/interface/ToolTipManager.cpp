/*
 * Copyright 2009-2012, Axel Dörfler, axeld@pinc-software.de.
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
								ToolTipView(BToolTip* tip);
	virtual						~ToolTipView();

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();

	virtual	void				FrameResized(float width, float height);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);

			void				HideTip();
			void				ShowTip();

			void				ResetWindowFrame();
			void				ResetWindowFrame(BPoint where);

			BToolTip*			Tip() const { return fToolTip; }
			bool				IsTipHidden() const { return fHidden; }

private:
			BToolTip*			fToolTip;
			bool				fHidden;
};


ToolTipView::ToolTipView(BToolTip* tip)
	:
	BView("tool tip", B_WILL_DRAW | B_FRAME_EVENTS),
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


ToolTipView::~ToolTipView()
{
	fToolTip->ReleaseReference();
}


void
ToolTipView::AttachedToWindow()
{
	SetEventMask(B_POINTER_EVENTS | B_KEYBOARD_EVENTS, 0);
	fToolTip->AttachedToWindow();
}


void
ToolTipView::DetachedFromWindow()
{
	BToolTipManager* manager = BToolTipManager::Manager();
	manager->Lock();

	RemoveChild(fToolTip->View());
		// don't delete this one!
	fToolTip->DetachedFromWindow();

	manager->Unlock();
}


void
ToolTipView::FrameResized(float width, float height)
{
	ResetWindowFrame();
}


void
ToolTipView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	if (fToolTip->IsSticky()) {
		ResetWindowFrame(ConvertToScreen(where));
	} else if (transit == B_ENTERED_VIEW) {
		// close instantly if the user managed to enter
		Window()->Quit();
	} else {
		// close with the preferred delay in case the mouse just moved
		HideTip();
	}
}


void
ToolTipView::KeyDown(const char* bytes, int32 numBytes)
{
	if (!fToolTip->IsSticky())
		HideTip();
}


void
ToolTipView::HideTip()
{
	if (fHidden)
		return;

	BMessage quit(kMsgCloseToolTip);
	BMessageRunner::StartSending(Window(), &quit,
		BToolTipManager::Manager()->HideDelay(), 1);
	fHidden = true;
}


void
ToolTipView::ShowTip()
{
	fHidden = false;
}


void
ToolTipView::ResetWindowFrame()
{
	BPoint where;
	GetMouse(&where, NULL, false);

	ResetWindowFrame(ConvertToScreen(where));
}


/*!	Tries to find the right frame to show the tool tip in, trying to use the
	alignment that the tool tip specifies.
	Makes sure the tool tip can be shown on screen in its entirety, ie. it will
	resize the window if necessary.
*/
void
ToolTipView::ResetWindowFrame(BPoint where)
{
	if (Window() == NULL)
		return;

	BSize size = PreferredSize();

	BScreen screen(Window());
	BRect screenFrame = screen.Frame().InsetBySelf(2, 2);
	BPoint offset = fToolTip->MouseRelativeLocation();

	// Ensure that the tip can be placed on screen completely

	if (size.width > screenFrame.Width())
		size.width = screenFrame.Width();

	if (size.width > where.x - screenFrame.left
		&& size.width > screenFrame.right - where.x) {
		// There is no space to put the tip to the left or the right of the
		// cursor, it can either be below or above it
		if (size.height > where.y - screenFrame.top
			&& where.y - screenFrame.top > screenFrame.Height() / 2) {
			size.height = where.y - offset.y - screenFrame.top;
		} else if (size.height > screenFrame.bottom - where.y
			&& screenFrame.bottom - where.y > screenFrame.Height() / 2) {
			size.height = screenFrame.bottom - where.y - offset.y;
		}
	}

	// Find best alignment, starting with the requested one

	BAlignment alignment = fToolTip->Alignment();
	BPoint location = where;
	bool doesNotFit = false;

	switch (alignment.horizontal) {
		case B_ALIGN_LEFT:
			location.x -= size.width + offset.x;
			if (location.x < screenFrame.left) {
				location.x = screenFrame.left;
				doesNotFit = true;
			}
			break;
		case B_ALIGN_CENTER:
			location.x -= size.width / 2 - offset.x;
			if (location.x < screenFrame.left) {
				location.x = screenFrame.left;
				doesNotFit = true;
			} else if (location.x + size.width > screenFrame.right) {
				location.x = screenFrame.right - size.width;
				doesNotFit = true;
			}
			break;

		default:
			location.x += offset.x;
			if (location.x + size.width > screenFrame.right) {
				location.x = screenFrame.right - size.width;
				doesNotFit = true;
			}
			break;
	}

	if ((doesNotFit && alignment.vertical == B_ALIGN_MIDDLE)
		|| (alignment.vertical == B_ALIGN_MIDDLE
			&& alignment.horizontal == B_ALIGN_CENTER))
		alignment.vertical = B_ALIGN_BOTTOM;

	while (true) {
		switch (alignment.vertical) {
			case B_ALIGN_TOP:
				location.y = where.y - size.height - offset.y;
				if (location.y < screenFrame.top) {
					alignment.vertical = B_ALIGN_BOTTOM;
					continue;
				}
				break;

			case B_ALIGN_MIDDLE:
				location.y -= size.height / 2 - offset.y;
				if (location.y < screenFrame.top)
					location.y = screenFrame.top;
				else if (location.y + size.height > screenFrame.bottom)
					location.y = screenFrame.bottom - size.height;
				break;

			default:
				location.y = where.y + offset.y;
				if (location.y + size.height > screenFrame.bottom) {
					alignment.vertical = B_ALIGN_TOP;
					continue;
				}
				break;
		}
		break;
	}

	where = location;

	// Cut off any out-of-screen areas

	if (screenFrame.left > where.x) {
		size.width -= where.x - screenFrame.left;
		where.x = screenFrame.left;
	} else if (screenFrame.right < where.x + size.width)
		size.width = screenFrame.right - where.x;

	if (screenFrame.top > where.y) {
		size.height -= where.y - screenFrame.top;
		where.y = screenFrame.top;
	} else if (screenFrame.bottom < where.y + size.height)
		size.height -= screenFrame.bottom - where.y;

	// Change window frame

	Window()->ResizeTo(size.width, size.height);
	Window()->MoveTo(where);
}


// #pragma mark -


ToolTipWindow::ToolTipWindow(BToolTip* tip, BPoint where, void* owner)
	:
	BWindow(BRect(0, 0, 250, 10).OffsetBySelf(where), "tool tip",
		B_BORDERED_WINDOW_LOOK, kMenuWindowFeel,
		B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_AUTO_UPDATE_SIZE_LIMITS
			| B_AVOID_FRONT | B_AVOID_FOCUS),
	fOwner(owner)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	BToolTipManager* manager = BToolTipManager::Manager();
	ToolTipView* view = new ToolTipView(tip);

	manager->Lock();
	AddChild(view);
	manager->Unlock();

	// figure out size and location

	view->ResetWindowFrame(where);
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
			reply.AddPointer("owner", fOwner);

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


// #pragma mark -


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


void
BToolTipManager::ShowTip(BToolTip* tip, BPoint where, void* owner)
{
	BToolTip* current = NULL;
	void* currentOwner = NULL;
	BMessage reply;
	if (fWindow.SendMessage(kMsgCurrentToolTip, &reply) == B_OK) {
		reply.FindPointer("current", (void**)&current);
		reply.FindPointer("owner", &currentOwner);
	}

	// Release reference from the message
	if (current != NULL)
		current->ReleaseReference();

	if (current == tip || currentOwner == owner) {
		fWindow.SendMessage(kMsgShowToolTip);
		return;
	}

	fWindow.SendMessage(kMsgHideToolTip);

	if (tip != NULL) {
		BWindow* window = new BPrivate::ToolTipWindow(tip, where, owner);
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


/*static*/ void
BToolTipManager::_InitSingleton()
{
	sDefaultInstance = new BToolTipManager();
}
