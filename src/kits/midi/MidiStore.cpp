//-----------------------------------------------------------------------------
//
#include <MidiStore.h>
#include <Entry.h>
#include <Path.h>
#include <iostream.h>
#include <fstream.h>
#include <string.h>
#include "MidiEvent.h"

#define PACK_4_U8_TO_U32(_d1, _d2) \
	_d2 = (uint32)_d1[0] << 24 | (uint32)_d1[1] << 16 | \
		(uint32)_d1[2] << 8 | (uint32)_d1[3];
#define PACK_2_U8_TO_U16(_d1, _d2) \
	_d2 = (uint16)_d1[0] << 8 | (uint16)_d1[1];

//-----------------------------------------------------------------------------
// Public Access Routines
BMidiStore::BMidiStore() {
	_evt_list = new BList();
	_tempo = 60;
	_cur_evt = 0;
}

BMidiStore::~BMidiStore() {
	int32 num_items = _evt_list->CountItems();
	for(int i = num_items - 1; i >= 0; i--) {
		delete (BMidiEvent *)_evt_list->RemoveItem(i);
	}
	_evt_list->MakeEmpty();
	delete _evt_list;
}

void BMidiStore::NoteOff(uchar chan, uchar note, uchar vel, uint32 time) {	
	BMidiEvent * e = new BMidiEvent;
	e->opcode = BMidiEvent::OP_NOTE_OFF;
	e->time = B_NOW;
	e->data.note_off.channel = chan;
	e->data.note_off.note = note;
	e->data.note_off.velocity = vel;
	_evt_list->AddItem(e);
}
    	                 
void BMidiStore::NoteOn(uchar chan, uchar note, uchar vel, uint32 time) {
	BMidiEvent * e = new BMidiEvent;
	e->opcode = BMidiEvent::OP_NOTE_ON;
	e->time = B_NOW;
	e->data.note_on.channel = chan;
	e->data.note_on.note = note;
	e->data.note_on.velocity = vel;
	_evt_list->AddItem(e);
}

void BMidiStore::KeyPressure(uchar chan, uchar note, uchar pres,
	uint32 time) {
	BMidiEvent * e = new BMidiEvent;
	e->opcode = BMidiEvent::OP_KEY_PRESSURE;
	e->time = B_NOW;
	e->data.key_pressure.channel = chan;
	e->data.key_pressure.note = note;
	e->data.key_pressure.pressure = pres;
	_evt_list->AddItem(e);
}

void BMidiStore::ControlChange(uchar chan, uchar ctrl_num, uchar ctrl_val,
	uint32 time) {
	BMidiEvent * e = new BMidiEvent;
	e->opcode = BMidiEvent::OP_CONTROL_CHANGE;
	e->time = B_NOW;
	e->data.control_change.channel = chan;
	e->data.control_change.number = ctrl_num;
	e->data.control_change.value = ctrl_val;
	_evt_list->AddItem(e);
}
                               
void BMidiStore::ProgramChange(uchar chan, uchar prog_num, uint32 time) {
	BMidiEvent * e = new BMidiEvent;
	e->opcode = BMidiEvent::OP_PROGRAM_CHANGE;
	e->time = B_NOW;
	e->data.program_change.channel = chan;
	e->data.program_change.number = prog_num;
	_evt_list->AddItem(e);
}

void BMidiStore::ChannelPressure(uchar chan, uchar pres, uint32 time) {                                
	BMidiEvent * e = new BMidiEvent;
	e->opcode = BMidiEvent::OP_CHANNEL_PRESSURE;
	e->time = B_NOW;
	e->data.channel_pressure.channel = chan;
	e->data.channel_pressure.pressure = pres;
	_evt_list->AddItem(e);
}                                 
                                 
void BMidiStore::PitchBend(uchar chan, uchar lsb, uchar msb, uint32 time) {
	BMidiEvent * e = new BMidiEvent;
	e->opcode = BMidiEvent::OP_PITCH_BEND;
	e->time = B_NOW;
	e->data.pitch_bend.channel = chan;
	e->data.pitch_bend.lsb = lsb;
	e->data.pitch_bend.msb = msb;
	_evt_list->AddItem(e);
}

