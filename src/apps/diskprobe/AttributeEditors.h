/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ATTRIBUTE_EDITORS_H
#define ATTRIBUTE_EDITORS_H


#include <View.h>

class DataEditor;


class TypeEditorView : public BView {
	public:
		TypeEditorView(BRect rect, const char *name, uint32 resizingMode, uint32 flags)
			: BView(rect, name, resizingMode, flags)
		{
		}

		virtual void CommitChanges() = 0;
};

extern TypeEditorView *GetTypeEditorFor(BRect rect, DataEditor &editor);

#endif	/* ATTRIBUTE_EDITORS_H */
