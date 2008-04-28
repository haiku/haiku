/*
 * Copyright 2008, Ralf Schülke, teammaui@web.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef PAIRS_VIEW_H
#define PAIRS_VIEW_H


#include <View.h>
class TopButton;

class PairsView : public BView {
public:
								PairsView(BRect frame, const char* name,
									uint32 resizingMode);
		
	virtual						~PairsView();
	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				CreateGameBoard();
			
			TopButton*			fDeckCard[16];
			int					GetIconFromPos(int pos);
					
private:
			void				_SetPairsBoard();
			void				_ReadRandomIcons();
			void				_GenarateCardPos();

			BMessage*			fButtonMessage;
			BBitmap*			fCard[8];
			int					fRandPos[16];
			int					fPosX[16];
			int					fPosY[16];
};


#endif	// PAIRS_VIEW_H
