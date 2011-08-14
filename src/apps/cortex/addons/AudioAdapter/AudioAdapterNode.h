/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// AudioAdapterNode.h
#ifndef AUDIO_ADAPTER_NODE_H
#define AUDIO_ADAPTER_NODE_H


#include "AudioFilterNode.h"
#include "AudioAdapterParams.h"


class _AudioAdapterNode : public AudioFilterNode {
	typedef	AudioFilterNode _inherited;

	public:	// ctor/dtor
		virtual ~_AudioAdapterNode();
		_AudioAdapterNode(const char* name, IAudioOpFactory* opFactory,
			BMediaAddOn* addOn = 0);

	public:	// AudioFilterNode
		status_t getRequiredInputFormat(media_format& ioFormat);
		status_t getPreferredInputFormat(media_format& ioFormat);
		status_t getRequiredOutputFormat(media_format& ioFormat);

		status_t getPreferredOutputFormat(media_format& ioFormat);

		status_t validateProposedInputFormat(
			const media_format& preferredFormat,
			media_format& ioProposedFormat);

		status_t validateProposedOutputFormat(
			const media_format&	preferredFormat,
			media_format& ioProposedFormat);

		virtual void SetParameterValue(int32 id, bigtime_t changeTime,
			const void* value, size_t size); //nyi

	public:	// BBufferProducer/Consumer
		virtual status_t Connected(const media_source&	source,
			const media_destination& destination,
			const media_format& format,
			media_input* outInput);

		virtual void Connect(status_t status, const media_source& source,
			const media_destination& destination,
			const media_format& format,
			char* ioName);

	private:
		void _attemptInputFormatChange(
		const media_multi_audio_format& format); //nyi

		void _attemptOutputFormatChange(
			const media_multi_audio_format& format); //nyi

		void _broadcastInputFormatParams();
		void _broadcastOutputFormatParams();
};

#endif	// AUDIO_ADAPTER_NODE_H
