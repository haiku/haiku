/*
 * Copyright 2021, Adrien Destugues <pulkomandy@pulkomandy.tk>
 * Distributed under terms of the MIT license.
 */

#ifndef FONT_H
#define FONT_H


#include <stdint.h>


struct FramebufferFont {
	int glyphWidth;
	int glyphHeight;
	uint8_t data[];
};


extern FramebufferFont smallFont;
extern FramebufferFont spleen12Font;


#endif /* !FONT_H */
