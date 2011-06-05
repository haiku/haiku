/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Hamish Morrison <hamish@lavabit.com>
 *		Axel Dörfler <axeld@pinc-software.de>
 */
#ifndef NETWORK_TIME_VIEW_H
#define NETWORK_TIME_VIEW_H


#include <LayoutBuilder.h>


class BButton;
class BCheckBox;
class BListView;
class BTextControl;
class BMessage;
class BMessenger;
class BPath;
class Settings;


static const uint32 kMsgNetworkTimeSettings = 'ntst';
static const uint32 kMsgSetDefaultServer = 'setd';
static const uint32 kMsgAddServer = 'asrv';
static const uint32 kMsgRemoveServer = 'rsrv';
static const uint32 kMsgResetServerList = 'rstl';
static const uint32 kMsgTryAllServers = 'tras';
static const uint32 kMsgSynchronizeAtBoot = 'synb';
static const uint32 kMsgSynchronize = 'sync';
static const uint32 kMsgStopSynchronization = 'stps';
static const uint32 kMsgSynchronizationResult = 'syrs';
static const uint32 kMsgNetworkTimeChange = 'ntch';


status_t
update_time(const Settings& settings, BMessenger* messenger,
	thread_id* thread);


status_t
update_time(const Settings& settings, const char** errorString,
	int32* errorCode);


class Settings {
public:
							Settings();
							~Settings();

			void			AddServer(const char* server);
			const char*		GetServer(int32 index) const;
			void			RemoveServer(const char* server);
			void			SetDefaultServer(int32 index);
			int32			GetDefaultServer() const;
			void			SetTryAllServers(bool boolean);
			bool			GetTryAllServers() const;
			void			SetSynchronizeAtBoot(bool boolean);
			bool			GetSynchronizeAtBoot() const;

			void			ResetServersToDefaults();
			void			ResetToDefaults();
			void			Revert();
			bool			SettingsChanged();
			
			status_t		Load();
			status_t		Save();

private:
			int32			_GetStringByValue(const char* name,
								const char* value);
			status_t		_GetPath(BPath& path);

			BMessage		fMessage;
			BMessage		fOldMessage;
			bool			fWasUpdated;
};


class NetworkTimeView : public BGroupView {
public:
 							NetworkTimeView(const char* name);
 		
	virtual	void			MessageReceived(BMessage* message);
	virtual	void			AttachedToWindow();
	
			bool			CheckCanRevert();
private:
 			void	 		_InitView();
 			void 			_UpdateServerList();
 			void			_DoneSynchronizing();

			Settings		fSettings;

			BTextControl*	fServerTextControl;
			BButton*		fAddButton;
			BButton*		fRemoveButton;
			BButton*		fResetButton;

 			BListView*		fServerListView;
 			BCheckBox* 		fTryAllServersCheckBox;
 			BCheckBox* 		fSynchronizeAtBootCheckBox;
 			BButton* 		fSynchronizeButton;

	 		thread_id		fUpdateThread;
};
	
#endif

