/*
 * Copyright (c) 2005, David McPaul
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef MP4_STRUCTS_H
#define MP4_STRUCTS_H


#include <SupportDefs.h>


#define AUDIO_NONE			'NONE'
#define AUDIO_RAW			'raw '
#define AUDIO_TWOS1			'twos'
#define AUDIO_TWOS2			'sowt'
#define AUDIO_IMA4			'ima4'
#define AUDIO_MS_PCM02		0x6D730002
#define AUDIO_INTEL_PCM17	0x6D730011
#define AUDIO_MPEG3_CBR		0x6D730055

// this is all from the avi reader.  we rework it for mp4
struct mp4_main_header {
	uint32 micro_sec_per_frame;
	uint32 max_bytes_per_sec;
	uint32 padding_granularity;
	uint32 flags;
	uint32 total_frames;
	uint32 initial_frames;
	uint32 streams;
	uint32 suggested_buffer_size;
	uint32 width;
	uint32 height;
};

struct mp4_stream_header
{
	uint32 fourcc_type;
	uint32 fourcc_handler;
	uint32 flags;
	uint16 priority;
	uint16 language;
	uint32 initial_frames;
	uint32 scale;
	uint32 rate;
	uint32 start;
	uint32 length;
	uint32 suggested_buffer_size;
	uint32 quality;
	uint32 sample_size;
    int16 rect_left;
    int16 rect_top;
    int16 rect_right;
    int16 rect_bottom;
};

struct VideoMetaData
{
	uint32 compression;
	uint32 codecSubType;
	uint32 BufferSize;
	uint32 width;
	uint32 height;
	uint16 planes;
	uint16 bit_count;
	uint32 image_size;
	uint32 HorizontalResolution;
	uint32 VerticalResolution;
	uint32 FrameCount;
	float FrameRate;		// Frames per second
	uint8 *theDecoderConfig;
	size_t DecoderConfigSize;
};

struct AudioMetaData
{
	uint32 compression;		// compression used (ie codecid)
	uint32 codecSubType;	// Additional codecid
	uint16 NoOfChannels;	// 1 = mono, 2 = stereo
	uint16 SampleSize;		// bits per sample
	float SampleRate;		// Samples per second
	uint32 BufferSize;		// (Sample Rate * NoOfchannels * SampleSize * SamplesPerFrame) / 8 = bytes per second
	uint32 FrameSize;		// No Of Samples in 1 frame of audio (SamplesPerFrame)
	uint32 BitRate;			// Average Bitrate
	uint8 *theDecoderConfig;
	size_t DecoderConfigSize;
};

struct TimeToSample {
	uint32	Count;
	uint32	Duration;
};

struct CompTimeToSample {
	uint32	Count;
	uint32	Offset;
};

struct SampleToChunk {
	uint32	FirstChunk;
	uint32	SamplesPerChunk;
	uint32	SampleDescriptionID;
	uint32	TotalPrevSamples;
};

// Note standard is 32bits offsets but later is 64
// We standardise on 64 and convert older formats upwards
struct ChunkToOffset {
	uint64	Offset;
};

struct SyncSample {
	uint32	SyncSampleNo;
};

struct SampleSizeEntry {
	uint32	EntrySize;
};

struct mvhdV0 {
	uint32	CreationTime;
	uint32	ModificationTime;
	uint32	TimeScale;
	uint32	Duration;
	uint32	PreferredRate;		// Fixed point 16.16
	uint16	PreferredVolume;	// Fixed point 8.8
	uint16	Reserved1;
	uint32	Reserved2[2];
	uint32	Matrix[9];
	uint32	pre_defined[6];
	uint32	NextTrackID;
};

struct mvhdV1 {
	uint64	CreationTime;
	uint64	ModificationTime;
	uint32	TimeScale;
	uint64	Duration;
	uint32	PreferredRate;		// Fixed point 16.16
	uint16	PreferredVolume;	// Fixed point 8.8
	uint16	Reserved1;
	uint32	Reserved2[2];
	uint32	Matrix[9];
	uint32	pre_defined[6];
	uint32	NextTrackID;
};

struct tkhdV0 {
	uint32	CreationTime;
	uint32	ModificationTime;
	uint32	TrackID;
	uint32	Reserved1;
	uint32	Duration;
	uint32	Reserved2[2];
	uint16	Layer;
	uint16	AlternateGroup;
	uint16	Volume;			// Fixed 8.8
	uint16	Reserved3;
	int32	MatrixStructure[9];
	uint32	TrackWidth;		// Fixed 16.16
	uint32	TrackHeight;	// Fixed 16.16
};

struct tkhdV1 {
	uint64	CreationTime;
	uint64	ModificationTime;
	uint32	TrackID;
	uint32	Reserved1;
	uint64	Duration;
	uint32	Reserved2[2];
	uint16	Layer;
	uint16	AlternateGroup;
	uint16	Volume;			// Fixed 8.8
	uint16	Reserved3;
	int32	MatrixStructure[9];
	uint32	TrackWidth;		// Fixed 16.16
	uint32	TrackHeight;	// Fixed 16.16
};

struct mdhdV0 {
	uint32	CreationTime;
	uint32	ModificationTime;
	uint32	TimeScale;
	uint32	Duration;
	uint16	Language;  // Actually should be 1 bit followed by int(5)[3]
	uint16	Reserved;
};

struct mdhdV1 {
	uint64	CreationTime;
	uint64	ModificationTime;
	uint32	TimeScale;
	uint64	Duration;
	uint16	Language;  // Actually should be 1 bit followed by int(5)[3]
	uint16	Reserved;
};

struct hdlr {
	uint32	pre_defined;
	uint32	handler_type;
	uint32	Reserved[3];
};

struct vmhd {
	uint16	GraphicsMode;
	uint16	OpColour[3];
};

struct smhd {
	uint16	Balance;
	uint16	Reserved;
};

struct array_header {
	uint32	NoEntries;
};

struct SampleEntry {
	uint8	Reserved[6];
	uint16	DataReference;
};

struct AudioSampleEntry {
	uint32	Reserved[2];
	uint16	ChannelCount;
	uint16	SampleSize;
	uint16	pre_defined;
	uint16	reserved;
	uint32	SampleRate;		// 16.16
};

struct AudioDescription {
	AudioSampleEntry	theAudioSampleEntry;
	uint32				codecid;
	uint32				codecSubType;
	uint32				FrameSize;
	uint32				BufferSize;
	uint32				BitRate;
	uint8				*theDecoderConfig;
	size_t				DecoderConfigSize;
};

struct VideoSampleEntry {
	uint16	pre_defined1;
	uint16	reserved1;
	uint32	pre_defined2[3];
	uint16	Width;
	uint16	Height;
	uint32	HorizontalResolution;
	uint32	VerticalResolution;
	uint32	reserved2;
	uint16	FrameCount;
	char	CompressorName[32];
	uint16	Depth;
	uint16	pre_defined3;
};

struct VideoDescription {
	VideoSampleEntry	theVideoSampleEntry;
	uint32				codecid;
	uint32				codecSubType;
	uint8				*theDecoderConfig;
	size_t				DecoderConfigSize;
};

// The parts of the AAC ESDS we care about
struct AACHeader {
	uint8	objTypeIndex;
	uint8	sampleRateIndex;
	uint8	totalChannels;
	uint16	frameSize;
	uint16	adtsBuffer;
	uint8	totalDataBlocksInFrame;
};

#endif	// MP4_STRUCTS_H
