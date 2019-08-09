/*
 * Copyright 1998-1999 Be, Inc. All Rights Reserved.
 * Copyright 2003-2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef CODYCAM_H
#define CODYCAM_H


#include <string.h>

#include <Application.h>
#include <Box.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuField.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>

#include "Settings.h"
#include "VideoConsumer.h"


class BMediaRoster;


enum {
	msg_filename	= 'file',

	msg_rate_changed = 'rate',

	msg_translate	= 'tran',

	msg_upl_client	= 'ucli',
	msg_server		= 'srvr',
	msg_login		= 'lgin',
	msg_password	= 'pswd',
	msg_directory	= 'drct',
	msg_passiveftp	= 'pasv',
	msg_pswrd_text	= 'pstx',

	msg_start		= 'strt',
	msg_stop		= 'stop',

	msg_setup		= 'setp',
	msg_video		= 'vdeo',

	msg_control_win = 'ctlw'
};


struct capture_rate {
	const char* name;
	time_t		seconds;
};


// NOTE: These are translated once the app catalog is loaded.
capture_rate kCaptureRates[] = {
	{"Every 15 seconds", 15 },
	{"Every 30 seconds", 30 },
	{"Every minute", 60 },
	{"Every 5 minutes", 5 * 60 },
	{"Every 10 minutes", 10 * 60 },
	{"Every 15 minutes", 15 * 60 },
	{"Every 30 minutes", 30 * 60 },
	{"Every hour", 60 * 60 },
	{"Every 2 hours", 2 * 60 * 60 },
	{"Every 4 hours", 4 * 60 * 60 },
	{"Every 8 hours", 8 * 60 * 60 },
	{"Every 24 hours", 24 * 60 * 60 },
	{"Never", 0 }
};


const int32 kCaptureRatesCount = sizeof(kCaptureRates) / sizeof(capture_rate);


const char* kUploadClients[] = {
	// NOTE: These are translated once the app catalog is loaded.
	"FTP",
	"SFTP",
	"Local"
};


const int32 kUploadClientsCount = sizeof(kUploadClients) / sizeof(char*);

class VideoWindow;
class ControlWindow;

class CodyCam : public BApplication {
public:
							CodyCam();
	virtual					~CodyCam();

			void			ReadyToRun();
	virtual	bool			QuitRequested();
	virtual	void			MessageReceived(BMessage* message);

private:
			status_t		_SetUpNodes();
			void			_TearDownNodes();

			BMediaRoster*	fMediaRoster;
			media_node		fTimeSourceNode;
			media_node		fProducerNode;
			VideoConsumer*	fVideoConsumer;
			media_output	fProducerOut;
			media_input		fConsumerIn;
			VideoWindow*	fWindow;
			port_id			fPort;
			ControlWindow*	fVideoControlWindow;
};


class VideoWindow : public BWindow {
public:
							VideoWindow(const char* title,
								window_type type, uint32 flags,
								port_id* consumerport);
							~VideoWindow();

	virtual	bool			QuitRequested();
	virtual	void			MessageReceived(BMessage* message);

			void			ApplyControls();

			BView*			VideoView();
			BStringView*	StatusLine();
			void			ToggleMenuOnOff();

			void			ErrorAlert(const char*, status_t);

private:
			void			_BuildCaptureControls();

			void			_SetUpSettings(const char* filename,
								const char* dirname);
			void			_UploadClientChanged();
			void			_QuitSettings();

private:
			media_node*		fProducer;
			port_id*		fPortPtr;

			BView*			fVideoView;
			BTextView*		fErrorView;

			BTextControl*	fFileName;
			BBox*			fCaptureSetupBox;
			BMenu*			fCaptureRateMenu;
			BMenuField*		fCaptureRateSelector;
			BMenu*			fImageFormatMenu;
			BMenuField*		fImageFormatSelector;
			BMenu*			fUploadClientMenu;
			BMenuField*		fUploadClientSelector;
			BBox*			fFtpSetupBox;
			BTextControl*	fServerName;
			BTextControl*	fLoginId;
			BTextControl*	fPassword;
			BTextControl*	fDirectory;
			BCheckBox*		fPassiveFtp;
			BBox*			fStatusBox;
			BStringView*	fStatusLine;

			ftp_msg_info	fFtpInfo;

			Settings*		fSettings;

			BMenu* 			fMenu;

			StringValueSetting*		fServerSetting;
			StringValueSetting*		fLoginSetting;
			StringValueSetting*		fPasswordSetting;
			StringValueSetting*		fDirectorySetting;
			BooleanValueSetting*	fPassiveFtpSetting;
			StringValueSetting*		fFilenameSetting;
			StringValueSetting*		fImageFormatSettings;

			EnumeratedStringValueSetting*	fUploadClientSetting;
			EnumeratedStringValueSetting*	fCaptureRateSetting;
};


class ControlWindow : public BWindow {
public:
							ControlWindow(BView* controls,
								media_node node);
			void			MessageReceived(BMessage* message);
			bool			QuitRequested();

private:
			BView*			fView;
			media_node		fNode;
};

#endif	// CODYCAM_H
