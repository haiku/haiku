/*******************************************************************************
/
/	File:			FileInterface.h
/
/   Description:  A BFileInterface is something which can read and/or write files for the
/	use of other Media Kit participants.
/
/	Copyright 1997-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#if !defined(_FILE_INTERFACE_H)
#define _FILE_INTERFACE_H

#include <MediaDefs.h>
#include <MediaNode.h>


class BFileInterface :
	public virtual BMediaNode
{
protected:

virtual	~BFileInterface();	/* force vtable */

public:

		/* look, no hands! */

protected:

		BFileInterface();
virtual	status_t HandleMessage(
				int32 message,
				const void * data,
				size_t size);

virtual	status_t GetNextFileFormat(
				int32 * cookie,
				media_file_format * out_format) = 0;
virtual	void DisposeFileFormatCookie(
				int32 cookie) = 0;

virtual	status_t GetDuration(
				bigtime_t * out_time) = 0;
virtual	status_t SniffRef(
				const entry_ref & file,
				char * out_mime_type,	/* 256 bytes */
				float * out_quality) = 0;
virtual	status_t SetRef(
				const entry_ref & file,
				bool create,
				bigtime_t * out_time) = 0;
virtual	status_t GetRef(
				entry_ref * out_ref,
				char * out_mime_type) = 0;

private:

	friend class BMediaNode;

		BFileInterface(	/* private unimplemented */
				const BFileInterface & clone);
		BFileInterface & operator=(
				const BFileInterface & clone);

		/* Mmmh, stuffing! */
virtual		status_t _Reserved_FileInterface_0(void *);
virtual		status_t _Reserved_FileInterface_1(void *);
virtual		status_t _Reserved_FileInterface_2(void *);
virtual		status_t _Reserved_FileInterface_3(void *);
virtual		status_t _Reserved_FileInterface_4(void *);
virtual		status_t _Reserved_FileInterface_5(void *);
virtual		status_t _Reserved_FileInterface_6(void *);
virtual		status_t _Reserved_FileInterface_7(void *);
virtual		status_t _Reserved_FileInterface_8(void *);
virtual		status_t _Reserved_FileInterface_9(void *);
virtual		status_t _Reserved_FileInterface_10(void *);
virtual		status_t _Reserved_FileInterface_11(void *);
virtual		status_t _Reserved_FileInterface_12(void *);
virtual		status_t _Reserved_FileInterface_13(void *);
virtual		status_t _Reserved_FileInterface_14(void *);
virtual		status_t _Reserved_FileInterface_15(void *);

		uint32 _reserved_file_interface_[16];

};


#endif /* _FILE_INTERFACE_H */

