/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef TYPE_EDITORS_H
#define TYPE_EDITORS_H


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

#endif	/* TYPE_EDITORS_H */
