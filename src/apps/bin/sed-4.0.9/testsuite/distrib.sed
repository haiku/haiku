#   This is straight out of C News
#
#
# All this does is massage the headers so they look like what news
# software expects.  To:, Cc: and Resent-*: headers are masked.
# Reply-To: is turned into references, which is questionable (could
# just as well be dropped.
#
# The From: line is rewritten to use the "address (comments)" form
# instead of "phrase <route>" form our mailer uses.  Also, addresses
# with no "@domainname" are assumed to originate locally, and so are
# given a domain.
#
# The Sender: field below reflects the address of the person who
# maintains our mailing lists.  The Approved: field is in a special
# form, so that we can do bidirectional gatewaying.  Any message
# in a newsgroup that bears this stamp will not be fed into the
# matching mailing list.

1i\
Path: mailnewsgateway
	:a
	/^[Rr]eceived:/b r
	/^[Nn]ewsgroups:/b r
	/^[Pp]ath:/b r
	/^[Tt][Oo]:/s/^/Original-/
	/^[Cc][Cc]:/s/^/Original-/
	/^[Rr][Ee][Ss][Ee][Nn][Tt]-.*/s/^/Original-/
	/^[Mm][Ee][Ss][Ss][Aa][Gg][Ee]-[Ii][Dd]:/s/@/.alt.buddha.fat.short.guy@/
	s/^[Ii]n-[Rr]eply-[Tt]o:/References:/
	/^From:/{
		s/<\([^@]*\)>$/<\1@$thissite>/
		s/^From:[ 	][	]*\(.*\)  *<\(.*\)>$/From: \2 (\1)/
		}
	s/-[Ii]d:/-ID:/
	s/^[Ss][Uu][Bb][Jj][Ee][Cc][Tt]:[ 	]*$/Subject: (none)/
	s/^\([^:]*:\)[	 ]*/\1 /
	/^$/{i\
Newsgroups: alt.buddha.short.fat.guy\
Distribution: world\
Sender: news@cygnus.com\
Approved: alt.buddha.short.fat.guy@cygnus.com
	b e
	}
	p
	n
	b a
	:r
	s/.*//g
	n
	/^[ 	]/b r
	b a
	:e
	p
	n
	b e
