// LogWriter.h

#ifndef LogWriter_H
#define LogWriter_H 1

#include <kernel/OS.h>
//#include <storage/Entry.h>
//#include <storage/File.h>
#include <Entry.h>
#include <File.h>
#include <support/String.h>
#include <set>

// logging message data structure
struct log_message
{
	bigtime_t tstamp;					// time that LogWriter::Log() was called, in real time
	bigtime_t now;						// time that LogWriter::Log() was called, according to the time source
	union										// various bits of data for different kinds of logged actions
	{
		struct
		{
			bigtime_t start_time;		// buffer's start time stamp
			bigtime_t offset;				// how late or early the buffer was handled
		} buffer_data;
		struct
		{
			int32 status;
		} data_status;
		struct
		{
			int32 id;							// parameter ID
			float value;						// new value
		} param;
		struct
		{
			int32 what;
		} unknown;
	};
};

// logging message 'what' codes
enum log_what
{
	LOG_QUIT = 'Quit',					// time to quit; tear down the thread

	// messages related to BMediaNode methods
	LOG_SET_RUN_MODE = 'Rnmd',
	LOG_PREROLL = 'Prll',
	LOG_SET_TIME_SOURCE = 'TSrc',
	LOG_REQUEST_COMPLETED = 'Rqcm',

	// messages related to BControllable methods
	LOG_GET_PARAM_VALUE = 'PVal',
	LOG_SET_PARAM_VALUE = 'SVal',

	// messages related to BBufferProducer methods
	LOG_FORMAT_SUGG_REQ = 'FSRq',
	LOG_FORMAT_PROPOSAL = 'FPro',
	LOG_FORMAT_CHANGE_REQ = 'FCRq',
	LOG_SET_BUFFER_GROUP = 'SBfG',
	LOG_VIDEO_CLIP_CHANGED = 'VClp',
	LOG_GET_LATENCY = 'GLat',
	LOG_PREPARE_TO_CONNECT = 'PCon',
	LOG_CONNECT = 'Cnct',
	LOG_DISCONNECT = 'Dsct',
	LOG_LATE_NOTICE_RECEIVED = 'LNRc',
	LOG_ENABLE_OUTPUT = 'EnOP',
	LOG_SET_PLAY_RATE = 'PlRt',
	LOG_ADDITIONAL_BUFFER = 'AdBf',
	LOG_LATENCY_CHANGED = 'LtCh',

	// messages related to BBufferConsumer methods
	LOG_HANDLE_MESSAGE = 'Mesg',
	LOG_ACCEPT_FORMAT = 'Frmt',
	LOG_BUFFER_RECEIVED = 'Bufr',
	LOG_PRODUCER_DATA_STATUS = 'PDSt',
	LOG_CONNECTED = 'Cntd',
	LOG_DISCONNECTED = 'Dctd',
	LOG_FORMAT_CHANGED = 'Fmtc',
	LOG_SEEK_TAG = 'STrq',

	// messages related to BMediaEventLooper methods
	LOG_REGISTERED = 'Rgst',
	LOG_START = 'Strt',
	LOG_STOP = 'Stop',
	LOG_SEEK = 'Seek',
	LOG_TIMEWARP = 'Warp',

	// messages about handling things in BMediaEventLooper::HandleEvent()
	LOG_HANDLE_EVENT = 'HEvt',
	LOG_HANDLE_UNKNOWN = 'Uknw',		// unknown event code in HandleEvent()
	LOG_BUFFER_HANDLED = 'Bfhd',
	LOG_START_HANDLED = 'Sthd',
	LOG_STOP_HANDLED = 'Sphd',
	LOG_SEEK_HANDLED = 'Skhd',
	LOG_WARP_HANDLED = 'Wphd',
	LOG_DATA_STATUS_HANDLED = 'DShd',
	LOG_SET_PARAM_HANDLED = 'SPrm',
	LOG_INVALID_PARAM_HANDLED = 'BadP'
};

// LogWriter class
class LogWriter
{
public:
	// Set:  output for the log_what members is disabled
	typedef set<log_what> FilterSet;

public:
	LogWriter(const entry_ref& logRef);
	~LogWriter();

	// write a message to the log
	void Log(log_what what, const log_message& data);

	// filtering control for logged messages
	void SetEnabled(log_what what, bool enable);
	void EnableAllMessages();
	void DisableAllMessages();

private:
	entry_ref mLogRef;
	thread_id mLogThread;
	port_id mPort;
	BFile mLogFile;
	BString mWriteBuf;
	FilterSet mFilters;

	// called by the LogWriter's thread
	void HandleMessage(log_what what, const log_message& msg);

	friend int32 LogWriterLoggingThread(void* arg);
};

#endif
