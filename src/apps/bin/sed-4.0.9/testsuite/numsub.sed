# the first one matches, the second doesn't
1s/foo/bar/10
2s/foo/bar/20

# The second line should be deleted.  ssed 3.55-3.58 do not.
t
d
