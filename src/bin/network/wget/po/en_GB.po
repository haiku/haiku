# English (British) translation.
# Copyright (C) 2004 Free Software Foundation, Inc.
# Gareth Owen <gowen72@yahoo.com>, 2004.
#
msgid ""
msgstr ""
"Project-Id-Version: wget 1.9.1\n"
"POT-Creation-Date: 2003-10-11 16:21+0200\n"
"PO-Revision-Date: 2004-05-27 21:46-0400\n"
"Last-Translator: Gareth Owen <gowen72@yahoo.com>\n"
"Language-Team: English (British) <en_gb@li.org>\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"

#: src/connect.c:88
#, c-format
msgid "Unable to convert `%s' to a bind address.  Reverting to ANY.\n"
msgstr "Unable to convert `%s' to a bind address.  Reverting to ANY.\n"

#: src/connect.c:165
#, c-format
msgid "Connecting to %s[%s]:%hu... "
msgstr "Connecting to %s[%s]:%hu... "

#: src/connect.c:168
#, c-format
msgid "Connecting to %s:%hu... "
msgstr "Connecting to %s:%hu... "

#: src/connect.c:222
msgid "connected.\n"
msgstr "connected.\n"

#: src/convert.c:171
#, c-format
msgid "Converted %d files in %.2f seconds.\n"
msgstr "Converted %d files in %.2f seconds.\n"

#: src/convert.c:197
#, c-format
msgid "Converting %s... "
msgstr "Converting %s... "

#: src/convert.c:210
msgid "nothing to do.\n"
msgstr "nothing to do.\n"

#: src/convert.c:218 src/convert.c:242
#, c-format
msgid "Cannot convert links in %s: %s\n"
msgstr "Cannot convert links in %s: %s\n"

#: src/convert.c:233
#, c-format
msgid "Unable to delete `%s': %s\n"
msgstr "Unable to delete `%s': %s\n"

#: src/convert.c:439
#, c-format
msgid "Cannot back up %s as %s: %s\n"
msgstr "Cannot back up %s as %s: %s\n"

#: src/cookies.c:606
#, c-format
msgid "Error in Set-Cookie, field `%s'"
msgstr "Error in Set-Cookie, field `%s'"

#: src/cookies.c:629
#, c-format
msgid "Syntax error in Set-Cookie: %s at position %d.\n"
msgstr "Syntax error in Set-Cookie: %s at position %d.\n"

#: src/cookies.c:1426
#, c-format
msgid "Cannot open cookies file `%s': %s\n"
msgstr "Cannot open cookies file `%s': %s\n"

#: src/cookies.c:1438
#, c-format
msgid "Error writing to `%s': %s\n"
msgstr "Error writing to `%s': %s\n"

#: src/cookies.c:1442
#, c-format
msgid "Error closing `%s': %s\n"
msgstr "Error closing `%s': %s\n"

#: src/ftp-ls.c:812
msgid "Unsupported listing type, trying Unix listing parser.\n"
msgstr "Unsupported listing type, trying Unix listing parser.\n"

#: src/ftp-ls.c:857 src/ftp-ls.c:859
#, c-format
msgid "Index of /%s on %s:%d"
msgstr "Index of /%s on %s:%d"

#: src/ftp-ls.c:882
msgid "time unknown       "
msgstr "time unknown       "

#: src/ftp-ls.c:886
msgid "File        "
msgstr "File        "

#: src/ftp-ls.c:889
msgid "Directory   "
msgstr "Directory   "

#: src/ftp-ls.c:892
msgid "Link        "
msgstr "Link        "

#: src/ftp-ls.c:895
msgid "Not sure    "
msgstr "Not sure    "

#: src/ftp-ls.c:913
#, c-format
msgid " (%s bytes)"
msgstr " (%s bytes)"

#. Second: Login with proper USER/PASS sequence.
#: src/ftp.c:202
#, c-format
msgid "Logging in as %s ... "
msgstr "Logging in as %s ... "

#: src/ftp.c:215 src/ftp.c:268 src/ftp.c:299 src/ftp.c:353 src/ftp.c:468
#: src/ftp.c:519 src/ftp.c:551 src/ftp.c:611 src/ftp.c:675 src/ftp.c:748
#: src/ftp.c:796
msgid "Error in server response, closing control connection.\n"
msgstr "Error in server response, closing control connection.\n"

#: src/ftp.c:223
msgid "Error in server greeting.\n"
msgstr "Error in server greeting.\n"

#: src/ftp.c:231 src/ftp.c:362 src/ftp.c:477 src/ftp.c:560 src/ftp.c:621
#: src/ftp.c:685 src/ftp.c:758 src/ftp.c:806
msgid "Write failed, closing control connection.\n"
msgstr "Write failed, closing control connection.\n"

#: src/ftp.c:238
msgid "The server refuses login.\n"
msgstr "The server refuses login.\n"

#: src/ftp.c:245
msgid "Login incorrect.\n"
msgstr "Login incorrect.\n"

#: src/ftp.c:252
msgid "Logged in!\n"
msgstr "Logged in!\n"

#: src/ftp.c:277
msgid "Server error, can't determine system type.\n"
msgstr "Server error, can't determine system type.\n"

#: src/ftp.c:287 src/ftp.c:596 src/ftp.c:659 src/ftp.c:716
msgid "done.    "
msgstr "done.    "

#: src/ftp.c:341 src/ftp.c:498 src/ftp.c:533 src/ftp.c:779 src/ftp.c:827
msgid "done.\n"
msgstr "done.\n"

#: src/ftp.c:370
#, c-format
msgid "Unknown type `%c', closing control connection.\n"
msgstr "Unknown type `%c', closing control connection.\n"

#: src/ftp.c:383
msgid "done.  "
msgstr "done.  "

