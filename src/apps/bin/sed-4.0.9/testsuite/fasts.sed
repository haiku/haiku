# test `fast' substitutions

h
s/a//
p
g
s/a//g
p
g
s/^a//p
g
s/^a//g
p
g
s/not present//g
p
g
s/^[a-z]//g
p
g
s/a$//
p
g

y/a/b/
h
s/b//
p
g
s/b//g
p
g
s/^b//p
g
s/^b//g
p
g
s/^[a-z]//g
p
g
s/b$//
p
g



