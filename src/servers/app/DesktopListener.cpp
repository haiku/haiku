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


#define FOR_ALL_DESKTOP_LISTENER 											\
	for (DesktopListener* listener = fDesktopListenerList.First();			\
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))


void
DesktopObservable::InvokeAddWindow(Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	FOR_ALL_DESKTOP_LISTENER
		listener->AddWindow(window);
}


void
DesktopObservable::InvokeRemoveWindow(Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	FOR_ALL_DESKTOP_LISTENER
		listener->RemoveWindow(window);
}


void
DesktopObservable::InvokeKeyEvent(uint32 what, int32 key, int32 modifiers)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	FOR_ALL_DESKTOP_LISTENER
		listener->KeyEvent(what, key, modifiers);	
}


void
DesktopObservable::InvokeMouseEvent(BMessage* message)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	FOR_ALL_DESKTOP_LISTENER
		listener->MouseEvent(message);
}


void
DesktopObservable::InvokeMouseDown(Window* window, BMessage* message,
	const BPoint& where)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	FOR_ALL_DESKTOP_LISTENER
		listener->MouseDown(window, message, where);
}


void
DesktopObservable::InvokeMouseUp(Window* window, BMessage* message,
	const BPoint& where)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	FOR_ALL_DESKTOP_LISTENER
		listener->MouseUp(window, message, where);
}


void
DesktopObservable::InvokeMouseMoved(Window* window, BMessage* message,
	const BPoint& where)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	FOR_ALL_DESKTOP_LISTENER
		listener->MouseMoved(window, message, where);
}


void
DesktopObservable::InvokeMoveWindow(Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	FOR_ALL_DESKTOP_LISTENER
		listener->MoveWindow(window);
}


void
DesktopObservable::InvokeResizeWindow(Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	FOR_ALL_DESKTOP_LISTENER
		listener->ResizeWindow(window);
}


void
DesktopObservable::InvokeActivateWindow(Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	FOR_ALL_DESKTOP_LISTENER
		listener->ActivateWindow(window);
}


void
DesktopObservable::InvokeSendWindowBehind(Window* window, Window* behindOf)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	FOR_ALL_DESKTOP_LISTENER
		listener->SendWindowBehind(window, behindOf);
}


bool
DesktopObservable::InvokeSetDecoratorSettings(Window* window,
	const BMessage& settings)
{
	if (fWeAreInvoking)
		return false;
	InvokeGuard invokeGuard(fWeAreInvoking);

	bool changed = false;
	FOR_ALL_DESKTOP_LISTENER
		changed = changed | listener->SetDecoratorSettings(window, settings);

	return changed;
}


void
DesktopObservable::InvokeGetDecoratorSettings(Window* window, BMessage& settings)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	FOR_ALL_DESKTOP_LISTENER
		listener->GetDecoratorSettings(window, settings);
}


void
DesktopObservable::InvokeSetWindowWorkspaces(Window* window, uint32 workspaces)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	FOR_ALL_DESKTOP_LISTENER
		listener->SetWindowWorkspaces(window, workspaces);
}


void
DesktopObservable::InvokeShowWindow(Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	FOR_ALL_DESKTOP_LISTENER
		listener->ShowWindow(window);
}


void
DesktopObservable::InvokeHideWindow(Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	FOR_ALL_DESKTOP_LISTENER
		listener->HideWindow(window);
}


void
DesktopObservable::InvokeMinimizeWindow(Window* window, bool minimize)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	FOR_ALL_DESKTOP_LISTENER
		listener->MinimizeWindow(window, minimize);
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
