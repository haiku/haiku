#include <MediaDefs.h>
#include <stdio.h>
#include <string.h>

#include "MixerUtils.h"

void string_for_channel_mask(char *str, uint32 mask)
{
	str[0] = 0;
	if (mask == 0) {
		strcpy(str, "<none>");
		return;
	}
	#define DECODE(type, text)	if (mask & (type)) \
		 do { strcat(str, text); mask &= ~(type); if (mask != 0) strcat(str, ", "); } while (0)
	DECODE(B_CHANNEL_LEFT, "Left");
	DECODE(B_CHANNEL_RIGHT, "Right");
	DECODE(B_CHANNEL_CENTER, "Center");
	DECODE(B_CHANNEL_SUB, "Sub");
	DECODE(B_CHANNEL_REARLEFT, "Rear-Left");
	DECODE(B_CHANNEL_REARRIGHT, "Rear-Right");
	DECODE(B_CHANNEL_FRONT_LEFT_CENTER, "Front-Left-Center");
	DECODE(B_CHANNEL_FRONT_RIGHT_CENTER, "Front-Right-Center");
	DECODE(B_CHANNEL_BACK_CENTER, "Back-Center");
	DECODE(B_CHANNEL_SIDE_LEFT, "Side-Left");
	DECODE(B_CHANNEL_SIDE_RIGHT, "Side-Right");
	DECODE(B_CHANNEL_TOP_CENTER, "Top-Center");
	DECODE(B_CHANNEL_TOP_FRONT_LEFT, "Top-Front-Left");
	DECODE(B_CHANNEL_TOP_FRONT_CENTER, "Top-Front-Center");
	DECODE(B_CHANNEL_TOP_FRONT_RIGHT, "Top-Front-Right");
	DECODE(B_CHANNEL_TOP_BACK_LEFT, "Top-Back-Left");
	DECODE(B_CHANNEL_TOP_BACK_CENTER, "Top-Back-Center");
	DECODE(B_CHANNEL_TOP_BACK_RIGHT, "Top-Back-Right");
	#undef DECODE
	if (mask)
		sprintf(str + strlen(str), "0x%08X", mask);
}

int count_nonzero_bits(uint32 value)
{
	int count = 0;
	for (int i = 0; i < 32; i++)
		if (value & (1 << i))
			count++;
	return count;
}

void fix_multiaudio_format(media_multi_audio_format *format)
{
	if (format->format == media_raw_audio_format::B_AUDIO_INT) {
		if (format->valid_bits != 0 && (format->valid_bits < 16 || format->valid_bits >= 32))
			format->valid_bits = 0;
	}
	switch (format->channel_count) {
		case 0:
			format->channel_mask = 0;
			format->matrix_mask = 0;
			break;
		case 1:
			if (count_nonzero_bits(format->channel_mask) != 1) {
				format->channel_mask = B_CHANNEL_LEFT;
				format->matrix_mask = 0;
			}
			break;
		case 2:
			if (count_nonzero_bits(format->channel_mask) != 2) {
				format->channel_mask = B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
				format->matrix_mask = 0;
			}
			break;
		case 4:
			if (count_nonzero_bits(format->channel_mask) != 4) {
				format->channel_mask = B_CHANNEL_LEFT | B_CHANNEL_RIGHT | B_CHANNEL_REARLEFT | B_CHANNEL_REARRIGHT;
				format->matrix_mask = 0;
			}
			break;
		case 5:
			if (count_nonzero_bits(format->channel_mask) != 5) {
				format->channel_mask = B_CHANNEL_LEFT | B_CHANNEL_RIGHT | B_CHANNEL_REARLEFT | B_CHANNEL_REARRIGHT | B_CHANNEL_CENTER;
				format->matrix_mask = 0;
			}
			break;
		case 6:
			if (count_nonzero_bits(format->channel_mask) != 6) {
				format->channel_mask = B_CHANNEL_LEFT | B_CHANNEL_RIGHT | B_CHANNEL_REARLEFT | B_CHANNEL_REARRIGHT | B_CHANNEL_CENTER | B_CHANNEL_SUB;
				format->matrix_mask = 0;
			}
			break;
		case 7:
			if (count_nonzero_bits(format->channel_mask) != 7) {
				format->channel_mask = B_CHANNEL_LEFT | B_CHANNEL_RIGHT | B_CHANNEL_REARLEFT | B_CHANNEL_REARRIGHT | B_CHANNEL_CENTER | B_CHANNEL_SUB | B_CHANNEL_BACK_CENTER;
				format->matrix_mask = 0;
			}
			break;
	}
}
