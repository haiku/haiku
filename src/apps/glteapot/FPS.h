/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <GL/gl.h>

class FPS {
	public:
		FPS();
		~FPS();
		static void drawCounter(GLfloat frameRate);

	private:
		static void drawChar(GLfloat x, GLfloat y, GLint number);
};

