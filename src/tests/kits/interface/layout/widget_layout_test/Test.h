/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_TEST_H
#define WIDGET_LAYOUT_TEST_TEST_H


#include <Handler.h>
#include <String.h>


class BView;
class View;


class Test : public BHandler {
public:
								Test(const char* name, const char* description,
									BView* view = NULL);
	virtual						~Test();

			const char*			Name() const;
			const char*			Description() const;

	virtual	BView*				GetView() const;
	virtual	void				SetView(BView* view);

	virtual	void				ActivateTest(View* controls);
	virtual	void				DectivateTest();

private:
			BString				fName;
			BString				fDescription;
			BView*				fView;
};


#endif	// WIDGET_LAYOUT_TEST_TEST_H
