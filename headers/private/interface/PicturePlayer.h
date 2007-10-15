/*
 * Copyright 2001-2007, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 *		Marcus Overhagen (marcus@overhagen.de)
 */
#ifndef	_PICTURE_PLAYER_H
#define	_PICTURE_PLAYER_H

/*!	PicturePlayer is used to play picture data. */


#include <GraphicsDefs.h>
#include <Point.h>
#include <Rect.h>
#include <DataIO.h>

class BList;


namespace BPrivate {

class PicturePlayer {
public:
	PicturePlayer();
	PicturePlayer(const void *data, size_t size, BList *pictures);
	virtual	~PicturePlayer();

	status_t	Play(void **callBackTable, int32 tableEntries,
					void *userData);

private:
	const void*	fData;
	size_t		fSize;
	BList*		fPictures;
};

} // namespace BPrivate

#endif // _PICTURE_PLAYER_H
