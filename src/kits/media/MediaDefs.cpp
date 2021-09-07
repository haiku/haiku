/*
 * Copyright 2015, Dario Casalinuovo
 * Copyright 2004, 2006, Jérôme Duval.
 * Copyright 2003-2004, Andrew Bachmann.
 * Copyright 2002-2004, 2006 Marcus Overhagen.
 * Copyright 2002, Eric Jaessler.
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include <MediaDefs.h>

#include <Application.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <IconUtils.h>
#include <LaunchRoster.h>
#include <Locale.h>
#include <MediaNode.h>
#include <MediaRoster.h>
#include <Node.h>
#include <Notification.h>
#include <Roster.h>

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "AddOnManager.h"
#include "DataExchange.h"
#include "MediaDebug.h"
#include "MediaMisc.h"
#include "MediaRosterEx.h"


#define META_DATA_MAX_SIZE			(16 << 20)
#define META_DATA_AREA_MIN_SIZE		32000

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MediaDefs"


// #pragma mark - media_destination


media_destination::media_destination(port_id port, int32 id)
	:
	port(port),
	id(id)
{
}


media_destination::media_destination(const media_destination& clone)
	:
	port(clone.port),
	id(clone.id)
{
}


media_destination&
media_destination::operator=(const media_destination& clone)
{
	port = clone.port;
	id = clone.id;
	return *this;
}


media_destination::media_destination()
	:
	port(-1),
	id(-1)
{
}


media_destination::~media_destination()
{
}


media_destination media_destination::null(-1, -1);


// #pragma mark - media_source


media_source::media_source(port_id port,
						   int32 id)
	:
	port(port),
	id(id)
{
}


media_source::media_source(const media_source& clone)
	:
	port(clone.port),
	id(clone.id)
{
}


media_source&
media_source::operator=(const media_source& clone)
{
	port = clone.port;
	id = clone.id;
	return *this;
}


media_source::media_source()
	:
	port(-1),
	id(-1)
{
}


media_source::~media_source()
{
}


media_source media_source::null(-1, -1);


// #pragma mark -


bool
operator==(const media_destination& a, const media_destination& b)
{
	return a.port == b.port && a.id == b.id;
}


bool
operator!=(const media_destination& a, const media_destination& b)
{
	return a.port != b.port || a.id != b.id;
}


bool
operator<(const media_destination& a, const media_destination& b)
{
	UNIMPLEMENTED();
	return false;
}


bool
operator==(const media_source& a, const media_source& b)
{
	return a.port == b.port && a.id == b.id;
}


bool
operator!=(const media_source& a, const media_source& b)
{
	return a.port != b.port || a.id != b.id;
}


bool
operator<(const media_source& a, const media_source& b)
{
	UNIMPLEMENTED();
	return false;
}


bool
operator==(const media_node& a, const media_node& b)
{
	return a.node == b.node && a.port == b.port && a.kind == b.kind;
}


bool
operator!=(const media_node& a, const media_node& b)
{
	return a.node != b.node || a.port != b.port || a.kind != b.kind;
}


bool
operator<(const media_node& a, const media_node& b)
{
	UNIMPLEMENTED();
	return false;
}


// #pragma mark -


#if __GNUC__ == 2
const media_multi_audio_format media_raw_audio_format::wildcard
	= media_multi_audio_format();

const media_multi_audio_format media_multi_audio_format::wildcard
	= media_multi_audio_format();
#else
const media_multi_audio_format media_raw_audio_format::wildcard = {};

const media_multi_audio_format media_multi_audio_format::wildcard = {};
#endif

const media_encoded_audio_format media_encoded_audio_format::wildcard = {};

const media_video_display_info media_video_display_info::wildcard = {};

const media_raw_video_format media_raw_video_format::wildcard = {};

const media_encoded_video_format media_encoded_video_format::wildcard = {};

const media_multistream_format media_multistream_format::wildcard = {};


// #pragma mark - media_format::Matches() support


static bool
raw_audio_format_matches(const media_raw_audio_format& a,
	const media_raw_audio_format& b)
{
	if (a.frame_rate != 0 && b.frame_rate != 0 && a.frame_rate != b.frame_rate)
		return false;
	if (a.channel_count != 0 && b.channel_count != 0
		&& a.channel_count != b.channel_count) {
		return false;
	}
	if (a.format != 0 && b.format != 0 && a.format != b.format)
		return false;
	if (a.byte_order != 0 && b.byte_order != 0 && a.byte_order != b.byte_order)
		return false;
	if (a.buffer_size != 0 && b.buffer_size != 0
		&& a.buffer_size != b.buffer_size) {
		return false;
	}
	if (a.frame_rate != 0 && b.frame_rate != 0 && a.frame_rate != b.frame_rate)
		return false;
	return true;
}


static bool
multi_audio_info_matches(const media_multi_audio_info& a,
	const media_multi_audio_info& b)
{
	if (a.channel_mask != 0 && b.channel_mask != 0
		&& a.channel_mask != b.channel_mask) {
		return false;
	}
	if (a.valid_bits != 0 && b.valid_bits != 0 && a.valid_bits != b.valid_bits)
		return false;
	if (a.matrix_mask != 0 && b.matrix_mask != 0
		&& a.matrix_mask != b.matrix_mask) {
		return false;
	}
	return true;
}


static bool
multi_audio_format_matches(const media_multi_audio_format& a,
	const media_multi_audio_format& b)
{
	return raw_audio_format_matches(a, b) && multi_audio_info_matches(a, b);
}


static bool
raw_video_format_matches(const media_raw_video_format& a,
	const media_raw_video_format& b)
{
	if (a.field_rate != 0 && b.field_rate != 0
		&& a.field_rate != b.field_rate) {
		return false;
	}
	if (a.interlace != 0 && b.interlace != 0
		&& a.interlace != b.interlace) {
		return false;
	}
	if (a.first_active != 0 && b.first_active != 0
		&& a.first_active != b.first_active) {
		return false;
	}
	if (a.last_active != 0 && b.last_active != 0
		&& a.last_active != b.last_active) {
		return false;
	}
	if (a.orientation != 0 && b.orientation != 0
		&& a.orientation != b.orientation) {
		return false;
	}
	if (a.pixel_width_aspect != 0 && b.pixel_width_aspect != 0
		&& a.pixel_width_aspect != b.pixel_width_aspect) {
		return false;
	}
	if (a.pixel_height_aspect != 0 && b.pixel_height_aspect != 0
		&& a.pixel_height_aspect != b.pixel_height_aspect) {
		return false;
	}
	if (a.display.format != 0 && b.display.format != 0
		&& a.display.format != b.display.format) {
		return false;
	}
	if (a.display.line_width != 0 && b.display.line_width != 0
		&& a.display.line_width != b.display.line_width) {
		return false;
	}
	if (a.display.line_count != 0 && b.display.line_count != 0
		&& a.display.line_count != b.display.line_count) {
		return false;
	}
	if (a.display.bytes_per_row != 0 && b.display.bytes_per_row != 0
		&& a.display.bytes_per_row != b.display.bytes_per_row) {
		return false;
	}
	if (a.display.pixel_offset != 0 && b.display.pixel_offset != 0
		&& a.display.pixel_offset != b.display.pixel_offset) {
		return false;
	}
	if (a.display.line_offset != 0 && b.display.line_offset != 0
		&& a.display.line_offset != b.display.line_offset) {
		return false;
	}
	if (a.display.flags != 0 && b.display.flags != 0
		&& a.display.flags != b.display.flags) {
		return false;
	}

	return true;
}


static bool
multistream_format_matches(const media_multistream_format& a,
	const media_multistream_format& b)
{
	if (a.avg_bit_rate != 0 && b.avg_bit_rate != 0
		&& a.avg_bit_rate != b.avg_bit_rate) {
		return false;
	}
	if (a.max_bit_rate != 0 && b.max_bit_rate != 0
		&& a.max_bit_rate != b.max_bit_rate) {
		return false;
	}
	if (a.avg_chunk_size != 0 && b.avg_chunk_size != 0
		&& a.avg_chunk_size != b.avg_chunk_size) {
		return false;
	}
	if (a.max_chunk_size != 0 && b.max_chunk_size != 0
		&& a.max_chunk_size != b.max_chunk_size) {
		return false;
	}
	if (a.flags != 0 && b.flags != 0 && a.flags != b.flags)
		return false;
	if (a.format != 0 && b.format != 0 && a.format != b.format)
		return false;

	if (a.format == 0 && b.format == 0) {
		// TODO: How do we compare two formats with no type?
		return true;
	}

	switch ((a.format != 0) ? a.format : b.format) {
		default:
			return true; // TODO: really?

		case media_multistream_format::B_VID:
			if (a.u.vid.frame_rate != 0 && b.u.vid.frame_rate != 0
				&& a.u.vid.frame_rate != b.u.vid.frame_rate) {
				return false;
			}
			if (a.u.vid.width != 0 && b.u.vid.width != 0
				&& a.u.vid.width != b.u.vid.width) {
				return false;
			}
			if (a.u.vid.height != 0 && b.u.vid.height != 0
				&& a.u.vid.height != b.u.vid.height) {
				return false;
			}
			if (a.u.vid.space != 0 && b.u.vid.space != 0
				&& a.u.vid.space != b.u.vid.space) {
				return false;
			}
			if (a.u.vid.sampling_rate != 0 && b.u.vid.sampling_rate != 0
				&& a.u.vid.sampling_rate != b.u.vid.sampling_rate) {
				return false;
			}
			if (a.u.vid.sample_format != 0 && b.u.vid.sample_format != 0
				&& a.u.vid.sample_format != b.u.vid.sample_format) {
				return false;
			}
			if (a.u.vid.byte_order != 0 && b.u.vid.byte_order != 0
				&& a.u.vid.byte_order != b.u.vid.byte_order) {
				return false;
			}
			if (a.u.vid.channel_count != 0 && b.u.vid.channel_count != 0
				&& a.u.vid.channel_count != b.u.vid.channel_count) {
				return false;
			}
			return true;

		case media_multistream_format::B_AVI:
			if (a.u.avi.us_per_frame != 0 && b.u.avi.us_per_frame != 0
				&& a.u.avi.us_per_frame != b.u.avi.us_per_frame) {
				return false;
			}
			if (a.u.avi.width != 0 && b.u.avi.width != 0
				&& a.u.avi.width != b.u.avi.width) {
				return false;
			}
			if (a.u.avi.height != 0 && b.u.avi.height != 0
				&& a.u.avi.height != b.u.avi.height) {
				return false;
			}
			if (a.u.avi.type_count != 0 && b.u.avi.type_count != 0
				&& a.u.avi.type_count != b.u.avi.type_count) {
				return false;
			}
			if (a.u.avi.types[0] != 0 && b.u.avi.types[0] != 0
				&& a.u.avi.types[0] != b.u.avi.types[0]) {
				return false;
			}
			if (a.u.avi.types[1] != 0 && b.u.avi.types[1] != 0
				&& a.u.avi.types[1] != b.u.avi.types[1]) {
				return false;
			}
			if (a.u.avi.types[2] != 0 && b.u.avi.types[2] != 0
				&& a.u.avi.types[2] != b.u.avi.types[2]) {
				return false;
			}
			if (a.u.avi.types[3] != 0 && b.u.avi.types[3] != 0
				&& a.u.avi.types[3] != b.u.avi.types[3]) {
				return false;
			}
			if (a.u.avi.types[4] != 0 && b.u.avi.types[4] != 0
				&& a.u.avi.types[4] != b.u.avi.types[4]) {
				return false;
			}
			return true;
	}
}


static bool
encoded_audio_format_matches(const media_encoded_audio_format& a,
	const media_encoded_audio_format& b)
{
	if (!raw_audio_format_matches(a.output, b.output))
		return false;
	if (a.encoding != 0 && b.encoding != 0 && a.encoding != b.encoding)
		return false;
	if (a.bit_rate != 0 && b.bit_rate != 0 && a.bit_rate != b.bit_rate)
		return false;
	if (a.frame_size != 0 && b.frame_size != 0 && a.frame_size != b.frame_size)
		return false;
	if (!multi_audio_info_matches(a.multi_info, b.multi_info))
		return false;

	if (a.encoding == 0 && b.encoding == 0)
		return true; // can't compare

	switch((a.encoding != 0) ? a.encoding : b.encoding) {
		case media_encoded_audio_format::B_ANY:
		default:
			return true;
	}
}


static bool
encoded_video_format_matches(const media_encoded_video_format& a,
	const media_encoded_video_format& b)
{
	if (!raw_video_format_matches(a.output, b.output))
		return false;
	if (a.encoding != 0 && b.encoding != 0 && a.encoding != b.encoding)
		return false;

	if (a.avg_bit_rate != 0 && b.avg_bit_rate != 0
		&& a.avg_bit_rate != b.avg_bit_rate) {
		return false;
	}
	if (a.max_bit_rate != 0 && b.max_bit_rate != 0
		&& a.max_bit_rate != b.max_bit_rate) {
		return false;
	}
	if (a.frame_size != 0 && b.frame_size != 0
		&& a.frame_size != b.frame_size) {
		return false;
	}
	if (a.forward_history != 0 && b.forward_history != 0
		&& a.forward_history != b.forward_history) {
		return false;
	}
	if (a.backward_history != 0 && b.backward_history != 0
		&& a.backward_history != b.backward_history) {
		return false;
	}

	if (a.encoding == 0 && b.encoding == 0)
		return true; // can't compare

	switch((a.encoding != 0) ? a.encoding : b.encoding) {
		case media_encoded_video_format::B_ANY:
		default:
			return true;
	}
}


// #pragma mark - media_format::SpecializeTo() support


static void
raw_audio_format_specialize(media_raw_audio_format* format,
	const media_raw_audio_format* other)
{
	if (format->frame_rate == 0)
		format->frame_rate = other->frame_rate;
	if (format->channel_count == 0)
		format->channel_count = other->channel_count;
	if (format->format == 0)
		format->format = other->format;
	if (format->byte_order == 0)
		format->byte_order = other->byte_order;
	if (format->buffer_size == 0)
		format->buffer_size = other->buffer_size;
	if (format->frame_rate == 0)
		format->frame_rate = other->frame_rate;
}


static void
multi_audio_info_specialize(media_multi_audio_info* format,
	const media_multi_audio_info* other)
{
	if (format->channel_mask == 0)
		format->channel_mask = other->channel_mask;
	if (format->valid_bits == 0)
		format->valid_bits = other->valid_bits;
	if (format->matrix_mask == 0)
		format->matrix_mask = other->matrix_mask;
}


static void
multi_audio_format_specialize(media_multi_audio_format* format,
	const media_multi_audio_format* other)
{
	raw_audio_format_specialize(format, other);
	multi_audio_info_specialize(format, other);
}


static void
raw_video_format_specialize(media_raw_video_format* format,
	const media_raw_video_format* other)
{
	if (format->field_rate == 0)
		format->field_rate = other->field_rate;
	if (format->interlace == 0)
		format->interlace = other->interlace;
	if (format->first_active == 0)
		format->first_active = other->first_active;
	if (format->last_active == 0)
		format->last_active = other->last_active;
	if (format->orientation == 0)
		format->orientation = other->orientation;
	if (format->pixel_width_aspect == 0)
		format->pixel_width_aspect = other->pixel_width_aspect;
	if (format->pixel_height_aspect == 0)
		format->pixel_height_aspect = other->pixel_height_aspect;
	if (format->display.format == 0)
		format->display.format = other->display.format;
	if (format->display.line_width == 0)
		format->display.line_width = other->display.line_width;
	if (format->display.line_count == 0)
		format->display.line_count = other->display.line_count;
	if (format->display.bytes_per_row == 0)
		format->display.bytes_per_row = other->display.bytes_per_row;
	if (format->display.pixel_offset == 0)
		format->display.pixel_offset = other->display.pixel_offset;
	if (format->display.line_offset == 0)
		format->display.line_offset = other->display.line_offset;
	if (format->display.flags == 0)
		format->display.flags = other->display.flags;
}


static void
multistream_format_specialize(media_multistream_format* format,
	const media_multistream_format* other)
{
	if (format->avg_bit_rate == 0)
		format->avg_bit_rate = other->avg_bit_rate;
	if (format->max_bit_rate == 0)
		format->max_bit_rate = other->max_bit_rate;
	if (format->avg_chunk_size == 0)
		format->avg_chunk_size = other->avg_chunk_size;
	if (format->max_chunk_size == 0)
		format->max_chunk_size = other->max_chunk_size;
	if (format->flags == 0)
		format->flags = other->flags;
	if (format->format == 0)
		format->format = other->format;

	switch (format->format) {
		case media_multistream_format::B_VID:
			if (format->u.vid.frame_rate == 0)
				format->u.vid.frame_rate = other->u.vid.frame_rate;
			if (format->u.vid.width == 0)
				format->u.vid.width = other->u.vid.width;
			if (format->u.vid.height == 0)
				format->u.vid.height = other->u.vid.height;
			if (format->u.vid.space == 0)
				format->u.vid.space = other->u.vid.space;
			if (format->u.vid.sampling_rate == 0)
				format->u.vid.sampling_rate = other->u.vid.sampling_rate;
			if (format->u.vid.sample_format == 0)
				format->u.vid.sample_format = other->u.vid.sample_format;
			if (format->u.vid.byte_order == 0)
				format->u.vid.byte_order = other->u.vid.byte_order;
			if (format->u.vid.channel_count == 0)
				format->u.vid.channel_count = other->u.vid.channel_count;
			break;

		case media_multistream_format::B_AVI:
			if (format->u.avi.us_per_frame == 0)
				format->u.avi.us_per_frame = other->u.avi.us_per_frame;
			if (format->u.avi.width == 0)
				format->u.avi.width = other->u.avi.width;
			if (format->u.avi.height == 0)
				format->u.avi.height = other->u.avi.height;
			if (format->u.avi.type_count == 0)
				format->u.avi.type_count = other->u.avi.type_count;
			if (format->u.avi.types[0] == 0)
				format->u.avi.types[0] = other->u.avi.types[0];
			if (format->u.avi.types[1] == 0)
				format->u.avi.types[1] = other->u.avi.types[1];
			if (format->u.avi.types[2] == 0)
				format->u.avi.types[2] = other->u.avi.types[2];
			if (format->u.avi.types[3] == 0)
				format->u.avi.types[3] = other->u.avi.types[3];
			if (format->u.avi.types[4] == 0)
				format->u.avi.types[4] = other->u.avi.types[4];
			break;

		default:
			ERROR("media_format::SpecializeTo can't specialize "
				"media_multistream_format of format %" B_PRId32 "\n",
				format->format);
	}
}


static void
encoded_audio_format_specialize(media_encoded_audio_format* format,
	const media_encoded_audio_format* other)
{
	raw_audio_format_specialize(&format->output, &other->output);
	if (format->encoding == 0)
		format->encoding = other->encoding;
	if (format->bit_rate == 0)
		format->bit_rate = other->bit_rate;
	if (format->frame_size == 0)
		format->frame_size = other->frame_size;
	multi_audio_info_specialize(&format->multi_info, &other->multi_info);
}


static void
encoded_video_format_specialize(media_encoded_video_format* format,
	const media_encoded_video_format* other)
{
	raw_video_format_specialize(&format->output, &other->output);
	if (format->avg_bit_rate == 0)
		format->avg_bit_rate = other->avg_bit_rate;
	if (format->max_bit_rate == 0)
		format->max_bit_rate = other->max_bit_rate;
	if (format->encoding == 0)
		format->encoding = other->encoding;
	if (format->frame_size == 0)
		format->frame_size = other->frame_size;
	if (format->forward_history == 0)
		format->forward_history = other->forward_history;
	if (format->backward_history == 0)
		format->backward_history = other->backward_history;
}


// #pragma mark - media_format


bool
media_format::Matches(const media_format* other) const
{
	CALLED();

	if (type == 0 && other->type == 0) {
		// TODO: How do we compare two formats with no type?
		return true;
	}

	if (type != 0 && other->type != 0 && type != other->type)
		return false;

	switch ((type != 0) ? type : other->type) {
		case B_MEDIA_RAW_AUDIO:
			return multi_audio_format_matches(u.raw_audio, other->u.raw_audio);

		case B_MEDIA_RAW_VIDEO:
			return raw_video_format_matches(u.raw_video, other->u.raw_video);

		case B_MEDIA_MULTISTREAM:
			return multistream_format_matches(u.multistream,
				other->u.multistream);

		case B_MEDIA_ENCODED_AUDIO:
			return encoded_audio_format_matches(u.encoded_audio,
				other->u.encoded_audio);

		case B_MEDIA_ENCODED_VIDEO:
			return encoded_video_format_matches(u.encoded_video,
				other->u.encoded_video);

		default:
			return true; // TODO: really?
	}
}


void
media_format::SpecializeTo(const media_format* otherFormat)
{
	CALLED();

	if (type == 0 && otherFormat->type == 0) {
		ERROR("media_format::SpecializeTo can't specialize wildcard to other "
			"wildcard format\n");
		return;
	}

	if (type == 0)
		type = otherFormat->type;

	switch (type) {
		case B_MEDIA_RAW_AUDIO:
			multi_audio_format_specialize(&u.raw_audio,
				&otherFormat->u.raw_audio);
			return;

		case B_MEDIA_RAW_VIDEO:
			raw_video_format_specialize(&u.raw_video,
				&otherFormat->u.raw_video);
			return;

		case B_MEDIA_MULTISTREAM:
			multistream_format_specialize(&u.multistream,
				&otherFormat->u.multistream);
			return;

		case B_MEDIA_ENCODED_AUDIO:
			encoded_audio_format_specialize(&u.encoded_audio,
				&otherFormat->u.encoded_audio);
			return;

		case B_MEDIA_ENCODED_VIDEO:
			encoded_video_format_specialize(&u.encoded_video,
				&otherFormat->u.encoded_video);
			return;

		default:
			ERROR("media_format::SpecializeTo can't specialize format "
				"type %d\n", type);
	}
}


status_t
media_format::SetMetaData(const void* data, size_t size)
{
	if (!data || size > META_DATA_MAX_SIZE)
		return B_BAD_VALUE;

	void* new_addr;
	area_id new_area;
	if (size < META_DATA_AREA_MIN_SIZE) {
		new_area = B_BAD_VALUE;
		new_addr = malloc(size);
		if (!new_addr)
			return B_NO_MEMORY;
	} else {
		new_area = create_area("meta_data_area", &new_addr, B_ANY_ADDRESS,
			ROUND_UP_TO_PAGE(size), B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (new_area < 0)
			return (status_t)new_area;
	}

	if (meta_data_area > 0)
		delete_area(meta_data_area);
	else
		free(meta_data);

	meta_data = new_addr;
	meta_data_size = size;
	meta_data_area = new_area;

	memcpy(meta_data, data, size);

	if (meta_data_area > 0)
		set_area_protection(meta_data_area, B_READ_AREA);

	return B_OK;
}


const void*
media_format::MetaData() const
{
	return meta_data;
}


int32
media_format::MetaDataSize() const
{
	return meta_data_size;
}


void
media_format::Unflatten(const char *flatBuffer)
{
	// TODO: we should not!!! make flat copies of media_format
	memcpy(this, flatBuffer, sizeof(*this));
	meta_data = NULL;
	meta_data_area = B_BAD_VALUE;
}


void
media_format::Clear()
{
	memset(this, 0x00, sizeof(*this));
	meta_data = NULL;
	meta_data_area = B_BAD_VALUE;
}


media_format::media_format()
{
	this->Clear();
}


media_format::media_format(const media_format& other)
{
	this->Clear();
	*this = other;
}


media_format::~media_format()
{
	if (meta_data_area > 0)
		delete_area(meta_data_area);
	else
		free(meta_data);
}


// final
media_format&
media_format::operator=(const media_format& clone)
{
	// get rid of this format's meta data
	this->~media_format();
		// danger: using only ~media_format() would call the constructor

	// make a binary copy
	memcpy(this, &clone, sizeof(*this));
	// some binary copies are invalid:
	meta_data = NULL;
	meta_data_area = B_BAD_VALUE;

	// clone or copy the meta data
	if (clone.meta_data) {
		if (clone.meta_data_area != B_BAD_VALUE) {
			meta_data_area = clone_area("meta_data_clone_area", &meta_data,
				B_ANY_ADDRESS, B_READ_AREA, clone.meta_data_area);
			if (meta_data_area < 0) {
				// whoops, we just lost our meta data
				meta_data = NULL;
				meta_data_size = 0;
			}
		} else {
			meta_data = malloc(meta_data_size);
			if (meta_data) {
				memcpy(meta_data, clone.meta_data, meta_data_size);
			} else {
				// whoops, we just lost our meta data
				meta_data_size = 0;
			}
		}
	}
	return *this;
}


// #pragma mark -


bool
operator==(const media_raw_audio_format& a, const media_raw_audio_format& b)
{
	return a.frame_rate == b.frame_rate
		&& a.channel_count == b.channel_count
		&& a.format == b.format
		&& a.byte_order == b.byte_order
		&& a.buffer_size == b.buffer_size;
}


bool
operator==(const media_multi_audio_info& a, const media_multi_audio_info& b)
{
	return a.channel_mask == b.channel_mask
		&& a.valid_bits == b.valid_bits
		&& a.matrix_mask == b.matrix_mask;
}


bool
operator==(const media_multi_audio_format& a,
	const media_multi_audio_format& b)
{
	return (media_raw_audio_format)a == (media_raw_audio_format)b
		&& (media_multi_audio_info)a == (media_multi_audio_info)b;
}


bool
operator==(const media_encoded_audio_format& a,
	const media_encoded_audio_format& b)
{
	return a.output == b.output
		&& a.encoding == b.encoding
		&& a.bit_rate == b.bit_rate
		&& a.frame_size == b.frame_size
		&& a.multi_info == b.multi_info;
}


bool
operator==(const media_video_display_info& a,
	const media_video_display_info& b)
{
	return a.format == b.format
		&& a.line_width == b.line_width
		&& a.line_count == b.line_count
		&& a.bytes_per_row == b.bytes_per_row
		&& a.pixel_offset == b.pixel_offset
		&& a.line_offset == b.line_offset
		&& a.flags == b.flags;
}


bool
operator==(const media_raw_video_format& a, const media_raw_video_format& b)
{
	return a.field_rate == b.field_rate
		&& a.interlace == b.interlace
		&& a.first_active == b.first_active
		&& a.last_active == b.last_active
		&& a.orientation == b.orientation
		&& a.pixel_width_aspect == b.pixel_width_aspect
		&& a.pixel_height_aspect == b.pixel_height_aspect
		&& a.display == b.display;
}


bool
operator==(const media_encoded_video_format& a,
	const media_encoded_video_format& b)
{
	return a.output == b.output
		&& a.avg_bit_rate == b.avg_bit_rate
		&& a.max_bit_rate == b.max_bit_rate
		&& a.encoding == b.encoding
		&& a.frame_size == b.frame_size
		&& a.forward_history == b.forward_history
		&& a.backward_history == b.backward_history;
}


bool
operator==(const media_multistream_format::vid_info& a,
	const media_multistream_format::vid_info& b)
{
	return a.frame_rate == b.frame_rate
		&& a.width == b.width
		&& a.height == b.height
		&& a.space == b.space
		&& a.sampling_rate == b.sampling_rate
		&& a.sample_format == b.sample_format
		&& a.byte_order == b.byte_order
		&& a.channel_count == b.channel_count;
}


bool
operator==(const media_multistream_format::avi_info& a,
	const media_multistream_format::avi_info& b)
{
	return a.us_per_frame == b.us_per_frame
		&& a.width == b.width
		&& a.height == b.height
		&& a.type_count == b.type_count
		&& a.types[0] == b.types[0]
		&& a.types[1] == b.types[1]
		&& a.types[2] == b.types[2]
		&& a.types[3] == b.types[3]
		&& a.types[4] == b.types[4];
}


bool
operator==(const media_multistream_format& a,
	const media_multistream_format& b)
{
	if (a.avg_bit_rate != b.avg_bit_rate
		|| a.max_bit_rate != b.max_bit_rate
		|| a.avg_chunk_size != b.avg_chunk_size
		|| a.max_chunk_size != b.max_chunk_size
		|| a.format != b.format
		|| a.flags != b.flags) {
		return false;
	}

	switch (a.format) {
		case media_multistream_format::B_VID:
			return a.u.vid == b.u.vid;

		case media_multistream_format::B_AVI:
			return a.u.avi == b.u.avi;

		default:
			return true; // TODO: really?
	}
}


bool
operator==(const media_format& a, const media_format& b)
{
	if (a.type != b.type
		|| a.user_data_type != b.user_data_type
		// TODO: compare user_data[48] ?
		|| a.require_flags != b.require_flags
		|| a.deny_flags != b.deny_flags) {
		return false;
	}

	switch (a.type) {
		case B_MEDIA_RAW_AUDIO:
			return a.u.raw_audio == b.u.raw_audio;

		case B_MEDIA_RAW_VIDEO:
			return a.u.raw_video == b.u.raw_video;

		case B_MEDIA_MULTISTREAM:
			return a.u.multistream == b.u.multistream;

		case B_MEDIA_ENCODED_AUDIO:
			return a.u.encoded_audio == b.u.encoded_audio;

		case B_MEDIA_ENCODED_VIDEO:
			return a.u.encoded_video == b.u.encoded_video;

		default:
			return true; // TODO: really?
	}
}


// #pragma mark -


/*! return \c true if a and b are compatible (accounting for wildcards)
	a is the format you want to feed to something accepting b
*/
bool
format_is_compatible(const media_format& a, const media_format& b)
{
	return a.Matches(&b);
}


