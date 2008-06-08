/*
 * Copyright 2007-2008 Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Ruiz Dorantes	oliver.ruiz.dorantes_at_gmail.com
 *		Ryan Leavengood			leavengood@gmail.com
 *		Fredrik Modéen 			fredrik@modeen.se
 */
 
#ifndef _JOY_WIN_H
#define _JOY_WIN_H

#include <Window.h>

class BJoystick;
class BCheckBox;
class BStringView;
class BListView;
class PortItem;
class BButton;
class BEntry;
class BFile;

class JoyWin : public BWindow {
	public:
		JoyWin(BRect frame,const char *title);
		virtual         ~JoyWin();
		virtual	void	MessageReceived(BMessage *message);
		virtual	bool	QuitRequested();

	private:		
		status_t		_AddToList(BListView *list, uint32 command, 
							const char* rootPath, BEntry *rootEntry = NULL);
		
		status_t		_Calibrate();
		status_t		_PerformProbe(const char* path);
		status_t		_ApplyChanges();
		status_t		_GetSettings();
		status_t		_CheckJoystickExist(const char* path);
		
		/*Show Alert Boxes*/
		status_t		_ShowProbeMesage(const char* str);
		status_t		_ShowCantFindFileMessage(const char* port);
		void	 		_ShowNoDeviceConnectedMessage(const char* joy, 
							const char* port);
		void			_ShowNoCompatibleJoystickMessage();
		
		/*Util*/
		BString			_FixPathToName(const char* port);
		BString			_BuildDisabledJoysticksFile();
		PortItem*		_GetSelectedItem(BListView* list);
		void			_SelectDeselectJoystick(BListView* list, bool enable);
		int32			_FindStringItemInList(BListView *view, 
						PortItem *item);
		BString			_FindFilePathForSymLink(const char* symLinkPath, 
						PortItem *item);
		status_t		_FindStick(const char* name);
		const char*		_FindSettingString(const char* name, const char* strPath);
		bool 			_GetLine(BString& string);

	//this one are used when we select joystick when portare selected
		bool 			fSystemUsedSelect;

		BJoystick*		fJoystick;
		BCheckBox*		fCheckbox;
		BStringView*	fGamePortS;
		BStringView*	fConControllerS;
		BListView*		fGamePortL;
		BListView*		fConControllerL;
		BButton*		fCalibrateButton;
		BButton*		fProbeButton;
		
		BFile*			fFileTempProbeJoystick;
//		int32  			fSourceEncoding;
   		char    		fBuffer[4096];
   		off_t   		fPositionInBuffer;
   		ssize_t 		fAmtRead;
};

#endif	/* _JOY_WIN_H */
