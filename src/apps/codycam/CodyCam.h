/* CodyCam.h */
#ifndef CODYCAM_H
#define CODYCAM_H


#include "Settings.h"
#include "VideoConsumer.h"

#include <string.h>

#include <Application.h>
#include <Box.h>
#include <CheckBox.h>
#include <Menu.h>
#include <MenuField.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>



class BMediaRoster;


enum {
	msg_filename	= 'file',

	msg_rate_15s	= 'r15s',
	msg_rate_30s	= 'r30s',
	msg_rate_1m		= 'r1m ',
	msg_rate_5m		= 'r5m ',
	msg_rate_10m	= 'r10m',
	msg_rate_15m	= 'r15m',
	msg_rate_30m	= 'r30m',
	msg_rate_1h		= 'r1h ',	
	msg_rate_2h		= 'r2h ',	
	msg_rate_4h		= 'r4h ',	
	msg_rate_8h		= 'r8h ',	
	msg_rate_24h	= 'r24h',
	msg_rate_never	= 'nevr',	

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

	msg_about		= 'abut',
	msg_setup		= 'setp',
	msg_video		= 'vdeo',
	
	msg_control_win = 'ctlw'
};


const char* kCaptureRate[] = {
	"Every 15 seconds",
	"Every 30 seconds",
	"Every minute",
	"Every 5 minutes",
	"Every 10 minutes",
	"Every 15 minutes",
	"Every 30 minutes",
	"Every hour",
	"Every 2 hours",
	"Every 4 hours",
	"Every 8 hours",
	"Every 24 hours",
	"Never",	
	0
};


const char* kUploadClient[] = {
	"FTP",
	"SFTP",
	0
};


class CodyCam : public BApplication {
	public:
		CodyCam();
		virtual ~CodyCam();

		void ReadyToRun();
		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage* message);

	private:
		status_t		_SetUpNodes();
		void			_TearDownNodes();

		BMediaRoster*	fMediaRoster; 
		media_node		fTimeSourceNode;
		media_node		fProducerNode;
		VideoConsumer*	fVideoConsumer;
		media_output	fProducerOut;
		media_input		fConsumerIn;
		BWindow*		fWindow;
		port_id			fPort;
		BWindow*		fVideoControlWindow;
};


class VideoWindow : public BWindow {
	public:
		VideoWindow(BRect frame, const char* title, window_type type,
			uint32 flags, port_id* consumerport);
		~VideoWindow();

		virtual	bool QuitRequested();
		virtual void MessageReceived(BMessage* message);

		void ApplyControls();

		BView* VideoView();
		BStringView* StatusLine();

	private:
		void _BuildCaptureControls(BView* theView);

		void _SetUpSettings(const char* filename, const char* dirname);
		void _QuitSettings();

	private:
		media_node*				fProducer;
		port_id*				fPortPtr;

		BView*					fView;
		BView*					fVideoView;

		BTextControl*			fFileName;
		BBox*					fCaptureSetupBox;
		BMenu*					fCaptureRateMenu;
		BMenuField*				fCaptureRateSelector;
		BMenu*					fImageFormatMenu;
		BMenuField*				fImageFormatSelector;
		BMenu*					fUploadClientMenu;
		BMenuField*				fUploadClientSelector;
		BBox*					fFtpSetupBox;
		BTextControl*			fServerName;
		BTextControl*			fLoginId;
		BTextControl*			fPassword;	
		BTextControl*			fDirectory;
		BCheckBox*				fPassiveFtp;
		BBox*					fStatusBox;
		BStringView*			fStatusLine;

		ftp_msg_info			fFtpInfo;

		Settings*				fSettings;
		StringValueSetting*		fServerSetting;
		StringValueSetting*		fLoginSetting;
		StringValueSetting*		fPasswordSetting;
		StringValueSetting*		fDirectorySetting;
		BooleanValueSetting*	fPassiveFtpSetting;
		StringValueSetting*		fFilenameSetting;
		StringValueSetting*		fImageFormatSettings;
		EnumeratedStringValueSetting* fCaptureRateSetting;
};


class ControlWindow : public BWindow {
	public:
		ControlWindow(const BRect& frame, BView* controls, media_node node);
		void MessageReceived(BMessage* message);
		bool QuitRequested();

	private:
		BView*		fView;
		media_node	fNode;
};

#endif	// CODYCAM_H
