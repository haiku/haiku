/*
 * Copyright 2021, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef LIBRAW_H
#define LIBRAW_H


#include <DataIO.h>
#include <SupportDefs.h>

#include <libraw/libraw_const.h>


class LibRaw;
class LibRaw_haiku_datastream;


struct image_meta_info {
	char	manufacturer[64];
	char	model[64];
	char	software[64];
	float	flash_used;
	float	iso_speed;
	float	shutter;
	float	aperture;
	float	focal_length;
	double	pixel_aspect;
	uint32	raw_width;
	uint32	raw_height;
	int		flip;
	uint32	dng_version;
	uint32	shot_order;
	int32	black;
	int32	maximum;
	float	camera_multipliers[4];
	float	pre_multipliers[4];
	float	rgb_camera[3][4];	/* RGB from camera color */
	time_t	timestamp;
};

struct image_data_info {
	uint32	width;
	uint32	height;
	uint32	output_width;
	uint32	output_height;
	uint32	bits_per_sample;
	uint32	compression;
	uint32	photometric_interpretation;
	uint32	flip;
	uint32	samples;
	uint32	bytes;
	off_t	data_offset;
	bool	is_raw;
};

#define COMPRESSION_NONE		1
#define COMPRESSION_OLD_JPEG	6		// Old JPEG (before 6.0)
#define COMPRESSION_PACKBITS	32773	// Macintosh RLE


typedef void (*monitor_hook)(const char* message, float percentage, void* data);

class LibRAW {
public:
								LibRAW(BPositionIO& stream);
								~LibRAW();

			status_t			Identify();
			status_t			ReadImageAt(uint32 index, uint8*& outputBuffer,
									size_t& bufferSize);

			void				GetMetaInfo(image_meta_info& metaInfo) const;
			uint32				CountImages() const;
			status_t			ImageAt(uint32 index, image_data_info& info)
									const;

			status_t			GetEXIFTag(off_t& offset, size_t& length,
									bool& bigEndian) const;
			void				SetProgressMonitor(monitor_hook hook,
									void* data);

			void				SetHalfSize(bool half);

private:
	static	int					ProgressCallback(void *data,
									enum LibRaw_progress p,
									int iteration, int expected);
			int					_ProgressCallback(enum LibRaw_progress p,
									int iteration, int expected);

			monitor_hook		fProgressMonitor;
			void*				fProgressData;

			LibRaw*				fRaw;
			LibRaw_haiku_datastream* fDatastream;
};


#endif	// LIBRAW_H
