USB Webcam driver
(c) 2004,2005,2006 François Revol.

Current version of my USB Webcam driver. WORK IN PROGRESS!
Uses the USB Kit (userland API, needs libusb) to publish a
media node representing the webcam.

It currently only works with my Sonix webcam (3Euro cheapo
cam), but is modular enough to easily expand it, some code
is already there to detect Quickcams.

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

References:

* Sonix linux drivers (several of them):
http://sourceforge.net/projects/sonix/
http://freshmeat.net/projects/sonic-snap/?branch_id=55324&release_id=183982
http://tgnard.free.fr/linux/

* Other webcam drivers:
http://zc0302.sourceforge.net/zc0302.php?page=cams

* CMOS Sensor datasheets (rather, marketing buzz):
http://www.tascorp.com.tw/product_file/TAS5110C1B_Brief_V0.3.pdf
http://www.tascorp.com.tw/product_file/TAS5130D1B_Brief_V0.3.pdf

* Linux USB stack:
http://www.iglu.org.il/lxr/source/include/linux/usb.h

* Linux V4L webcam list:
http://linuxtv.org/v4lwiki/index.php/Webcams