#: src/ftp.c:389
msgid "==> CWD not needed.\n"
msgstr "==> CWD not needed.\n"

#: src/ftp.c:484
#, c-format
msgid ""
"No such directory `%s'.\n"
"\n"
msgstr ""
"No such directory `%s'.\n"
"\n"

#. do not CWD
#: src/ftp.c:502
msgid "==> CWD not required.\n"
msgstr "==> CWD not required.\n"

#: src/ftp.c:567
msgid "Cannot initiate PASV transfer.\n"
msgstr "Cannot initiate PASV transfer.\n"

#: src/ftp.c:571
msgid "Cannot parse PASV response.\n"
msgstr "Cannot parse PASV response.\n"

#: src/ftp.c:588
#, c-format
msgid "couldn't connect to %s:%hu: %s\n"
msgstr "couldn't connect to %s:%hu: %s\n"

#: src/ftp.c:638
#, c-format
msgid "Bind error (%s).\n"
msgstr "Bind error (%s).\n"

#: src/ftp.c:645
msgid "Invalid PORT.\n"
msgstr "Invalid PORT.\n"

#: src/ftp.c:698
#, c-format
msgid ""
"\n"
"REST failed; will not truncate `%s'.\n"
msgstr ""
"\n"
"REST failed; will not truncate `%s'.\n"

#: src/ftp.c:705
msgid ""
"\n"
"REST failed, starting from scratch.\n"
msgstr ""
"\n"
"REST failed, starting from scratch.\n"

#: src/ftp.c:766
#, c-format
msgid ""
"No such file `%s'.\n"
"\n"
msgstr ""
"No such file `%s'.\n"
"\n"

#: src/ftp.c:814
#, c-format
msgid ""
"No such file or directory `%s'.\n"
"\n"
msgstr ""
"No such file or directory `%s'.\n"
"\n"

#: src/ftp.c:898 src/ftp.c:906
#, c-format
msgid "Length: %s"
msgstr "Length: %s"

#: src/ftp.c:900 src/ftp.c:908
#, c-format
msgid " [%s to go]"
msgstr " [%s to go]"

#: src/ftp.c:910
msgid " (unauthoritative)\n"
msgstr " (unauthoritative)\n"

#: src/ftp.c:936
#, c-format
msgid "%s: %s, closing control connection.\n"
msgstr "%s: %s, closing control connection.\n"

#: src/ftp.c:944
#, c-format
msgid "%s (%s) - Data connection: %s; "
msgstr "%s (%s) - Data connection: %s; "

#: src/ftp.c:961
msgid "Control connection closed.\n"
msgstr "Control connection closed.\n"

#: src/ftp.c:979
msgid "Data transfer aborted.\n"
msgstr "Data transfer aborted.\n"

#: src/ftp.c:1044
#, c-format
msgid "File `%s' already there, not retrieving.\n"
msgstr "File `%s' already there, not retrieving.\n"

#: src/ftp.c:1114 src/http.c:1716
#, c-format
msgid "(try:%2d)"
msgstr "(try:%2d)"

#: src/ftp.c:1180 src/http.c:1975
#, c-format
msgid ""
"%s (%s) - `%s' saved [%ld]\n"
"\n"
msgstr ""
"%s (%s) - `%s' saved [%ld]\n"
"\n"

#: src/ftp.c:1222 src/main.c:890 src/recur.c:377 src/retr.c:596
#, c-format
msgid "Removing %s.\n"
msgstr "Removing %s.\n"

#: src/ftp.c:1264
#, c-format
msgid "Using `%s' as listing tmp file.\n"
msgstr "Using `%s' as listing tmp file.\n"

#: src/ftp.c:1279
#, c-format
msgid "Removed `%s'.\n"
msgstr "Removed `%s'.\n"

#: src/ftp.c:1314
#, c-format
msgid "Recursion depth %d exceeded max. depth %d.\n"
msgstr "Recursion depth %d exceeded max. depth %d.\n"

#. Remote file is older, file sizes can be compared and
#. are both equal.
#: src/ftp.c:1384
#, c-format
msgid "Remote file no newer than local file `%s' -- not retrieving.\n"
msgstr "Remote file no newer than local file `%s' -- not retrieving.\n"

#. Remote file is newer or sizes cannot be matched
#: src/ftp.c:1391
#, c-format
msgid ""
"Remote file is newer than local file `%s' -- retrieving.\n"
"\n"
msgstr ""
"Remote file is newer than local file `%s' -- retrieving.\n"
"\n"

#. Sizes do not match
#: src/ftp.c:1398
#, c-format
msgid ""
"The sizes do not match (local %ld) -- retrieving.\n"
"\n"
msgstr ""
"The sizes do not match (local %ld) -- retrieving.\n"
"\n"

#: src/ftp.c:1415
msgid "Invalid name of the symlink, skipping.\n"
msgstr "Invalid name of the symlink, skipping.\n"

#: src/ftp.c:1432
#, c-format
msgid ""
"Already have correct symlink %s -> %s\n"
"\n"
msgstr ""
"Already have correct symlink %s -> %s\n"
"\n"

#: src/ftp.c:1440
#, c-format
msgid "Creating symlink %s -> %s\n"
msgstr "Creating symlink %s -> %s\n"

#: src/ftp.c:1451
#, c-format
msgid "Symlinks not supported, skipping symlink `%s'.\n"
msgstr "Symlinks not supported, skipping symlink `%s'.\n"

#: src/ftp.c:1463
#, c-format
msgid "Skipping directory `%s'.\n"
msgstr "Skipping directory `%s'.\n"

#: src/ftp.c:1472
#, c-format
msgid "%s: unknown/unsupported file type.\n"
msgstr "%s: unknown/unsupported file type.\n"

#: src/ftp.c:1499
#, c-format
msgid "%s: corrupt time-stamp.\n"
msgstr "%s: corrupt time-stamp.\n"

