//! From BeBook examples.


#include <Application.h>
#include <Sound.h>
#include <SoundPlayer.h>


typedef struct cookie_record {
	float value;
	float direction;
} cookie_record;


void
BufferProc(void* _cookie, void* buffer, size_t size,
	const media_raw_audio_format& format)
{
	// We're going to be cheap and only work for floating-point audio

	if (format.format != media_raw_audio_format::B_AUDIO_FLOAT)
		return;

	// Now fill the buffer with sound!

	cookie_record* cookie = (cookie_record*)_cookie;
	uint32 channelCount = format.channel_count;
	size_t floatSize = size / 4;
	float* buf = (float*)buffer;

	for (size_t i = 0; i < floatSize; i += channelCount) {
		for (size_t j = 0; j < channelCount; j++) {
			buf[i + j] = cookie->value;
		}

		if (cookie->direction == 1.0 && cookie->value >= 1.0)
			cookie->direction = -1.0;
		else if (cookie->direction == -1.0 && cookie->value <= -1.0)
			cookie->direction = 1.0;

		cookie->value += cookie->direction * (1.0 / 64.0);
	}
}


int
main()
{
	BApplication app("application/dzwiek");

	cookie_record cookie;

	cookie.value = 0.0;
	cookie.direction = 1.0;

	BSoundPlayer player("wave_player", BufferProc, NULL, &cookie);
	player.Start();
	player.SetHasData(true);

	sleep(5);

	player.Stop();
}
