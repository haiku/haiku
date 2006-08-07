define(`ANCHORIDX',`0')dnl
define(`MANPAGE',`define(`MANPG',$1)')dnl
divert(-1)dnl
define(`ANCHOR',`divert(0)define(`ANCHORIDX',incr(ANCHORIDX))dnl
<TR><TD>$1</TD><TD><A HREF="HTMLNAME`#'AFU`_'ANCHORIDX">$2</A></TD><TD><A HREF="../man/MANPG.html">MANPG</A></TD></TR>
divert(-1)')