#: src/ftp.c:1524
#, c-format
msgid "Will not retrieve dirs since depth is %d (max %d).\n"
msgstr "Will not retrieve dirs since depth is %d (max %d).\n"

#: src/ftp.c:1574
#, c-format
msgid "Not descending to `%s' as it is excluded/not-included.\n"
msgstr "Not descending to `%s' as it is excluded/not-included.\n"

#: src/ftp.c:1639 src/ftp.c:1652
#, c-format
msgid "Rejecting `%s'.\n"
msgstr "Rejecting `%s'.\n"

#. No luck.
#. #### This message SUCKS.  We should see what was the
#. reason that nothing was retrieved.
#: src/ftp.c:1698
#, c-format
msgid "No matches on pattern `%s'.\n"
msgstr "No matches on pattern `%s'.\n"

#: src/ftp.c:1764
#, c-format
msgid "Wrote HTML-ized index to `%s' [%ld].\n"
msgstr "Wrote HTML-ised index to `%s' [%ld].\n"

#: src/ftp.c:1769
#, c-format
msgid "Wrote HTML-ized index to `%s'.\n"
msgstr "Wrote HTML-ised index to `%s'.\n"

#: src/gen_sslfunc.c:117
msgid "Could not seed OpenSSL PRNG; disabling SSL.\n"
msgstr "Could not seed OpenSSL PRNG; disabling SSL.\n"

#: src/getopt.c:675
#, c-format
msgid "%s: option `%s' is ambiguous\n"
msgstr "%s: option `%s' is ambiguous\n"

#: src/getopt.c:700
#, c-format
msgid "%s: option `--%s' doesn't allow an argument\n"
msgstr "%s: option `--%s' doesn't allow an argument\n"

#: src/getopt.c:705
#, c-format
msgid "%s: option `%c%s' doesn't allow an argument\n"
msgstr "%s: option `%c%s' doesn't allow an argument\n"

#: src/getopt.c:723 src/getopt.c:896
#, c-format
msgid "%s: option `%s' requires an argument\n"
msgstr "%s: option `%s' requires an argument\n"

#. --option
#: src/getopt.c:752
#, c-format
msgid "%s: unrecognized option `--%s'\n"
msgstr "%s: unrecognised option `--%s'\n"

#. +option or -option
#: src/getopt.c:756
#, c-format
msgid "%s: unrecognized option `%c%s'\n"
msgstr "%s: unrecognised option `%c%s'\n"

#. 1003.2 specifies the format of this message.
#: src/getopt.c:782
#, c-format
msgid "%s: illegal option -- %c\n"
msgstr "%s: illegal option -- %c\n"

#: src/getopt.c:785
#, c-format
msgid "%s: invalid option -- %c\n"
msgstr "%s: invalid option -- %c\n"

#. 1003.2 specifies the format of this message.
#: src/getopt.c:815 src/getopt.c:945
#, c-format
msgid "%s: option requires an argument -- %c\n"
msgstr "%s: option requires an argument -- %c\n"

#: src/getopt.c:862
#, c-format
msgid "%s: option `-W %s' is ambiguous\n"
msgstr "%s: option `-W %s' is ambiguous\n"

#: src/getopt.c:880
#, c-format
msgid "%s: option `-W %s' doesn't allow an argument\n"
msgstr "%s: option `-W %s' doesn't allow an argument\n"

#: src/host.c:636
#, c-format
msgid "Resolving %s... "
msgstr "Resolving %s... "

#: src/host.c:656 src/host.c:672
#, c-format
msgid "failed: %s.\n"
msgstr "failed: %s.\n"

#: src/host.c:674
msgid "failed: timed out.\n"
msgstr "failed: timed out.\n"

#: src/host.c:762
msgid "Host not found"
msgstr "Host not found"

#: src/host.c:764
msgid "Unknown error"
msgstr "Unknown error"

#: src/html-url.c:293
#, c-format
msgid "%s: Cannot resolve incomplete link %s.\n"
msgstr "%s: Cannot resolve incomplete link %s.\n"

#. this is fatal
#: src/http.c:674
msgid "Failed to set up an SSL context\n"
msgstr "Failed to set up an SSL context\n"

#: src/http.c:680
#, c-format
msgid "Failed to load certificates from %s\n"
msgstr "Failed to load certificates from %s\n"

#: src/http.c:684 src/http.c:692
msgid "Trying without the specified certificate\n"
msgstr "Trying without the specified certificate\n"

#: src/http.c:688
#, c-format
msgid "Failed to get certificate key from %s\n"
msgstr "Failed to get certificate key from %s\n"

#: src/http.c:761 src/http.c:1809
msgid "Unable to establish SSL connection.\n"
msgstr "Unable to establish SSL connection.\n"

#: src/http.c:770
#, c-format
msgid "Reusing connection to %s:%hu.\n"
msgstr "Reusing connection to %s:%hu.\n"

#: src/http.c:1034
#, c-format
msgid "Failed writing HTTP request: %s.\n"
msgstr "Failed writing HTTP request: %s.\n"

#: src/http.c:1039
#, c-format
msgid "%s request sent, awaiting response... "
msgstr "%s request sent, awaiting response... "

#: src/http.c:1083
msgid "End of file while parsing headers.\n"
msgstr "End of file while parsing headers.\n"

#: src/http.c:1093
#, c-format
msgid "Read error (%s) in headers.\n"
msgstr "Read error (%s) in headers.\n"

#: src/http.c:1128
msgid "No data received"
msgstr "No data received"

#: src/http.c:1130
msgid "Malformed status line"
msgstr "Malformed status line"

#: src/http.c:1135
msgid "(no description)"
msgstr "(no description)"

#: src/http.c:1267
msgid "Authorization failed.\n"
msgstr "Authorization failed.\n"

