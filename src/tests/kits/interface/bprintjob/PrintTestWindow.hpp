#ifndef PRINTTESTWINDOW_HPP
#define PRINTTESTWINDOW_HPP

class PrintTestWindow;

#include <Window.h>

class PrintTestWindow : public BWindow
{
	typedef BWindow Inherited;
public:
	PrintTestWindow();
	bool QuitRequested();
private:
	void BuildGUI();
};

#endif
