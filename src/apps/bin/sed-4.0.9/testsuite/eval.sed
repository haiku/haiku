1d

	#Try eval command
	/cpu/!b2
	e../sed/sed 1q eval.in2

:2
p
i---
h

	#Try eval option
	s,.* *cpu *,../sed/sed 1q eval.in2; echo "&",e

:3
p
g
i---

	h
	#Try eval option with print
	s,.* *cpu.*,../sed/sed 1q eval.in2,ep
	g


:4
p
i---

$!d

#Do some more tests
s/.*/Doing some more tests -----------------------/p
s,.*,../sed/sed 1q eval.in2,ep
i---
s,.*,../sed/sed 1q eval.in2,pe
i---
s,.*,../sed/sed 1q eval.in2,
h
e
p
g
i---
s/^/echo /ep
i---
s/^fubar$/echo wozthis/e