bool
string_for_format(const media_format& f, char* buf, size_t size)
{
	char encoding[10]; /* maybe Be wanted to use some 4CCs ? */
	const char* videoOrientation = "0"; /* I'd use "NC", R5 uses 0. */

	if (buf == NULL)
		return false;
	switch (f.type) {
	case B_MEDIA_RAW_AUDIO:
		snprintf(buf, size,
			"raw_audio;%g;%" B_PRIu32 ";0x%" B_PRIx32 ";%" B_PRIu32 ";0x%"
				B_PRIxSIZE ";0x%#" B_PRIx32 ";%d;0x%04x",
			f.u.raw_audio.frame_rate,
			f.u.raw_audio.channel_count,
			f.u.raw_audio.format,
			f.u.raw_audio.byte_order,
			f.u.raw_audio.buffer_size,
			f.u.raw_audio.channel_mask,
			f.u.raw_audio.valid_bits,
			f.u.raw_audio.matrix_mask);
		return true;
	case B_MEDIA_RAW_VIDEO:
		if (f.u.raw_video.orientation == B_VIDEO_TOP_LEFT_RIGHT)
			videoOrientation = "TopLR";
		else if (f.u.raw_video.orientation == B_VIDEO_BOTTOM_LEFT_RIGHT)
			videoOrientation = "BotLR";
		snprintf(buf, size, "raw_video;%g;0x%x;%" B_PRIu32 ";%" B_PRIu32 ";%"
				B_PRIu32 ";%" B_PRIu32 ";%s;%d;%d",
			f.u.raw_video.field_rate,
			f.u.raw_video.display.format,
			f.u.raw_video.interlace,
			f.u.raw_video.display.line_width,
			f.u.raw_video.display.line_count,
			f.u.raw_video.first_active,
			videoOrientation,
			f.u.raw_video.pixel_width_aspect,
			f.u.raw_video.pixel_height_aspect);
		return true;
	case B_MEDIA_ENCODED_AUDIO:
		snprintf(encoding, 10, "%d", f.u.encoded_audio.encoding);
		snprintf(buf, size,
			"caudio;%s;%g;%ld;(%g;%" B_PRIu32 ";0x%" B_PRIx32 ";%" B_PRIu32
				";0x%" B_PRIxSIZE ";0x%08" B_PRIx32 ";%d;0x%04x)",
			encoding,
			f.u.encoded_audio.bit_rate,
			f.u.encoded_audio.frame_size,
			f.u.encoded_audio.output.frame_rate,
			f.u.encoded_audio.output.channel_count,
			f.u.encoded_audio.output.format,
			f.u.encoded_audio.output.byte_order,
			f.u.encoded_audio.output.buffer_size,
			f.u.encoded_audio.multi_info.channel_mask,
			f.u.encoded_audio.multi_info.valid_bits,
			f.u.encoded_audio.multi_info.matrix_mask);
		return true;
	case B_MEDIA_ENCODED_VIDEO:
		snprintf(encoding, 10, "%d", f.u.encoded_video.encoding);
		if (f.u.encoded_video.output.orientation == B_VIDEO_TOP_LEFT_RIGHT)
			videoOrientation = "TopLR";
		else if (f.u.encoded_video.output.orientation == B_VIDEO_BOTTOM_LEFT_RIGHT)
			videoOrientation = "BotLR";
		snprintf(buf, size,
			"cvideo;%s;%g;%g;%" B_PRIuSIZE ";(%g;0x%x;%" B_PRIu32 ";%" B_PRIu32
				";%" B_PRIu32 ";%" B_PRIu32 ";%s;%d;%d)",
			encoding,
			f.u.encoded_video.avg_bit_rate,
			f.u.encoded_video.max_bit_rate,
			f.u.encoded_video.frame_size,
			f.u.encoded_video.output.field_rate,
			f.u.encoded_video.output.display.format,
			f.u.encoded_video.output.interlace,
			f.u.encoded_video.output.display.line_width,
			f.u.encoded_video.output.display.line_count,
			f.u.encoded_video.output.first_active,
			videoOrientation,
			f.u.encoded_video.output.pixel_width_aspect,
			f.u.encoded_video.output.pixel_height_aspect);
		return true;
	default:
		snprintf(buf, size, "%d-", f.type);
		unsigned char* p = (unsigned char*)&(f.u);
		size -= strlen(buf);
		buf += strlen(buf);
		for (int i = 0; (size > 2) && (i < 96); i++) {
			snprintf(buf, 3, "%2.2x", *(p + i));
			buf+=2;
			size-=2;
		}
		return true; // ?
	}
	return false;
}


