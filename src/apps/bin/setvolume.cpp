/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <MediaRoster.h>
#include <ParameterWeb.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


extern const char *__progname;
static const char *sProgramName = __progname;


int
main(int argc, char **argv)
{
	BApplication app("application/x-vnd.haiku.setvolume");

	BMediaRoster *roster = BMediaRoster::Roster();
	if (roster == NULL) {
		fprintf(stderr, "%s: media roster not available\n", sProgramName);
		return 1;
	}

	media_node mixer;
	status_t status = roster->GetAudioMixer(&mixer);
	if (status != B_OK) {
		fprintf(stderr, "%s: cannot get audio mixer: %s\n", sProgramName, strerror(status));
		return 1;
	}

	BParameterWeb *web;
	status = roster->GetParameterWebFor(mixer, &web);

	roster->ReleaseNode(mixer);
		// the web is all we need :-)

	if (status != B_OK) {
		fprintf(stderr, "%s: cannot get parameter web for audio mixer: %s\n",
			sProgramName, strerror(status));
		return 1;
	}

	BContinuousParameter *gain = NULL;

	BParameter *parameter;
	for (int32 index = 0; (parameter = web->ParameterAt(index)) != NULL; index++) {
		if (!strcmp(parameter->Kind(), B_MASTER_GAIN)) {
			gain = dynamic_cast<BContinuousParameter *>(parameter);
			break;
		}
	}

	if (gain == NULL) {
		fprintf(stderr, "%s: could not found master gain!\n", sProgramName);
		delete web;
		return 1;
	}

	float volume = 0.0;

	if (argc > 1) {
		char *end;
		volume = strtod(argv[1], &end);
		if (end == argv[1]) {
			fprintf(stderr, "usage: %s <volume>\n", sProgramName);
		} else {
			// make sure our parameter is in range
			if (volume > gain->MaxValue())
				volume = gain->MaxValue();
			else if (volume < gain->MinValue())
				volume = gain->MinValue();

			gain->SetValue(&volume, sizeof(volume), system_time());
		}
	}

	bigtime_t when;
	size_t size = sizeof(volume);
	gain->GetValue(&volume, &size, &when);

	printf("Current volume: %g (min = %g, max = %g)\n", volume, gain->MinValue(), gain->MaxValue());

	delete web;
	return 0;
}

