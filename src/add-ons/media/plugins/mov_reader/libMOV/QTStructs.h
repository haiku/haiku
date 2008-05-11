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
#ifndef QT_STRUCTS_H
#define QT_STRUCTS_H


#include <SupportDefs.h>


struct xxxx {
	uint8	Version;
	uint8	Flags1;
	uint8	Flags2;
	uint8	Flags3;
	uint32	CreationTime;
	uint32	ModificationTime;
	uint32	TimeScale;
	uint32	Duration;
	uint16	Language;
	uint16	Quality;
};

#define AUDIO_NONE			'NONE'
#define AUDIO_RAW			'raw '
#define AUDIO_TWOS1			'twos'
#define AUDIO_TWOS2			'sowt'
#define AUDIO_IMA4			'ima4'
#define AUDIO_MS_PCM02		0x6D730002
#define AUDIO_INTEL_PCM17	0x6D730011
#define AUDIO_MPEG3_CBR		0x6D730055

// this is all from the avi reader.  we will rework it for mov
struct mov_main_header
{
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

struct mov_stream_header
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
	uint32 size;
	uint32 width;
	uint32 height;
	uint16 planes;
	uint16 bit_count;
	uint32 image_size;
	uint32 HorizontalResolution;
	uint32 VerticalResolution;
	float FrameRate;
	uint8	*theVOL;
	size_t VOLSize;
};

struct AudioMetaData
{
	uint32 compression;		// compression type used
	uint16 NoOfChannels;	// 1 = mono, 2 = stereo
	uint16 SampleSize;		// bits per sample
	float SampleRate;		// Samples per second
	uint32 BufferSize;		// (Sample Rate * NoOfchannels * SampleSize) / 8 = bytes per second
	uint32 FrameSize;		// Size of a Frame (NoOfChannels * SampleSize) / 8
	float BitRate;			// Bitrate of audio
	uint8 *theVOL;
	size_t VOLSize;
};

struct wave_format_ex
{
	uint16 format_tag;
	uint16 channels;
	uint32 frames_per_sec;
	uint32 avg_bytes_per_sec;
	uint16 block_align;
	uint16 bits_per_sample;
	uint16 extra_size;
};

struct stts {
	uint8	Version;
	uint8	Flags1;
	uint8	Flags2;
	uint8	Flags3;
	uint32	NoEntries;
};

struct TimeToSample {
	uint32	Count;
	uint32	Duration;
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
	uint64	SyncSampleNo;
};

struct SampleSizeHeader {
	uint8	Version;
	uint8	Flags1;
	uint8	Flags2;
	uint8	Flags3;
	uint32	SampleSize;
	uint32	NoEntries;
};

struct SampleSizeEntry {
	uint32	EntrySize;
};

struct mvhd {
	uint8	Version;
	uint8	Flags1;
	uint8	Flags2;
	uint8	Flags3;
	uint32	CreationTime;
	uint32	ModificationTime;
	uint32	TimeScale;
	uint32	Duration;
	uint32	PreferredRate;
	uint16	PreferredVolume;
	uint8	Reserved[10];
	uint8	Matrix[36];
	uint32	PreviewTime;
	uint32	PreviewDuration;
	uint32	PosterTime;
	uint32	SelectionTime;
	uint32	SelectionDuration;
	uint32	CurrentTime;
	uint32	NextTrackID;
};

struct tkhdV0 {
	uint8	Flags1;
	uint8	Flags2;
	uint8	Flags3;
	uint32	CreationTime;
	uint32	ModificationTime;
	uint32	TrackID;
	uint32	Reserved1;
	uint32	Duration;
	uint64	Reserved2;
	uint16	Layer;
	uint16	AlternateGroup;
	uint16	Volume;
	uint16	Reserved3;
	uint8	MatrixStructure[36];
	uint32	TrackWidth;
	uint32	TrackHeight;	
};

struct tkhdV1 {
	uint8	Flags1;
	uint8	Flags2;
	uint8	Flags3;
	uint64	CreationTime;
	uint64	ModificationTime;
	uint32	TrackID;
	uint32	Reserved1;
	uint32	Duration;
	uint64	Reserved2;
	uint16	Layer;
	uint16	AlternateGroup;
	uint16	Volume;
	uint16	Reserved3;
	uint8	MatrixStructure[36];
	uint32	TrackWidth;
	uint32	TrackHeight;	
};

struct mdhd {
	uint8	Version;
	uint8	Flags1;
	uint8	Flags2;
	uint8	Flags3;
	uint32	CreationTime;
	uint32	ModificationTime;
	uint32	TimeScale;
	uint32	Duration;
	uint16	Language;
	uint16	Quality;
};

struct hdlr {
	uint8	Version;
	uint8	Flags1;
	uint8	Flags2;
	uint8	Flags3;
	uint32	ComponentType;
	uint32	ComponentSubType;
	uint32	ComponentManufacturer;
	uint32	ComponentFlags;
	uint32	ComponentFlagsMask;
};

struct vmhd {
	uint8	Version;
	uint8	Flags1;
	uint8	Flags2;
	uint8	Flags3;
	uint16	GraphicsMode;
	uint16	OpColourRed;
	uint16	OpColourGreen;
	uint16	OpColourBlue;
};

struct smhd {
	uint8	Version;
	uint8	Flags1;
	uint8	Flags2;
	uint8	Flags3;
	uint16	Balance;
	uint16	Reserved;
};

struct array_header {
	uint8	Version;
	uint8	Flags1;
	uint8	Flags2;
	uint8	Flags3;
	uint32	NoEntries;
};

struct SampleDescBase {
	uint32	Size;
	uint32	DataFormat;
	uint8	Reserved[6];
	uint16	DataReference;
};

#define SampleDescBaseSize 16

struct SoundDescription {
	uint16	Version;
	uint16	Revision;
	uint32	Vendor;
	uint16	NoOfChannels;
	uint16	SampleSize;
	uint16	CompressionID;
	uint16	PacketSize;
	uint32	SampleRate;
};

#define SoundDescriptionSize 20

struct SoundDescriptionV0 {
	SampleDescBase	basefields;
	SoundDescription desc;
};

struct SoundDescriptionV1 {
	SampleDescBase	basefields;
	SoundDescription desc;
	uint32	samplesPerPacket;
	uint32	bytesPerPacket;
	uint32	bytesPerFrame;
	uint32	bytesPerSample;
	// These are optional data from atoms that follow
	wave_format_ex theWaveFormat;		// wave atom is a copy of wave_format_ex
	uint8 *theVOL;
	size_t VOLSize;
};

struct VideoDescription {
	uint16	Version;
	uint16	Revision;
	uint32	Vendor;
	uint32	TemporaralQuality;
	uint32	SpacialQuality;
	uint16	Width;
	uint16	Height;
	uint32	HorizontalResolution;
	uint32	VerticalResolution;
	uint32	DataSize;
	uint16	FrameCount;
	char	CompressorName[32];
	uint16	Depth;
	uint16	ColourTableID;
	// Optional atoms follow
};

#define VideoDescriptionSize 70

struct VideoDescriptionV0 {
	SampleDescBase	basefields;
	VideoDescription desc;
	uint8	*theVOL;
	size_t VOLSize;
};

#endif	// QT_STRUCTS_H

