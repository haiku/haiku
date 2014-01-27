/*
 * Copyright 2008 Ralf Schülke, ralf.schuelke@googlemail.com.
 * Copyright 2010 Adam Smith <adamd.smith@utoronto.ca>
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ralf Schülke, ralf.schuelke@googlemail.com
 *		John Scipione, jscipione@gmail.com
 *		Adam Smith, adamd.smith@utoronto.ca
 */
#ifndef PAIRS_WINDOW_H
#define PAIRS_WINDOW_H


#include <Window.h>


class BMenu;
class BMessageRunner;
class PairsView;


class PairsWindow : public BWindow {
public:
								PairsWindow();
		virtual					~PairsWindow();

		virtual	void			MessageReceived(BMessage* message);

		void					NewGame();
		void					SetGameSize(uint8 rows, uint8 cols);

private:
				void			_MakeGameView(uint8 rows, uint8 cols);
				void			_MakeMenuBar();
				void			_ResizeWindow(uint8 rows, uint8 cols);

				BView*			fBackgroundView;
				PairsView*		fPairsView;
				BMenuBar*		fMenuBar;
				BMessageRunner*	fPairComparing;
				bool			fIsFirstClick;
				bool			fIsPairsActive;
				int32			fPairCardPosition;
				int32			fPairCardTmpPosition;
				int32			fButtonTmpPosition;
				int32			fButtonPosition;
				int32			fButtonClicks;
				int32			fFinishPairs;
				BMenu*			fIconSizeMenu;
};

#endif	// PAIRS_WINDOW_H
