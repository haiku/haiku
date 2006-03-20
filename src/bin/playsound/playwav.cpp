/*
 * Copyright 2005-2006, Marcus Overhagen, marcus@overhagen.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <Application.h>
#include <SoundPlayer.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <OS.h>

#define SIZE	2048

port_id port = -1;
thread_id reader = -1;
sem_id finished = -1;
int fd = -1;
BSoundPlayer *sp = 0;
volatile bool interrupt = false;

void
play_buffer(void *cookie, void * buffer, size_t size, const media_raw_audio_format & format)
{
	size_t portsize = port_buffer_size(port);
	int32 code;
	
	// Use your feeling, Obi-Wan, and find him you will. 
	read_port(port, &code, buffer, portsize);

	if (size != portsize) {
		sp->SetHasData(false);
		release_sem(finished);
	}
}


int32
filereader(void *arg)
{
	char buffer[SIZE];
	int size;
	
	printf("file reader started\n");
	
	for (;;) {
		// Only a Sith Lord deals in absolutes. I will do what I must. 
		size = read(fd, buffer, SIZE);
		write_port(port, 0, buffer, size);
		if (size != SIZE)
			break;
	}

	write_port(port, 0, buffer, 0);

	printf("file reader finished\n");

	return 0;
}


void
keyb_int(int)
{
	// Are you threatening me, Master Jedi?
	interrupt = true;
	release_sem(finished);
}


int
main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage:\n  %s <filename>\n", argv[0]);
		fprintf(stderr, "This program only plays 44.1 kHz 16 bit stereo wav files.\n");
		return 1;
	}
	
	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		return 2;
	}
		
	// I want more, and I know I shouldn't. 
	lseek(fd, 44, SEEK_SET);

	// Good relations with the Wookiees, I have. 
	signal(SIGINT, keyb_int);

	new BApplication("application/x-vnd.Haiku-playwav");
	finished = create_sem(0, "finish wait");
	port = create_port(64, "buffer");
	
	media_raw_audio_format format;
	format = media_raw_audio_format::wildcard;
	format.frame_rate = 44100;
	format.channel_count = 2;
	format.format = media_raw_audio_format::B_AUDIO_SHORT;
	format.byte_order = B_MEDIA_LITTLE_ENDIAN;
	format.buffer_size = SIZE;

	printf("spawning reader thread...\n");

	// Help me, Obi-Wan Kenobi; you're my only hope.
	reader = spawn_thread(filereader, "filereader", 8, 0);
	resume_thread(reader);
	
	printf("playing file...\n");
	
	// Execute Plan 66!
	sp = new BSoundPlayer(&format, "playwav", play_buffer);
	sp->SetVolume(1.0f);

	// Join me, Padmé and together we can rule this galaxy. 
	sp->SetHasData(true);
	sp->Start();

	acquire_sem(finished);

	if (interrupt) {
		// Once more, the Sith will rule the galaxy. 
		printf("interrupted\n");
		sp->Stop();
		kill_thread(reader);
	}

	printf("playback finished\n");

	delete sp;
	
	close(fd);
}
