/*
 * Copyright 2008 Ralf Schülke, ralf.schuelke@googlemail.com.
 * Copyright 2010 Adam Smith <adamd.smith@utoronto.ca>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef PAIRS_WINDOW_H
#define PAIRS_WINDOW_H

#include <Window.h>

class PairsView;
class BMessageRunner;


class PairsWindow : public BWindow {
public:
								PairsWindow();
		virtual					~PairsWindow();

		virtual	void			MessageReceived(BMessage* message);

		void					NewGame();
		void					SetGameSize(int width, int height);

private:
				void			_MakeGameView(int width, int height);
				void			_MakeMenuBar();

				BView*			fBackgroundView;
				PairsView*		fPairsView;
				BMenuBar*		fMenuBar;
				BMessageRunner*	fPairComparing;
				bool			fIsFirstClick;
				bool			fIsPairsActive;
				int				fPairCard;
				int				fPairCardTmp;
				int				fButtonTmp;
				int				fButton;
				int				fButtonClicks;
				int				fFinishPairs;
};

#endif	// PAIRS_WINDOW_H
