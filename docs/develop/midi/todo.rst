Midi Kit TO DO List
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Communicating with device drivers.** The midi_server already has a
pretty good parser that turns an incoming stream of bytes into MIDI
messages. It uses read() to read a single byte at a time. However, the
midi_driver.h file lists a number of ioctl() opcodes that we are
currently not using. Should we? In addition, do we really need to spawn
a new thread for each device? The R5 midi_server doesn't appear to do
this. An optional feature is to implement "running status" for MIDI OUT
ports (i.e. when writing to the device driver). This would be pretty
simple to add.

**BMidiStore is slow.** Importing a Standard MIDI File of a few hundred
kilobytes takes too long for my taste. The one from R5 is at least twice
as fast. It is important to speed this up since BMidiStore is used by
BMidiSynthFile to play MIDI files. We don't want games to slow down too
much.

**MPU401 kernel module.** Greg Crain did a great job of writing this
module. Unfortunately, we only know how the v1 interface works; v2 is
not documented. What's worse, most Be R5 drivers use v2. Currently, the
module returns B_ERROR when a device is opened with v2. Is this going to
be a problem for us? It depends on whether we will be able to use the
closed-source Be drivers with our own kernel — if not, then we can
simply ignore v2.

**Watching /dev/midi for changes.** Whenever a new device appears in
/dev/midi, the midi_server must create and publish a new MidiProducer
and MidiConsumer for that device. When a device disappears, its
endpoints must be removed again. Philippe Houdoin suggested we use the
device_watcher for this, but R5 doesn't appear to do it that way. Either
it uses node monitoring or doesn't do this at all. Our midi_server
already has a DeviceWatcher class, but it only examines the entries from
/dev/midi when the server starts, not while the server is running.

**BMidiSynthFile::Fade()** Right now this simply calls Stop(). We could
set a flag in BMidiStore (which handles our playback), which would then
slowly reduce the volume and abort the loop after a few seconds. But we
need to have the softsynth in order to tune this properly.

**Must be_synth be deleted when the app quits?** I have not found a word
about this, nor a way to test what happens in R5. For example, the
BMidiSynth constructor creates a BSynth object (if none already
existed), but we cannot destroy be_synth from the BMidiSynth destructor
because it may still be used in other places in the code (BSynth is not
refcounted). We could add the following code to libmidi.so to clean up
properly, but I don't know if it is really necessary:

   ::

      namespace BPrivate
      {
          static struct BSynthKiller
          {
              ~BSynthKiller()
              {
                  delete be_synth;
              }
          }
          synth_killer;
      }

**midiparser kernel module.** midi_driver.h (from the Sonic Vibes driver
sample code) contains the definition of a "midiparser" kernel module.
This is a very simple module that makes it easy to recognize where MIDI
messages begin and end, but apparently doesn't tell you what they mean.
In R5, this module lives in /boot/beos/system/add-ons/kernel/media. Does
anyone use this module? Is it necessary for us to provide it?
Personally, I'd say foggeddaboutit.
