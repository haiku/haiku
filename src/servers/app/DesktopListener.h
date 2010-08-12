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

	virtual void				WindowAdded(Window* window) = 0;
	virtual void				WindowRemoved(Window* window) = 0;
	
	virtual void				KeyPressed(uint32 what, int32 key,
									int32 modifiers) = 0;
	virtual void				MouseEvent(BMessage* message) = 0;
	virtual void				MouseDown(Window* window, BMessage* message,
									const BPoint& where) = 0;
	virtual void				MouseUp(Window* window, BMessage* message,
									const BPoint& where) = 0;
	virtual void				MouseMoved(Window* window, BMessage* message,
									const BPoint& where) = 0;

	virtual void				WindowMoved(Window* window) = 0;
	virtual void				WindowResized(Window* window) = 0;
	virtual void				WindowActitvated(Window* window) = 0;
	virtual void				WindowSentBehind(Window* window,
									Window* behindOf) = 0;
	virtual void				WindowWorkspacesChanged(Window* window,
									uint32 workspaces) = 0;
	virtual void				WindowMinimized(Window* window,
									bool minimize) = 0;

	virtual void				WindowTabLocationChanged(Window* window,
									float location) = 0;

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

		void				WindowAdded(Window* window);
		void				WindowRemoved(Window* window);

		void				KeyPressed(uint32 what, int32 key,
								int32 modifiers);
		void				MouseEvent(BMessage* message);
		void				MouseDown(Window* window, BMessage* message,
									const BPoint& where);
		void				MouseUp(Window* window, BMessage* message,
									const BPoint& where);
		void				MouseMoved(Window* window, BMessage* message,
									const BPoint& where);

		void				WindowMoved(Window* window);
		void				WindowResized(Window* window);
		void				WindowActitvated(Window* window);
		void				WindowSentBehind(Window* window, Window* behindOf);
		void				WindowWorkspacesChanged(Window* window,
									uint32 workspaces);
		void				WindowMinimized(Window* window, bool minimize);

		void				WindowTabLocationChanged(Window* window,
								float location);

		bool				SetDecoratorSettings(Window* window,
								const BMessage& settings);
		void				GetDecoratorSettings(Window* window,
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
