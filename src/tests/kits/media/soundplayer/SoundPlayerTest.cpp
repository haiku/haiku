#include <Application.h>
#include <SoundPlayer.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "OS.h"

#define NAMESPACE
//#define NAMESPACE	BExperimental::

#define FILENAME "/boot/home/test1.wav"

sem_id finished = -1;
int fd = -1;
NAMESPACE BSoundPlayer *sp = 0;

inline float abs(float f) { return f < 0 ? -f : f; }

void PlayBuffer(void *cookie, void * buffer, size_t size, const media_raw_audio_format & format)
{
	if (size != (size_t)read(fd, buffer, size)) {
		sp->SetHasData(false);
		release_sem(finished);
	}
}

int main(int argc, char *argv[])
{
	fd = open((argc > 1) ? argv[1] : FILENAME, O_RDONLY);
	if (fd < 0)
		return -1;
		
	lseek(fd, 44, SEEK_SET); // skip wav header

	new BApplication("application/x-vnd.SoundPlayTest");
	finished = create_sem(0, "finish wait");
	
	media_raw_audio_format format;
	format = media_raw_audio_format::wildcard;
	format.frame_rate = 44100;
	format.channel_count = 2;
	format.format = media_raw_audio_format::B_AUDIO_SHORT;
	format.byte_order = B_MEDIA_LITTLE_ENDIAN;
	format.buffer_size = 4 * 4096;

	
	sp = new NAMESPACE BSoundPlayer(&format, "sound player test", PlayBuffer);

	printf("playing soundfile\n");
	sp->SetHasData(true);
	sp->Start();

	printf("volume now %.2f\n", sp->Volume());
	printf("volumeDB now %.2f\n", sp->VolumeDB());

	float f;
	media_node out_node;
	int32 out_parameter;
	float out_min_dB;
	float out_max_dB;

	sp->GetVolumeInfo(&out_node, &out_parameter, &out_min_dB, &out_max_dB);

	printf("out_min_dB %.2f\n", out_min_dB);
	printf("out_max_dB %.2f\n", out_max_dB);

	/* Initial volume is not unrelyable! sometimes 0, 0.4, 1.0 and 3.2 have been observed */
	sp->SetVolume(1.0f);

	printf("waiting 5 seconds\n");
	snooze(5000000);

	printf("skipping 10MB now\n");
	lseek(fd, 10000000, SEEK_CUR); // skip 10 MB

	printf("waiting 3 seconds");
	snooze(3000000);

	do {
		printf("\nDoing %% ramping...");
		printf("\nramping down\n");
		for (f = 1.0f; f >= 0.0; f -= ((f < 0.08) ? 0.005 : 0.05)) {
			sp->SetVolume(f);
			printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bvolume now %.2f, should be %.2f", sp->Volume(), f);
			fflush(stdout);
			snooze(250000);
		}
	
		if (B_OK == acquire_sem_etc(finished,1, B_TIMEOUT, 0))
			break;
	
		printf("\nramping up\n");
		for (f = 0; f <= 1.0f; f += ((f < 0.08) ? 0.005 : 0.05)) {
			sp->SetVolume(f);
			printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bvolume now %.2f, should be %.2f", sp->Volume(), f);
			fflush(stdout);
			snooze(250000);
		}

		if (B_OK == acquire_sem_etc(finished,1, B_TIMEOUT, 0))
			break;

		printf("\nDoing DB ramping...");

		printf("\nramping down\n");
		for (f = out_max_dB; f >= out_min_dB; f -= abs(out_max_dB - out_min_dB) / 50) {
			sp->SetVolumeDB(f);
			printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bvolume now %6.2f, should be %6.2f", sp->VolumeDB(), f);
			fflush(stdout);
			snooze(250000);
		}

		if (B_OK == acquire_sem_etc(finished,1, B_TIMEOUT, 0))
			break;

		printf("\nramping up\n");
		for (f = out_min_dB; f <= out_max_dB; f += abs(out_max_dB - out_min_dB) / 50) {
			sp->SetVolumeDB(f);
			printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bvolume now %+6.2f, should be %6.2f", sp->VolumeDB(), f);
			fflush(stdout);
			snooze(250000);
		}
	} while (B_OK != acquire_sem_etc(finished,1, B_TIMEOUT, 0));

	printf("\nplayback finished\n");

	delete sp;
	
	close(fd);
}
