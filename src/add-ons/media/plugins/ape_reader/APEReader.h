#ifndef	___APEReader_H_
#define	___APEReader_H_


#include "MACLib.h"
#include "MonkeysAudioMIMEType.h"
#include "PositionBridgeIO.h"

#include "Reader.h"		// Haiku private header

using namespace BCodecKit;


const int32		BLOCK_COUNT = 1024*4;	// number of blocks, get from MACLib at once
const int32		BUFFER_SIZE = 1024*4;	// size of audio data passing to Media Kit
const int32		MEDIA_FILE_FORMAT_VERSION = 100;	// media_file_format::version


class	TAPEReader : public BReader
{
public:
	TAPEReader();
	virtual				~TAPEReader();

	virtual	const char*	Copyright();

	virtual	status_t	Sniff(int32* oStreamCount);

	virtual	void		GetFileFormatInfo(media_file_format* oMFF);

	virtual	status_t	AllocateCookie(int32 oStreamNumber, void** oCookie);
	virtual	status_t	FreeCookie(void* oCookie);

	virtual	status_t	GetStreamInfo(void* oCookie, int64* oFrameCount,
							bigtime_t* oDuration, media_format* oFormat,
							const void** oInfoBuffer, size_t* oInfoSize);

	virtual	status_t		Seek(void *cookie, uint32 flags,
								int64 *frame, bigtime_t *time);

	virtual	status_t		FindKeyFrame(void* cookie, uint32 flags,
								int64* frame, bigtime_t* time);

	virtual	status_t	GetNextChunk(void* oCookie, const void** oChunkBuffer,
								size_t* oChunkSize, media_header* oMediaHeader);

private:
	typedef	BReader	SUPER;

	bigtime_t			CurrentTime() const;
	status_t			LoadAPECheck() const;
	status_t			ReadBlocks();
	void				Unset();

	char*				mDecodedData;				// data after decoding
	int64				mDataSize;
	int64				mPlayPos;
	int64				mReadPos;
	int64				mReadPosTotal;
	status_t			mLoadAPECheck;
	BPositionIO*		mSrcPIO;
	IAPEDecompress*		mDecomp;
	TPositionBridgeIO	mPositionBridgeIO;
};


class	TAPEReaderPlugin : public BReaderPlugin
{
public:
	TAPEReaderPlugin();
	virtual	~TAPEReaderPlugin();

	virtual	BReader* NewReader();
};


#endif	// ___APEReader_H_
