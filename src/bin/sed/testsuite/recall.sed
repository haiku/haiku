# Check that the empty regex recalls the last *executed* regex,
# not the last *compiled* regex
p
s/e/X/p
:x
s//Y/p
/f/bx
