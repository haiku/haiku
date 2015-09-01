/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef MIDI_SETTINGS_PRIVATE_H_
#define MIDI_SETTINGS_PRIVATE_H_

#include <StorageDefs.h>
#include <SupportDefs.h>

namespace BPrivate {

struct midi_settings {
	char soundfont_file[B_FILE_NAME_LENGTH];
};

status_t read_midi_settings(struct midi_settings* settings);
status_t write_midi_settings(struct midi_settings settings);

};


#endif /* MIDI_SETTINGS_PRIVATE_H_ */
