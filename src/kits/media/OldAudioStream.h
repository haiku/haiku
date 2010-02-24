/******************************************************************************

	File:	AudioStream.h

	Copyright 1995-97, Be Incorporated

******************************************************************************/
#ifndef _AUDIO_STREAM_H
#define _AUDIO_STREAM_H


#include "OldBufferStream.h"


/* ================
   Class definition for BADCStream and BDACStream
   ================ */

class BADCStream : public BAbstractBufferStream
{
public:

					BADCStream();
	virtual			~BADCStream();

	status_t		SetADCInput(int32 device);
	status_t		ADCInput(int32* device) const;

	status_t		SetSamplingRate(float sRate);
	status_t		SamplingRate(float* sRate) const;

	status_t		BoostMic(bool boost);
	bool			IsMicBoosted() const;

	status_t		SetStreamBuffers(size_t bufferSize, int32 bufferCount);

protected:

	virtual BMessenger*	Server() const; 		/* message pipe to server */
	virtual stream_id	StreamID() const;		/* stream identifier */

private:

virtual	void		_ReservedADCStream1();
virtual	void		_ReservedADCStream2();
virtual	void		_ReservedADCStream3();

	BMessenger*		fServer;
	stream_id		fStreamID;
	uint32			_reserved[4];
};


class BDACStream : public BAbstractBufferStream
{
public:

					BDACStream();
	virtual			~BDACStream();

	status_t		SetSamplingRate(float sRate);
	status_t		SamplingRate(float* sRate) const;

	status_t		SetVolume(int32 device,
							  float l_volume,
							  float r_volume);

	status_t		GetVolume(int32 device,
							  float *l_volume,
							  float *r_volume,
							  bool *enabled) const;

	status_t		EnableDevice(int32 device, bool enable);
	bool			IsDeviceEnabled(int32 device) const;

	status_t		SetStreamBuffers(size_t bufferSize, int32 bufferCount);

protected:

	virtual BMessenger*	Server() const; 		/* message pipe to server */
	virtual stream_id	StreamID() const;		/* stream identifier */

private:

virtual	void		_ReservedDACStream1();
virtual	void		_ReservedDACStream2();
virtual	void		_ReservedDACStream3();

	BMessenger*		fServer;
	stream_id		fStreamID;
	uint32			_reserved[4];
};

#endif			// #ifdef _AUDIO_STREAM_H
