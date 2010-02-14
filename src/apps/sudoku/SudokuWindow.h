/*
 * Copyright 2007-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SUDOKU_WINDOW_H
#define SUDOKU_WINDOW_H


#include <Window.h>


class BFile;
class BFilePanel;
class BMenu;
class BMenuItem;
class GenerateSudoku;
class ProgressWindow;
class SudokuView;


class SudokuWindow : public BWindow {
public:
								SudokuWindow();
	virtual						~SudokuWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

private:
			status_t			_OpenSettings(BFile& file, uint32 mode);
			status_t			_LoadSettings(BMessage& settings);
			status_t			_SaveSettings();

			void				_ResetStoredState();
			void				_MessageDropped(BMessage *message);
			void				_Generate(int32 level);
			int32				_Level() const;
			void				_SetLevel(int32 level);

private:
			BFilePanel*			fOpenPanel;
			BFilePanel*			fSavePanel;
			ProgressWindow*		fProgressWindow;
			SudokuView*			fSudokuView;
			GenerateSudoku*		fGenerator;
			BMenuItem*			fRestoreStateItem;
			BMenuItem*			fUndoItem;
			BMenuItem*			fRedoItem;
			BMenu*				fNewMenu;
			BMessage*			fStoredState;
			uint32				fExportFormat;
};

#endif	// SUDOKU_WINDOW_H