#: src/http.c:1274
msgid "Unknown authentication scheme.\n"
msgstr "Unknown authentication scheme.\n"

#: src/http.c:1314
#, c-format
msgid "Location: %s%s\n"
msgstr "Location: %s%s\n"

#: src/http.c:1315 src/http.c:1454
msgid "unspecified"
msgstr "unspecified"

#: src/http.c:1316
msgid " [following]"
msgstr " [following]"

#: src/http.c:1383
msgid ""
"\n"
"    The file is already fully retrieved; nothing to do.\n"
"\n"
msgstr ""
"\n"
"    The file is already fully retrieved; nothing to do.\n"
"\n"

#: src/http.c:1401
#, c-format
msgid ""
"\n"
"Continued download failed on this file, which conflicts with `-c'.\n"
"Refusing to truncate existing file `%s'.\n"
"\n"
msgstr ""
"\n"
"Continued download failed on this file, which conflicts with `-c'.\n"
"Refusing to truncate existing file `%s'.\n"
"\n"

#. No need to print this output if the body won't be
#. downloaded at all, or if the original server response is
#. printed.
#: src/http.c:1444
msgid "Length: "
msgstr "Length: "

#: src/http.c:1449
#, c-format
msgid " (%s to go)"
msgstr " (%s to go)"

#: src/http.c:1454
msgid "ignored"
msgstr "ignored"

#: src/http.c:1598
msgid "Warning: wildcards not supported in HTTP.\n"
msgstr "Warning: wildcards not supported in HTTP.\n"

#. If opt.noclobber is turned on and file already exists, do not
#. retrieve the file
#: src/http.c:1628
#, c-format
msgid "File `%s' already there, will not retrieve.\n"
msgstr "File `%s' already there, will not retrieve.\n"

#: src/http.c:1800
#, c-format
msgid "Cannot write to `%s' (%s).\n"
msgstr "Cannot write to `%s' (%s).\n"

#: src/http.c:1819
#, c-format
msgid "ERROR: Redirection (%d) without location.\n"
msgstr "ERROR: Redirection (%d) without location.\n"

#: src/http.c:1851
#, c-format
msgid "%s ERROR %d: %s.\n"
msgstr "%s ERROR %d: %s.\n"

#: src/http.c:1864
msgid "Last-modified header missing -- time-stamps turned off.\n"
msgstr "Last-modified header missing -- time-stamps turned off.\n"

#: src/http.c:1872
msgid "Last-modified header invalid -- time-stamp ignored.\n"
msgstr "Last-modified header invalid -- time-stamp ignored.\n"

#: src/http.c:1895
#, c-format
msgid ""
"Server file no newer than local file `%s' -- not retrieving.\n"
"\n"
msgstr ""
"Server file no newer than local file `%s' -- not retrieving.\n"
"\n"

#: src/http.c:1903
#, c-format
msgid "The sizes do not match (local %ld) -- retrieving.\n"
msgstr "The sizes do not match (local %ld) -- retrieving.\n"

#: src/http.c:1907
msgid "Remote file is newer, retrieving.\n"
msgstr "Remote file is newer, retrieving.\n"

#: src/http.c:1948
#, c-format
msgid ""
"%s (%s) - `%s' saved [%ld/%ld]\n"
"\n"
msgstr ""
"%s (%s) - `%s' saved [%ld/%ld]\n"
"\n"

#: src/http.c:1998
#, c-format
msgid "%s (%s) - Connection closed at byte %ld. "
msgstr "%s (%s) - Connection closed at byte %ld. "

#: src/http.c:2007
#, c-format
msgid ""
"%s (%s) - `%s' saved [%ld/%ld])\n"
"\n"
msgstr ""
"%s (%s) - `%s' saved [%ld/%ld])\n"
"\n"

#: src/http.c:2028
#, c-format
msgid "%s (%s) - Connection closed at byte %ld/%ld. "
msgstr "%s (%s) - Connection closed at byte %ld/%ld. "

#: src/http.c:2040
#, c-format
msgid "%s (%s) - Read error at byte %ld (%s)."
msgstr "%s (%s) - Read error at byte %ld (%s)."

#: src/http.c:2049
#, c-format
msgid "%s (%s) - Read error at byte %ld/%ld (%s). "
msgstr "%s (%s) - Read error at byte %ld/%ld (%s). "

#: src/init.c:342
#, c-format
msgid "%s: WGETRC points to %s, which doesn't exist.\n"
msgstr "%s: WGETRC points to %s, which doesn't exist.\n"

#: src/init.c:398 src/netrc.c:276
#, c-format
msgid "%s: Cannot read %s (%s).\n"
msgstr "%s: Cannot read %s (%s).\n"

#: src/init.c:416 src/init.c:422
#, c-format
msgid "%s: Error in %s at line %d.\n"
msgstr "%s: Error in %s at line %d.\n"

#: src/init.c:454
#, c-format
msgid "%s: Warning: Both system and user wgetrc point to `%s'.\n"
msgstr "%s: Warning: Both system and user wgetrc point to `%s'.\n"

#: src/init.c:594
#, c-format
msgid "%s: Invalid --execute command `%s'\n"
msgstr "%s: Invalid --execute command `%s'\n"

#: src/init.c:630
#, c-format
msgid "%s: %s: Invalid boolean `%s', use `on' or `off'.\n"
msgstr "%s: %s: Invalid boolean `%s', use `on' or `off'.\n"

#: src/init.c:673
#, c-format
msgid "%s: %s: Invalid boolean `%s', use always, on, off, or never.\n"
msgstr "%s: %s: Invalid boolean `%s', use always, on, off, or never.\n"

#: src/init.c:691
#, c-format
msgid "%s: %s: Invalid number `%s'.\n"
msgstr "%s: %s: Invalid number `%s'.\n"

