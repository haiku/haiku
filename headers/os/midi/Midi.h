//-----------------------------------------------------------------------------
#ifndef _MIDI_H_
#define _MIDI_H_

#include <MidiDefs.h>
#include <OS.h>

class BMidiEvent;
class BList;

class BMidi {
public:
	BMidi();
	virtual ~BMidi();

	virtual void NoteOff(uchar channel, uchar note,
		uchar velocity, uint32 time = B_NOW);
	virtual void NoteOn(uchar channel, uchar note,
		uchar velocity, uint32 time = B_NOW);
	virtual void KeyPressure(uchar channel, uchar note,
		uchar pressure, uint32 time = B_NOW);
	virtual void ControlChange(uchar channel, uchar control_number,
		uchar control_value, uint32 time = B_NOW);
	virtual void ProgramChange(uchar channel, uchar program_number,
		uint32 time = B_NOW);
	virtual void ChannelPressure(uchar channel, uchar pressure,
		uint32 time = B_NOW);
	virtual void PitchBend(uchar channel, uchar lsb,
		uchar msb, uint32 time = B_NOW);
	virtual void SystemExclusive(void * data, size_t data_length,
		uint32 time = B_NOW);
	virtual void SystemCommon(uchar status_byte, uchar data1,
		uchar data2, uint32 time = B_NOW);
	virtual void SystemRealTime(uchar status_byte, uint32 time = B_NOW);
	virtual void TempoChange(int32 beats_per_minute, uint32 time = B_NOW);
	virtual void AllNotesOff(bool just_channel = true, uint32 time = B_NOW);

	virtual status_t Start();
	virtual void Stop();
    
	bool IsRunning() const;
	void Connect(BMidi * to_object);
	void Disconnect(BMidi * from_object);
	bool IsConnected(BMidi * to_object) const;
	BList * Connections() const;
	void SnoozeUntil(uint32 time) const;

protected:
	void SprayNoteOff(uchar channel, uchar note,
		uchar velocity, uint32 time) const;
	void SprayNoteOn(uchar channel, uchar note,
		uchar velocity, uint32 time) const;
	void SprayKeyPressure(uchar channel, uchar note,
		uchar pressure, uint32 time) const;
	void SprayControlChange(uchar channel, uchar control_number,
		uchar control_value, uint32 time) const;
	void SprayProgramChange(uchar channel, uchar program_number,
		uint32 time) const;
	void SprayChannelPressure(uchar channel, uchar pressure,
		uint32 time) const;
	void SprayPitchBend(uchar channel, uchar lsb,
		uchar msb, uint32 time) const;
	void SpraySystemExclusive(void * data, size_t data_length,
		uint32 time) const;
	void SpraySystemCommon(uchar status_byte, uchar data1,
		uchar data2, uint32 time) const;
	void SpraySystemRealTime(uchar status_byte, uint32 time) const;
	void SprayTempoChange(int32 beats_per_minute, uint32 time) const;

	bool KeepRunning();

private:
	virtual void Run();
	void _Inflow();
	void _SprayEvent(BMidiEvent *) const;
	static int32 _RunThread(void * data);
	static int32 _InflowThread(void * data);

private:
	BList * _con_list;
	thread_id _run_thread_id;
	thread_id _inflow_thread_id;
	port_id _inflow_port;
	bool _is_running;
	bool _is_inflowing;
	bool _keep_running;
};

#endif //_MIDI_H_
