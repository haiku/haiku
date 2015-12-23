/*
 * Copyright 2015, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Julian Harnath <julian.harnath@rwth-aachen.de>
 */
#ifndef PICTURE_BOUNDING_BOX_H
#define PICTURE_BOUNDING_BOX_H


class BRect;
class DrawState;
class ServerPicture;


class PictureBoundingBoxPlayer {
public:
	class State;

public:
	static	void				Play(ServerPicture* picture,
									const DrawState* drawState,
									BRect* outBoundingBox);
};


#endif // PICTURE_BOUNDING_BOX_H
