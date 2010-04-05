/*
 * Copyright 2008 Ralf Schülke, ralf.schuelke@googlemail.com.
 * Copyright 2010 Adam Smith <adamd.smith@utoronto.ca>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PAIRS_VIEW_H
#define PAIRS_VIEW_H


#include <View.h>


class TopButton;

class PairsView : public BView {
public:
								PairsView(BRect frame, const char* name,
									int width, int height,
									uint32 resizingMode);

	virtual						~PairsView();
	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				CreateGameBoard();

			int 				fWidth;
			int					fHeight;
			int					fNumOfCards;

			BList				fDeckCard;
			int					GetIconFromPos(int pos);

private:
			void				_SetPairsBoard();
			void				_ReadRandomIcons();
			void				_GenerateCardPos();
			bool				_HasBitmap(BList& bitmaps, BBitmap* bitmap);

			BMessage*			fButtonMessage;
			BList				fCardBitmaps;
			int*				fRandPos;
			int*				fPosX;
			int*				fPosY;
};


#endif	// PAIRS_VIEW_H
