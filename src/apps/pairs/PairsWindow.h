/*
 * Copyright 2007, Ralf Schülke, teammaui@web.de. All rights reserved.
 * Distributed under the terms of the MIT License.
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

private:
				BView*			fBackgroundView;
				PairsView*		fPairsView;
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
