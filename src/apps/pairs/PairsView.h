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
#ifndef PAIRS_VIEW_H
#define PAIRS_VIEW_H


#include <ObjectList.h>
#include <View.h>


const uint8 kSmallIconSize = 32;
const uint8 kMediumIconSize = 64;
const uint8 kLargeIconSize = 128;

const uint32 kMsgCardButton = 'card';


class BBitmap;
class PairsButton;


class PairsView : public BView {
public:
								PairsView(BRect frame, const char* name,
									uint8 rows, uint8 cols, uint8 iconSize);

	virtual						~PairsView();
	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float newWidth, float newHeight);

	virtual	void				CreateGameBoard();

			int32				Rows() const { return fRows; };
			int32				Cols() const { return fCols; };
	BObjectList<PairsButton>*	PairsButtonList() const
									{ return fPairsButtonList; };

			int32				GetIconPosition(int32 index);

			int32				IconSize() const { return fIconSize; };
			void				SetIconSize(int32 size) { fIconSize = size; };

			int32				Spacing() const { return fIconSize / 6; };

private:
			void				_GenerateCardPositions();
			void				_ReadRandomIcons();
			void				_SetPairsBoard();
			void				_SetPositions();

			uint8				fRows;
			uint8				fCols;
			uint8				fIconSize;
			int32				fButtonsCount;
			int32				fCardsCount;
	BObjectList<PairsButton>*	fPairsButtonList;
	BObjectList<BBitmap>*		fSmallBitmapsList;
	BObjectList<BBitmap>*		fMediumBitmapsList;
	BObjectList<BBitmap>*		fLargeBitmapsList;
			int32*				fRandomPosition;
			int32*				fPositionX;
			int32*				fPositionY;
};


#endif	// PAIRS_VIEW_H
