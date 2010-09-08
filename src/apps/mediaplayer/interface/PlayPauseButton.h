/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>.
 * Distributed under the terms of the MIT License.
 */
#ifndef PLAY_PAUSE_BUTTON_H
#define PLAY_PAUSE_BUTTON_H


#include "SymbolButton.h"


class PlayPauseButton : public SymbolButton {
public:
								PlayPauseButton(const char* name,
									BShape* playSymbolShape,
									BShape* pauseSymbolShape,
									BMessage* message = NULL,
									uint32 borders
										= BControlLook::B_ALL_BORDERS);

	virtual						~PlayPauseButton();

	// BButton interface
	virtual	void				Draw(BRect updateRect);
	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();

	// PlayPauseButton
			void				SetPlaying();
			void				SetPaused();
			void				SetStopped();

			void				SetSymbols(BShape* playSymbolShape,
									BShape* pauseSymbolShape);

private:
			void				_SetPlaybackState(uint32 state);

private:
			BShape*				fPlaySymbol;
			BShape*				fPauseSymbol;
			enum {
				kStopped = 0,
				kPaused,
				kPlaying
			};
			uint32				fPlaybackState;
};


#endif	// PLAY_PAUSE_BUTTON_H
