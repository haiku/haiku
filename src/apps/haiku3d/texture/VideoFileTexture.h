/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */
#ifndef _VIDEO_FILE_TEXTURE_H
#define _VIDEO_FILE_TEXTURE_H

#include "Texture.h"


class BMediaFile;
class BMediaTrack;
class BBitmap;


class VideoFileTexture : public Texture {
public:
						VideoFileTexture(const char* fileName);
		virtual			~VideoFileTexture();

		virtual	void	Update(float dt);
protected:
		void			_Load(const char* fileName);

		BMediaFile*		fMediaFile;
		BMediaTrack*	fVideoTrack;
		BBitmap*		fVideoBitmap;
};

#endif /* _VIDEO_TEXTURE_H */
