/*
 * Part of the Open BeOS project
 * (C) 2002 Be United
 * Creation: Sandor Vroemisse
 */

#include <be/app/Application.h>
#include "KeymapApplication.h"


int main ()
{
	new KeymapApplication;
	be_app->Run();
	delete be_app;
	return 0;
}
