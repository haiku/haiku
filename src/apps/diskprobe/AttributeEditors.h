/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef ATTRIBUTE_EDITORS_H
#define ATTRIBUTE_EDITORS_H


#include <Rect.h>

class BView;
class DataEditor;


extern BView *GetTypeEditorFor(BRect rect, DataEditor &editor);


#endif	/* ATTRIBUTE_EDITORS_H */
