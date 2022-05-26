# os-probe for the Haiku Computer Operating System

This is the Linux "os-probes" file to detect Haiku OS and to automatically add
it to the GRUB boot menu.  Mostly relevant for x86 BIOS based computers.

Copy the 83haiku file to your Linux system in the os-probes subdirectory,
usually (in Fedora at least) it will be /usr/libexec/os-probes/mounted/83haiku
You can find older 83haiku versions in the repository history, though the
latest should be able to detect older (pre-package manager) Haiku too.

Then regenerate the GRUB boot configuration file.  This will happen
automatically the next time your kernel is updated.  To do it manually,
for old school MBR BIOS boot computers, the command is
`grub2-mkconfig --output /boot/grub2/grub.cfg`
If it doesn't find the Haiku partitions, try manually mounting them in Linux
and rerun the grub command.

Computers using the newer UEFI boot system have a EFI/HAIKU/BOOTX64.EFI file
that you manually install to your EFI partition, and booting is done
differently, so you don't need this 83Haiku file for them.  See
[UEFI Booting Haiku](https://www.haiku-os.org/guides/uefi_booting/) instead.

The original seems to have come from Debian and was written by François Revol.
It's in the
[Debian os-prober package](https://packages.debian.org/search?keywords=os-prober).
There's also a big discussion about updating it in
[Debian Bug Report #732696](https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=732696).
Latest version is now at https://git.haiku-os.org/haiku/tree/3rdparty/os_probe

_AGMS20210927_
