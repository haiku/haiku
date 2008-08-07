//------------------------------------------------------------------------------
//
//  EchoGals/Echo24 BeOS Driver for Echo audio cards
//
//	Copyright (c) 2005, Jérôme Duval
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.

#include "echo.h"
#include "debug.h"
#include "midi_driver.h"

static status_t midi_open(const char *name, uint32 flags, void **cookie);
static status_t midi_close(void *cookie);
static status_t midi_free(void *cookie);
static status_t midi_control(void *cookie, uint32 op, void *data, size_t len);
static status_t midi_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t midi_write(void *cookie, off_t pos, const void *data, size_t *len);


device_hooks midi_hooks = {
	&midi_open,
	&midi_close,
	&midi_free,
	&midi_control,
	&midi_read,
	&midi_write,
	NULL,		/* select */
	NULL,		/* deselect */
	NULL,		/* readv */
	NULL		/* writev */
};

static status_t
midi_open(const char* name, uint32 flags, void** cookie)
{
	int ix;
	
	LOG(("midi_open()\n"));

	*cookie = NULL;
	for (ix=0; ix<num_cards; ix++) {
		if (!strcmp(name, cards[ix].midi.name)) {
			break;
		}
	}
	if (ix >= num_cards) {
		LOG(("bad device\n"));
		return ENODEV;
	}

	*cookie = &cards[ix];
	atomic_add(&cards[ix].midi.count, 1);
	memset(&cards[ix].midi.context, 0, sizeof(cards[ix].midi.context));
	cards[ix].pEG->OpenMidiInput(&cards[ix].midi.context);
	return B_OK;
}


static status_t
midi_close(void* cookie)
{
	LOG(("midi_close()\n"));
	return B_OK;
}


static status_t
midi_free(void* cookie)
{
	echo_dev *card = (echo_dev *) cookie;
	
	LOG(("midi_free()\n"));
	
	card->pEG->CloseMidiInput(&card->midi.context);
	
	atomic_add(&card->midi.count, -1);
	LOG(("midi_free() done\n"));
	return B_OK;
}


static status_t
midi_control(void* cookie, uint32 iop, void* data, size_t len)
{
	LOG(("midi_control()\n"));
	
	return B_ERROR;
}


static status_t
midi_read(void* cookie, off_t pos, void* ptr, size_t* nread)
{
	echo_dev *card = (echo_dev *) cookie;
	ECHOSTATUS 		err;
	DWORD			midiData;
	LONGLONG 		timestamp;
	PBYTE			next = (PBYTE)ptr;
	
	LOG(("midi_read()\n"));
	
	if (acquire_sem(card->midi.midi_ready_sem) != B_OK)
		return B_ERROR;
	
	err = card->pEG->ReadMidiByte(&card->midi.context, midiData, timestamp);
	if (err == ECHOSTATUS_OK) {
		*nread = 1;
		*next = midiData;
		LOG(("midi_read() : 0x%lx\n", *next));
		return B_OK;
	}
	return B_ERROR;
}


static status_t
midi_write(void* cookie, off_t pos, const void* ptr, size_t* nwritten)
{
	echo_dev *card = (echo_dev *) cookie;
	ECHOSTATUS err;
	
	LOG(("midi_write()\n"));
	
	err = card->pEG->WriteMidi(*nwritten, (PBYTE)ptr, nwritten);
	return (err != ECHOSTATUS_OK) ? B_ERROR : B_OK;
}
