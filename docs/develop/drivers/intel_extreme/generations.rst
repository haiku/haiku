Intel video hardware generations
################################

This file summarizes the different generations of Intel hardware, because the
naming is a bit inconsistent and it's hard to follow which is which sometimes.
The devices can be referred to by year of introduction, generation number,
chipset commercial number, or internal codenames.

Generation 1
============

These are the i740 and i810 devices handled by intel_810. No further info will
be provided here.

Generation 2 / 2002
===================

i830, 845, 85x, 865

Generation 3 / 2004
===================

This is the first generation to be documented at intellinuxgraphics.org.
Generation 2 devices are quite similar for the modesetting part, but not
identical.

GMA 900 (i915G)
GMA 950 (i945G)
GMA 3000 (946GZ, Q965, Q963)
GMA 3100 (G31, G33, Q33 et Q35)
GMA 3150 (Pineview for Atom CPUs)

Generation 4 / 2006
===================

GMA X3000 (i965G)
GMA X3100 (i965GM)
GMA X3500 (G35)
GMA 4500 (Q43, Q45)
GMA 4500M / 4500HD (GL40, GS45, GM45, GM47)
GMA X4500 / X4500HD (G41, G43 (X4500), G45 (X4500HD))

Generation 5 / 2010
===================

Westmere / Clarkdale, Arrandale / Iron Lake / Ibex Peak

Switches from the traditional northbridge / southbridge to the new
"platform control hub" design. Essentially, most of the northbridge functions
are now directly in the CPU package.

Generation 6 / 2011
===================

Sandybridge / Cougar Point

The northbridge and CPU are now even on the same die.

Generation 7 / 2012
===================

Ivy Bridge / Panther Point and Haswell / Lynx Point

A lot of the video output hardware is moved from the northbridge (CPU) to the
PCH. This makes sense because it allows to match the PCH chipset (soldered on
to the motherboard) with the video ports (also soldered there). Otherwise, the
CPU generation would define which ports are usable or not.

This impacts several things in the modesetting sequence, as well as the address
of the registers which are moved.

This is also the first generation to support 3 independant displays, which
also impacts the register layout in many places.

Generation 8 / 2013
===================

Broadwell / Wildcat Point and Braswell

Generation 9 / 2015
===================

Skylake / Sunrise Point, Apollo Lake, Kaby Lake / Union Point

Generation 10
=============

Cannon Point / Coffee Lake

Generation 11
=============

Ice Lake

Generation 12
=============

Tiger Lake