// #pragma mark -


bool
operator==(const media_file_format_id& a, const media_file_format_id& b)
{
	return a.node == b.node && a.device == b.device
		&& a.internal_id == b.internal_id;
}


bool
operator<(const media_file_format_id& a, const media_file_format_id& b)
{
	return a.internal_id < b.internal_id;
}


// #pragma mark -


//! Use this function to iterate through available file format writers.
status_t
get_next_file_format(int32* cookie, media_file_format* mff)
{
	if (cookie == NULL || mff == NULL)
		return B_BAD_VALUE;

	status_t ret = AddOnManager::GetInstance()->GetFileFormat(mff, *cookie);
	if (ret != B_OK)
		return ret;

	*cookie = *cookie + 1;

	return B_OK;
}


// #pragma mark -


// final & verified
const char* B_MEDIA_SERVER_SIGNATURE = "application/x-vnd.Be.media-server";
const char* B_MEDIA_ADDON_SERVER_SIGNATURE = "application/x-vnd.Be.addon-host";

const type_code B_CODEC_TYPE_INFO = 0x040807b2;


// #pragma mark -


// shutdown_media_server() and launch_media_server()
// are provided by libbe.so in BeOS R5

#define MEDIA_SERVICE_NOTIFICATION_ID "MediaServiceNotificationID"


