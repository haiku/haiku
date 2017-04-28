/*
 * playsound - command line sound file player for Haiku
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <FileGameSound.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>

volatile bool interrupted = false;

static void
keyb_int(int)
{
	interrupted = true;
}


static void
usage()
{
	fprintf(stderr, "playsound - command line sound file player for Haiku\n");
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  playsound [--loop] [--gain <value>] [--pan <value>] <filename>\n");
	fprintf(stderr, "  gain value in percent, can be 0 (silence) to 100 (normal) or higher\n");
	fprintf(stderr, "  pan value in percent, can be -100 (left) to 100 (right)\n");
}


int
main(int argc, char *argv[])
{
	const char *file;
	bool loop = false;
	int gain = 100;
	int pan = 0;
	
	while (1) { 
		int c;
		int option_index = 0; 
		static struct option long_options[] = 
		{ 
			{"loop", no_argument, 0, 'l'}, 
			{"gain", required_argument, 0, 'g'}, 
			{"pan", required_argument, 0, 'p'}, 
			{0, 0, 0, 0} 
		}; 
		
		c = getopt_long (argc, argv, "lgp:", long_options, &option_index); 
		if (c == -1) 
			break; 

		switch (c) { 
			case 'l': 
				loop = true;
		 		break; 

			case 'g':
				gain = atoi(optarg);
				break; 

			case 'p':
				pan = atoi(optarg);
				break; 

			default:
				usage();
				return 1;
		}
	}

	if (optind < argc) {
		file = argv[optind];
	} else {
		usage();
		return 1;
	}
	
	// test if file exists
	int fd = open(file, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Can't open file %s\n", file);
		return 1;
	}
	close(fd);

	signal(SIGINT, keyb_int);

	BFileGameSound snd(file, loop);
	status_t err;
	
	err = snd.InitCheck();
	if (err < B_OK) {
		fprintf(stderr, "Init failed, error 0x%08lx (%s)\n", err, strerror(err));
		return 1;
	}
	
	err = snd.SetGain(gain / 100.0);
	if (err < B_OK) {
		fprintf(stderr, "Setting gain failed, error 0x%08lx (%s)\n", err, strerror(err));
		return 1;
	}

	err = snd.SetPan(pan / 100.0);
	if (err < B_OK) {
		fprintf(stderr, "Setting pan failed, error 0x%08lx (%s)\n", err, strerror(err));
		return 1;
	}

	err = snd.Preload();
	if (err < B_OK) {
		fprintf(stderr, "Preload failed, error 0x%08lx (%s)\n", err, strerror(err));
		return 1;
	}

	err = snd.StartPlaying();
	if (err < B_OK) {
		fprintf(stderr, "Start playing failed, error 0x%08lx (%s)\n", err, strerror(err));
		return 1;
	}
	
	while (snd.IsPlaying() && !interrupted)
		snooze(50000);

	err = snd.StopPlaying();
	if (err < B_OK) {
		fprintf(stderr, "Stop playing failed, error 0x%08lx (%s)\n", err, strerror(err));
		return 1;
	}

	return 0;
}
