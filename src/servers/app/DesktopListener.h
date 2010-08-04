/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef DESKTOP_LISTENER_H
#define DESKTOP_LISTENER_H


#include <util/DoublyLinkedList.h>

#include <Point.h>


class BMessage;
class Desktop;
class Window;


class DesktopListener : public DoublyLinkedListLinkImpl<DesktopListener> {
public:
	virtual						~DesktopListener();

	virtual	void				ListenerRegistered(Desktop* desktop) = 0;
	virtual	void				ListenerUnregistered() = 0;

	virtual void				AddWindow(Window* window) = 0;
	virtual void				RemoveWindow(Window* window) = 0;
	
	virtual void				KeyEvent(uint32 what, int32 key,
									int32 modifiers) = 0;
	virtual void				MouseEvent(BMessage* message) = 0;
	virtual void				MouseDown(Window* window, BMessage* message,
									const BPoint& where) = 0;
	virtual void				MouseUp(Window* window, BMessage* message,
									const BPoint& where) = 0;
	virtual void				MouseMoved(Window* window, BMessage* message,
									const BPoint& where) = 0;

	virtual void				MoveWindow(Window* window) = 0;
	virtual void				ResizeWindow(Window* window) = 0;
	virtual void				ActivateWindow(Window* window) = 0;
	virtual void				SendWindowBehind(Window* window,
									Window* behindOf) = 0;
	virtual void				SetWindowWorkspaces(Window* window,
									uint32 workspaces) = 0;
	virtual void				ShowWindow(Window* window) = 0;
	virtual void				HideWindow(Window* window) = 0;
	virtual void				MinimizeWindow(Window* window,
									bool minimize) = 0;

	virtual bool				SetDecoratorSettings(Window* window,
									const BMessage& settings) = 0;
	virtual void				GetDecoratorSettings(Window* window,
									BMessage& settings) = 0;
};


typedef DoublyLinkedList<DesktopListener> DesktopListenerDLList;


class DesktopObservable {
public:
							DesktopObservable();

		void				RegisterListener(DesktopListener* listener,
								Desktop* desktop);
		void				UnregisterListener(DesktopListener* listener);
	const DesktopListenerDLList&	GetDesktopListenerList();

		void				InvokeAddWindow(Window* window);
		void				InvokeRemoveWindow(Window* window);

		void				InvokeKeyEvent(uint32 what, int32 key,
								int32 modifiers);
		void				InvokeMouseEvent(BMessage* message);
		void				InvokeMouseDown(Window* window, BMessage* message,
									const BPoint& where);
		void				InvokeMouseUp(Window* window, BMessage* message,
									const BPoint& where);
		void				InvokeMouseMoved(Window* window, BMessage* message,
									const BPoint& where);

		void				InvokeMoveWindow(Window* window);
		void				InvokeResizeWindow(Window* window);
		void				InvokeActivateWindow(Window* window);
		void				InvokeSendWindowBehind(Window* window,
								Window* behindOf);
		void				InvokeSetWindowWorkspaces(Window* window,
									uint32 workspaces);
		void				InvokeShowWindow(Window* window);
		void				InvokeHideWindow(Window* window);
		void				InvokeMinimizeWindow(Window* window, bool minimize);

		bool				InvokeSetDecoratorSettings(Window* window,
								const BMessage& settings);
		void				InvokeGetDecoratorSettings(Window* window,
								BMessage& settings);

private:
		class InvokeGuard {
			public:
				InvokeGuard(bool& invoking);
				~InvokeGuard();
			private:
				bool&	fInvoking;
		};

		DesktopListenerDLList	fDesktopListenerList;
		
		// prevent recursive invokes
		bool					fWeAreInvoking;
};

#endif
