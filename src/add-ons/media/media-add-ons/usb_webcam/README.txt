USB Webcam driver
(c) 2004-2008 François Revol.
Parts (c) Be,Inc. (ProducerNode sample code).

Current version of my USB Webcam driver. WORK IN PROGRESS!
Uses the USB Kit (userland API, needs libusb) to publish a
media node representing the webcam.

It currently only works with my Sonix webcam (3Euro cheapo
cam, using an SN9C120 chip), but is modular enough to easily 
expand it, some code is already there to detect Quickcams.

Note however that most webcams only support isochronous 
transfers, so will NOT work in R5 or even Zeta as isochronous 
is mostly not working. That's the reason for only supporting
my Sonix webcam as it is bulk capable.
As soon as iso support is added to the USB stack and the 
USB Kit it should be possible to support other webcams quite 
easily.

For now you should be able to build it under Zeta with the
makefile provided.

There are 3 kinds of device-specific folders :
addons/ contains actual usb chip support code for each device.
sensors/ contains code to handle CMOS sensors, as each model 
	of a specific brand usually have a different CMOS chip 
	despite a common usb chip.
cstransforms/ for colorspace transforms so other device using 
	the same weird colorspace can reuse the code (not yet 
	used, sonix has its own code for now). It should be 
	possible to use Translator-based transforms, making 
	it easy to support webcams sending JPEG pictures. 
	Another option is to turn all cstransforms into actual
	Translators usable by other apps, or also media codecs 
	but that would be more work for few added value.

Other notes and TODO:
- finish the bayer cstransform and use that instead of copied 
(MIT) code in Sonix addon.
- implement handling picture sizes correctly (currently forced 
in the ProducerNode and the Sonix code to 320x240)
- there are currently 2 Deframer classes, the StreamingDeframer 
although more complex seems to work much better than the 
BufferingDeframer. Make my mind about them.
- write isochronous code when USB Kit supports it
- add quickcam support (I have some code around) (requires iso)
- add code to support Fuji FinePix to merge the FinePix addon 
from bebits ? (need to find one to test)
- design an extensible API to publish possible controls as 
ParameterWeb or forward ParameterWeb changes right to the 
device-specific addon and provide default handlers for usual 
controls ?

References and other drivers of interest:

* Logitech opensource effort:
http://www.quickcamteam.net/

* Sonix linux drivers (several of them):
http://sourceforge.net/projects/sonix/ -- http://sonix.sourceforge.net/
http://freshmeat.net/projects/sonic-snap/?branch_id=55324&release_id=183982
http://tgnard.free.fr/linux/
(datasheet)
http://www.mnementh.co.uk/sonix/sn9c102.pdf

* some of the (many!) linux quickcam drivers:
http://www.lrr.in.tum.de/~acher/quickcam/quickcam.html
http://www.seismo.ethz.ch/linux/webcam.html

* NW80x based:
http://nw802.cvs.sourceforge.net NW80x based (like the QuickCam I have here)
http://tuukkat.awardspace.com/quickcam/quickcam.html for PID 0xd001
http://blognux.free.fr/sources/EasyCam2/04032006_11:11/drivers/nw802/
datasheets:
http://www.digchip.com/datasheets/parts/datasheet/132/NW800.php

* Creative's own list of linux drivers:
http://connect.creativelabs.com/opensource/Lists/Webcam%20Support/AllItems.aspx

* Other webcam drivers:
http://zc0302.sourceforge.net/zc0302.php?page=cams
http://www.smcc.demon.nl/webcam/ (philips)
http://www.medias.ne.jp/~takam/bsd/NetBSD.html
http://blognux.free.fr/sources/EasyCam2/04032006_19:49/
http://www.wifi.com.ar/english/doc/webcam/ov511cameras.html
http://mxhaard.free.fr/spca5xx.html

http://lkml.indiana.edu/hypermail/linux/kernel/0904.0/03427.html

* CMOS Sensor datasheets (rather, marketing buzz):
http://mxhaard.free.fr/spca50x/Doc/ many
http://www.tascorp.com.tw/product_file/TAS5110C1B_Brief_V0.3.pdf
http://www.tascorp.com.tw/product_file/TAS5130D1B_Brief_V0.3.pdf
http://www.mnementh.co.uk/sonix/hv7131e1.pdf
Divio NW80x:
http://www.digchip.com/datasheets/parts/datasheet/132/NW800.php
http://www.digchip.com/datasheets/parts/datasheet/132/NW802.php
http://web.archive.org/web/*/divio.com/*
All from eTOMS (ET31X110 would be == NW800 but isn't there):
http://www.etomscorp.com/english/webdesign/product_search.asp
http://web.archive.org/web/*re_pd_sr_1nr_50/http://etomscorp.com/*
Agilent HDCS:
http://www.ortodoxism.ro/datasheets2/2/05jj45dcrga6zr0zjg7hrde83cpy.pdf

* Linux USB stack:
http://www.iglu.org.il/lxr/source/include/linux/usb.h

* Linux V4L webcam list:
http://linuxtv.org/v4lwiki/index.php/Webcams

* Linux source code crossref:
http://lxr.linux.no/linux

* Fuji FinePix BeOS driver, should probably be merged at some point:
http://bebits.com/app/4185

* Macam generic OSX webcam driver (interesting but sadly GPL and in ObjC):
http://webcam-osx.sourceforge.net/index.html
http://webcam-osx.sourceforge.net/cameras/index.php list of supported cams in OSX
http://sourceforge.net/projects/webcam-osx
