#include "PrintTestWindow.hpp"

#include "PrintTestView.hpp"

#include <Application.h>
#include <MenuItem.h>
#include <MenuBar.h>
#include <View.h>

PrintTestWindow::PrintTestWindow()
	: Inherited(BRect(100,100,500,300), "Haiku Printing", B_DOCUMENT_WINDOW, 0)
{
	BuildGUI();
}

bool PrintTestWindow::QuitRequested()
{
	bool isOk = Inherited::QuitRequested();
	if (isOk) {
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	
	return isOk;
}


void PrintTestWindow::BuildGUI()
{
	BView* backdrop = new BView(Bounds(), "backdrop", B_FOLLOW_ALL, B_WILL_DRAW);
	backdrop->SetViewColor(::ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(backdrop);
	
	BMenuBar* mb = new BMenuBar(Bounds(), "menubar");
	BMenu* m = new BMenu("File");
		m->AddItem(new BMenuItem("Page Setup" B_UTF8_ELLIPSIS, new BMessage('PStp'), 'P', B_SHIFT_KEY));
		m->AddItem(new BMenuItem("Print" B_UTF8_ELLIPSIS, new BMessage('Prnt'), 'P'));
		m->AddSeparatorItem();
		m->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
		m->SetTargetForItems(be_app_messenger);
	mb->AddItem(m);

	m = new BMenu("Edit");
		m->AddItem(new BMenuItem("Undo", new BMessage(B_UNDO), 'Z'));
		m->AddSeparatorItem();
		m->AddItem(new BMenuItem("Cut", new BMessage(B_CUT), 'X'));
		m->AddItem(new BMenuItem("Copy", new BMessage(B_COPY), 'C'));
		m->AddItem(new BMenuItem("Paste", new BMessage(B_PASTE), 'V'));
		m->AddItem(new BMenuItem("Clear", new BMessage(B_DELETE)));
		m->AddSeparatorItem();
		m->AddItem(new BMenuItem("Select All", new BMessage(B_SELECT_ALL)));
	mb->AddItem(m);

	backdrop->AddChild(mb);

	BRect b = Bounds();
	b.top = mb->Bounds().bottom +1;
	backdrop->AddChild(new PrintTestView(b));
}

