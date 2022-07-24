Welcome to the Web Search FileSystem for BeOS™, Zeta™ and Haiku®.
Copyright© 2004, 2005, François Revol.

DuckDuckGo is a trademark of DuckDuckGo.
BeOS is a trademark of PalmSource.
Zeta is a trademark of yellowTAB GmbH.
Haiku is a trademark of Haiku, Inc.

REQUIRES USERLANDFS

mkdir /Web; mount -t userlandfs -p websearchfs /Web

Use "Search the Web" query template in this folder to search anything on the web.
The search results are kindly provided by DuckDuckGo (https://duckduckgo.com).

or from the commande line:

    query -v /Web '((name=="*QUESTION*")&&(BEOS:TYPE=="application/x-vnd.Be-bookmark"))'

where QUESTION is what you want to search. (websearchfs currently filters queries to only answer those).
You will want to pipe that to catattr to get the URLs:

    query -v /Web '((name=="*site:haiku-os.org userlandfs*")&&(BEOS:TYPE=="application/x-vnd.Be-bookmark"))' | xargs catattr META:url

(note that won't work with sync_unlink true in the settings)

Included are 2 scripts, search and imlucky that runs such a query using the arguments given on the command line.

Enjoy.
