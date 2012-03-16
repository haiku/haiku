/*	PROJECT:		3Dmov
	AUTHORS:		Zenja Solaja
	COPYRIGHT:		2009 Haiku Inc
	DESCRIPTION:	Haiku version of the famous BeInc demo 3Dmov.
	
					TODO:
					There is a bug in Haiku which doesn't allow a 2nd BGLView to be created.
					Originally I tried AddChild(new BGLView), however nothing would be drawn in the view.
					On Zeta, both AddChild(new BGLView) and new MainWindow work without any problems.
*/

#include <Catalog.h>
#include <GLView.h>
#include <InterfaceKit.h>
#include <Messenger.h>

#include "MainWindow.h"
#include "ViewObject.h"
#include "ViewBook.h"
#include "ViewCube.h"
#include "ViewSphere.h"

//	Local definitions
static const float MENU_BAR_HEIGHT = 19;

static const unsigned int	MSG_FILE_OPEN			= 'fopn';
static const unsigned int	MSG_SHAPE_BOOK			= 'spbk';
static const unsigned int	MSG_SHAPE_CUBE			= 'spcb';
static const unsigned int	MSG_SHAPE_SPHERE		= 'spsp';
static const unsigned int	MSG_OPTION_WIREFRAME	= 'opwf';
static const unsigned int	MSG_FULLSCREEN			= 'full';


// 	Local functions
static int32 animation_thread(void *cookie);

//	Local variables
static int sMainWindowCount = 0;	// keep track of number of spawned windows	

/*	FUNCTION:		MainWindow :: MainWindow
	ARGUMENTS:		frame
					shape
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
MainWindow :: MainWindow(BRect frame, SHAPE shape)
	:
	BDirectWindow(frame, B_TRANSLATE_SYSTEM_NAME("3DMov"), B_TITLED_WINDOW, 0)
{
	sMainWindowCount++;
	
	fOptionWireframe = false;
	
	//	Add menu bar
	frame.OffsetTo(B_ORIGIN);
	BRect aRect = frame;
	aRect.bottom = MENU_BAR_HEIGHT;
	frame.top = MENU_BAR_HEIGHT;
	SetupMenuBar(aRect);
	
	//	Add book view
	switch (shape)
	{
		case BOOK:		AddChild(fCurrentView = new ViewBook(frame));	break;
		case CUBE:		AddChild(fCurrentView = new ViewCube(frame));	break;
		case SPHERE:	AddChild(fCurrentView = new ViewSphere(frame));	break;
		case NUMBER_OF_SHAPES: break;
	}

	//AddShortcut('f', B_COMMAND_KEY, new BMessage(MSG_FULLSCREEN));
		
	//	Window should never be larger than 2048
	SetSizeLimits(40.0, 2047.0, 40.0, 2047.0);
	Show();
	
	fAnimationThreadID = spawn_thread(animation_thread, "Animation Thread", B_NORMAL_PRIORITY, fCurrentView);
	resume_thread(fAnimationThreadID);
}

/*	FUNCTION:		MainWindow :: ~MainWindow
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
MainWindow :: ~MainWindow()
{
	kill_thread(fAnimationThreadID);
	RemoveChild(fCurrentView);
	delete fCurrentView;
	
	sMainWindowCount--;
	if (sMainWindowCount == 0)
		be_app->PostMessage(B_QUIT_REQUESTED);
}

/*	FUNCTION:		MainWindow :: SetupMenuBar
	ARGUMENTS:		frame
	RETURN:			n/a
	DESCRIPTION:	Initialise menu bar
*/
void MainWindow :: SetupMenuBar(BRect frame)
{
	//	File
	BMenu *menu_file = new BMenu("File");
	//menu_file->AddItem(new BMenuItem("Open", new BMessage(MSG_FILE_OPEN), 'N'));
	menu_file->AddItem(new BMenuItem("Exit", new BMessage(B_QUIT_REQUESTED)));
		
	//	Shape
	BMenu *menu_shape = new BMenu("Shape");
	menu_shape->AddItem(new BMenuItem("Book", new BMessage(MSG_SHAPE_BOOK), '1'));
	menu_shape->AddItem(new BMenuItem("Cube", new BMessage(MSG_SHAPE_CUBE), '2'));
	menu_shape->AddItem(new BMenuItem("Sphere", new BMessage(MSG_SHAPE_SPHERE), '3'));
	
	//	Options
	BMenu *menu_options = new BMenu("Options");
	menu_options->AddItem(new BMenuItem("Wireframe", new BMessage(MSG_OPTION_WIREFRAME), 'S'));
	//menu_options->AddItem(new BMenuItem("Fullscreen", new BMessage(MSG_FULLSCREEN), 'F'));
	
	//	Menu bar
	fMenuBar = new BMenuBar(frame, "menubar");
	fMenuBar->AddItem(menu_file);
	fMenuBar->AddItem(menu_shape);
	fMenuBar->AddItem(menu_options);
	AddChild(fMenuBar);
}

