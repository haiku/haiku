/**Test app for set_mouse_position function of the game kit
	@author Tim de Jong
	@date 09/09/2002
	@version 1.0
 */
#include <Application.h>
#include <Screen.h>
#include <WindowScreen.h>
#include <stdio.h>
#include <stdlib.h>

#define DELAY 2000

class TestMouse: public BApplication
{
	public:
		TestMouse(int times);

};

//screenheight and screenwidth
int32 height, width;

void mouse_move_up(int32 x)
{
	for (int32 t = (height); t > 0; t--)
	{
		 set_mouse_position(x, t);
		 snooze(DELAY);
	}
}

void mouse_move_down(int32 x)
{
	for (int32 t = 0; t < (height); t++)
	{
		 set_mouse_position(x,t);		
		 snooze(DELAY);
	}
}

void mouse_move_left(int32 y)
{
	for (int32 t = (width); t > 0; t--)
	{
		 set_mouse_position(t,y);
		 snooze(DELAY);
	}
}

void mouse_move_right(int32 y)
{
	for (int32 t = 0; t < (width); t++)
	{
		 set_mouse_position(t,y);
		 snooze(DELAY);
	}
}

void mouse_move_diagonal1(int32 x, int32 y)
{
	int32 t1 = x;
	double xmin = (double)x/y;
	for (int32 t2 = y; t2 > 0; t2--)
	{	
		t1 = int32(t1 - xmin);
		set_mouse_position(t1,t2);
		snooze(DELAY);
	}
}

void mouse_move_diagonal2(int32 x, int32 y)
{
	int32 t1 = x;
	double xplus = (double)width/y;
	for (int32 t2 = y; t2 > 0; t2--)
	{		 
		t1 = int32(t1 + xplus);
		set_mouse_position(t1,t2);
		snooze(DELAY);
	}
}

int main(int argc, char *argv[])
{ 
	int times;

	if (argc != 2)
	{
		printf("\n Usage: %s param: times, where times is the number of loops \n", argv[0]);
		times = 1;
	} 
	else
	{
		times = atoi(argv[1]);
	}

	if (times > 0)
	{
		printf("\n times = %d \n", times);
		TestMouse test(times);
		test.Run();
	}
	else 
		printf("\n Error! times must be an integer greater than 0 \n");
	return 0;
}

TestMouse::TestMouse(int times)
				:BApplication("application/x-vnd.test_mouse")
{

	//determine screenBounds
	BScreen screen;
	BRect screenBounds = screen.Frame();
	//save screenwidth and screenheight in height and width 
	height = int32(screenBounds.bottom - screenBounds.top) - 5;
	width =  int32(screenBounds.right - screenBounds.left) - 5;
			
	for (int t = 0; t < times; t++)
	{
		//make mouse moves 
		mouse_move_down(0);
		mouse_move_right(height);
		mouse_move_diagonal1(width,height);
		mouse_move_right(0);
		mouse_move_down(width);
		mouse_move_left(height);
		mouse_move_diagonal2(0,height); 	
	}
	//quit app
	be_app -> PostMessage(B_QUIT_REQUESTED);		
}
