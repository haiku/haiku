#! /bin/sed -nf

s/.*/&;9aaaaaaaaa8aaaaaaaa7aaaaaaa6aaaaaa5aaaaa4aaaa3aaa2aa1a0/
:encode
s/\(a*\)\([0-9]\)\([0-9]*;.*\2\(a*\)\)/\1\1\1\1\1\1\1\1\1\1\4\3/
tencode
s/;.*//

# Compute a few common factors for speed.  Clear the subst flag
t7a

# These are placed here to make the flow harder to understand :-)
:2
a\
2
b2a
:3
a\
3
b3a
:5
a\
5
b5a
:7
a\
7

:7a
s/^\(aa*\)\1\{6\}$/\1/
t7
:5a
s/^\(aa*\)\1\{4\}$/\1/
t5
:3a
s/^\(aa*\)\1\1$/\1/
t3
:2a
s/^\(aa*\)\1$/\1/
t2

/^a$/b

# The quotient of dividing by 11 is a limit to the remaining prime factors
s/^\(aa*\)\1\{10\}/\1=&/

# Pattern space looks like CANDIDATE\nNUMBER.  When a candidate is valid,
# the number is divided and the candidate is tried again
:factor
/^\(a\{7,\}\)=\1\1*$/! {
  # Decrement CANDIDATE, and search again if it is still >1
  s/^a//
  /^aa/b factor

  # Print the last remaining factor: since it is stored in the NUMBER
  # rather than in the CANDIDATE, swap 'em: now NUMBER=1
  s/\(.*\)=\(.*\)/\2=\1/
}

# We have a prime factor in CANDIDATE! Print it
h
s/=.*/;;0a1aa2aaa3aaaa4aaaaa5aaaaaa6aaaaaaa7aaaaaaaa8aaaaaaaaa9/

:decode
s/^\(a*\)\1\{9\}\(a\{0,9\}\)\([0-9]*;.*[^a]\2\([0-9]\)\)/\1\4\3/
/^a/tdecode
s/;.*//p

g
:divide
s/^\(a*\)\(=b*\)\1/\1\2b/
tdivide
y/b/a/

# If NUMBER = 1, we don't have any more factors
/aa$/bfactor
