/*
 * Copyright 2004-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, jerome.duval@free.fr
 *		Marcus Overhagen, marcus@overhagen.de
 *		Jérôme Lévêque, leveque.jerome@gmail.com
 */


#ifndef _ICE1712_MULTI_H_
#define _ICE1712_MULTI_H_

#include "ice1712.h"

#include <hmulti_audio.h>

#define ICE1712_MULTI_CONTROL_FIRSTID		(0x80000000)

#define ICE1712_MULTI_CONTROL_TYPE_MASK		(0x70000000)
#define ICE1712_MULTI_CONTROL_TYPE_COMBO	(0x10000000)
#define ICE1712_MULTI_CONTROL_TYPE_VOLUME	(0x20000000)
#define ICE1712_MULTI_CONTROL_TYPE_OUTPUT	(0x30000000)
#define ICE1712_MULTI_CONTROL_TYPE_OTHER4	(0x40000000)
#define ICE1712_MULTI_CONTROL_TYPE_OTHER5	(0x50000000)
#define ICE1712_MULTI_CONTROL_TYPE_OTHER6	(0x60000000)
#define ICE1712_MULTI_CONTROL_TYPE_OTHER7	(0x70000000)

#define ICE1712_MULTI_CONTROL_CHANNEL_MASK	(0x0FF00000)
#define ICE1712_MULTI_CONTROL_INDEX_MASK	(0x00000FFF)

#define ICE1712_MULTI_SET_CHANNEL(_c_)		((_c_ << 20) & \
	ICE1712_MULTI_CONTROL_CHANNEL_MASK)
#define ICE1712_MULTI_GET_CHANNEL(_c_)		((_c_ & \
	ICE1712_MULTI_CONTROL_CHANNEL_MASK) >> 20)

#define ICE1712_MULTI_SET_INDEX(_i_)		(_i_ & \
	ICE1712_MULTI_CONTROL_INDEX_MASK)
#define ICE1712_MULTI_GET_INDEX(_i_)		(_i_ & \
	ICE1712_MULTI_CONTROL_INDEX_MASK)

/*
#define ICE1712_MULTI_CONTROL_VOLUME_PB		(0x00010000)
#define ICE1712_MULTI_CONTROL_VOLUME_REC	(0x00020000)
#define ICE1712_MULTI_CONTROL_MUTE			(0x00040000)
#define ICE1712_MULTI_CONTROL_MUX			(0x00080000)
#define ICE1712_MULTI_CONTROL_MASK			(0x00FF0000)
#define ICE1712_MULTI_CONTROL_CHANNEL_MASK	(0x000000FF)
*/

#define CONTROL_IS_MASTER (0)

status_t ice1712Get_Description(ice1712 *card, multi_description *data);
status_t ice1712Get_Channel(ice1712 *card, multi_channel_enable *data);
status_t ice1712Set_Channel(ice1712 *card, multi_channel_enable *data);
status_t ice1712Get_Format(ice1712 *card, multi_format_info *data);
status_t ice1712Set_Format(ice1712 *card, multi_format_info *data);
status_t ice1712Get_MixValue(ice1712 *card, multi_mix_value_info *data);
status_t ice1712Set_MixValue(ice1712 *card, multi_mix_value_info *data);
status_t ice1712Get_MixValueChannel(ice1712 *card,
	multi_mix_channel_info *data);
status_t ice1712Get_MixValueControls(ice1712 *card,
	multi_mix_control_info *data);
status_t ice1712Get_MixValueConnections(ice1712 *card,
	multi_mix_connection_info *data);
status_t ice1712Buffer_Get(ice1712 *card, multi_buffer_list *data);
status_t ice1712Buffer_Exchange(ice1712 *card, multi_buffer_info *data);
status_t ice1712Buffer_Stop(ice1712 *card);

#endif //_ICE1712_MULTI_H_

