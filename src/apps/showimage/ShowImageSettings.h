/*
 * Copyright 2003-2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer, laplace@haiku-os.org
 */
#ifndef SHOW_IMAGE_SETTINGS_H
#define SHOW_IMAGE_SETTINGS_H


#include <Locker.h>
#include <Message.h>


class BFile;


class ShowImageSettings {
public:
								ShowImageSettings();
	virtual						~ShowImageSettings();

			bool				Lock();
			void				Unlock();

			bool				GetBool(const char* name, bool defaultValue);
			int32				GetInt32(const char* name, int32 defaultValue);
			float				GetFloat(const char* name, float defaultValue);
			BRect				GetRect(const char* name, BRect defaultValue);
			bigtime_t			GetTime(const char* name,
									bigtime_t defaultValue);
			const char* 		GetString(const char* name,
									const char* defaultValue);

			void				SetBool(const char* name, bool value);
			void				SetInt32(const char* name, int32 value);
			void				SetFloat(const char* name, float value);
			void				SetRect(const char* name, BRect value);
			void				SetTime(const char* name, bigtime_t value);
			void				SetString(const char* name, const char* value);

private:
			bool				_OpenSettingsFile(BFile* file, bool forReading);
			void				_Load();
			void				_Save();

private:
			BLocker				fLock;
			BMessage			fSettings;
			bool				fUpdated;
};


#endif	// SHOW_IMAGE_SETTINGS_H
