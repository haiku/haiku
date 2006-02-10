/*
 * Copyright 2001-2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 */

/**	PicturePlayer is used to create and play picture data. */

#ifndef	_TPICTURE_H
#define	_TPICTURE_H


#include <GraphicsDefs.h>
#include <Point.h>
#include <Rect.h>
#include <DataIO.h>


class PicturePlayer {
public:
					PicturePlayer();
					PicturePlayer(void *data, int32 size, BList *pictures);
virtual				~PicturePlayer();

		int16		GetOp();
		bool		GetBool();
		int16		GetInt8();
		int16		GetInt16();
		int32		GetInt32();
		int64		GetInt64();
		float		GetFloat();
		BPoint		GetCoord();
		BRect		GetRect();
		rgb_color	GetColor();
		//void		GetString(char *);

		void		*GetData(int32);
		void		GetData(void *data, int32 size);

		void		AddInt8(int8);
		void		AddInt16(int16);
		void		AddInt32(int32);
		void		AddInt64(int64);
		void		AddFloat(float);
		void		AddCoord(BPoint);
		void		AddRect(BRect);
		void		AddColor(rgb_color);
		void		AddString(char *);

		void		AddData(void *data, int32 size);

		//			SwapOp();
		//			SwapInt8();
		//			SwapInt16();
		//			SwapInt32();
		//			SwapInt64();
		//			SwapFloat();
		//			SwapCoord();
		//			SwapRect();
		//			SwapIRect();
		//			SwapColor();
		//			SwapString();

		//			Swap();

		//			CheckPattern();

		void		BeginOp(int32);
		void		EndOp();

		void		EnterStateChange();
		void		ExitStateChange();

		void		EnterFontChange();
		void		ExitFontChange();

		status_t	Play(void **callBackTable, int32 tableEntries,
						void *userData);
		status_t	Rewind();

private:
		BMemoryIO	fData;
		int32		fSize;
		BList		*fPictures;
};
//------------------------------------------------------------------------------

status_t do_playback(void *, long, BList *, void **, long, void *);

#endif // _TPICTURE_H
