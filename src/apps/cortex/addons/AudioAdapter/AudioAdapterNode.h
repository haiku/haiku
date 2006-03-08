// AudioAdapterNode.h

#ifndef __AudioAdapterNode_H__
#define __AudioAdapterNode_H__

#include "AudioFilterNode.h"
#include "AudioAdapterParams.h"

class _AudioAdapterNode :
	public	AudioFilterNode
{
	typedef	AudioFilterNode _inherited;

public:													// ctor/dtor
	virtual ~_AudioAdapterNode();
	_AudioAdapterNode(
		const char*									name,
		IAudioOpFactory*						opFactory,
		BMediaAddOn*								addOn=0);

public:													// AudioFilterNode
	status_t getRequiredInputFormat(
		media_format&								ioFormat);
	status_t getPreferredInputFormat(
		media_format&								ioFormat);
		
	status_t getRequiredOutputFormat(
		media_format&								ioFormat);
	
	status_t getPreferredOutputFormat(
		media_format&								ioFormat);
	
	status_t validateProposedInputFormat(
		const media_format&					preferredFormat,
		media_format&								ioProposedFormat);

	status_t validateProposedOutputFormat(
		const media_format&					preferredFormat,
		media_format&								ioProposedFormat);

	virtual void SetParameterValue(
		int32												id,
		bigtime_t										changeTime,
		const void*									value,
		size_t											size); //nyi

public:													// BBufferProducer/Consumer

	virtual status_t Connected(
		const media_source&					source,
		const media_destination&		destination,
		const media_format&					format,
		media_input*								outInput);

	virtual void Connect(
		status_t										status,
		const media_source&					source,
		const media_destination&		destination,
		const media_format&					format,
		char*												ioName);
		
private:
	void _attemptInputFormatChange(
		const media_multi_audio_format& format); //nyi

	void _attemptOutputFormatChange(
		const media_multi_audio_format& format); //nyi

	void _broadcastInputFormatParams();
	void _broadcastOutputFormatParams();
};



#endif /*__AudioAdapterNode_H__*/