#: src/init.c:930 src/init.c:949
#, c-format
msgid "%s: %s: Invalid byte value `%s'\n"
msgstr "%s: %s: Invalid byte value `%s'\n"

#: src/init.c:974
#, c-format
msgid "%s: %s: Invalid time period `%s'\n"
msgstr "%s: %s: Invalid time period `%s'\n"

#: src/init.c:1051
#, c-format
msgid "%s: %s: Invalid header `%s'.\n"
msgstr "%s: %s: Invalid header `%s'.\n"

#: src/init.c:1106
#, c-format
msgid "%s: %s: Invalid progress type `%s'.\n"
msgstr "%s: %s: Invalid progress type `%s'.\n"

#: src/init.c:1157
#, c-format
msgid "%s: %s: Invalid restriction `%s', use `unix' or `windows'.\n"
msgstr "%s: %s: Invalid restriction `%s', use `unix' or `windows'.\n"

#: src/init.c:1198
#, c-format
msgid "%s: %s: Invalid value `%s'.\n"
msgstr "%s: %s: Invalid value `%s'.\n"

#: src/log.c:636
#, c-format
msgid ""
"\n"
"%s received, redirecting output to `%s'.\n"
msgstr ""
"\n"
"%s received, redirecting output to `%s'.\n"

#. Eek!  Opening the alternate log file has failed.  Nothing we
#. can do but disable printing completely.
#: src/log.c:643
#, c-format
msgid "%s: %s; disabling logging.\n"
msgstr "%s: %s; disabling logging.\n"

#: src/main.c:127
#, c-format
msgid "Usage: %s [OPTION]... [URL]...\n"
msgstr "Usage: %s [OPTION]... [URL]...\n"

#: src/main.c:135
#, c-format
msgid "GNU Wget %s, a non-interactive network retriever.\n"
msgstr "GNU Wget %s, a non-interactive network retriever.\n"

#. Had to split this in parts, so the #@@#%# Ultrix compiler and cpp
#. don't bitch.  Also, it makes translation much easier.
#: src/main.c:140
msgid ""
"\n"
"Mandatory arguments to long options are mandatory for short options too.\n"
"\n"
msgstr ""
"\n"
"Mandatory arguments to long options are mandatory for short options too.\n"
"\n"

#: src/main.c:144
msgid ""
"Startup:\n"
"  -V,  --version           display the version of Wget and exit.\n"
"  -h,  --help              print this help.\n"
"  -b,  --background        go to background after startup.\n"
"  -e,  --execute=COMMAND   execute a `.wgetrc'-style command.\n"
"\n"
msgstr ""
"Startup:\n"
"  -V,  --version           display the version of Wget and exit.\n"
"  -h,  --help              print this help.\n"
"  -b,  --background        go to background after startup.\n"
"  -e,  --execute=COMMAND   execute a `.wgetrc'-style command.\n"
"\n"

#: src/main.c:151
msgid ""
"Logging and input file:\n"
"  -o,  --output-file=FILE     log messages to FILE.\n"
"  -a,  --append-output=FILE   append messages to FILE.\n"
"  -d,  --debug                print debug output.\n"
"  -q,  --quiet                quiet (no output).\n"
"  -v,  --verbose              be verbose (this is the default).\n"
"  -nv, --non-verbose          turn off verboseness, without being quiet.\n"
"  -i,  --input-file=FILE      download URLs found in FILE.\n"
"  -F,  --force-html           treat input file as HTML.\n"
"  -B,  --base=URL             prepends URL to relative links in -F -i file.\n"
"\n"
msgstr ""
"Logging and input file:\n"
"  -o,  --output-file=FILE     log messages to FILE.\n"
"  -a,  --append-output=FILE   append messages to FILE.\n"
"  -d,  --debug                print debug output.\n"
"  -q,  --quiet                quiet (no output).\n"
"  -v,  --verbose              be verbose (this is the default).\n"
"  -nv, --non-verbose          turn off verboseness, without being quiet.\n"
"  -i,  --input-file=FILE      download URLs found in FILE.\n"
"  -F,  --force-html           treat input file as HTML.\n"
"  -B,  --base=URL             prepends URL to relative links in -F -i file.\n"
"\n"

