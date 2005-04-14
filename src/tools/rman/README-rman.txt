The home location for PolyglotMan is polyglotman.sourceforge.net 


*** INSTALLING ***

Set BINDIR in the Makefile to where you keep your binaries and MANDIR
to where you keep your man pages (in their source form).  (If you're
using PolyglotMan with TkMan, BINDIR needs to be a component of your
bin PATH.)  After properly editing the Makefile, type `make install'.
Thereafter (perhaps after a `rehash') type `rman' to invoke PolyglotMan.
PolyglotMan requires an ANSI C compiler.  To compile on a Macintosh
under MPW, use Makefile.mac.

If you send me bug reports and/or suggestions for new features,
include the version of PolyglotMan (available by typing `rman -v').
PolyglotMan doesn't parse every aspect of every man page perfectly, but
if it blows up spectacularly where it doesn't seem like it should, you
can send me the man page (or a representative man page if it blows up
on a class of man pages) in BOTH: (1) [tn]roff source form, from
.../man/manX and (2) formatted form (as formatted by `nroff -man'),
uuencoded to preserve the control characters, from .../man/catX.

If you discover a bug and you obtained PolyglotMan at some other site,
check the home site to see if a newer version has already fixed the problem.

Be sure to look in the contrib directory for WWW server interfaces,
a batch converter, and a wrapper for SCO.

--------------------------------------------------

*** NOTES ON CURRENT VERSION ***

Help!  I'm looking for people to help with the following projects.
(1) Better RTF output format.  The current one works, but could be
made better.  (2) Extending the macro sets for source recognition.  If
you write an output format or otherwise improve PolyglotMan, please
send in your code so that I may share the wealth in future releases.
(3) Fixing output for various (accented?) characters in the Latin1
character set.

--------------------------------------------------


License

This software is distributed under the Artistic License (see
http://www.opensource.org/licenses/artistic-license.html).

(This version of PolyglotMan represents a complete rewrite of bs2tk,
which was packaged with TkMan in 1993, which is copyrighted by the
Regents of the University of California, and therefore is not under
their jurisdiction.)
