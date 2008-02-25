/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>
#include <Button.h>
#include <List.h>
#include <StringView.h>
#include <TextControl.h>
#include <TextView.h>
#include <Window.h>

#include "Area.h"
#include "BALMLayout.h"
#include "OperatorType.h"
#include "XTab.h"
#include "YTab.h"


using namespace BALM;

class Test2Window : public BWindow {
public:
	Test2Window(BRect frame)
		: BWindow(frame, "Test2", B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
	{			
		fLS = new BALMLayout();
				
		SetLayout(fLS);
		
		button1 = new BButton("button1");
		button2 = new BButton("button2");
		button3 = new BButton("button3");
		button4 = new BButton("button4");
		button5 = new BButton("button5");
		button6 = new BButton("button6");
		checkedListBox1 = new BButton("checkedListBox1");
		textBox1 = new BButton("textBox1");
		textBox2 = new BButton("textBox2");
		richTextBox1 = new BButton("richTextBox1");
		//~ textBox1 = new BTextView(frame, "textBox1", NULL, NULL);
		//~ textBox2 = new BTextView(frame, "textBox2", NULL, NULL);
		//~ richTextBox1 = new BTextControl(NULL, "richTextBox1", NULL);
		listView1 = new BButton("listView1");
		label1 = new BStringView(frame, "label1", "label1");
		label2 = new BStringView(frame, "label2", "label2");
		label3 = new BStringView(frame, "label3", "label3");
		
		fLS->SetPerformancePath("/boot/home/Desktop/test2_performance.txt");
		SetBALMLayout();
		
		Show();
	}
	
	
	void
	SetBALMLayout()
	{
		XTab* x1 = fLS->AddXTab();
		XTab* x2 = fLS->AddXTab();
		XTab* x3 = fLS->AddXTab();
		YTab* y1 = fLS->AddYTab();
		YTab* y2 = fLS->AddYTab();
		YTab* y3 = fLS->AddYTab();
		YTab* y4 = fLS->AddYTab();
		YTab* y5 = fLS->AddYTab();

		fLS->AddArea(fLS->Left(), fLS->Top(), x1, y1, button1);
		fLS->AddArea(x1, fLS->Top(), x2, y1, button2);
		fLS->AddArea(fLS->Left(), y1, x1, y2, button3);
		fLS->AddArea(x1, y1, x2, y2, button4);
		fLS->AddArea(fLS->Left(), y2, x1, y3, button5);
		fLS->AddArea(x1, y2, x2, y3, button6);

		fLS->AddConstraint(2.0, x1, -1.0, x2, OperatorType(EQ), 0.0);
		fLS->AddConstraint(2.0, y1, -1.0, y2, OperatorType(EQ), 0.0);
		fLS->AddConstraint(1.0, y1, 1.0, y2, -1.0, y3, OperatorType(EQ), 0.0);
		
		fLS->AddArea(fLS->Left(), y3, x2, y4, checkedListBox1);
		fLS->AddArea(x2, fLS->Top(), fLS->Right(), y2, textBox1);
		fLS->AddArea(fLS->Left(), y4, x3, y5, listView1);
		fLS->AddArea(x3, y2, fLS->Right(), y5, textBox2);

		Area* richTextBox1Area = fLS->AddArea(x2, y2, x3, y4, richTextBox1);
		richTextBox1Area->SetLeftInset(10);
		richTextBox1Area->SetTopInset(10);
		richTextBox1Area->SetRightInset(10);
		richTextBox1Area->SetBottomInset(10);

		Area* label1Area = fLS->AddArea(fLS->Left(), y5, x2, fLS->Bottom(), label1);
		label1Area->SetHAlignment(B_ALIGN_LEFT);
		label1Area->SetTopInset(4);
		label1Area->SetBottomInset(4);

		Area* label2Area = fLS->AddArea(x2, y5, x3, fLS->Bottom(), label2);
		label2Area->SetHAlignment(B_ALIGN_HORIZONTAL_CENTER);
		label2Area->SetTopInset(4);
		label2Area->SetBottomInset(4);

		Area* label3Area = fLS->AddArea(x3, y5, fLS->Right(), fLS->Bottom(), label3);
		label3Area->SetHAlignment(B_ALIGN_RIGHT);
		label3Area->SetTopInset(4);
		label3Area->SetBottomInset(4);
	}
		
	
private:
	BALMLayout* fLS;
	BButton* button1;
	BButton* button2;
	BButton* button3;
	BButton* button4;
	BButton* button5;
	BButton* button6;
	BButton* checkedListBox1;
	BButton* textBox1;
	BButton* textBox2;
	BButton* richTextBox1;
	//~ BTextView* textBox1;
	//~ BTextView* textBox2;
	//~ BTextControl* richTextBox1;
	BButton* listView1;
	BStringView* label1;
	BStringView* label2;
	BStringView* label3;
};


class Test2 : public BApplication {
public:
	Test2()
		: BApplication("application/x-vnd.haiku.test2")
	{
		BRect frameRect;
		frameRect.Set(100, 100, 610, 456);
		Test2Window* theWindow = new Test2Window(frameRect);
	}
};


int
main()
{
	Test2 test;
	test.Run();
	return 0;
}

