#include "APRMain.h"
#include <stdio.h>

APRApplication::APRApplication()
  :BApplication("application/x-vnd.obos-Appearance")
{
	BRect rect;

	// This is just the size and location of the window when Show() is called	
	rect.Set(100,100,580,220);
	aprwin=new APRWindow(rect);
	aprwin->Show();
}

int main(int, char**)
{	
	APRApplication myApplication;

	// This function doesn't return until the application quits
	myApplication.Run();

	return(0);
}
