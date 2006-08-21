// Test works like this:
// Start the app from the terminal
// 1. Click on "menu ONE"
// 2. Click on "menu TWO".
// Examine the output.

#include <Application.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Window.h>

#include <cstdio>


class TestWindow : public BWindow {
public:
	TestWindow();
	virtual void MenusBeginning();
	virtual void MenusEnded();
};


void
show_window(BWindow *window)
{
	BMenuBar *bar = new BMenuBar(BRect(0, 0, 10, 10), "menuBar");
	
	BMenu *menu = new BMenu("menu ONE");
	
	menu->AddItem(new BMenuItem("ONE", new BMessage('1ONE')));
	menu->AddItem(new BMenuItem("TWO", new BMessage('2TWO')));
	bar->AddItem(menu);

	menu = new BMenu("menu TWO");
	menu->AddItem(new BMenuItem("ONE", new BMessage('1ONE')));
	menu->AddItem(new BMenuItem("TWO", new BMessage('2TWO')));
	bar->AddItem(menu);
	
	window->AddChild(bar);
	window->Show();
}


int main()
{
	BApplication app("application/x-vnd.menu-test");
	BWindow *window = new TestWindow();
	show_window(window);
	app.Run();
}


TestWindow::TestWindow()
	:BWindow(BRect(100, 100, 400, 300), "menu test", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	
}


void
TestWindow::MenusBeginning()
{
	printf("MenusBeginning()\n");
}


void
TestWindow::MenusEnded()
{
	printf("MenusEnded()\n");
}
