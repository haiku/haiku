/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef GLOBALS_H
#define GLOBALS_H


class BClipboard;
class BFont;

extern BClipboard* gMouseClipboard;
	// clipboard used for mouse copy'n'paste


bool IsFontUsable(const BFont& font);

#endif	// GLOBALS_H
