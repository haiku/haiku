# A kind of clone of dc geared towards binary operations.
# by Paolo Bonzini
#
# commands available:
#   conversion commands
#     b      convert decimal to binary
#     d      convert binary to decimal
#
#   arithmetic commands
#     <      shift left binary by decimal number of bits (11 3< gives 11000)
#     >      shift right binary by decimal number of bits (1011 2> gives 10)
#     &      binary AND (between two binary operands)
#     |      binary OR (between two binary operands)
#     ^      binary XOR (between two binary operands)
#     ~      binary NOT (between one binary operand)
#
#   stack manipulation commands
#     c      clear stack
#     P      pop stack top
#     D      duplicate stack top
#     x      exchange top two elements
#     r      rotate stack counter-clockwise (second element becomes first)
#     R      rotate stack clockwise (last element becomes first)
#
#   other commands
#     l      print stack (stack top is first)
#     p      print stack top
#     q      quit, print stack top if any (cq is quiet quit)
#
# The only shortcoming is that you'd better not attempt conversions of
# values above 1000 or so.
#
# This version keeps the stack and the current command in hold space and
# the commands in pattern space; it is just a bit slower than binary2.sed
# but more size optimized for broken seds which have a 199-command limit
# (though binary2.sed does not have this much).
#
# --------------------------------------------------------------------------
# This was actually used in a one-disk distribution of Linux to compute
# netmasks as follows (1 parameter => compute netmask e.g. 24 becomes
# 255.255.255.0; 2 parameters => given host address and netmask compute
# network and broadcast addresses):
#
# if [ $# = 1 ]; then 
#   OUTPUT='$1.$2.$3.$4'
#   set 255.255.255.255 $1
# else
#   OUTPUT='$1.$2.$3.$4 $5.$6.$7.$8'
# fi
# 
# if [ `expr $2 : ".*\\."` -gt 0 ]; then
#   MASK="$2 br b8<r b16<r b24< R|R|R|"
# else
#   MASK="$2b 31b ^d D
#         11111111111111111111111111111111 x>1> x<1<"
# fi
# 
# set `echo "$1 br b8<r b16<r b24< R|R|R| D    # Load address
#            $MASK D ~r                        # Load mask
# 
#            & DDD 24>dpP 16>11111111& dpP 8>11111111& dpP 11111111& dpP
#            | DDD 24>dpP 16>11111111& dpP 8>11111111& dpP 11111111& dpP
#       " | sed -f binary.sed`
#
# eval echo $OUTPUT
# --------------------------------------------------------------------------

:cmd
s/^[\n\t ]*//
s/^#.*//
/^$/ {
  $b quit
  N
  t cmd
}
/^[0-9][0-9]*/ {
  G
  h
  s/^[0-9][0-9]* *\([^\n]*\).*/\1/
  x
  s/^\([0-9][0-9]*\)[^\n]*/\1/
  x
  t cmd
}

/^[^DPxrRcplqbd&|^~<>]/bbad

H
x
s/\(\n[^\n]\)[^\n]*$/\1/

/D$/ s/^[^\n]*\n/&&/
/P$/ s/^[^\n]*\n//
/x$/ s/^\([^\n]*\n\)\([^\n]*\n\)/\2\1/
/r$/ s/^\([^\n]*\n\)\(.*\)\(..\)/\2\1\3/
/R$/ s/^\(.*\n\)\([^\n]*\n\)\(..\)/\2\1\3/
/c$/ s/.*//
/p$/ P
/l$/ {
  s/...$//
  p
  t cmd
}

/q$/ {
  :quit
  /.../P
  d
}