/*	FUNCTION:		MainWindow :: MessageReceived
	ARGUMENTS:		message
	RETURN:			n/a
	DESCRIPTION:	Called by BeOS	
*/
void MainWindow :: MessageReceived(BMessage *message)
{
	switch (message->what)
	{
		case MSG_FULLSCREEN:
			SetFullScreen(!IsFullScreen());
			break;
			
		/*	TODO - Due to a bug when creating a 2nd BGLView in Haiku, I've decided to spawn a new window
			instead of creating a new BGLView (and using AddChild(new_view) / RemoveChild(old_view).
			Under Zeta, there is no problem replacing the current view with a new view.
		*/
		
		case MSG_SHAPE_BOOK:
			new MainWindow(BRect(50, 50, 400+50, 300+50), BOOK);
			break;
		
		case MSG_SHAPE_CUBE:
			new MainWindow(BRect(50, 50, 400+50, 300+50), CUBE);
			break;
		
		case MSG_SHAPE_SPHERE:
			new MainWindow(BRect(50, 50, 400+50, 300+50), SPHERE);
			break;
		
		case MSG_OPTION_WIREFRAME:
			fOptionWireframe = !fOptionWireframe;
			fCurrentView->ToggleWireframe(fOptionWireframe);
			break;
			
		case 'DATA':	// user drag/dropped file from Tracker
		{
			BPoint point;
			message->FindPoint("_drop_point_", &point);
			BRect frame = Frame();
			point.x -= frame.left;
			point.y -= frame.top;
			
			entry_ref aRef;
			message->FindRef("refs", &aRef);
			
			fCurrentView->DragDrop(&aRef, point.x, point.y);
			break;
		}
			
		default:
			BDirectWindow::MessageReceived(message);
			break;
	}
}

/*	FUNCTION:		MainWindow :: QuitRequested
	ARGUMENTS:		none
	RETURN:			true if success
	DESCRIPTION:	Called when window closed	
*/
bool MainWindow :: QuitRequested()
{
	if (fCurrentView != NULL)
		fCurrentView->EnableDirectMode(false);
	//be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

/*	FUNCTION:		MainWindow :: DirectConnected
	ARGUMENTS:		info
	RETURN:			n/a
	DESCRIPTION:	Hook functions called when full screen mode toggled	
*/
void MainWindow :: DirectConnected(direct_buffer_info *info)
{
	if (fCurrentView != NULL) {
		fCurrentView->DirectConnected(info);
		fCurrentView->EnableDirectMode(true);
	}
}

/*	FUNCTION:		animation_thread
	ARGUMENTS:		cookie
	RETURN:			exit status
	DESCRIPTION:	Main rendering thread	
*/
#include <stdio.h>
static int32 animation_thread(void *cookie)
{
	ViewObject *view = (ViewObject *) cookie;
	while (1)
	{
		view->Render();
		view->GLCheckError();
		snooze(1);
	}
	return B_OK;	// keep compiler happy
}

