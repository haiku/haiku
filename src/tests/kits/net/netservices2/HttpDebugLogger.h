/*
 * Copyright 2021 Haiku, inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef HTTP_DEBUG_LOGGER_H
#define HTTP_DEBUG_LOGGER_H

#include <File.h>
#include <Looper.h>


class HttpDebugLogger : public BLooper
{
public:
								HttpDebugLogger();
			void				SetConsoleLogging(bool enabled = true);
			void				SetFileLogging(const char* path);

protected:
	virtual	void				MessageReceived(BMessage* message) override;

private:
			bool				fConsoleLogging = false;
			BFile				fLogFile;
};

#endif // HTTP_DEBUG_LOGGER_H
