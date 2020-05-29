/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2018-2020, Andrew Lindesay <apl@lindesay.co.nz>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef APP_H
#define APP_H


#include <Application.h>


class BEntry;
class MainWindow;


class App : public BApplication {
public:
								App();
	virtual						~App();

	virtual	bool				QuitRequested();
	virtual	void				ReadyToRun();
			bool				IsFirstRun();

	virtual	void				MessageReceived(BMessage* message);
	virtual void				RefsReceived(BMessage* message);
	virtual void				ArgvReceived(int32 argc, char* argv[]);

private:
			void				_AlertSimpleError(BMessage* message);
			void				_Open(const BEntry& entry);
			void				_ShowWindow(MainWindow* window);

			bool				_LoadSettings(BMessage& settings);
			void				_StoreSettings(const BMessage& windowSettings);

			void				_CheckPackageDaemonRuns();
			bool				_LaunchPackageDaemon();

			bool				_CheckTestFile();
	static	bool				_CheckIsFirstRun();

private:
			MainWindow*			fMainWindow;
			int32				fWindowCount;

			BMessage			fSettings;
			bool				fSettingsRead;

			bool				fIsFirstRun;
};


#endif // APP_H
