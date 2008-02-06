
#include <Application.h>
#include <File.h>
#include <Button.h>
#include <Window.h>

#include "Area.h"
#include "Column.h"
#include "BALMLayout.h"
#include "Row.h"


using namespace BALM;

class TableTestWindow : public BWindow {
public:
	TableTestWindow(BRect frame) : BWindow(frame, "TableTest", B_TITLED_WINDOW,
			B_QUIT_ON_WINDOW_CLOSE) {
		
		BALMLayout* fLS = new BALMLayout();
		
		SetLayout(fLS);
		
		fLS->SetPerformancePath("/boot/home/Desktop/table_test_performance.txt");
		
		Column* c1 = fLS->AddColumn(fLS->Left(), fLS->Right());
		Row* r1 = fLS->AddRow(fLS->Top(), NULL);
		Row* r3 = fLS->AddRow(NULL, fLS->Bottom());
		r1->SetNext(r3);
		Row* r2 = fLS->AddRow();
		r2->InsertAfter(r1);
		
		BButton* b1 = new BButton("A1");
		Area* a1 = fLS->AddArea(r1, c1, b1);
		a1->SetHAlignment(B_ALIGN_LEFT);
		a1->SetVAlignment(B_ALIGN_TOP);
		
		BButton* b2 = new BButton("A2");
		Area* a2 = fLS->AddArea(r2, c1, b2);
		a2->SetHAlignment(B_ALIGN_HORIZONTAL_CENTER);
		a2->SetVAlignment(B_ALIGN_VERTICAL_CENTER);
				
		BButton* b3 = new BButton("A3");
		Area* a3 = fLS->AddArea(r3, c1, b3);
		a3->SetHAlignment(B_ALIGN_RIGHT);
		a3->SetVAlignment(B_ALIGN_BOTTOM);
		
		r2->HasSameHeightAs(r1);
		r3->HasSameHeightAs(r1);
		
		Show();
	}
};


class TableTest : public BApplication {
public:
	TableTest() : BApplication("application/x-vnd.haiku.table-test") {
		BRect frameRect;
		frameRect.Set(100, 100, 400, 400);
		TableTestWindow* theWindow = new TableTestWindow(frameRect);
	}
};


int main() {
	TableTest test;
	test.Run();
	return 0;
}
