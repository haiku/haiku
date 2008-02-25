/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>
#include <Button.h>
#include <List.h>
#include <Window.h>

#include "Area.h"
#include "BALMLayout.h"
#include "OperatorType.h"
#include "XTab.h"
#include "YTab.h"


using namespace BALM;

class SimpleTestWindow : public BWindow {
public:
	SimpleTestWindow(BRect frame) 
		: BWindow(frame, "SimpleTest", B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
	{
		fLS = new BALMLayout();
		SetLayout(fLS);
				
		button1 = new BButton("button1");		
		SetBALMLayout();
		
		Show();
	}
	
	
	void
	SetBALMLayout()
	{
		Area* a = fLS->AddArea(fLS->Left(), fLS->Top(), fLS->Right(), fLS->Bottom(), button1);
		a->SetLeftInset(10);
		fLS->Save("/boot/home/Desktop/test1_ls.txt");

		
//		fLS->AddConstraint(2.0, x1, -1.0, fLS->Right(), OperatorType(EQ), 0.0);		
//		fLS->AddConstraint(2.0, y1, -1.0, fLS->Bottom(), OperatorType(EQ), 0.0);
	}
	
	
private:
	BALMLayout* fLS;
	BButton* button1;
};


class SimpleTest : public BApplication {
public:
	SimpleTest() 
		: BApplication("application/x-vnd.haiku.simpletest") 
	{
		BRect frameRect;
		frameRect.Set(100, 100, 392, 366);
		SimpleTestWindow* theWindow = new SimpleTestWindow(frameRect);
	}
};


int
main()
{
	SimpleTest test;
	test.Run();
	return 0;
}

