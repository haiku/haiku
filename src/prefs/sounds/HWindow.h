#ifndef __HWINDOW_H__
#define __HWINDOW_H__

#include <Window.h>
#include <FilePanel.h>
#include <FileGameSound.h>

class HEventList;
class HTypeList;

enum{
	M_PLAY_MESSAGE = 'MPLM',
	M_STOP_MESSAGE = 'MSTO',
	M_REMOVE_MESSAGE = 'MREM',
	M_ITEM_MESSAGE = 'MITE',
	M_OTHER_MESSAGE = 'MOTH',
	M_NONE_MESSAGE = 'MNON',
	M_ADD_EVENT = 'MADE',
	M_REMOVE_EVENT = 'MREE',
	M_OPEN_WITH = 'MOPW'
};

class HWindow :public BWindow {
public:
						HWindow(BRect rect ,const char* name);
protected:
		virtual			~HWindow();
		virtual	void	MessageReceived(BMessage *message);
		virtual	bool	QuitRequested();
		virtual void	DispatchMessage(BMessage *message
										,BHandler *handler);
				void	InitGUI();
				void	InitMenu();
				void	SetupMenuField();
				void	Pulse();
				void	RemoveEvent();
private:
			//HTypeList*	fTypeList;
			HEventList*	fEventList;
		typedef	BWindow	_inherited;
			BFilePanel*	fFilePanel;
			BFileGameSound *fPlayer;
};
#endif