void
notify_system(float progress, const char* message)
{
	BNotification notification(B_PROGRESS_NOTIFICATION);
	notification.SetMessageID(MEDIA_SERVICE_NOTIFICATION_ID);
	notification.SetProgress(progress);
	notification.SetGroup(B_TRANSLATE("Media Service"));
	notification.SetContent(message);

	app_info info;
	be_app->GetAppInfo(&info);
	BBitmap icon(BRect(0, 0, 32, 32), B_RGBA32);
	BNode node(&info.ref);
	BIconUtils::GetVectorIcon(&node, "BEOS:ICON", &icon);
	notification.SetIcon(&icon);

	notification.Send();
}


void
progress_shutdown(int stage,
	bool (*progress)(int stage, const char* message, void* cookie),
	void* cookie)
{
	// parameter "message" is no longer used. It is kept for compatibility with
	// BeOS as this is used as a shutdown_media_server callback.

	TRACE("stage: %i\n", stage);
	const char* string = "Unknown stage";
	switch (stage) {
		case 10:
			string = B_TRANSLATE("Stopping media server" B_UTF8_ELLIPSIS);
			break;
		case 20:
			string = B_TRANSLATE("Waiting for media_server to quit.");
			break;
		case 40:
			string = B_TRANSLATE("Telling media_addon_server to quit.");
			break;
		case 50:
			string = B_TRANSLATE("Waiting for media_addon_server to quit.");
			break;
		case 70:
			string = B_TRANSLATE("Cleaning up.");
			break;
		case 100:
			string = B_TRANSLATE("Done shutting down.");
			break;
	}

	if (progress == NULL)
		notify_system(stage / 100.0f, string);
	else
		progress(stage, string, cookie);
}