/b$/ {
  # Decimal to binary via analog form
  s/^\([^\n]*\)/-&;9876543210aaaaaaaaa/
  :d2bloop1
  s/\(a*\)-\(.\)\([^;]*;[0-9]*\2.\{9\}\(a*\)\)/\1\1\1\1\1\1\1\1\1\1\4-\3/
  t d2bloop1
  s/-;9876543210aaaaaaaaa/;a01!/
  :d2bloop2
  s/\(a*\)\1\(a\{0,1\}\)\(;\2.\(.\)[^!]*!\)/\1\3\4/
  /^a/b d2bloop2
  s/[^!]*!//
}

/d$/ {
  # Binary to decimal via analog form
  s/^\([^\n]*\)/-&;10a/
  :b2dloop1
  s/\(a*\)-\(.\)\([^;]*;[0-9]*\2.\(a*\)\)/\1\1\4-\3/
  t b2dloop1
  s/-;10a/;aaaaaaaaa0123456789!/
  :b2dloop2
  s/\(a*\)\1\1\1\1\1\1\1\1\1\(a\{0,9\}\)\(;\2.\{9\}\(.\)[^!]*!\)/\1\3\4/
  /^a/b b2dloop2
  s/[^!]*!//
}

/&$/ {
  # Binary AND
  s/\([^\n]*\)\n\([^\n]*\)/-\1-\2-111 01000/
  :andloop
  s/\([^-]*\)-\([^-]*\)\([^-]\)-\([^-]*\)\([^-]\)-\([01 ]*\3\5\([01]\)\)/\7\1-\2-\4-\6/
  t andloop
  s/^0*\([^-]*\)-[^\n]*/\1/
  s/^\n/0&/
}

/\^$/ {
  # Binary XOR
  s/\([^\n]*\)\n\([^\n]*\)/-\1-\2-000 01101/
  b orloop
}

/|$/ {
  # Binary OR
  s/\([^\n]*\)\n\([^\n]*\)/-\1-\2-000 10111/
  :orloop
  s/\([^-]*\)-\([^-]*\)\([^-]\)-\([^-]*\)\([^-]\)-\([01 ]*\3\5\([01]\)\)/\7\1-\2-\4-\6/
  t orloop
  s/\([^-]*\)-\([^-]*\)-\([^-]*\)-[^\n]*/\2\3\1/
}

/~$/ {
  # Binary NOT
  s/^\(.\)\([^\n]*\n\)/\1-010-\2/
  :notloop
  s/\(.\)-0\{0,1\}\1\(.\)0\{0,1\}-\([01\n]\)/\2\3-010-/
  t notloop

  # If result is 00001..., \3 does not match (it looks for -10) and we just
  # remove the table and leading zeros.  If result is 0000...0, \3 matches
  # (it looks for -0), \4 is a zero and we leave a lone zero as top of the
  # stack.

  s/0*\(1\{0,1\}\)\([^-]*\)-\(\1\(0\)\)\{0,1\}[^-]*-/\4\1\2/
}

/<$/ {
  # Left shift, convert to analog and add a binary digit for each analog digit
  s/^\([^\n]*\)/-&;9876543210aaaaaaaaa/
  :lshloop1
  s/\(a*\)-\(.\)\([^;]*;[0-9]*\2.\{9\}\(a*\)\)/\1\1\1\1\1\1\1\1\1\1\4-\3/
  t lshloop1
  s/^\(a*\)-;9876543210aaaaaaaaa\n\([^\n]*\)/\2\1/
  s/a/0/g
}

/>$/ {
  # Right shift, convert to analog and remove a binary digit for each analog digit
  s/^\([^\n]*\)/-&;9876543210aaaaaaaaa/
  :rshloop1
  s/\(a*\)-\(.\)\([^;]*;[0-9]*\2.\{9\}\(a*\)\)/\1\1\1\1\1\1\1\1\1\1\4-\3/
  t rshloop1
  s/^\(a*\)-;9876543210aaaaaaaaa\n\([^\n]*\)/\2\1/
  :rshloop2
  s/.a//
  s/^aa*/0/
  /a\n/b rshloop2
}

s/..$//
x
:bad
s/^.//
tcmd
