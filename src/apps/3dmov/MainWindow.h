/*	PROJECT:		3Dmov
	AUTHORS:		Zenja Solaja
	COPYRIGHT:		2009 Haiku Inc
	DESCRIPTION:	Haiku version of the famous BeInc demo 3Dmov
					Just drag'n'drop media files to the 3D objects
*/

#ifndef _MAIN_WINDOW_H_
#define _MAIN_WINDOW_H_

#include "DirectWindow.h"

class BMenuBar;
class ViewObject;

/************************************
	Main application window
*************************************/
class MainWindow : public BDirectWindow
{
public:
	enum SHAPE
	{
		BOOK,
		CUBE,
		SPHERE,
		NUMBER_OF_SHAPES
	};
	
			MainWindow(BRect frame, SHAPE shape);
			~MainWindow();
			
	void	MessageReceived(BMessage *message);
	void	DirectConnected(direct_buffer_info *info);
	bool	QuitRequested();
	
private:
	void	SetupMenuBar(BRect frame);
	
	BMenuBar		*fMenuBar;
	bool			fOptionWireframe;
	int32			fAnimationThreadID;
	ViewObject		*fCurrentView;
};

#endif
