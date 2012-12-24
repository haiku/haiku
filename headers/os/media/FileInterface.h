/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FILE_INTERFACE_H
#define _FILE_INTERFACE_H


#include <MediaNode.h>


struct entry_ref;


class BFileInterface : public virtual BMediaNode {
protected:
	virtual						~BFileInterface();

protected:
								BFileInterface();

	virtual	status_t			HandleMessage(int32 message, const void* data,
									size_t size);

	virtual	status_t			GetNextFileFormat(int32* cookie,
									media_file_format* _format) = 0;
	virtual	void				DisposeFileFormatCookie(int32 cookie) = 0;

	virtual	status_t			GetDuration(bigtime_t* _time) = 0;
	virtual	status_t			SniffRef(const entry_ref& file,
									char* _mimeType, // 256 bytes
									float* _quality) = 0;
	virtual	status_t			SetRef(const entry_ref& file,
									bool create, bigtime_t* _time) = 0;
	virtual	status_t			GetRef(entry_ref* _ref, char* _mimeType) = 0;

	// TODO: Needs a Perform() virtual method!

private:
	// FBC padding and forbidden methods
	friend class BMediaNode;

								BFileInterface(const BFileInterface& other);
			BFileInterface&		operator=(const BFileInterface& other);

	virtual	status_t			_Reserved_FileInterface_0(void*);
	virtual	status_t			_Reserved_FileInterface_1(void*);
	virtual	status_t			_Reserved_FileInterface_2(void*);
	virtual	status_t			_Reserved_FileInterface_3(void*);
	virtual	status_t			_Reserved_FileInterface_4(void*);
	virtual	status_t			_Reserved_FileInterface_5(void*);
	virtual	status_t			_Reserved_FileInterface_6(void*);
	virtual	status_t			_Reserved_FileInterface_7(void*);
	virtual	status_t			_Reserved_FileInterface_8(void*);
	virtual	status_t			_Reserved_FileInterface_9(void*);
	virtual	status_t			_Reserved_FileInterface_10(void*);
	virtual	status_t			_Reserved_FileInterface_11(void*);
	virtual	status_t			_Reserved_FileInterface_12(void*);
	virtual	status_t			_Reserved_FileInterface_13(void*);
	virtual	status_t			_Reserved_FileInterface_14(void*);
	virtual	status_t			_Reserved_FileInterface_15(void*);

			uint32				_reserved_file_interface_[16];
};

#endif // _FILE_INTERFACE_H

