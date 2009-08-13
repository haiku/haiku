/*
 * Copyright 2003-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer, laplace@haiku-os.org
 */
#ifndef SHOW_IMAGE_SETTINGS_H
#define SHOW_IMAGE_SETTINGS_H


#include <File.h>
#include <Message.h>
#include <Locker.h>


class ShowImageSettings {
public:
					ShowImageSettings();
					~ShowImageSettings();

	bool			Lock();
	bool			GetBool(const char* name, bool defaultValue);
	int32			GetInt32(const char* name, int32 defaultValue);
	float			GetFloat(const char* name, float defaultValue);
	BRect			GetRect(const char* name, BRect defaultValue);
	const char* 	GetString(const char* name, const char* defaultValue);
	void			SetBool(const char* name, bool value);
	void			SetInt32(const char* name, int32 value);
	void			SetFloat(const char* name, float value);
	void			SetRect(const char* name, BRect value);
	void			SetString(const char* name, const char* value);
	void			Unlock();
	
private:
	bool			OpenSettingsFile(BFile* file, bool forReading);
	void			Load();
	void			Save();

	BLocker			fLock;
	BMessage		fSettings;
};


#endif	// SHOW_IMAGE_SETTINGS_H
