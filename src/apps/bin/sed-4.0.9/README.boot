Because a working sed is a prerequisite for running the ``configure''
script, I have provided the script ``bootstrap.sh'' which will attempt
to build a version of sed adequate for running ``configure''.  If it
fails, edit the ``config.h'' file that was created according to the
comments found therein, and then try running ``bootstrap.sh'' again.

The bootstrap build is quite likely to babble on and on with
various compiler warnings.  You may want to tell bootstrap.sh
how to invoke your compiler with warnings disabled.  For example,
with a Bourne-like shell and gcc one could use:
  $ CC='gcc -w' sh bootstrap.sh
or with a csh-like shell, one could try:
  % env CC='gcc -w' sh bootstrap.sh

Once you get a working version of sed, temporarily install sed/sed
somewhere in your $PATH, and then really re-build the normal way
(starting with ``sh configure''); the bootstrap version is almost
certainly more crippled than it needs to be on your machine.

I don't much care to hear about any bugs in ``bootstrap'' versions
of sed beyond those which actually keep the ``bootstrap'' version from
building, or sed's configure script from running properly.  I am
especially uninterested in compiler warnings from the bootstrap build.
