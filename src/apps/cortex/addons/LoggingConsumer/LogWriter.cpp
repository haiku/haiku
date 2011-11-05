/*
 * Copyright 1991-1999, Be Incorporated.
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// LogWriter.cpp

#include "LogWriter.h"
#include <stdio.h>
#include <string.h>

// this is the simpleminded implementation of a lookup function;
// could also use some sort of map.  this has the advantage of not
// requiring any runtime initialization.
static const char* log_what_to_string(log_what code)
{
	const char* s = "Unknown log_what code!";
	switch (code)
	{
	case LOG_QUIT : s = "LOG_QUIT"; break;
	case LOG_SET_RUN_MODE  : s = "LOG_SET_RUN_MODE"; break;
	case LOG_PREROLL : s = "LOG_PREROLL"; break;
	case LOG_SET_TIME_SOURCE : s = "LOG_SET_TIME_SOURCE"; break;
	case LOG_REQUEST_COMPLETED : s = "LOG_REQUEST_COMPLETED"; break;
	case LOG_GET_PARAM_VALUE : s = "LOG_GET_PARAM_VALUE"; break;
	case LOG_SET_PARAM_VALUE : s = "LOG_SET_PARAM_VALUE"; break;
	case LOG_FORMAT_SUGG_REQ : s = "LOG_FORMAT_SUGG_REQ"; break;
	case LOG_FORMAT_PROPOSAL : s = "LOG_FORMAT_PROPOSAL"; break;
	case LOG_FORMAT_CHANGE_REQ : s = "LOG_FORMAT_CHANGE_REQ"; break;
	case LOG_SET_BUFFER_GROUP : s = "LOG_SET_BUFFER_GROUP"; break;
	case LOG_VIDEO_CLIP_CHANGED : s = "LOG_VIDEO_CLIP_CHANGED"; break;
	case LOG_GET_LATENCY : s = "LOG_GET_LATENCY"; break;
	case LOG_PREPARE_TO_CONNECT : s = "LOG_PREPARE_TO_CONNECT"; break;
	case LOG_CONNECT : s = "LOG_CONNECT"; break;
	case LOG_DISCONNECT : s = "LOG_DISCONNECT"; break;
	case LOG_LATE_NOTICE_RECEIVED : s = "LOG_LATE_NOTICE_RECEIVED"; break;
	case LOG_ENABLE_OUTPUT : s = "LOG_ENABLE_OUTPUT"; break;
	case LOG_SET_PLAY_RATE : s = "LOG_SET_PLAY_RATE"; break;
	case LOG_ADDITIONAL_BUFFER : s = "LOG_ADDITIONAL_BUFFER"; break;
	case LOG_LATENCY_CHANGED : s = "LOG_LATENCY_CHANGED"; break;
	case LOG_HANDLE_MESSAGE : s = "LOG_HANDLE_MESSAGE"; break;
	case LOG_ACCEPT_FORMAT : s = "LOG_ACCEPT_FORMAT"; break;
	case LOG_BUFFER_RECEIVED : s = "LOG_BUFFER_RECEIVED"; break;
	case LOG_PRODUCER_DATA_STATUS : s = "LOG_PRODUCER_DATA_STATUS"; break;
	case LOG_CONNECTED : s = "LOG_CONNECTED"; break;
	case LOG_DISCONNECTED : s = "LOG_DISCONNECTED"; break;
	case LOG_FORMAT_CHANGED : s = "LOG_FORMAT_CHANGED"; break;
	case LOG_SEEK_TAG : s = "LOG_SEEK_TAG"; break;
	case LOG_REGISTERED : s = "LOG_REGISTERED"; break;
	case LOG_START : s = "LOG_START"; break;
	case LOG_STOP : s = "LOG_STOP"; break;
	case LOG_SEEK : s = "LOG_SEEK"; break;
	case LOG_TIMEWARP : s = "LOG_TIMEWARP"; break;
	case LOG_HANDLE_EVENT : s = "LOG_HANDLE_EVENT"; break;
	case LOG_HANDLE_UNKNOWN : s = "LOG_HANDLE_UNKNOWN"; break;
	case LOG_BUFFER_HANDLED : s = "LOG_BUFFER_HANDLED"; break;
	case LOG_START_HANDLED : s = "LOG_START_HANDLED"; break;
	case LOG_STOP_HANDLED : s = "LOG_STOP_HANDLED"; break;
	case LOG_SEEK_HANDLED : s = "LOG_SEEK_HANDLED"; break;
	case LOG_WARP_HANDLED : s = "LOG_WARP_HANDLED"; break;
	case LOG_DATA_STATUS_HANDLED : s = "LOG_DATA_STATUS_HANDLED"; break;
	case LOG_SET_PARAM_HANDLED : s = "LOG_SET_PARAM_HANDLED"; break;
	case LOG_INVALID_PARAM_HANDLED : s = "LOG_INVALID_PARAM_HANDLED"; break;
	}
	return s;
}

// Logging thread function
int32 LogWriterLoggingThread(void* arg)
{
	LogWriter* obj = static_cast<LogWriter*>(arg);
	port_id port = obj->mPort;

	// event loop

	bool done = false;
	do
	{
		log_message msg;
		int32 what;
		status_t n_bytes = ::read_port(port, &what, &msg, sizeof(log_message));
		if (n_bytes > 0)
		{
			obj->HandleMessage((log_what) what, msg);
			if (LOG_QUIT == what) done = true;
		}
		else
		{
			fprintf(stderr, "LogWriter failed (%s) in ::read_port()!\n", strerror(n_bytes));
		}
	} while (!done);

	// got the "quit" message; now we're done
	return 0;
}

// --------------------
// LogWriter class implementation
LogWriter::LogWriter(const entry_ref& logRef)
	:	mLogRef(logRef)
{
	mPort = ::create_port(64, "LogWriter");
	mLogThread = ::spawn_thread(&LogWriterLoggingThread, "LogWriter", B_NORMAL_PRIORITY, this);
	mLogFile.SetTo(&logRef, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	::resume_thread(mLogThread);
}

LogWriter::~LogWriter()
{
	printf("LogWriter::~LogWriter() called\n");
	status_t err;
	log_message msg;

	Log(LOG_QUIT, msg);
	::wait_for_thread(mLogThread, &err);
}

// Log a message
//
// This method, called by the client, really just enqueues a message to the writer thread,
// which will deal with it in the HandleMessage() method.
void
LogWriter::Log(log_what what, const log_message& data)
{
	bigtime_t now = ::system_time();
	log_message& nc_data = const_cast<log_message&>(data);
	nc_data.tstamp = now;
	::write_port(mPort, (int32) what, &data, sizeof(log_message));
}

// Enable or disable a particular log_what code's output
void
LogWriter::SetEnabled(log_what what, bool enable)
{
	if (enable)	 mFilters.erase(what);
	else mFilters.insert(what);
}

// enabling everything means just clearing out the filter set
void
LogWriter::EnableAllMessages()
{
	mFilters.clear();
}

// disabling everything is more tedious -- we have to add them all to the
// filter set, one by one
void
LogWriter::DisableAllMessages()
{
//	mFilters.insert(LOG_QUIT);				// don't disable our quit messages
	mFilters.insert(LOG_SET_RUN_MODE);
	mFilters.insert(LOG_PREROLL);
	mFilters.insert(LOG_SET_TIME_SOURCE);
	mFilters.insert(LOG_REQUEST_COMPLETED);
	mFilters.insert(LOG_GET_PARAM_VALUE);
	mFilters.insert(LOG_SET_PARAM_VALUE);
	mFilters.insert(LOG_FORMAT_SUGG_REQ);
	mFilters.insert(LOG_FORMAT_PROPOSAL);
	mFilters.insert(LOG_FORMAT_CHANGE_REQ);
	mFilters.insert(LOG_SET_BUFFER_GROUP);
	mFilters.insert(LOG_VIDEO_CLIP_CHANGED);
	mFilters.insert(LOG_GET_LATENCY);
	mFilters.insert(LOG_PREPARE_TO_CONNECT);
	mFilters.insert(LOG_CONNECT);
	mFilters.insert(LOG_DISCONNECT);
	mFilters.insert(LOG_LATE_NOTICE_RECEIVED);
	mFilters.insert(LOG_ENABLE_OUTPUT);
	mFilters.insert(LOG_SET_PLAY_RATE);
	mFilters.insert(LOG_ADDITIONAL_BUFFER);
	mFilters.insert(LOG_LATENCY_CHANGED);
	mFilters.insert(LOG_HANDLE_MESSAGE);
	mFilters.insert(LOG_ACCEPT_FORMAT);
	mFilters.insert(LOG_BUFFER_RECEIVED);
	mFilters.insert(LOG_PRODUCER_DATA_STATUS);
	mFilters.insert(LOG_CONNECTED);
	mFilters.insert(LOG_DISCONNECTED);
	mFilters.insert(LOG_FORMAT_CHANGED);
	mFilters.insert(LOG_SEEK_TAG);
	mFilters.insert(LOG_REGISTERED);
	mFilters.insert(LOG_START);
	mFilters.insert(LOG_STOP);
	mFilters.insert(LOG_SEEK);
	mFilters.insert(LOG_TIMEWARP);
	mFilters.insert(LOG_HANDLE_EVENT);
//	mFilters.insert(LOG_HANDLE_UNKNOWN);			// don't disable the "unknown message" messages
	mFilters.insert(LOG_BUFFER_HANDLED);
	mFilters.insert(LOG_START_HANDLED);
	mFilters.insert(LOG_STOP_HANDLED);
	mFilters.insert(LOG_SEEK_HANDLED);
	mFilters.insert(LOG_WARP_HANDLED);
	mFilters.insert(LOG_DATA_STATUS_HANDLED);
	mFilters.insert(LOG_SET_PARAM_HANDLED);
	mFilters.insert(LOG_INVALID_PARAM_HANDLED);
}

// Writer thread's message handling function -- this is where messages are actuall
// formatted and written to the log file
void
LogWriter::HandleMessage(log_what what, const log_message& msg)
{
	char buf[256];		// scratch buffer for building logged output

	// if we've been told to ignore this message type, just return without doing anything else
	if (mFilters.find(what) != mFilters.end()) return;

	// always write the message's type and timestamp
	sprintf(buf, "%-24s : realtime = %Ld, perftime = %Ld\n", log_what_to_string(what), msg.tstamp, msg.now);
	mWriteBuf = buf;

	// put any special per-message-type handling here
	switch (what)
	{
	case LOG_QUIT:
		mWriteBuf += "\tLogWriter thread terminating\n";
		break;

	case LOG_BUFFER_RECEIVED:
		if (msg.buffer_data.offset < 0)
		{
			sprintf(buf, "\tstart = %Ld, offset = %Ld\n", msg.buffer_data.start_time, msg.buffer_data.offset);
			mWriteBuf += buf;
			sprintf(buf, "\tBuffer received *LATE*\n");
			mWriteBuf += buf;
		}
		break;

	case LOG_SET_PARAM_HANDLED:
		sprintf(buf, "\tparam id = %ld, value = %f\n", msg.param.id, msg.param.value);
		mWriteBuf += buf;
		break;

	case LOG_INVALID_PARAM_HANDLED:
	case LOG_GET_PARAM_VALUE:
		sprintf(buf, "\tparam id = %ld\n", msg.param.id);
		mWriteBuf += buf;
		break;

	case LOG_BUFFER_HANDLED:
		sprintf(buf, "\tstart = %Ld, offset = %Ld\n", msg.buffer_data.start_time, msg.buffer_data.offset);
		mWriteBuf += buf;
		if (msg.buffer_data.offset < 0)
		{
			sprintf(buf, "\tBuffer handled *LATE*\n");
			mWriteBuf += buf;
		}
		break;

	case LOG_DATA_STATUS_HANDLED:
		sprintf(buf, "\tstatus = %d\n", int(msg.data_status.status));
		mWriteBuf += buf;
		break;

	case LOG_HANDLE_UNKNOWN:
		sprintf(buf, "\tUNKNOWN EVENT CODE: %d\n", int(msg.unknown.what));
		mWriteBuf += buf;
		break;

	default:
		break;
	}

	// actually write the log message to the file now
	mLogFile.Write(mWriteBuf.String(), mWriteBuf.Length());
}
