/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEDIA_FILES_H
#define _MEDIA_FILES_H


#include <MediaDefs.h>
#include <List.h>
#include <String.h>

struct entry_ref;


class BMediaFiles {
public:

								BMediaFiles();
	virtual						~BMediaFiles();

	virtual	status_t			RewindTypes();
	virtual	status_t			GetNextType(BString* _type);

	virtual	status_t			RewindRefs(const char* type);
	virtual	status_t			GetNextRef(BString* _type,
									entry_ref* _ref = NULL);

	virtual	status_t			GetRefFor(const char* type, const char* item,
									entry_ref* _ref);
	virtual	status_t			SetRefFor(const char* type, const char* item,
									const entry_ref& ref);

			status_t			GetAudioGainFor(const char* type,
									const char* item, float* _gain);
			status_t			SetAudioGainFor(const char* type,
									const char* item, float gain);

	// TODO: Rename this to "ClearRefFor" when breaking BC.
	virtual	status_t			RemoveRefFor(const char* type,
									const char* item, const entry_ref& ref);

	virtual	status_t			RemoveItem(const char* type, const char* item);

	static	const char			B_SOUNDS[];

	// TODO: Needs Perform() for FBC reasons!

private:
	// FBC padding

			status_t			_Reserved_MediaFiles_0(void*, ...);
	virtual	status_t			_Reserved_MediaFiles_1(void*, ...);
	virtual	status_t			_Reserved_MediaFiles_2(void*, ...);
	virtual	status_t			_Reserved_MediaFiles_3(void*, ...);
	virtual	status_t			_Reserved_MediaFiles_4(void*, ...);
	virtual	status_t			_Reserved_MediaFiles_5(void*, ...);
	virtual	status_t			_Reserved_MediaFiles_6(void*, ...);
	virtual	status_t			_Reserved_MediaFiles_7(void*, ...);

			void				_ClearTypes();
			void				_ClearItems();

private:
			BList				fTypes;
			int					fTypeIndex;
			BString				fCurrentType;
			BList				fItems;
			int					fItemIndex;
};

#endif // _MEDIA_FILES_H

