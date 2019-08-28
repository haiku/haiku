#include <Application.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <OS.h>

#define USE_NEW_PLAYER 0

#if USE_NEW_PLAYER
  #define NAMESPACE	BExperimental::
  #include "NewSoundPlayer.h"
#else
  #define NAMESPACE
  #include <SoundPlayer.h>
#endif

#define FILENAME "/boot/home/test1.wav"
#define SIZE	2048

port_id port = -1;
sem_id finished = -1;
int fd = -1;
NAMESPACE BSoundPlayer *sp = 0;

void PlayBuffer(void *cookie, void * buffer, size_t size, const media_raw_audio_format & format)
{
	size_t portsize = port_buffer_size(port);
	int32 code;
	
	read_port(port, &code, buffer, portsize);

	if (size != portsize) {
		sp->SetHasData(false);
		release_sem(finished);
	}
}

int32 filereader(void *arg)
{
	char buffer[SIZE];
	int size;
	
	printf("file reader started\n");
	
	for (;;) {
		size = read(fd, buffer, SIZE);
		write_port(port, 0, buffer, size);
		if (size != SIZE)
			break;
	}

	write_port(port, 0, buffer, 0);

	printf("file reader finished\n");

	return 0;
}

int main(int argc, char *argv[])
{
	fd = open((argc > 1) ? argv[1] : FILENAME, O_RDONLY);
	if (fd < 0)
		return -1;
		
	lseek(fd, 44, SEEK_SET); // skip wav header

	new BApplication("application/x-vnd.playfile");
	finished = create_sem(0, "finish wait");
	port = create_port(64, "buffer");
	
	media_raw_audio_format format;
	format = media_raw_audio_format::wildcard;
	format.frame_rate = 44100;
	format.channel_count = 2;
	format.format = media_raw_audio_format::B_AUDIO_SHORT;
	format.byte_order = B_MEDIA_LITTLE_ENDIAN;
	format.buffer_size = 2048;

	printf("spawning reader thread...\n");
	
	resume_thread(spawn_thread(filereader, "filereader", 8, 0));
	
	snooze(1000000);

	printf("playing soundfile...\n");
	
	sp = new NAMESPACE BSoundPlayer(&format, "sound player test", PlayBuffer);

	sp->SetHasData(true);
	sp->Start();

	sp->SetVolume(1.0f);

	// wait for playback end
	acquire_sem(finished);

	printf("\nplayback finished\n");

	delete sp;
	
	close(fd);
}
