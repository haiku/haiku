/*
 * Copyright 2005-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef WORKSPACE_H
#define WORKSPACE_H


#include <InterfaceDefs.h>


class Desktop;
class Window;


class Workspace {
	public:
		Workspace(Desktop& desktop, int32 index);
		~Workspace();

		const rgb_color& Color() const;
		void		SetColor(const rgb_color& color, bool makeDefault);
		bool		IsCurrent() const
						{ return fCurrentWorkspace; }

		status_t	GetNextWindow(Window*& _window, BPoint& _leftTop);
		status_t	GetPreviousWindow(Window*& _window, BPoint& _leftTop);
		void		RewindWindows();

		class Private;

	private:
		Workspace::Private& fWorkspace;
		Desktop&	fDesktop;
		Window*		fCurrent;
		bool		fCurrentWorkspace;
};

#endif	/* WORKSPACE_H */
