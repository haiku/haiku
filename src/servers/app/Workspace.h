/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef WORKSPACE_H
#define WORKSPACE_H


#include "RGBColor.h"

#include <ObjectList.h>
#include <String.h>


class RootLayer;
class WindowLayer;


struct display_info {
	BString			identifier;
	BPoint			origin;
	display_mode	mode;
};

struct window_layer_info {
	BPoint			position;
	WindowLayer*	window;
};

class Workspace {
	public:
		Workspace();
		~Workspace();

		void				SetWindows(const BObjectList<window_layer_info>& windows);
		bool				AddWindow(WindowLayer* window);
		void				RemoveWindow(WindowLayer* window);

		int32				CountWindows() const { return fWindows.CountItems(); }
		window_layer_info*	WindowAt(int32 index) const { return fWindows.ItemAt(index); }

		// displays

		void				SetDisplaysFromDesktop(Desktop* desktop);

		int32				CountDisplays() const { return fDisplays.CountItems(); }
		const display_info*	DisplayAt(int32 index) const { return fDisplays.ItemAt(index); }

		// configuration

		const RGBColor&		Color() const { return fColor; }
		void				SetColor(const RGBColor& color);

		void				SetSettings(BMessage& settings);
		void				GetSettings(BMessage& settings);

	private:
		void				_SetDefaults();

		BObjectList<window_layer_info> fWindows;
		WindowLayer*		fFront;
		WindowLayer*		fFocus;

		BObjectList<display_info> fDisplays;

		RGBColor			fColor;
};

extern const BPoint kInvalidWindowPosition;

#endif	/* WORKSPACE_H */
