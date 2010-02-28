/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/
#ifndef _WORDS_H
#define _WORDS_H


#include <List.h>
#include <String.h>

#include "WIndex.h"


typedef enum {
	COMPARE,
	GENERATE
} metaphlag;


bool metaphone(const char* Word, char* Metaph, metaphlag Flag);
int word_match(const char* reference, const char* test);
int32 suffix_word(char* dst, const char* src, char flag);
void sort_word_list(BList* matches, const char* reference);


class Words : public WIndex {
public:
							Words(bool useMetaphone = true);
							Words(BPositionIO* thes, bool useMetaphone = true);
							Words(const char* dataPath, const char* indexPath,
								bool useMetaphone);
	virtual					~Words(void);

	virtual	status_t		BuildIndex(void);
	virtual	int32			GetKey(const char* s);

			int32			FindBestMatches(BList* matches, const char* word);

protected:
			bool			fUseMetaphone;
};

#endif // #ifndef _WORDS_H

