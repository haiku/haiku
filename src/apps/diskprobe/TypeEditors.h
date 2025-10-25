/*
 * Copyright 2004-2018, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef TYPE_EDITORS_H
#define TYPE_EDITORS_H


#include <View.h>


class DataEditor;


class TypeEditorView : public BView {
public:
								TypeEditorView(const char* name, uint32 flags,
									DataEditor& editor);
	virtual						~TypeEditorView();

	virtual	void				CommitChanges();
	virtual	bool				TypeMatches();

protected:
			DataEditor&			fEditor;
};


extern TypeEditorView* GetTypeEditorFor(DataEditor& editor);
extern status_t GetNthTypeEditor(int32 index, const char** _name);
extern TypeEditorView* GetTypeEditorAt(int32 index, DataEditor& editor);


#endif	/* TYPE_EDITORS_H */
