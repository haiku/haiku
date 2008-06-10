OpenSound media addon for BeOS and Haiku.

Copyright (c) 2007, François Revol (revol@free.fr)
Distributed under the terms of the MIT License.

OpenSound System v4.0 drivers (c) 4Front Technologies.
Port by François Revol (revol@free.fr)
Released under BSD licence. Sources available by mail.


Release and dev notes

This is a Media Kit add-on that integrates OpenSound System v4.0 drivers into the native BeOS and Haiku media framework. It publishes a media node that can be used as default system audio output and input. Each OSS-supported card has its own media node. One or more input are published on the node per playback engine on the card, same for outputs on recording engines. The first in/output is for raw audio format, next ones being for other custom encoded formats the card might support (AC3, MPEG audio layer 2, Vorbis...), if enabled. The node publishes a parameter web (used for the settings view) reflecting the mixer layout as reported by OSS.

Bugs and non-features

* Current version should support playback and recording. The list of supported devices is the same as the list of supported devices under Linux <http://manuals.opensound.com/devlists/Linux.html> except USB devices.

* The drivers and media node addon should work in R5, dano/Zeta, and Haiku without trouble. On Haiku be sure to have a kernel newer than 2007/08/02, previous versions had a bug in open_module_list() preventing the drivers to be found.

* support for non-raw (AC3, MPEG...) formats is not finished (MediaNode assumes raw in some places). 

* minimal mixer support is implemented.
* "radio-like" checkboxes don't correctly sync (rec source select...)
* only the first mixer device is used for a card, but I don't know any card that uses more than 1 mixer (maybe pro cards ?) :)

* currently all OSS drivers are built into the same binary add-ons/kernel/media/oss that is opened by the oss_loader, for simplicity, as they use a lot of external symbols from the oss core module. 




