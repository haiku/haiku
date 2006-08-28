USB Webcam driver
(c) 2004,2005,2006 François Revol.

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

Making a Jamfile might get tricky as several source files are
created by the makefile itself to include addons and censors
in the build.

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

References:

* Sonix linux drivers (several of them):
http://sourceforge.net/projects/sonix/ -- http://sonix.sourceforge.net/
http://freshmeat.net/projects/sonic-snap/?branch_id=55324&release_id=183982
http://tgnard.free.fr/linux/
(datasheet)
http://www.mnementh.co.uk/sonix/sn9c102.pdf

* Other webcam drivers:
http://zc0302.sourceforge.net/zc0302.php?page=cams
http://www.medias.ne.jp/~takam/bsd/NetBSD.html

* CMOS Sensor datasheets (rather, marketing buzz):
http://www.tascorp.com.tw/product_file/TAS5110C1B_Brief_V0.3.pdf
http://www.tascorp.com.tw/product_file/TAS5130D1B_Brief_V0.3.pdf

* Linux USB stack:
http://www.iglu.org.il/lxr/source/include/linux/usb.h

* Linux V4L webcam list:
http://linuxtv.org/v4lwiki/index.php/Webcams

* Fuji FinePix BeOS driver, should probably be merged at some point:
http://bebits.com/app/4185