#: src/main.c:163
msgid ""
"Download:\n"
"  -t,  --tries=NUMBER           set number of retries to NUMBER (0 unlimits).\n"
"       --retry-connrefused      retry even if connection is refused.\n"
"  -O   --output-document=FILE   write documents to FILE.\n"
"  -nc, --no-clobber             don't clobber existing files or use .# suffixes.\n"
"  -c,  --continue               resume getting a partially-downloaded file.\n"
"       --progress=TYPE          select progress gauge type.\n"
"  -N,  --timestamping           don't re-retrieve files unless newer than local.\n"
"  -S,  --server-response        print server response.\n"
"       --spider                 don't download anything.\n"
"  -T,  --timeout=SECONDS        set all timeout values to SECONDS.\n"
"       --dns-timeout=SECS       set the DNS lookup timeout to SECS.\n"
"       --connect-timeout=SECS   set the connect timeout to SECS.\n"
"       --read-timeout=SECS      set the read timeout to SECS.\n"
"  -w,  --wait=SECONDS           wait SECONDS between retrievals.\n"
"       --waitretry=SECONDS      wait 1...SECONDS between retries of a retrieval.\n"
"       --random-wait            wait from 0...2*WAIT secs between retrievals.\n"
"  -Y,  --proxy=on/off           turn proxy on or off.\n"
"  -Q,  --quota=NUMBER           set retrieval quota to NUMBER.\n"
"       --bind-address=ADDRESS   bind to ADDRESS (hostname or IP) on local host.\n"
"       --limit-rate=RATE        limit download rate to RATE.\n"
"       --dns-cache=off          disable caching DNS lookups.\n"
"       --restrict-file-names=OS restrict chars in file names to ones OS allows.\n"
"\n"
msgstr ""
"Download:\n"
"  -t,  --tries=NUMBER           set number of retries to NUMBER (0 unlimits).\n"
"       --retry-connrefused      retry even if connection is refused.\n"
"  -O   --output-document=FILE   write documents to FILE.\n"
"  -nc, --no-clobber             don't clobber existing files or use .# suffixes.\n"
"  -c,  --continue               resume getting a partially-downloaded file.\n"
"       --progress=TYPE          select progress gauge type.\n"
"  -N,  --timestamping           don't re-retrieve files unless newer than local.\n"
"  -S,  --server-response        print server response.\n"
"       --spider                 don't download anything.\n"
"  -T,  --timeout=SECONDS        set all timeout values to SECONDS.\n"
"       --dns-timeout=SECS       set the DNS lookup timeout to SECS.\n"
"       --connect-timeout=SECS   set the connect timeout to SECS.\n"
"       --read-timeout=SECS      set the read timeout to SECS.\n"
"  -w,  --wait=SECONDS           wait SECONDS between retrievals.\n"
"       --waitretry=SECONDS      wait 1...SECONDS between retries of a retrieval.\n"
"       --random-wait            wait from 0...2*WAIT secs between retrievals.\n"
"  -Y,  --proxy=on/off           turn proxy on or off.\n"
"  -Q,  --quota=NUMBER           set retrieval quota to NUMBER.\n"
"       --bind-address=ADDRESS   bind to ADDRESS (hostname or IP) on local host.\n"
"       --limit-rate=RATE        limit download rate to RATE.\n"
"       --dns-cache=off          disable caching DNS lookups.\n"
"       --restrict-file-names=OS restrict chars in file names to ones OS allows.\n"
"\n"

#: src/main.c:188
msgid ""
"Directories:\n"
"  -nd, --no-directories            don't create directories.\n"
"  -x,  --force-directories         force creation of directories.\n"
"  -nH, --no-host-directories       don't create host directories.\n"
"  -P,  --directory-prefix=PREFIX   save files to PREFIX/...\n"
"       --cut-dirs=NUMBER           ignore NUMBER remote directory components.\n"
"\n"
msgstr ""
"Directories:\n"
"  -nd, --no-directories            don't create directories.\n"
"  -x,  --force-directories         force creation of directories.\n"
"  -nH, --no-host-directories       don't create host directories.\n"
"  -P,  --directory-prefix=PREFIX   save files to PREFIX/...\n"
"       --cut-dirs=NUMBER           ignore NUMBER remote directory components.\n"
"\n"

#: src/main.c:196
msgid ""
"HTTP options:\n"
"       --http-user=USER      set http user to USER.\n"
"       --http-passwd=PASS    set http password to PASS.\n"
"  -C,  --cache=on/off        (dis)allow server-cached data (normally allowed).\n"
"  -E,  --html-extension      save all text/html documents with .html extension.\n"
"       --ignore-length       ignore `Content-Length' header field.\n"
"       --header=STRING       insert STRING among the headers.\n"
"       --proxy-user=USER     set USER as proxy username.\n"
"       --proxy-passwd=PASS   set PASS as proxy password.\n"
"       --referer=URL         include `Referer: URL' header in HTTP request.\n"
"  -s,  --save-headers        save the HTTP headers to file.\n"
"  -U,  --user-agent=AGENT    identify as AGENT instead of Wget/VERSION.\n"
"       --no-http-keep-alive  disable HTTP keep-alive (persistent connections).\n"
"       --cookies=off         don't use cookies.\n"
"       --load-cookies=FILE   load cookies from FILE before session.\n"
"       --save-cookies=FILE   save cookies to FILE after session.\n"
"       --post-data=STRING    use the POST method; send STRING as the data.\n"
"       --post-file=FILE      use the POST method; send contents of FILE.\n"
"\n"
msgstr ""
"HTTP options:\n"
"       --http-user=USER      set http user to USER.\n"
"       --http-passwd=PASS    set http password to PASS.\n"
"  -C,  --cache=on/off        (dis)allow server-cached data (normally allowed).\n"
"  -E,  --html-extension      save all text/html documents with .html extension.\n"
"       --ignore-length       ignore `Content-Length' header field.\n"
"       --header=STRING       insert STRING among the headers.\n"
"       --proxy-user=USER     set USER as proxy username.\n"
"       --proxy-passwd=PASS   set PASS as proxy password.\n"
"       --referer=URL         include `Referer: URL' header in HTTP request.\n"
"  -s,  --save-headers        save the HTTP headers to file.\n"
"  -U,  --user-agent=AGENT    identify as AGENT instead of Wget/VERSION.\n"
"       --no-http-keep-alive  disable HTTP keep-alive (persistent connections).\n"
"       --cookies=off         don't use cookies.\n"
"       --load-cookies=FILE   load cookies from FILE before session.\n"
"       --save-cookies=FILE   save cookies to FILE after session.\n"
"       --post-data=STRING    use the POST method; send STRING as the data.\n"
"       --post-file=FILE      use the POST method; send contents of FILE.\n"
"\n"

