define(`ANCHORIDX',`0')dnl
define(`MANPAGE',`define(`MANPG',$1)dnl
|=====================================================================
   --  | Man page <A HREF="../man/MANPG.html">MANPG</A>
   --  |=====================================================================')dnl
define(`ANCHOR',`define(`ANCHORIDX',incr(ANCHORIDX))dnl
`#'1A NAME="AFU`_'ANCHORIDX"`#'2dnl
define(`CFUNAME',`$1')define(`AFUNAME',`$2')dnl
|')
define(`AKA',``AKA': <A HREF="../man/MANPG.html">CFUNAME</A>')dnl
define(`ALIAS',``AKA': $1')dnl
