/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */


#include "DesktopListener.h"


DesktopListener::~DesktopListener()
{

}


DesktopObservable::DesktopObservable()
	:
	fWeAreInvoking(false)
{

}


void
DesktopObservable::RegisterListener(DesktopListener* listener, Desktop* desktop)
{
	fDesktopListenerList.Add(listener);
	listener->ListenerRegistered(desktop);
}


void
DesktopObservable::UnregisterListener(DesktopListener* listener)
{
	fDesktopListenerList.Remove(listener);
	listener->ListenerUnregistered();
}


const DesktopListenerDLList&
DesktopObservable::GetDesktopListenerList()
{
	return fDesktopListenerList;
}


bool
DesktopObservable::MessageForListener(Window* sender,
	BPrivate::LinkReceiver& link, BPrivate::LinkSender& reply)
{
	int32 identifier;
	link.Read<int32>(&identifier);
	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener)) {
		if (listener->Identifier() == identifier) {
			if (!listener->HandleMessage(sender, link, reply))
				break;
			return true;
		}
	}
	return false;
}


void
DesktopObservable::NotifyWindowAdded(Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowAdded(window);
}


void
DesktopObservable::NotifyWindowRemoved(Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowRemoved(window);
}


bool
DesktopObservable::NotifyKeyPressed(uint32 what, int32 key, int32 modifiers)
{
	if (fWeAreInvoking)
		return false;
	InvokeGuard invokeGuard(fWeAreInvoking);

	bool skipEvent = false;
	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener)) {
		if (listener->KeyPressed(what, key, modifiers))
			skipEvent = true;
	}
	return skipEvent;
}


void
DesktopObservable::NotifyMouseEvent(BMessage* message)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->MouseEvent(message);
}


void
DesktopObservable::NotifyMouseDown(Window* window, BMessage* message,
	const BPoint& where)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->MouseDown(window, message, where);
}


void
DesktopObservable::NotifyMouseUp(Window* window, BMessage* message,
	const BPoint& where)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->MouseUp(window, message, where);
}


void
DesktopObservable::NotifyMouseMoved(Window* window, BMessage* message,
	const BPoint& where)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->MouseMoved(window, message, where);
}


void
DesktopObservable::NotifyWindowMoved(Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowMoved(window);
}


void
DesktopObservable::NotifyWindowResized(Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowResized(window);
}


void
DesktopObservable::NotifyWindowActivated(Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowActitvated(window);
}


void
DesktopObservable::NotifyWindowSentBehind(Window* window, Window* behindOf)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowSentBehind(window, behindOf);
}


void
DesktopObservable::NotifyWindowWorkspacesChanged(Window* window,
	uint32 workspaces)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowWorkspacesChanged(window, workspaces);
}


void
DesktopObservable::NotifyWindowMinimized(Window* window, bool minimize)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowMinimized(window, minimize);
}


void
DesktopObservable::NotifyWindowTabLocationChanged(Window* window,
	float location, bool isShifting)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowTabLocationChanged(window, location, isShifting);
}


void
DesktopObservable::NotifySizeLimitsChanged(Window* window, int32 minWidth,
	int32 maxWidth, int32 minHeight, int32 maxHeight)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->SizeLimitsChanged(window, minWidth, maxWidth, minHeight,
			maxHeight);
}


void
DesktopObservable::NotifyWindowLookChanged(Window* window, window_look look)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowLookChanged(window, look);
}


void
DesktopObservable::NotifyWindowFeelChanged(Window* window, window_feel feel)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowFeelChanged(window, feel);
}


bool
DesktopObservable::SetDecoratorSettings(Window* window,
	const BMessage& settings)
{
	if (fWeAreInvoking)
		return false;
	InvokeGuard invokeGuard(fWeAreInvoking);

	bool changed = false;
	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		changed = changed | listener->SetDecoratorSettings(window, settings);

	return changed;
}


void
DesktopObservable::GetDecoratorSettings(Window* window, BMessage& settings)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->GetDecoratorSettings(window, settings);
}


DesktopObservable::InvokeGuard::InvokeGuard(bool& invoking)
	:
	fInvoking(invoking)
{
	fInvoking = true;
}


DesktopObservable::InvokeGuard::~InvokeGuard()
{
	fInvoking = false;
}
