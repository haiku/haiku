/*	PROJECT:		3Dmov
	AUTHORS:		Zenja Solaja
	COPYRIGHT:		2009 Haiku Inc
	DESCRIPTION:	Haiku version of the famous BeInc demo 3Dmov

					There is an issue in Haiku when creating a 2nd BGLView which doesn't exist in Zeta
					(see MainWindow.cpp for details).  Until that issue is resolved,
					I've allowed specifying of shape type (0-2) as a command line argument.
*/

#include <Application.h>

#include "MainWindow.h"

/*****************************
	Main application
******************************/
class MainApp : public BApplication
{
public:
				MainApp(MainWindow::SHAPE shape);

private:
	MainWindow	*fWindow;
};

/*	FUNCTION:		MainApp :: MainApp
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Application constructor
*/
MainApp :: MainApp(MainWindow::SHAPE shape)
	: BApplication("application/x-vnd.Haiku-3DMov")
{
	BRect frame(50, 50, 50+400, 50+300);
	fWindow = new MainWindow(frame, shape);
}

/******************************
	Program entry point
*******************************/

/*	FUNCTION:		main
	ARGS:			arc		number of command line arguments
					argv	vector to arguments
	RETURN:			Exit status
	DESCRIPTION:	main program entry point
*/
int main(int argc, char **argv)
{
	//	Check if shape specified on command line
	MainWindow::SHAPE shape = MainWindow::BOOK;
	if (argc > 1)
	{
		int value = *argv[1] - '0';
		if (value >= 0 && value < MainWindow::NUMBER_OF_SHAPES)
			shape = (MainWindow::SHAPE) value;
	}

	MainApp *app = new MainApp(shape);
	app->Run();
	delete app;
}
