# straight out of an autoconf-generated configure.
# The input should look just like the input after this is run.
#
# Protect against being on the right side of a sed subst in config.status. 
s/%@/@@/; s/@%/@@/; s/%g$/@g/; /@g$/s/[\\\\&%]/\\\\&/g; 
 s/@@/%@/; s/@@/@%/; s/@g$/%g/
