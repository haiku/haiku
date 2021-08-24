The Sam460ex Haiku port
=======================

Complications for this port comes from the fact that the CPU used was designed for embedded devices,
and has a much simpler MMU than the one on desktop machines. As a result, completely different
memory management code needs to be written.

U-Boot commands
---------------

no-fdt no-initrd
****************

seems the U-Boot input buffer is quite limited, can't paste much more on single line in minicom

  setenv ipaddr 192.168.4.100; tftpboot 0x4000000 192.168.4.2:haiku_loader_linux.ub; bootm 0x4000000

with FDT and tgz as initrd
**************************

  setenv ipaddr 192.168.4.100
  tftpboot 0x4000000 192.168.4.2:haiku_loader_linux.ub
  tftpboot 0x8000000 192.168.4.2:haiku_initrd.ub
  tftpboot 0xc000000 192.168.4.2:sam460ex.dtb
  fdt addr 0xc000000
  fdt header
  bootm 0x4000000 0x8000000 0xc000000 plop

for environment
***************

  setenv booth1 'setenv ipaddr 192.168.4.100; tftpboot 0x4000000 192.168.4.2:haiku_loader_linux.ub'
  setenv booth2 'tftpboot 0x8000000 192.168.4.2:haiku_initrd.ub'
  setenv booth3 'tftpboot 0xc000000 192.168.4.2:sam460ex.dtb'
  setenv booth4 'bootm 0x4000000 0x8000000 0xc000000 plop'
  setenv booth 'run booth1; run booth2; run booth3; run booth4'
  saveenv
  run booth

TODOs
-----

* U-Boot API?
* move Partenope hack to proper official U-Boot API?
* reserved regs?

  BoardSetup +=:?
  TARGET_BOOT_CCFLAGS += -ffixed-r2 -ffixed-r14 -ffixed-r29 ;
  TARGET_BOOT_C++FLAGS += -ffixed-r2 -ffixed-r14 -ffixed-r29 ;

* kdebug/disasm/ppc http://code.google.com/p/ppcd/

Other ports
-----------

* `AROS port <https://www.gitorious.org/aros/aros/commits/sam460>`_
* `Linux port <http://kernel.org/doc/ols/2003/ols2003-pages-340-350.pdf>`_
* `NetBSD <https://wiki.netbsd.org/users/rkujawa/sam4x0/>`_

PowerPC information
-------------------

Classic
*******

* http://class.ee.iastate.edu/cpre211/labs/quickrefPPC.html
* http://www.ibm.com/developerworks/library/l-ppc/
* http://www.csd.uwo.ca/~mburrel/stuff/ppc-asm.html

Book-E
******

* http://www.linux-kvm.org/page/PowerPC_Book_E_MMU
* http://wiki.freebsd.org/powerpc/BookE
* http://en.wikipedia.org/wiki/Memory_management_unit#PowerPC

ePAPR
*****

* https://www.power.org/wp-content/uploads/2012/06/Power_ePAPR_APPROVED_v1.1.pdf
* PPC440: http://elinux.org/Book_E_and_PPC_440

amcc 4x0
********

* http://c0ff33.net/drop/PPC440_UM2013.pdf
* http://www.embeddeddeveloper.com/assets/processors/amcc/datasheets/PP460EX_DS2063.pdf

Freescale 440
*************

This version has a different mmu!!

* http://www.freescale.com/files/32bit/doc/white_paper/POWRPCARCPRMRM.pdf

FDT
---

* http://www.denx.de/wiki/U-Boot/UBootFdtInfo
* http://wiki.freebsd.org/FlattenedDeviceTree#Supporting_library_.28libfdt.29
* (see also arm docs)

Sam440 dts
**********

* http://lxr.linux.no/linux+v3.4/arch/powerpc/boot/dts/sam440ep.dts
* Sam460ex dts: identical to amcc,Canyonlands !?
* http://www.denx.de/wiki/view/DULG/Appendix#Section_13.1.

OpenFirmware framebuffer
************************

(not really usable from U-Boot (yet?))

* http://www.feedface.com/howto/forth.html
* http://mail-index.netbsd.org/port-macppc/2004/12/13/0046.html
* http://lists.freebsd.org/pipermail/svn-src-user/2012-January/004806.html
* http://www.openfirmware.info/Bindings

