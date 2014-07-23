/*
 * Copyright 2014 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include <List.h>
#include <String.h>
#include <View.h>
#include <Window.h>


class BMenuField;
class TestView;


class Test {
public:
								Test(const char* name);
	virtual						~Test();

			const char*			Name() const
									{ return fName.String(); }

	virtual	void				Draw(BView* view, BRect updateRect) = 0;

private:
			BString				fName;
};


class TestWindow : public BWindow {
public:
								TestWindow(const char* title);
	virtual						~TestWindow();

	virtual	void				MessageReceived(BMessage* message);

			void				AddTest(Test* test);
			void				SetToTest(int32 index);

private:
			TestView*			fTestView;

			BMenuField*			fTestSelectionField;

			BList				fTests;
};


