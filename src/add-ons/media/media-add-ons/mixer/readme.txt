OpenBeOS Audio Mixer
David Shipman, 2002

Overview

The node is based on a mediaeventlooper, using the HandleEvent loop to compile
the mixed buffer output. 
Each input creates a ringbuffer (fixed size "looped" buffer) for its input - this way
buffer contents can be placed in the ringbuffer and recycled immediately.

Inputs are maintained using a list of input_channel structs - inputs are created/
destroyed dynamically when producers connect/disconnect.

Done

- node functions as a replacement for the BeOS R5 mixer.media-addon
- IO/conversion between stereo B_AUDIO_FLOAT and B_AUDIO_SHORT streams

Tested

- Output to emu10k1 (SBLive)
- Input from CL-Amp and emu10k1-in

Bugs

- Soundplay, extractor nodes, and a variety of others connect, but sound really bad!
- When input stops, the current ringbuffer contents continue to be played

To Do (lots) (vaguely in order of importance)

- fix bugs
- support for remaining formats
- samplerate conversion
- Interface (BControllable)
- multithreading (separate mix thread)
- tidy up code

Notes :

Parts of this program are based on code under the Be Sample Code License.
This is included in the archive, in accordance with the license.