void BMidiStore::SystemExclusive(void * data, size_t data_len, uint32 time) {
	BMidiEvent * e = new BMidiEvent;
	e->opcode = BMidiEvent::OP_SYSTEM_EXCLUSIVE;
	e->time = B_NOW;
	e->data.system_exclusive.data = (uint8 *)data;
	e->data.system_exclusive.length = data_len;
	_evt_list->AddItem(e);
}

void BMidiStore::SystemCommon(uchar stat_byte, uchar data1, uchar data2,
	uint32 time) {
	BMidiEvent * e = new BMidiEvent;
	e->opcode = BMidiEvent::OP_SYSTEM_COMMON;
	e->time = B_NOW;
	e->data.system_common.status = stat_byte;
	e->data.system_common.data1 = data1;
	e->data.system_common.data2 = data2;
	_evt_list->AddItem(e);
}

void BMidiStore::SystemRealTime(uchar stat_byte, uint32 time) {
	BMidiEvent * e = new BMidiEvent;
	e->opcode = BMidiEvent::OP_SYSTEM_REAL_TIME;
	e->time = B_NOW;
	e->data.system_real_time.status = stat_byte;
	_evt_list->AddItem(e);
}

void BMidiStore::TempoChange(int32 bpm, uint32 time) {
	BMidiEvent * e = new BMidiEvent;
	e->opcode = BMidiEvent::OP_TEMPO_CHANGE;
	e->time = B_NOW;
	e->data.tempo_change.beats_per_minute = bpm;
	_evt_list->AddItem(e);
}

void BMidiStore::AllNotesOff(bool just_chan, uint32 time) {
	BMidiEvent * e = new BMidiEvent;
	e->opcode = BMidiEvent::OP_ALL_NOTES_OFF;
	e->time = B_NOW;
	e->data.all_notes_off.just_channel = just_chan;
	_evt_list->AddItem(e);
}

status_t BMidiStore::Import(const entry_ref * ref) {
	BEntry file_entry(ref,true);
	BPath file_path;
	file_entry.GetPath(&file_path);
	ifstream inf(file_path.Path(), ios::in | ios::binary);
	if(!inf) {
		return B_ERROR;
	}
	inf.seekg(0,ios::end);
	uint32 file_len = inf.tellg();
	if(file_len < 16) {
		return B_BAD_MIDI_DATA;
	}
	inf.seekg(ios::beg);
	uint8 * data = new uint8[file_len];
	if(data == NULL) {
		return B_ERROR;
	}
	inf.read(data,file_len);
	inf.close();
	uint8 * d = data;
	if(strncmp((const char *)d,"MThd",4) != 0) {
		return B_BAD_MIDI_DATA;
	}
	d += 4;
	uint32 hdr_len;
	PACK_4_U8_TO_U32(d,hdr_len);
	if(hdr_len != 6) {
		return B_BAD_MIDI_DATA;
	}
	d += 4;
	uint32 format;
	uint32 tracks;
	uint32 division;
	PACK_2_U8_TO_U16(d,format);
	d += 2;
	PACK_2_U8_TO_U16(d,tracks);
	d += 2;
	PACK_2_U8_TO_U16(d,division);
	if(!(division & 0x8000)) {
		_ticks_per_beat = division;
	}
	d += 2;
	try {
		switch(format) {
		case 0:
			_DecodeFormat0Tracks(d,tracks,file_len-(data-d));
			break;
		case 1:
			_DecodeFormat1Tracks(d,tracks,file_len-(data-d));
			break;
		case 2:
			_DecodeFormat2Tracks(d,tracks,file_len-(data-d));
			break;
		default:
			return B_BAD_MIDI_DATA;
		}
	} catch(status_t err) {
		return err;
	}
	return B_OK;
}

status_t BMidiStore::Export(const entry_ref * ref, int32 format) {
	ofstream ouf(ref->name, ios::out | ios::binary);
	if(!ouf) {
		return B_ERROR;
	}
	ouf.write("MHdr",4);
	
	ouf.close();
	return B_OK;
}

void BMidiStore::SortEvents(bool force) {
	_evt_list->SortItems(_CompareEvents);
}

uint32 BMidiStore::CountEvents() const {
	return _evt_list->CountItems();
}

uint32 BMidiStore::CurrentEvent() const {
	return _cur_evt;
}

void BMidiStore::SetCurrentEvent(uint32 event_num) {
	_cur_evt = event_num;
}

uint32 BMidiStore::DeltaOfEvent(uint32 event_num) const {
	BMidiEvent * e = (BMidiEvent *)_evt_list->ItemAt(event_num);
	return e->time - _start_time;
}

