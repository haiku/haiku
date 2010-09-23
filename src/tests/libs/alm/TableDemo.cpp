/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>
#include <File.h>
#include <Button.h>
#include <Window.h>

#include "ALMLayout.h"


class TableDemoWindow : public BWindow {
public:
	TableDemoWindow(BRect frame) 
		: BWindow(frame, "ALM Table Demo", B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE) 
	{	
		// create a new BALMLayout and use  it for this window
		BALMLayout* layout = new BALMLayout();
		SetLayout(layout);
		
		Column* c1 = layout->AddColumn(layout->Left(), layout->Right());
		Row* r1 = layout->AddRow(layout->Top(), NULL);
		Row* r3 = layout->AddRow(NULL, layout->Bottom());
		r1->SetNext(r3);
		Row* r2 = layout->AddRow();
		r2->InsertAfter(r1);
		
		BButton* b1 = new BButton("A1");
		layout->AddView(b1, r1, c1);
		b1->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP));
		
		BButton* b2 = new BButton("A2");
		layout->AddView(b2, r2, c1);
		b2->SetExplicitAlignment(BAlignment(
			B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER));
				
		BButton* b3 = new BButton("A3");
		layout->AddView(b3, r3, c1);
		b3->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_BOTTOM));
		
		r2->HasSameHeightAs(r1);
		r3->HasSameHeightAs(r1);
	}
};


class TableDemo : public BApplication {
public:
	TableDemo() 
		: BApplication("application/x-vnd.haiku.table-demo")
	{
		BRect frameRect;
		frameRect.Set(100, 100, 400, 400);
		TableDemoWindow* window = new TableDemoWindow(frameRect);
		window->Show();
	}
};


int
main()
{
	TableDemo app;
	app.Run();
	return 0;
}

