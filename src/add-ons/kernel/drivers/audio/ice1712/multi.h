/*
 * ice1712 BeOS/Haiku Driver for VIA - VT1712 Multi Channel Audio Controller
 *
 * Copyright (c) 2002, Jerome Duval		(jerome.duval@free.fr)
 * Copyright (c) 2003, Marcus Overhagen	(marcus@overhagen.de)
 * Copyright (c) 2007, Jerome Leveque	(leveque.jerome@neuf.fr)
 *
 * All rights reserved
 * Distributed under the terms of the MIT license.
 */

#include "ice1712.h"

status_t ice1712_get_description(ice1712 *card, multi_description *data);
status_t ice1712_get_enabled_channels(ice1712 *card, multi_channel_enable *data);
status_t ice1712_set_enabled_channels(ice1712 *card, multi_channel_enable *data);
status_t ice1712_get_global_format(ice1712 *card, multi_format_info *data);
status_t ice1712_set_global_format(ice1712 *card, multi_format_info *data);
status_t ice1712_get_mix(ice1712 *card, multi_mix_value_info *data);
status_t ice1712_set_mix(ice1712 *card, multi_mix_value_info *data);
status_t ice1712_list_mix_channels(ice1712 *card, multi_mix_channel_info *data);
status_t ice1712_list_mix_controls(ice1712 *card, multi_mix_control_info *data);
status_t ice1712_list_mix_connections(ice1712 *card, multi_mix_connection_info *data);
status_t ice1712_get_buffers(ice1712 *card, multi_buffer_list *data);
status_t ice1712_buffer_exchange(ice1712 *card, multi_buffer_info *data);
status_t ice1712_buffer_force_stop(ice1712 *card);