#: src/main.c:217
msgid ""
"HTTPS (SSL) options:\n"
"       --sslcertfile=FILE     optional client certificate.\n"
"       --sslcertkey=KEYFILE   optional keyfile for this certificate.\n"
"       --egd-file=FILE        file name of the EGD socket.\n"
"       --sslcadir=DIR         dir where hash list of CA's are stored.\n"
"       --sslcafile=FILE       file with bundle of CA's\n"
"       --sslcerttype=0/1      Client-Cert type 0=PEM (default) / 1=ASN1 (DER)\n"
"       --sslcheckcert=0/1     Check the server cert agenst given CA\n"
"       --sslprotocol=0-3      choose SSL protocol; 0=automatic,\n"
"                              1=SSLv2 2=SSLv3 3=TLSv1\n"
"\n"
msgstr ""
"HTTPS (SSL) options:\n"
"       --sslcertfile=FILE     optional client certificate.\n"
"       --sslcertkey=KEYFILE   optional keyfile for this certificate.\n"
"       --egd-file=FILE        file name of the EGD socket.\n"
"       --sslcadir=DIR         dir where hash list of CA's are stored.\n"
"       --sslcafile=FILE       file with bundle of CA's\n"
"       --sslcerttype=0/1      Client-Cert type 0=PEM (default) / 1=ASN1 (DER)\n"
"       --sslcheckcert=0/1     Check the server cert agenst given CA\n"
"       --sslprotocol=0-3      choose SSL protocol; 0=automatic,\n"
"                              1=SSLv2 2=SSLv3 3=TLSv1\n"
"\n"

#: src/main.c:230
msgid ""
"FTP options:\n"
"  -nr, --dont-remove-listing   don't remove `.listing' files.\n"
"  -g,  --glob=on/off           turn file name globbing on or off.\n"
"       --passive-ftp           use the \"passive\" transfer mode.\n"
"       --retr-symlinks         when recursing, get linked-to files (not dirs).\n"
"\n"
msgstr ""
"FTP options:\n"
"  -nr, --dont-remove-listing   don't remove `.listing' files.\n"
"  -g,  --glob=on/off           turn file name globbing on or off.\n"
"       --passive-ftp           use the \"passive\" transfer mode.\n"
"       --retr-symlinks         when recursing, get linked-to files (not dirs).\n"
"\n"

#: src/main.c:237
msgid ""
"Recursive retrieval:\n"
"  -r,  --recursive          recursive download.\n"
"  -l,  --level=NUMBER       maximum recursion depth (inf or 0 for infinite).\n"
"       --delete-after       delete files locally after downloading them.\n"
"  -k,  --convert-links      convert non-relative links to relative.\n"
"  -K,  --backup-converted   before converting file X, back up as X.orig.\n"
"  -m,  --mirror             shortcut option equivalent to -r -N -l inf -nr.\n"
"  -p,  --page-requisites    get all images, etc. needed to display HTML page.\n"
"       --strict-comments    turn on strict (SGML) handling of HTML comments.\n"
"\n"
msgstr ""
"Recursive retrieval:\n"
"  -r,  --recursive          recursive download.\n"
"  -l,  --level=NUMBER       maximum recursion depth (inf or 0 for infinite).\n"
"       --delete-after       delete files locally after downloading them.\n"
"  -k,  --convert-links      convert non-relative links to relative.\n"
"  -K,  --backup-converted   before converting file X, back up as X.orig.\n"
"  -m,  --mirror             shortcut option equivalent to -r -N -l inf -nr.\n"
"  -p,  --page-requisites    get all images, etc. needed to display HTML page.\n"
"       --strict-comments    turn on strict (SGML) handling of HTML comments.\n"
"\n"

#: src/main.c:248
msgid ""
"Recursive accept/reject:\n"
"  -A,  --accept=LIST                comma-separated list of accepted extensions.\n"
"  -R,  --reject=LIST                comma-separated list of rejected extensions.\n"
"  -D,  --domains=LIST               comma-separated list of accepted domains.\n"
"       --exclude-domains=LIST       comma-separated list of rejected domains.\n"
"       --follow-ftp                 follow FTP links from HTML documents.\n"
"       --follow-tags=LIST           comma-separated list of followed HTML tags.\n"
"  -G,  --ignore-tags=LIST           comma-separated list of ignored HTML tags.\n"
"  -H,  --span-hosts                 go to foreign hosts when recursive.\n"
"  -L,  --relative                   follow relative links only.\n"
"  -I,  --include-directories=LIST   list of allowed directories.\n"
"  -X,  --exclude-directories=LIST   list of excluded directories.\n"
"  -np, --no-parent                  don't ascend to the parent directory.\n"
"\n"
msgstr ""
"Recursive accept/reject:\n"
"  -A,  --accept=LIST                comma-separated list of accepted extensions.\n"
"  -R,  --reject=LIST                comma-separated list of rejected extensions.\n"
"  -D,  --domains=LIST               comma-separated list of accepted domains.\n"
"       --exclude-domains=LIST       comma-separated list of rejected domains.\n"
"       --follow-ftp                 follow FTP links from HTML documents.\n"
"       --follow-tags=LIST           comma-separated list of followed HTML tags.\n"
"  -G,  --ignore-tags=LIST           comma-separated list of ignored HTML tags.\n"
"  -H,  --span-hosts                 go to foreign hosts when recursive.\n"
"  -L,  --relative                   follow relative links only.\n"
"  -I,  --include-directories=LIST   list of allowed directories.\n"
"  -X,  --exclude-directories=LIST   list of excluded directories.\n"
"  -np, --no-parent                  don't ascend to the parent directory.\n"
"\n"

#: src/main.c:263
msgid "Mail bug reports and suggestions to <bug-wget@gnu.org>.\n"
msgstr "Mail bug reports and suggestions to <bug-wget@gnu.org>.\n"

#: src/main.c:465
#, c-format
msgid "%s: debug support not compiled in.\n"
msgstr "%s: debug support not compiled in.\n"

#: src/main.c:517
msgid "Copyright (C) 2003 Free Software Foundation, Inc.\n"
msgstr "Copyright (C) 2003 Free Software Foundation, Inc.\n"

#: src/main.c:519
msgid ""
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
msgstr ""
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public Licence for more details.\n"

