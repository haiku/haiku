/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef WORKSPACE_H
#define WORKSPACE_H


#include <SupportDefs.h>


class Desktop;
class RGBColor;
class WindowLayer;


class Workspace {
	public:
		Workspace(Desktop& desktop, int32 index);
		~Workspace();

		const RGBColor& Color() const;
		bool		IsCurrent() const
						{ return fCurrentWorkspace; }

		status_t	GetNextWindow(WindowLayer*& _window, BPoint& _leftTop);
		void		RewindWindows();

		class Private;

	private:
		Workspace::Private& fWorkspace;
		Desktop&	fDesktop;
		WindowLayer* fCurrent;
		bool		fCurrentWorkspace;
};

#endif	/* WORKSPACE_H */
