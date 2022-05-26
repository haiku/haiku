/*****************************************************************************/
// BTranslatorItem
// Written by Michael Wilber, Haiku Translation Kit Team
//
// BTranslatorItem.h
//
// BStringItem based class for using a list of Translators in a BListView
//
//
// Copyright (c) 2003 Haiku Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef TRANSLATOR_ITEM_H
#define TRANSLATOR_ITEM_H

#include <ListItem.h>
#include <String.h>

// Group Options
enum {
	UNKNOWN_GROUP = 0,
	SYSTEM_TRANSLATOR = 1,
	USER_TRANSLATOR = 2
};

class BTranslatorItem : public BStringItem {
public:
	BTranslatorItem(const char *text, const char *path, int32 group);

	const char *Path() const;
	int32 Group() const;

private:
	BString fpath;
	int32 fgroup;
};

#endif // #ifndef TRANSLATOR_ITEM_H