uint32 BMidiStore::EventAtDelta(uint32 time) const {
	uint32 event_num = 0;
	
	return event_num;
}

uint32 BMidiStore::BeginTime() const {
	return _start_time;
}

void BMidiStore::SetTempo(int32 bpm) {
	_tempo = bpm;
}

int32 BMidiStore::Tempo() const {
	return _tempo;
}

void BMidiStore::Run() {
	_start_time = B_NOW;
	uint32 last_tick = 0;
	uint32 last_time = _start_time;
	while(KeepRunning()) {
		BMidiEvent * e = (BMidiEvent *)_evt_list->ItemAt(_cur_evt);
		if(e == NULL) {
			return;
		}
		uint32 tick = e->time;
		uint32 tick_delta = tick - last_tick;
		uint32 beat_len = (60000 / _tempo);
		uint32 delta_time = (beat_len * tick_delta) / _ticks_per_beat;
		uint32 time = last_time + delta_time;
		last_time = time;
		last_tick = tick;
		BMidiEvent::Data & d = e->data;
		cerr << "event 0x" << hex << e->opcode << " time = " << time << endl;
		switch(e->opcode) {
		case BMidiEvent::OP_NOTE_OFF:
			SprayNoteOff(d.note_off.channel,
				d.note_off.note, d.note_off.velocity,time);
			break;
		case BMidiEvent::OP_NOTE_ON:
			SprayNoteOn(d.note_on.channel,
				d.note_on.note,d.note_on.velocity,time);
			break;
		case BMidiEvent::OP_KEY_PRESSURE:
			SprayKeyPressure(d.key_pressure.channel,
				d.key_pressure.note,d.key_pressure.pressure,time);
			break;
		case BMidiEvent::OP_CONTROL_CHANGE:
			SprayControlChange(d.control_change.channel,
				d.control_change.number,d.control_change.value,time);
			break;
		case BMidiEvent::OP_PROGRAM_CHANGE:
			SprayProgramChange(d.program_change.channel,
				d.program_change.number,time);
			break;
		case BMidiEvent::OP_CHANNEL_PRESSURE:
			SprayChannelPressure(d.channel_pressure.channel,
				d.channel_pressure.pressure,time);
			break;
		case BMidiEvent::OP_PITCH_BEND:
			SprayPitchBend(d.pitch_bend.channel,
				d.pitch_bend.lsb,d.pitch_bend.msb,time);
			break;
		case BMidiEvent::OP_SYSTEM_EXCLUSIVE:
			SpraySystemExclusive(d.system_exclusive.data,
				d.system_exclusive.length,time);
			break;
		case BMidiEvent::OP_SYSTEM_COMMON:
			SpraySystemCommon(d.system_common.status,
				d.system_common.data1,d.system_common.data2,time);
			break;
		case BMidiEvent::OP_SYSTEM_REAL_TIME:
			SpraySystemRealTime(d.system_real_time.status,time);
			break;
		case BMidiEvent::OP_TEMPO_CHANGE:
			SprayTempoChange(d.tempo_change.beats_per_minute,time);
			_tempo = d.tempo_change.beats_per_minute;
			break;
		case BMidiEvent::OP_ALL_NOTES_OFF:
			break;
		default:
			break;
		}
		_cur_evt++;
	}
}

//-----------------------------------------------------------------------------
// Decode and Encode Routines
uint8* BMidiStore::_DecodeTrack(uint8* d) {
	if(strncmp((const char *)d,"MTrk",4) != 0) {
		throw B_BAD_MIDI_DATA;
	}
//		cerr << "Track " << (track+1) << " offset = " << (unsigned long)(d-data) << "\n";
	d += 4;
	uint32 trk_len;
	PACK_4_U8_TO_U32(d,trk_len);
	d += 4;
	uint32 ticks            = 0;
	uint8* track_last_byte = d + trk_len;
	uint8* next_chunk      = track_last_byte;
	BMidiEvent* event      = NULL;
	uint8 status           = 0;
	try {
		while(d < track_last_byte) {
			uint32 evt_dticks = _ReadVarLength(&d, track_last_byte);
			ticks += evt_dticks;
			event = new BMidiEvent();
			if ((d[0] & 0x80) != 0) { // running status
				status = d[0]; d ++;
			}
			_ReadEvent(status, &d, track_last_byte, event);
			event->time = ticks; // TODO: convert ticks to milliseconds
			if(event->opcode == BMidiEvent::OP_TRACK_END) {
				delete event; event = NULL;
				break;
			}
			if (event->opcode != BMidiEvent::OP_NONE) {
				_evt_list->AddItem(event);
			} else { // skip unknown event
				delete event; event = NULL;
			}
		}
	} catch(status_t e) {
		delete event;
		throw e;
	}
	return next_chunk;
}