#: src/main.c:524
msgid ""
"\n"
"Originally written by Hrvoje Niksic <hniksic@xemacs.org>.\n"
msgstr ""
"\n"
"Originally written by Hrvoje Niksic <hniksic@xemacs.org>.\n"

#: src/main.c:703
#, c-format
msgid "%s: illegal option -- `-n%c'\n"
msgstr "%s: illegal option -- `-n%c'\n"

#. #### Something nicer should be printed here -- similar to the
#. pre-1.5 `--help' page.
#: src/main.c:706 src/main.c:748 src/main.c:794
#, c-format
msgid "Try `%s --help' for more options.\n"
msgstr "Try `%s --help' for more options.\n"

#: src/main.c:774
msgid "Can't be verbose and quiet at the same time.\n"
msgstr "Can't be verbose and quiet at the same time.\n"

#: src/main.c:780
msgid "Can't timestamp and not clobber old files at the same time.\n"
msgstr "Can't timestamp and not clobber old files at the same time.\n"

#. No URL specified.
#: src/main.c:789
#, c-format
msgid "%s: missing URL\n"
msgstr "%s: missing URL\n"

#: src/main.c:905
#, c-format
msgid "No URLs found in %s.\n"
msgstr "No URLs found in %s.\n"

#: src/main.c:914
#, c-format
msgid ""
"\n"
"FINISHED --%s--\n"
"Downloaded: %s bytes in %d files\n"
msgstr ""
"\n"
"FINISHED --%s--\n"
"Downloaded: %s bytes in %d files\n"

#: src/main.c:920
#, c-format
msgid "Download quota (%s bytes) EXCEEDED!\n"
msgstr "Download quota (%s bytes) EXCEEDED!\n"

#: src/mswindows.c:147
msgid "Continuing in background.\n"
msgstr "Continuing in background.\n"

#: src/mswindows.c:149 src/utils.c:487
#, c-format
msgid "Output will be written to `%s'.\n"
msgstr "Output will be written to `%s'.\n"

#: src/mswindows.c:245
#, c-format
msgid "Starting WinHelp %s\n"
msgstr "Starting WinHelp %s\n"

#: src/mswindows.c:272 src/mswindows.c:279
#, c-format
msgid "%s: Couldn't find usable socket driver.\n"
msgstr "%s: Couldn't find usable socket driver.\n"

#: src/netrc.c:380
#, c-format
msgid "%s: %s:%d: warning: \"%s\" token appears before any machine name\n"
msgstr "%s: %s:%d: warning: \"%s\" token appears before any machine name\n"

#: src/netrc.c:411
#, c-format
msgid "%s: %s:%d: unknown token \"%s\"\n"
msgstr "%s: %s:%d: unknown token \"%s\"\n"

#: src/netrc.c:475
#, c-format
msgid "Usage: %s NETRC [HOSTNAME]\n"
msgstr "Usage: %s NETRC [HOSTNAME]\n"

#: src/netrc.c:485
#, c-format
msgid "%s: cannot stat %s: %s\n"
msgstr "%s: cannot stat %s: %s\n"

#. Align the [ skipping ... ] line with the dots.  To do
#. that, insert the number of spaces equal to the number of
#. digits in the skipped amount in K.
#: src/progress.c:234
#, c-format
msgid ""
"\n"
"%*s[ skipping %dK ]"
msgstr ""
"\n"
"%*s[ skipping %dK ]"

#: src/progress.c:401
#, c-format
msgid "Invalid dot style specification `%s'; leaving unchanged.\n"
msgstr "Invalid dot style specification `%s'; leaving unchanged.\n"

#: src/recur.c:378
#, c-format
msgid "Removing %s since it should be rejected.\n"
msgstr "Removing %s since it should be rejected.\n"

#: src/res.c:549
msgid "Loading robots.txt; please ignore errors.\n"
msgstr "Loading robots.txt; please ignore errors.\n"

#: src/retr.c:400
#, c-format
msgid "Error parsing proxy URL %s: %s.\n"
msgstr "Error parsing proxy URL %s: %s.\n"

#: src/retr.c:408
#, c-format
msgid "Error in proxy URL %s: Must be HTTP.\n"
msgstr "Error in proxy URL %s: Must be HTTP.\n"

#: src/retr.c:493
#, c-format
msgid "%d redirections exceeded.\n"
msgstr "%d redirections exceeded.\n"

#: src/retr.c:617
msgid ""
"Giving up.\n"
"\n"
msgstr ""
"Giving up.\n"
"\n"

#: src/retr.c:617
msgid ""
"Retrying.\n"
"\n"
msgstr ""
"Retrying.\n"
"\n"

#: src/url.c:621
msgid "No error"
msgstr "No error"

#: src/url.c:623
msgid "Unsupported scheme"
msgstr "Unsupported scheme"

#: src/url.c:625
msgid "Empty host"
msgstr "Empty host"

#: src/url.c:627
msgid "Bad port number"
msgstr "Bad port number"

#: src/url.c:629
msgid "Invalid user name"
msgstr "Invalid user name"

#: src/url.c:631
msgid "Unterminated IPv6 numeric address"
msgstr "Unterminated IPv6 numeric address"

#: src/url.c:633
msgid "IPv6 addresses not supported"
msgstr "IPv6 addresses not supported"

#: src/url.c:635
msgid "Invalid IPv6 numeric address"
msgstr "Invalid IPv6 numeric address"

#: src/utils.c:120
#, c-format
msgid "%s: %s: Not enough memory.\n"
msgstr "%s: %s: Not enough memory.\n"

#. parent, no error
#: src/utils.c:485
#, c-format
msgid "Continuing in background, pid %d.\n"
msgstr "Continuing in background, pid %d.\n"

#: src/utils.c:529
#, c-format
msgid "Failed to unlink symlink `%s': %s\n"
msgstr "Failed to unlink symlink `%s': %s\n"
