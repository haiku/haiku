The Atari ST port
=================

Atari ST executables
--------------------

From: DaFi <webmaster@freudenstadt.net>

The specs for Atari ST executables (was listed as requested on www.wotsit.demon.co.uk/wanted.htm)...

applies for TOS, PRG, TTP, PRX, GTP, APP, ACC, ACX (different suffixes indicate different behavior of the program, i.e. TOS and TTP may not use the GEM GUI, while all the others may; only TTP and GTP can be called with parameters; ACC may be installed as desktop accessories; PRX and ACX mean the programs were disabled.

file structure:

+--------------------+---------------------------------------------------------------------------+
| [2] WORD PRG_magic | magic value 0x601a                                                        |
+--------------------+---------------------------------------------------------------------------+
| [4] LONG PRG_tsize | size of text segment                                                      |
+--------------------+---------------------------------------------------------------------------+
| [4] LONG PRG_dsize | size of data segment                                                      |
+--------------------+---------------------------------------------------------------------------+
| [4] LONG PRG_bsize | size of bss segment                                                       |
+--------------------+---------------------------------------------------------------------------+
| [4] LONG PRG_ssize | size of symbol table                                                      |
+--------------------+---------------------------------------------------------------------------+
| [4] LONG PRG_res1  | reserved                                                                  |
+--------------------+---------------------------------------------------------------------------+
| [4] LONG PRGFLAGS  | bit vector that defines additional process characteristics, as follows:   |
|                    |                                                                           |
|                    | * **Bit 0 PF_FASTLOAD** - if set, only the BSS area is cleared, otherwise,|
|                    |   the program's whole memory is cleared before loading                    |
|                    | * **Bit 1 PF_TTRAMLOAD** - if set, the program will be loaded into TT RAM |
|                    | * **Bit 2 PF_TTRAMMEM** - if set, the program will be allowed to allocate |
|                    |   memory from TT RAM                                                      |
|                    |                                                                           |
|                    | Bit 4 AND 5 as a two bit value with the following meanings:               |
|                    |                                                                           |
|                    | * 0 PF_PRIVATE - the processes entire memory space is considered private  |
|                    | * 1 PF_GLOBAL - the processes memory will be r/w-allowed for others       |
|                    | * 2 PF_SUPER - the memory will be r/w for itself and any supervisor proc  |
|                    | * 3 PF_READ - the memory will be readable by others                       |
+--------------------+---------------------------------------------------------------------------+
| [2] WORD ABSFLAG   | is NON-ZERO, if the program does not need to be relocated                 |
|                    |                                                                           |
|                    | is ZERO, if the program needs to be relocated                             |
|                    |                                                                           |
|                    | note: since some TOS versions handle files with ABSFLAG>0 incorrectly,    |
|                    | this value should be set to ZERO also for programs that need to be        |
|                    | relocated, and the FIXUP_offset should be set to 0.                       |
+--------------------+---------------------------------------------------------------------------+

From there on... (should be offset 0x1c)

[PRG_tsize] TEXT segment
[PRG_dsize] DATA segment
[PRG_ssize] Symbol table

[4] LONG FIXUP_offset - first LONG that needs to be relocated (offset to beginning of file)

From there on till the end of the file...

FIXUP table, with entries as follows:

[1] BYTE value

with value as follows:

- value=0  end of list
- value=1  advance 254 bytes
- value=2 to value=254 (only even values!) advance this many bytes and relocate the LONG found there.

That's it. You made it through to EOF.

A final note about fixing up (relocating) an executable: (pseudo-code)

The long value FIXUP_offset tells you your start adress. Let's call it "adr". So, now, that
you have adr, read the first byte of the table. 

(*) loop

- if it's 0, stop relocating -> you're done!
- if it's 1, add 254 to adr and read the next byte, jump back to the asterisk (*)
- if it's any other even value, add the value to your adr, then relocate the LONG at adr.
  (i.e. add the adress of the LONG to its value)

Useful resources
----------------

* http://toshyp.atari.org/en/index.html

* http://www.lysator.liu.se/~celeborn/sync/atari/misc.html
* http://www.lysator.liu.se/~celeborn/sync/atari/ATARI/F30.ZIP
* http://www.lysator.liu.se/~celeborn/sync/atari/ATARI/FALCLIB6.ZIP
* http://www.lysator.liu.se/~celeborn/sync/atari/ATARI/FALCREGS.ZIP

* http://fxr.watson.org/fxr/source/include/asm-m68k/atarihw.h?v=linux-2.4.22
* http://lxr.linux.no/linux+v2.6.27/arch/m68k/atari/config.c#L664

* http://www.atari-forum.com/wiki/index.php/MFP_MK68901

* http://ftp.netbsd.org/pub/NetBSD/NetBSD-current/src/sys/arch/atari/stand/xxboot/ahdi-xxboot/xxboot.ahdi.S

AHDI args

* http://ftp.netbsd.org/pub/NetBSD/NetBSD-current/src/sys/arch/atari/stand/xxboot/wdboot/wdboot.S
* http://ftp.netbsd.org/pub/NetBSD/NetBSD-current/src/sys/arch/atari/stand/xxboot/sdboot/sdboot.S
* http://ftp.netbsd.org/pub/NetBSD/NetBSD-current/src/sys/arch/atari/stand/xxboot/fdboot/fdboot.S

