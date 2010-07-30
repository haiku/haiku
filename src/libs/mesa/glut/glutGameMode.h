/*
 * Copyright 2010, Haiku Inc.
 * Authors:
 *		Philippe Houdoin <phoudoin %at% haiku-os %dot% org>
 *
 * Distributed under the terms of the MIT License.
 */

#include <GL/glut.h>


/*!	All information needed for game mode and
	subwindows (handled as similarly as possible).
*/
class GlutGameMode {
public:
	int	width;
	int	height;
	int	pixelDepth;
	int	refreshRate;
};
