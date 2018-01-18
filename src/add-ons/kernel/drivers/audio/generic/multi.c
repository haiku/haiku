/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2007-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


static status_t
multi_audio_control_generic(cookie_type* cookie, uint32 op, void* arg, size_t len)
{
	status_t status;
	switch (op) {
		case B_MULTI_GET_DESCRIPTION:
		{
			multi_description description;
			multi_channel_info channels[16];
			multi_channel_info* originalChannels;

			if (user_memcpy(&description, arg, sizeof(multi_description))
					!= B_OK)
				return B_BAD_ADDRESS;

			originalChannels = description.channels;
			description.channels = channels;
			if (description.request_channel_count > 16)
				description.request_channel_count = 16;

			status = get_description(cookie, &description);
			if (status != B_OK)
				return status;

			description.channels = originalChannels;
			if (user_memcpy(arg, &description, sizeof(multi_description))
					!= B_OK)
				return B_BAD_ADDRESS;
			return user_memcpy(originalChannels, channels,
				sizeof(multi_channel_info) * description.request_channel_count);
		}

		case B_MULTI_GET_ENABLED_CHANNELS:
		{
			multi_channel_enable* data = (multi_channel_enable*)arg;
			multi_channel_enable enable;
			uint32 enable_bits;
			uchar* orig_enable_bits;

			if (user_memcpy(&enable, data, sizeof(enable)) != B_OK
				|| !IS_USER_ADDRESS(enable.enable_bits)) {
				return B_BAD_ADDRESS;
			}

			orig_enable_bits = enable.enable_bits;
			enable.enable_bits = (uchar*)&enable_bits;
			status = get_enabled_channels(cookie, &enable);
			if (status != B_OK)
				return status;

			enable.enable_bits = orig_enable_bits;
			if (user_memcpy(enable.enable_bits, &enable_bits,
					sizeof(enable_bits)) < B_OK
				|| user_memcpy(arg, &enable, sizeof(multi_channel_enable)) < B_OK) {
				return B_BAD_ADDRESS;
			}

			return B_OK;
		}
		case B_MULTI_SET_ENABLED_CHANNELS:
			return B_OK;

		case B_MULTI_GET_GLOBAL_FORMAT:
		{
			multi_format_info info;
			if (user_memcpy(&info, arg, sizeof(multi_format_info)) != B_OK)
				return B_BAD_ADDRESS;

			status = get_global_format(cookie, &info);
			if (status != B_OK)
				return B_OK;
			return user_memcpy(arg, &info, sizeof(multi_format_info));
		}
		case B_MULTI_SET_GLOBAL_FORMAT:
		{
			multi_format_info info;
			if (user_memcpy(&info, arg, sizeof(multi_format_info)) != B_OK)
				return B_BAD_ADDRESS;

			status = set_global_format(cookie, &info);
			if (status != B_OK)
				return B_OK;
			return user_memcpy(arg, &info, sizeof(multi_format_info));
		}
		case B_MULTI_LIST_MIX_CHANNELS:
			return list_mix_channels(cookie, (multi_mix_channel_info*)arg);
		case B_MULTI_LIST_MIX_CONTROLS:
		{
			multi_mix_control_info info;
			multi_mix_control* original_controls;
			size_t allocSize;
			multi_mix_control *controls;

			if (user_memcpy(&info, arg, sizeof(multi_mix_control_info)) != B_OK)
				return B_BAD_ADDRESS;

			original_controls = info.controls;
			allocSize = sizeof(multi_mix_control) * info.control_count;
			controls = (multi_mix_control *)malloc(allocSize);
			if (controls == NULL)
				return B_NO_MEMORY;

			if (!IS_USER_ADDRESS(info.controls)
				|| user_memcpy(controls, info.controls, allocSize) < B_OK) {
				free(controls);
				return B_BAD_ADDRESS;
			}
			info.controls = controls;

			status = list_mix_controls(cookie, &info);
			if (status != B_OK) {
				free(controls);
				return status;
			}

			info.controls = original_controls;
			status = user_memcpy(info.controls, controls, allocSize);
			if (status == B_OK)
				status = user_memcpy(arg, &info, sizeof(multi_mix_control_info));
			if (status != B_OK)
				status = B_BAD_ADDRESS;
			free(controls);
			return status;
		}
		case B_MULTI_LIST_MIX_CONNECTIONS:
			return list_mix_connections(cookie,
				(multi_mix_connection_info*)arg);
		case B_MULTI_GET_MIX:
		{
			multi_mix_value_info info;
			multi_mix_value* original_values;
			size_t allocSize;
			multi_mix_value *values;

			if (user_memcpy(&info, arg, sizeof(multi_mix_value_info)) != B_OK)
				return B_BAD_ADDRESS;

			original_values = info.values;
			allocSize = sizeof(multi_mix_value) * info.item_count;
			values = (multi_mix_value *)malloc(allocSize);
			if (values == NULL)
				return B_NO_MEMORY;

			if (!IS_USER_ADDRESS(info.values)
				|| user_memcpy(values, info.values, allocSize) < B_OK) {
				free(values);
				return B_BAD_ADDRESS;
			}
			info.values = values;

			status = get_mix(cookie, &info);
			if (status != B_OK) {
				free(values);
				return status;
			}

			info.values = original_values;
			status = user_memcpy(info.values, values, allocSize);
			if (status == B_OK)
				status = user_memcpy(arg, &info, sizeof(multi_mix_value_info));
			if (status != B_OK)
				status = B_BAD_ADDRESS;
			free(values);
			return status;
		}
		case B_MULTI_SET_MIX:
		{
			multi_mix_value_info info;
			multi_mix_value* original_values;
			size_t allocSize;
			multi_mix_value *values;

			if (user_memcpy(&info, arg, sizeof(multi_mix_value_info)) != B_OK)
				return B_BAD_ADDRESS;

			original_values = info.values;
			allocSize = sizeof(multi_mix_value) * info.item_count;
			values = (multi_mix_value *)malloc(allocSize);
			if (values == NULL)
				return B_NO_MEMORY;

			if (!IS_USER_ADDRESS(info.values)
				|| user_memcpy(values, info.values, allocSize) < B_OK) {
				free(values);
				return B_BAD_ADDRESS;
			}
			info.values = values;

			status = set_mix(cookie, &info);
			if (status != B_OK) {
				free(values);
				return status;
			}

			info.values = original_values;
			status = user_memcpy(info.values, values, allocSize);
			if (status == B_OK)
				status = user_memcpy(arg, &info, sizeof(multi_mix_value_info));
			if (status != B_OK)
				status = B_BAD_ADDRESS;
			free(values);
			return status;
		}
		case B_MULTI_GET_BUFFERS:
		{
			multi_buffer_list list;
			if (user_memcpy(&list, arg, sizeof(multi_buffer_list)) != B_OK)
				return B_BAD_ADDRESS;
			{
				buffer_desc **original_playback_descs = list.playback_buffers;
				buffer_desc **original_record_descs = list.record_buffers;

				buffer_desc *playback_descs[list.request_playback_buffers];
				buffer_desc *record_descs[list.request_record_buffers];

				if (!IS_USER_ADDRESS(list.playback_buffers)
					|| user_memcpy(playback_descs, list.playback_buffers,
						sizeof(buffer_desc*) * list.request_playback_buffers)
						< B_OK
					|| !IS_USER_ADDRESS(list.record_buffers)
					|| user_memcpy(record_descs, list.record_buffers,
						sizeof(buffer_desc*) * list.request_record_buffers)
						< B_OK) {
					return B_BAD_ADDRESS;
				}

				list.playback_buffers = playback_descs;
				list.record_buffers = record_descs;
				status = get_buffers(cookie, &list);
				if (status != B_OK)
					return status;

				list.playback_buffers = original_playback_descs;
				list.record_buffers = original_record_descs;

				if (user_memcpy(arg, &list, sizeof(multi_buffer_list)) < B_OK
					|| user_memcpy(original_playback_descs, playback_descs,
						sizeof(buffer_desc*) * list.request_playback_buffers)
						< B_OK
					|| user_memcpy(original_record_descs, record_descs,
						sizeof(buffer_desc*) * list.request_record_buffers)
						< B_OK) {
					status = B_BAD_ADDRESS;
				}
			}

			return status;
		}
		case B_MULTI_BUFFER_EXCHANGE:
			return buffer_exchange(cookie, (multi_buffer_info*)arg);
		case B_MULTI_BUFFER_FORCE_STOP:
			return buffer_force_stop(cookie);

		case B_MULTI_GET_EVENT_INFO:
		case B_MULTI_SET_EVENT_INFO:
		case B_MULTI_GET_EVENT:
		case B_MULTI_GET_CHANNEL_FORMATS:
		case B_MULTI_SET_CHANNEL_FORMATS:
		case B_MULTI_SET_BUFFERS:
		case B_MULTI_SET_START_TIME:
			return B_ERROR;
	}

	return B_BAD_VALUE;
}

