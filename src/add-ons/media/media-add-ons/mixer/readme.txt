OpenBeOS Audio Mixer
David Shipman, 14/08/2002

Overview

The node is based on a mediaeventlooper, using the HandleEvent loop to compile the mixed buffer output. 
Each input creates a ringbuffer (fixed size "looped" buffer) for its input - this way buffer contents can be 
placed in the ringbuffer and the buffer recycled immediately.

Inputs are maintained using a list of mixer_input objects - inputs are created/destroyed dynamically when 
producers connect/disconnect.

Done

- node functions as a replacement for the BeOS R5 mixer.media-addon
- IO/conversion between the major audio types (FLOAT, SHORT)
- interface mostly done (all gain controls operational)
- mixing with different gain levels is functional

Tested

- Output to emu10k1 (SBLive)
- Input from CL-Amp, file-readers, SoundPlay, music software - almost all work well

To Do (vaguely in order of importance)

- fix bugs
- format negotation fine-tuning - some nodes still connect with weird formats
- rewrite of buffer mixing routines - at the moment its really inefficient, and a total mess
- complete interface (add mutes, panning)
- multithreading (separate mix thread)
- mixer save (to save parameter states when a node is disconnected)

Notes :

Parts of this program are based on code under the Be Sample Code License.
This is included in the archive, in accordance with the license.