#ifndef FILE_TYPES_WINDOW_H
#define FILE_TYPES_WINDOW_H

#include <Window.h>

class FileTypesView;

class FileTypesWindow
	: public BWindow
{
public:
					FileTypesWindow(BRect frame);
					~FileTypesWindow();
	
	virtual void	Quit();
	virtual bool 	QuitRequested();
	virtual void 	MessageReceived(BMessage *message);
	
private: 
};

#endif // FILE_TYPES_WINDOW_H
