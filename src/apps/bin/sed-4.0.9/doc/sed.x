.SH NAME
sed \- a Stream EDitor
.SH SYNOPSIS
.nf
sed [-V] [--version] [--help] [-n] [--quiet] [--silent]
    [-l N] [--line-length=N] [-u] [--unbuffered]
    [-r] [--regexp-extended] 
    [-e script] [--expression=script]
    [-f script-file] [--file=script-file]
    [script-if-no-other-script]
    [file...]
.fi
[DESCRIPTION]
.ds sd \fIsed\fP
.ds Sd \fISed\fP
\*(Sd is a stream editor.
A stream editor is used to perform basic text
transformations on an input stream
(a file or input from a pipeline).
While in some ways similar to an editor which
permits scripted edits (such as \fIed\fP),
\*(sd works by making only one pass over the
input(s), and is consequently more efficient.
But it is \*(sd's ability to filter text in a pipeline
which particularly distinguishes it from other types of
editors.

[COMMAND SYNOPSIS]
This is just a brief synopsis of \*(sd commands to serve as
a reminder to those who already know \*(sd;
other documentation (such as the texinfo document)
must be consulted for fuller descriptions.
.SS
Zero-address ``commands''
.TP
.RI :\  label
Label for
.B b
and
.B t
commands.
.TP
.RI # comment
The comment extends until the next newline (or the end of a
.B -e
script fragment).
.TP
}
The closing bracket of a { } block.
.SS
Zero- or One- address commands
.TP
=
Print the current line number.
.TP
a \e
.TP
.I text
Append
.IR text ,
which has each embedded newline preceded by a backslash.
.TP
i \e
.TP
.I text
Insert
.IR text ,
which has each embedded newline preceded by a backslash.
.TP
q
Immediately quit the \*(sd script without processing
any more input,
except that if auto-print is not disabled
the current pattern space will be printed.
.TP
Q
Immediately quit the \*(sd script without processing
any more input.
.TP
.RI r\  filename
Append text read from
.IR filename .
.TP
.RI R\  filename
Append a line read from
.IR filename .
.SS
Commands which accept address ranges
.TP
{
Begin a block of commands (end with a }).
.TP
.RI b\  label
Branch to
.IR label ;
if
.I label
is omitted, branch to end of script.
.TP
.RI t\  label
If a s/// has done a successful substitution since the
last input line was read and since the last t or T
command, then branch to
.IR label ;
if
.I label
is omitted, branch to end of script.
.TP
.RI T\  label
If no s/// has done a successful substitution since the
last input line was read and since the last t or T
command, then branch to
.IR label ;
if
.I label
is omitted, branch to end of script.
.TP
c \e
.TP
.I text
Replace the selected lines with
.IR text ,
which has each embedded newline preceded by a backslash.
.TP
d
Delete pattern space.
Start next cycle.
.TP
D
Delete up to the first embedded newline in the pattern space.
Start next cycle, but skip reading from the input
if there is still data in the pattern space.
.TP
h H
Copy/append pattern space to hold space.
.TP
g G
Copy/append hold space to pattern space.
.TP
x
Exchange the contents of the hold and pattern spaces.
.TP
l
List out the current line in a ``visually unambiguous'' form.
.TP
n N
Read/append the next line of input into the pattern space.
.TP
p
Print the current pattern space.
.TP
P
Print up to the first embedded newline of the current pattern space.
.TP
.RI s/ regexp / replacement /
Attempt to match
.I regexp
against the pattern space.
If successful, replace that portion matched
with
.IR replacement .
The
.I replacement
may contain the special character
.B &
to refer to that portion of the pattern space which matched,
and the special escapes \e1 through \e9 to refer to the
corresponding matching sub-expressions in the
.IR regexp .
.TP
.RI w\  filename
Write the current pattern space to
.IR filename .
.TP
.RI W\  filename
Write the first line of the current pattern space to
.IR filename .
.TP
.RI y/ source / dest /
Transliterate the characters in the pattern space which appear in
.I source
to the corresponding character in
.IR dest .
.SH
Addresses
\*(Sd commands can be given with no addresses, in which
case the command will be executed for all input lines;
with one address, in which case the command will only be executed
for input lines which match that address; or with two
addresses, in which case the command will be executed
for all input lines which match the inclusive range of
lines starting from the first address and continuing to
the second address.
Three things to note about address ranges:
the syntax is
.IR addr1 , addr2
(i.e., the addresses are separated by a comma);
the line which
.I addr1
matched will always be accepted,
even if
.I addr2
selects an earlier line;
and if
.I addr2
is a
.IR regexp ,
it will not be tested against the line that
.I addr1
matched.
.PP
After the address (or address-range),
and before the command, a
.B !
may be inserted,
which specifies that the command shall only be
executed if the address (or address-range) does
.B not
match.
.PP
The following address types are supported:
.TP
.I number
Match only the specified line
.IR number .
.TP
.IR first ~ step
Match every
.IR step 'th
line starting with line
.IR first .
For example, ``sed -n 1~2p'' will print all the odd-numbered lines in
the input stream, and the address 2~5 will match every fifth line,
starting with the second. (This is an extension.)
.TP
$
Match the last line.
.TP
.RI / regexp /
Match lines matching the regular expression
.IR regexp .
.TP
.BI \fR\e\fPc regexp c
Match lines matching the regular expression
.IR regexp .
The
.B c
may be any character.
.PP
GNU \*(sd also supports some special 2-address forms:
.TP
.RI 0, addr2
Start out in "matched first address" state, until
.I addr2
is found.
This is similar to
.RI 1, addr2 ,
except that if
.I addr2
matches the very first line of input the
.RI 0, addr2
form will be at the end of its range, whereas the
.RI 1, addr2
form will still be at the beginning of its range.
.TP
.IR addr1 ,+ N
Will match
.I addr1
and the
.I N
lines following
.IR addr1 .
.TP
.IR addr1 ,~ N
Will match
.I addr1
and the lines following
.I addr1
until the next line whose input line number is a multiple of
.IR N .

[REGULAR EXPRESSIONS]
POSIX.2 BREs
.I should
be supported, but they aren't completely because of performance
problems.
The
.B \en
sequence in a regular expression matches the newline character,
and similarly for
.BR \ea ,
.BR \et ,
and other sequences.

[SEE ALSO]
.BR awk (1),
.BR ed (1),
.BR grep (1),
.BR tr (1),
.BR perlre (1),
sed.info,
any of various books on \*(sd,
.na
the \*(sd FAQ (http://sed.sf.net/grabbag/tutorials/sedfaq.html),
http://sed.sf.net/grabbag/.

[BUGS]
.PP
E-mail bug reports to
.BR bonzini@gnu.org .
Be sure to include the word ``sed'' somewhere in the ``Subject:'' field.
Also, please include the output of ``sed --version'' in the body
of your report if at all possible.
