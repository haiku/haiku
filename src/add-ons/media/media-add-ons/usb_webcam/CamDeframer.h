/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CAM_DEFRAMER_H
#define _CAM_DEFRAMER_H

#include <OS.h>
#include <DataIO.h>
#include <Locker.h>
#include <List.h>
#include "CamFilterInterface.h"
class CamDevice;

#define CAMDEFRAMER_MAX_TAG_LEN 16
#define CAMDEFRAMER_MAX_QUEUED_FRAMES 50

enum {
ST_SYNC, /* waiting for start of frame */
ST_FRAME
};


/* should have a real Frame class someday */
class CamFrame : public BMallocIO {
public:
			CamFrame() : BMallocIO() { fStamp = system_time(); };
virtual		~CamFrame() {};
bigtime_t			Stamp() const { return fStamp; };
bigtime_t			fStamp;
};

class CamDeframer : public CamFilterInterface {
public:
			CamDeframer(CamDevice *device);
virtual 	~CamDeframer();
					// BPositionIO interface
					// read from translators/cs transforms
virtual ssize_t		Read(void *buffer, size_t size);
virtual ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
virtual off_t		Seek(off_t position, uint32 seek_mode);
virtual off_t		Position() const;
virtual status_t	SetSize(off_t size);
					// write from usb transfers
virtual ssize_t		Write(const void *buffer, size_t size);
virtual ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);

virtual status_t	WaitFrame(bigtime_t timeout);
virtual status_t	GetFrame(CamFrame **frame, bigtime_t *stamp); // caller deletes
virtual status_t	DropFrame();

status_t	RegisterSOFTags(const uint8 **tags, int count, size_t len, size_t skip);
status_t	RegisterEOFTags(const uint8 **tags, int count, size_t len, size_t skip);

protected:

int		FindTags(const uint8 *buf, size_t buflen, const uint8 **tags, int tagcount, size_t taglen, size_t skiplen, int *which=NULL);
int		FindSOF(const uint8 *buf, size_t buflen, int *which=NULL);
int		FindEOF(const uint8 *buf, size_t buflen, int *which=NULL);

CamFrame	*AllocFrame();

CamDevice	*fDevice;
size_t	fMinFrameSize;
size_t	fMaxFrameSize;
int	fState;
sem_id	fFrameSem;
BList	fFrames;
BLocker	fLocker;
CamFrame	*fCurrentFrame; /* the one we write to*/

/* tags */
const uint8 **fSOFTags;
const uint8 **fEOFTags;
int			fNumSOFTags;
int			fNumEOFTags;
size_t		fLenSOFTags;
size_t		fLenEOFTags;
size_t		fSkipSOFTags;
size_t		fSkipEOFTags;



};


#endif /* _CAM_DEFRAMER_H */
