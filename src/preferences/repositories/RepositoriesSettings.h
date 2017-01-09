/*
 * Copyright 2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill
 */
#ifndef REPOSITORIES_SETTINGS_H
#define REPOSITORIES_SETTINGS_H


#include <File.h>
#include <Message.h>
#include <Path.h>
#include <Point.h>
#include <Rect.h>
#include <String.h>
#include <StringList.h>


class RepositoriesSettings {
public:
							RepositoriesSettings();
	BRect					GetFrame();
	void					SetFrame(BRect frame);
	status_t				GetRepositories(int32& repoCount,
								BStringList& nameList, BStringList& urlList);
	void					SetRepositories(BStringList& nameList,
								BStringList& urlList);

private:
	BMessage				_ReadFromFile();
	status_t				_SaveToFile(BMessage settings);
	
	BPath					fFilePath;
	BFile					fFile;
	status_t				fInitStatus;
};


#endif
