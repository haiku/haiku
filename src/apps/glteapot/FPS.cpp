/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#include "FPS.h"

FPS::FPS()
{
}


FPS::~FPS()
{
}


void
FPS::drawChar(GLfloat x, GLfloat y, GLint number)
{
	static bool numbers[13][7] = {
		{ true, true, true, true, true, true, false }, 		/* 0 */
		{ false, true, true, false, false, false, false },	/* 1 */
		{ true, true, false, true, true, false, true },		/* 2 */
		{ true, true, true, true, false, false, true },		/* 3 */
		{ false, true, true, false, false, true, true },	/* 4 */
		{ true, false, true, true, false, true, true },		/* 5 */
		{ true, false, true, true, true, true, true },		/* 6 */
		{ true, true, true, false, false, false, false },	/* 7 */
		{ true, true, true, true, true, true, true },		/* 8 */
		{ true, true, true, false, false, true, true },		/* 9 */

		{ true, false, false, false, true, true, true },	/* F */
		{ true, true, false, false, true, true, true },		/* P */
		{ true, false, true, true, false, true, true },		/* S */
	};

	static GLfloat gap = 0.03;
	static GLfloat size = 1.0;

	static GLfloat x0 = -size / 4;
	static GLfloat x1 = -size / 4 + gap;
	static GLfloat x2 = -x1;
	static GLfloat x3 = -x0;

	static GLfloat y0 = size / 2;
	static GLfloat y1 = size / 2 - gap;
	static GLfloat y2 = 0 + gap;
	static GLfloat y3 = 0;
	static GLfloat y4 = -y2;
	static GLfloat y5 = -y1;
	static GLfloat y6 = -y0;

	glBegin(GL_LINES);
	if (numbers[number][0]) {
		glVertex2f(x1 + x, y0 + y);
		glVertex2f(x2 + x, y0 + y);
	}

	if (numbers[number][1]) {
		glVertex2f(x3 + x, y1 + y);
		glVertex2f(x3 + x, y2 + y);
	}

	if (numbers[number][2]) {
		glVertex2f(x3 + x, y4 + y);
		glVertex2f(x3 + x, y5 + y);
	}

	if (numbers[number][3]) {
		glVertex2f(x1 + x, y6 + y);
		glVertex2f(x2 + x, y6 + y);
	}

	if (numbers[number][4]) {
		glVertex2f(x0 + x, y5 + y);
		glVertex2f(x0 + x, y4 + y);
	}

	if (numbers[number][5]) {
		glVertex2f(x0 + x, y2 + y);
		glVertex2f(x0 + x, y1 + y);
	}

	if (numbers[number][6]) {
		glVertex2f(x1 + x, y3 + y);
		glVertex2f(x2 + x, y3 + y);
	}

	glEnd();
}


void
FPS::drawCounter(GLfloat frameRate)
{
	GLfloat pos = 0;
	int ifps = (int)frameRate;

	int count = 0;
	int number = 1;
	while (ifps > number) {
		number *= 10;
		count++;
	}

	number /= 10;
	for (int i = 0; i < count; i++) {
		drawChar(pos, 0, (ifps / number) % 10);
		pos += 1.0;
		if (number == 1)
			break;
		number /= 10;
	}

	pos += 0.5;
	drawChar(pos, 0, 10);
	pos += 1;
	drawChar(pos, 0, 11);
	pos += 1;
	drawChar(pos, 0, 12);
	pos += 1;
}
