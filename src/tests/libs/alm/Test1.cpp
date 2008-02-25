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

class Test1Window : public BWindow {
public:
	Test1Window(BRect frame) 
		: BWindow(frame, "Test1", B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
	{
		fLS = new BALMLayout();
		
		SetLayout(fLS);
				
		button1 = new BButton("button1");
		button2 = new BButton("button2");
		button3 = new BButton("button3");
		
		fLS->SetPerformancePath("/boot/home/Desktop/test1_performance.txt");
		SetBALMLayout();
		
		Show();
	}
	
	
	void
	SetBALMLayout()
	{
		XTab* x1 = fLS->AddXTab();
		YTab* y1 = fLS->AddYTab();
		
		fLS->AddArea(fLS->Left(), fLS->Top(), x1, y1, button1);
		fLS->AddArea(x1, fLS->Top(), fLS->Right(), y1, button2);
		fLS->AddArea(fLS->Left(), y1, fLS->Right(), fLS->Bottom(), button3);
		
		
		fLS->AddConstraint(2.0, x1, -1.0, fLS->Right(), OperatorType(EQ), 0.0);		
		fLS->AddConstraint(2.0, y1, -1.0, fLS->Bottom(), OperatorType(EQ), 0.0);
	}
	
	
private:
	BALMLayout* fLS;
	BButton* button1;
	BButton* button2;
	BButton* button3;
};


class Test1 : public BApplication {
public:
	Test1() 
		: BApplication("application/x-vnd.haiku.test1") 
	{
		BRect frameRect;
		frameRect.Set(100, 100, 392, 366);
		Test1Window* theWindow = new Test1Window(frameRect);
	}
};


int
main()
{
	Test1 test;
	test.Run();
	return 0;
}