status_t
shutdown_media_server(bigtime_t timeout,
	bool (*progress)(int stage, const char* message, void* cookie),
	void* cookie)
{
	BLaunchRoster launchRoster;
	launchRoster.StopTarget(B_MEDIA_SERVER_SIGNATURE);

	BMessage msg(B_QUIT_REQUESTED);
	status_t err = B_MEDIA_SYSTEM_FAILURE;
	bool shutdown = false;

	BMediaRoster* roster = BMediaRoster::Roster(&err);
	if (roster == NULL || err != B_OK)
		return err;

	if (progress == NULL && roster->Lock()) {
		MediaRosterEx(roster)->EnableLaunchNotification(true, true);
		roster->Unlock();
	}

	if ((err = msg.AddBool("be:_user_request", true)) != B_OK)
		return err;

	team_id mediaServer = be_roster->TeamFor(B_MEDIA_SERVER_SIGNATURE);
	team_id addOnServer = be_roster->TeamFor(B_MEDIA_ADDON_SERVER_SIGNATURE);

	if (mediaServer != B_ERROR) {
		BMessage reply;
		BMessenger messenger(B_MEDIA_SERVER_SIGNATURE, mediaServer);
		progress_shutdown(10, progress, cookie);

		err = messenger.SendMessage(&msg, &reply, 2000000, 2000000);
		reply.FindBool("_shutdown", &shutdown);
		if (err == B_TIMED_OUT || shutdown == false) {
			if (messenger.IsValid())
				kill_team(mediaServer);
		} else if (err != B_OK)
			return err;

		progress_shutdown(20, progress, cookie);

		int32 rv;
		if (reply.FindInt32("error", &rv) == B_OK && rv != B_OK)
			return rv;
	}

	if (addOnServer != B_ERROR) {
		shutdown = false;
		BMessage reply;
		BMessenger messenger(B_MEDIA_ADDON_SERVER_SIGNATURE, addOnServer);
		progress_shutdown(40, progress, cookie);

		// The media_server usually shutdown the media_addon_server,
		// if not let's do something.
		if (messenger.IsValid()) {
			err = messenger.SendMessage(&msg, &reply, 2000000, 2000000);
			reply.FindBool("_shutdown", &shutdown);
			if (err == B_TIMED_OUT || shutdown == false) {
				if (messenger.IsValid())
					kill_team(addOnServer);
			} else if (err != B_OK)
				return err;

			progress_shutdown(50, progress, cookie);

			int32 rv;
			if (reply.FindInt32("error", &rv) == B_OK && rv != B_OK)
				return rv;
		}
	}

	progress_shutdown(100, progress, cookie);
	return B_OK;
}


