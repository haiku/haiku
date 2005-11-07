/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Application.h>
#include <Window.h>
#include <TabView.h>
#include <Box.h>

#include "FontView.h"
#include "ButtonView.h"
#include "FontsSettings.h"

#define M_ENABLE_REVERT 'enrv'
#define M_REVERT 'rvrt'
#define M_SET_DEFAULTS 'stdf'

#define M_SET_PLAIN 'stpl'
#define M_SET_BOLD 'stbl'
#define M_SET_FIXED 'stfx'

class MainWindow : public BWindow {
	public:
		MainWindow();

		virtual	bool	QuitRequested();
		virtual	void	MessageReceived(BMessage *message);

	private:
		FontView		*fSelectorView;
		ButtonView		*fButtonView;

		FontsSettings	fSettings;
};

#endif	/* MAIN_WINDOW_H */