void BMidiStore::_DecodeFormat0Tracks(uint8 * data, uint16 tracks,
	uint32 len) {
	_DecodeTrack(data);
}

void BMidiStore::_DecodeFormat1Tracks(uint8 * data, uint16 tracks,
	uint32 len) {
	if(tracks == 0) {
		throw B_BAD_MIDI_DATA;
	}
	// simply merge all tracks into one and sort the events afterwards
	uint8 * next_track = data;
	for (int track = 0; track < tracks; track ++) {
		next_track = _DecodeTrack(next_track);
	}
	SortEvents();
}

void BMidiStore::_DecodeFormat2Tracks(uint8 * data, uint16 tracks,
	uint32 len) {	
	return;
}

void BMidiStore::_EncodeFormat0Tracks(uint8 *) {
}

void BMidiStore::_EncodeFormat1Tracks(uint8 *) {
}

void BMidiStore::_EncodeFormat2Tracks(uint8 *) {
}

void BMidiStore::_ReadEvent(uint8 status, uint8 ** data, uint8 * max_d, BMidiEvent * event) {
#define CHECK_DATA(_d) if((_d) > max_d) throw B_BAD_MIDI_DATA;
#define INC_DATA(_d) CHECK_DATA(_d); d = _d;
	uint8 * d = *data;
	if(d == NULL) {
		throw B_BAD_MIDI_DATA;
	}
//	cerr << "ReadEvent 0x" << hex << (int)status << "\n";
	if(status == 0xff) {
		uint32 len;
		if(d[0] == 0x0) {
			INC_DATA(d+1);
			if(d[0] == 0x00) {
				// Sequence number!
				INC_DATA(d+1);
			} else if(d[0] == 0x02) {
				// Sequence number!
				INC_DATA(d+3);
			} else {
				throw B_BAD_MIDI_DATA;
			}
		} else if(d[0] > 0x00 && d[0] < 0x0a) {
			INC_DATA(d+1);
			len = _ReadVarLength(&d,max_d);
			INC_DATA(d+len);
		} else if(d[0] == 0x2f) {
			INC_DATA(d+1);
			if(d[0] == 0x00) {
				INC_DATA(d+1);
				event->opcode = BMidiEvent::OP_TRACK_END;
			} else {
				throw B_BAD_MIDI_DATA;
			}
		} else if(d[0] == 0x51) {
			INC_DATA(d+1);
			if(d[0] == 0x03) {
				event->opcode = BMidiEvent::OP_TEMPO_CHANGE;
				INC_DATA(d+1);
				CHECK_DATA(d+2);
				uint32 mspb = (uint32)d[0] << 16 |
					(uint32)d[1] << 8 | (uint32)d[2];
				uint32 bpm = 60000000 / mspb;
				event->data.tempo_change.beats_per_minute = bpm;
				INC_DATA(d+3);
			} else {
				throw B_BAD_MIDI_DATA;
			}
		} else if(d[0] == 0x54) {
			INC_DATA(d+1);
			if(d[0] == 0x05) {
				INC_DATA(d+1);
				// SMPTE start time.
				// hour = d[0];
				// minute = d[1];
				// second = d[2];
				// frame = d[3];
				// fraction = d[4];
				INC_DATA(d+5);
			} else {
				throw B_BAD_MIDI_DATA;
			}
		} else if(d[0] == 0x58) {
			INC_DATA(d+1);
			if(d[0] == 0x04) {
				INC_DATA(d+1);
				// Time signature.
				// numerator = d[0];
				// denominator = d[1];
				// midi clocks = d[2];
				// 32nd notes in a quarter = d[3];
				INC_DATA(d+4);
			} else {
				throw B_BAD_MIDI_DATA;
			}
		} else if(d[0] == 0x59) {
			INC_DATA(d+1);
			if(d[0] == 0x02) {
				INC_DATA(d+1);
				// Key signature.
				// sf = d[0];
				// minor = d[1];
				INC_DATA(d+2);
			} else {
				throw B_BAD_MIDI_DATA;
			}
		} else if(d[0] == 0x7f) {
			INC_DATA(d+1);
			uint32 len = _ReadVarLength(&d,max_d);
			INC_DATA(d+len);
		} else if(d[0] == 0x20) {
			INC_DATA(d+1);
			if(d[0] == 0x01) {
				INC_DATA(d+2);
			} else {
				throw B_BAD_MIDI_DATA;
			}
		} else if(d[0] == 0x21) {
			INC_DATA(d+1);
			if(d[0] == 0x01) {
				INC_DATA(d+2);
			}
		} else {
			throw B_BAD_MIDI_DATA;
		}
	} else if(status == 0xf0) {
		// Manufacturer ID = d[0];
		while(d[0] != 0xf7) {
			// Read data eliding the msb.
			INC_DATA(d+1);
		}
	} else if((status & 0xf0) == 0x80) {
		event->opcode = BMidiEvent::OP_NOTE_OFF;
		event->data.note_off.channel = status & 0x0f;
		event->data.note_off.note = d[0];
		INC_DATA(d+1);
		event->data.note_off.velocity = d[0];
		INC_DATA(d+1);
	} else if((status & 0xf0) == 0x90) {
		event->opcode = BMidiEvent::OP_NOTE_ON;
		event->data.note_on.channel = status & 0x0f;
		event->data.note_on.note = d[0];
		INC_DATA(d+1);
		event->data.note_on.velocity = d[0];
		INC_DATA(d+1);
	} else if((status & 0xf0) == 0xa0) {
		event->opcode = BMidiEvent::OP_KEY_PRESSURE;
		event->data.key_pressure.channel = status & 0x0f;
		event->data.key_pressure.note = d[0];
		INC_DATA(d+1);
		event->data.key_pressure.pressure = d[0];
		INC_DATA(d+1);
	} else if((status & 0xf0) == 0xb0) {
		event->opcode = BMidiEvent::OP_CONTROL_CHANGE;
		event->data.control_change.channel = status & 0x0f;
		event->data.control_change.number = d[0];
		INC_DATA(d+1);
		event->data.control_change.value = d[0];
		INC_DATA(d+1);
	} else if((status & 0xf0) == 0xc0) {
		event->opcode = BMidiEvent::OP_PROGRAM_CHANGE;
		event->data.program_change.channel = status & 0x0f;
		event->data.program_change.number = d[0];
		INC_DATA(d+1);
	} else if((status & 0xf0) == 0xd0) {
		event->opcode = BMidiEvent::OP_CHANNEL_PRESSURE;
		event->data.channel_pressure.channel = status & 0x0f;
		event->data.channel_pressure.pressure = d[0];
		INC_DATA(d+1);
	} else if((status & 0xf0) == 0xe0) {
		event->opcode = BMidiEvent::OP_PITCH_BEND;
		event->data.pitch_bend.channel = d[0] & 0x0f;
		event->data.pitch_bend.lsb = d[0];
		INC_DATA(d+1);
		event->data.pitch_bend.msb = d[0];
		INC_DATA(d+1);
	} else {
		cerr << "Unsupported Code:0x" << hex << (int)status << endl;
		throw B_BAD_MIDI_DATA;
	}
	*data = d;
#undef INC_DATA
#undef CHECK_DATA
}

void BMidiStore::_WriteEvent(BMidiEvent * e) {
}

uint32 BMidiStore::_ReadVarLength(uint8 ** data, uint8 * max_d) {
	uint32 val;
	uint8 bytes = 1;
	uint8 byte = 0;
	uint8 * d = *data;
	val = *d;
	d++;
	if(val & 0x80) {
		val &= 0x7f;
		do {
			if(d > max_d) {
				throw B_BAD_MIDI_DATA;
			}
			byte = *d;
			val = (val << 7) + byte & 0x7f;
			bytes++;
			d++;
		} while(byte & 0x80);
	}
	*data = d;
	return val;
}


void BMidiStore::_WriteVarLength(uint32 length) {
}

int BMidiStore::_CompareEvents(const void * e1, const void *e2) {
	BMidiEvent * evt1 = (BMidiEvent *)e1;
	BMidiEvent * evt2 = (BMidiEvent *)e2;
	return evt1->time - evt2->time;
}