void
progress_startup(int stage,
	bool (*progress)(int stage, const char* message, void* cookie),
	void* cookie)
{
	// parameter "message" is no longer used. It is kept for compatibility with
	// BeOS as this is used as a shutdown_media_server callback.

	TRACE("stage: %i\n", stage);
	const char* string = "Unknown stage";
	switch (stage) {
		case 10:
			string = B_TRANSLATE("Stopping media server" B_UTF8_ELLIPSIS);
			break;
		case 20:
			string = B_TRANSLATE("Stopping media_addon_server.");
			break;
		case 50:
			string = B_TRANSLATE("Starting media_services.");
			break;
		case 90:
			string = B_TRANSLATE("Error occurred starting media services.");
			break;
		case 100:
			string = B_TRANSLATE("Ready for use.");
			break;
	}

	if (progress == NULL)
		notify_system(stage / 100.0f, string);
	else
		progress(stage, string, cookie);
}


status_t
launch_media_server(bigtime_t timeout,
	bool (*progress)(int stage, const char* message, void* cookie),
	void* cookie, uint32 flags)
{
	if (BMediaRoster::IsRunning())
		return B_ALREADY_RUNNING;

	status_t err = B_MEDIA_SYSTEM_FAILURE;
	BMediaRoster* roster = BMediaRoster::Roster(&err);
	if (roster == NULL || err != B_OK)
		return err;

	if (progress == NULL && roster->Lock()) {
		MediaRosterEx(roster)->EnableLaunchNotification(true, true);
		roster->Unlock();
	}

	// The media_server crashed
	if (be_roster->IsRunning(B_MEDIA_ADDON_SERVER_SIGNATURE)) {
		progress_startup(10, progress, cookie);
		kill_team(be_roster->TeamFor(B_MEDIA_ADDON_SERVER_SIGNATURE));
	}

	// The media_addon_server crashed
	if (be_roster->IsRunning(B_MEDIA_SERVER_SIGNATURE)) {
		progress_startup(20, progress, cookie);
		kill_team(be_roster->TeamFor(B_MEDIA_SERVER_SIGNATURE));
	}

	progress_startup(50, progress, cookie);

	err = BLaunchRoster().Start(B_MEDIA_SERVER_SIGNATURE);

	if (err != B_OK)
		progress_startup(90, progress, cookie);
	else if (progress != NULL) {
		progress_startup(100, progress, cookie);
		err = B_OK;
	}

	return err;
}


// #pragma mark - media_encode_info


media_encode_info::media_encode_info()
{
	flags = 0;
	used_data_size = 0;
	start_time = 0;
	time_to_encode = INT64_MAX;
	file_format_data = NULL;
	file_format_data_size = 0;
	codec_data = NULL;
	codec_data_size = 0;
}


media_decode_info::media_decode_info()
{
	time_to_decode = INT64_MAX;
	file_format_data = NULL;
	file_format_data_size = 0;
	codec_data = NULL;
	codec_data_size = 0;
}


