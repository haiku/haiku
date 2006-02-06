/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef WORKSPACE_PRIVATE_H
#define WORKSPACE_PRIVATE_H


#include "RGBColor.h"
#include "WindowList.h"
#include "Workspace.h"

#include <Accelerant.h>
#include <ObjectList.h>
#include <String.h>


struct display_info {
	BString			identifier;
	BPoint			origin;
	display_mode	mode;
};

class Workspace::Private {
	public:
		Private();
		~Private();

		int32				Index() const { return fWindows.Index(); }

		WindowList&			Windows() { return fWindows; }

		// displays

		void				SetDisplaysFromDesktop(Desktop* desktop);

		int32				CountDisplays() const { return fDisplays.CountItems(); }
		const display_info*	DisplayAt(int32 index) const { return fDisplays.ItemAt(index); }

		// configuration

		const RGBColor&		Color() const { return fColor; }
		void				SetColor(const RGBColor& color);

		void				RestoreConfiguration(const BMessage& settings);
		void				StoreConfiguration(BMessage& settings);

	private:
		void				_SetDefaults();

		WindowList			fWindows;
		WindowLayer*		fFront;
		WindowLayer*		fFocus;

		BObjectList<display_info> fDisplays;

		RGBColor			fColor;
};

#endif	/* WORKSPACE_PRIVATE_H */
