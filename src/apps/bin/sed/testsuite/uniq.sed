h

:b
# On the last line, print and exit
$b
N
/^\(.*\)\n\1$/ {
    # The two lines are identical.  Undo the effect of
    # the n command.
    g
    bb
}

# If the @code{N} command had added the last line, print and exit
$b

# The lines are different; print the first and go
# back working on the second.
P
D
