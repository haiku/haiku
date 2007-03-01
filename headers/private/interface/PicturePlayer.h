/*
 * Copyright 2001-2007, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 *		Marcus Overhagen (marcus@overhagen.de)
 */

/**	PicturePlayer is used to play picture data. */

#ifndef	_TPICTURE_H
#define	_TPICTURE_H


#include <GraphicsDefs.h>
#include <Point.h>
#include <Rect.h>
#include <DataIO.h>


class PicturePlayer {
public:
					PicturePlayer();
					PicturePlayer(const void *data, size_t size, BList *pictures);
virtual				~PicturePlayer();

		void		BeginOp(int32);
		void		EndOp();

		void		EnterStateChange();
		void		ExitStateChange();

		void		EnterFontChange();
		void		ExitFontChange();

		status_t	Play(void **callBackTable, int32 tableEntries,
						void *userData);

private:
		const void *fData;
		size_t		fSize;
		BList		*fPictures;
};

#endif // _TPICTURE_H
