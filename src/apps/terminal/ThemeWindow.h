/*
 * Copyright 2022, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef THEME_WINDOW_H
#define THEME_WINDOW_H


#include <Application.h>
#include <Button.h>
#include <Window.h>
#include <Message.h>

// local messages
const uint32 MSG_THEME_CLOSED = 'mstc';

class BFilePanel;
class PrefHandler;
class ThemeView;


class ThemeWindow : public BWindow {
public:
							ThemeWindow(const BMessenger &messenger);
			virtual			~ThemeWindow() {};

			virtual void	MessageReceived(BMessage *message);
			virtual void	Quit();
			virtual bool	QuitRequested();

private:
			void			_Save();
			void			_SaveAs();
			void			_Revert();
			void			_SaveRequested(BMessage *message);

		PrefHandler*		fPreviousPref;
		BFilePanel*			fSavePanel;

		ThemeView*			fThemeView;
		BButton*			fDefaultsButton;
		BButton*			fRevertButton;
		BButton*			fSaveAsFileButton;

		bool				fDirty;
		BMessenger			fTerminalMessenger;
};


#endif
