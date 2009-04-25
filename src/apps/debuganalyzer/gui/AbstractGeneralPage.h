/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ABSTRACT_GENERAL_PAGE_H
#define ABSTRACT_GENERAL_PAGE_H

#include <GroupView.h>
#include <StringView.h>


class BGridView;


class LabelView : public BStringView {
public:
	LabelView(const char* text)
		:
		BStringView(NULL, text)
	{
		SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT,
			B_ALIGN_VERTICAL_CENTER));
	}
};


class TextDataView : public BStringView {
public:
	TextDataView()
		:
		BStringView(NULL, "")
	{
		SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	}

	TextDataView(const char* text)
		:
		BStringView(NULL, text)
	{
		SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	}
};


class AbstractGeneralPage : public BGroupView {
public:
								AbstractGeneralPage();
	virtual						~AbstractGeneralPage();

protected:
			TextDataView*		AddDataView(const char* label,
									const char* text = NULL);

protected:
			BGridView*			fDataView;
};



#endif	// ABSTRACT_GENERAL_PAGE_H
