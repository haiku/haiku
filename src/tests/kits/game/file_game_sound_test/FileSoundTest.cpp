/**Class to test the BFileGameSound class
 	@author Tim de Jong
 	@date 21/09/2002
 	@version 1.0
 */

#include "FileSoundTest.h"
#include "FileSoundWindow.h"
#include <Rect.h>

int main()
{
	FileSoundTest test;
	test.Run();
	return 0;
}


FileSoundTest::FileSoundTest()
				:BApplication("application/x-vnd.FileSoundTest")
{
	//show app window
	BRect windowBounds(50,50,400,170);
	FileSoundWindow *window = new FileSoundWindow(windowBounds);
	window -> Show();
}
