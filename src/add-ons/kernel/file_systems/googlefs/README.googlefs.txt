Welcome to the Google™ FileSystem for BeOS™, Zeta™ and Haiku®.
Copyright© 2004, 2005, François Revol.
Google is a trademark of Google,Inc.
BeOS is a trademark of PalmSource.
Zeta is a trademark of yellowTAB GmbH.
Haiku is a trademark of Haiku, Inc.


REQUIRES BONE

mkdir /google; ndmount googlefs /google

Use "Search Google" query template in this folder to ask google anything.

or:

query -v /google '((name=="*QUESTION*")&&(BEOS:TYPE=="application/x-vnd.Be-bookmark"))'
where QUESTION is what you want to ask google. (googlefs currently filters queries to only answer those)
( you will want to pipe that to catattr:
query -v /google '((name=="*site:bebits.com bone*")&&(BEOS:TYPE=="application/x-vnd.Be-bookmark"))' | xargs catattr META:url
note that won't work with sync_unlink true in the settings)

Included are 2 scripts, google and imlucky that runs such a query using the arguments given on the command line.

An addon for Ingo Weinhold's UserlandFS is provided, compiled with debug printouts, so you can see it at work.
To use it, startUserlandFSServer and run:
ufs_mount googlefs /dev/zero /google

Enjoy.
