# -*- coding: utf-8 -*-
###
# Copyright (c) 2012, François Revol
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#   * Redistributions of source code must retain the above copyright notice,
#     this list of conditions, and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions, and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#   * Neither the name of the author of this software nor the name of
#     contributors to this software may be used to endorse or promote products
#     derived from this software without specific prior written consent.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
###

import supybot.conf as conf
import supybot.utils as utils
from supybot.commands import *
import supybot.callbacks as callbacks
import supybot.log as log

import random

import errors

class Haiku(callbacks.Plugin):
    def __init__(self, irc):
        #log.error("plop")
        #print "plop"
        self.fortunes = []
        fortunes = open("/home/revol/devel/haiku/trunk/data/system/data/fortunes/Haiku", "r")
        print fortunes
        if fortunes:
            fortune = ""
            for line in fortunes:
                if line == "%\n":
                    self.fortunes.append(fortune)
                    fortune = ""
                else:
                    fortune += line
            fortunes.close()

        self.__parent = super(Haiku, self)
        self.__parent.__init__(irc)

    def error(self, irc, msg, args):
        """<error>

        Returns the code and value for the given <error> value or code.
        """
        print "Haiku: error"
        print args
        if not args:
            raise callbacks.ArgumentError
        for arg in args:
            err = None
            if arg in errors.errors:
                err = errors.errors[arg]
                key = arg
            else:
                try:
                    i = int(arg, 0)
                except ValueError:
                    irc.errorInvalid('argument', arg, Raise=True)
                for e in errors.errors:
                    if errors.errors[e]['value'] == i:
                        err = errors.errors[e]
                        key = e
            if err is None:
                irc.reply("Unknown error '%s'" % (arg))
            else:
                expr = err['expr']
                value = err['value']
                r = "%s = %s = 0x%x (%d): %s" % (key, expr, int(value), value, err['str'])
                irc.reply(r)

    def haiku(self, irc, msg, args):
        """

        Returns a random haiku.
        """

        if len(self.fortunes) > 0:
            h = self.fortunes[int(random.random() * len(self.fortunes))].split('\n')
            for l in h:
                if l != "":
                    irc.reply(l, prefixNick = False)
        else:
            irc.reply("No haiku found")
        
    def reportbugs(self, irc, msg, args):
        """

        Returns some infos on reporting bugs.
        """

        to = None
        if len(args) > 0:
            to = args[0]
        t = "If you think there is a bug in Haiku, please report a bug so we can remember to fix it. cf. http://dev.haiku-os.org/wiki/ReportingBugs"
        irc.reply(t, to = to)

    def patchwanted(self, irc, msg, args):
        """

        Returns some infos on submitting patches.
        """

        to = None
        if len(args) > 0:
            to = args[0]
        t = "Haiku is Free Software, therefore you can fix it yourself and send a patch, which would likely be very appreciated. cf. http://dev.haiku-os.org/wiki/SubmittingPatches"
        irc.reply(t, to = to)

    def download(self, irc, msg, args):
        """

        Returns download links.
        """

        to = None
        if len(args) > 0:
            to = args[0]
        t = "Current release: http://www.haiku-os.org/get-haiku - Nightly builds: http://haiku-files.org/haiku/development/"
        irc.reply(t, to = to)

    def dl(self, irc, msg, args):
        """

        Returns download links.
        """
        return self.download(irc, msg, args)

    def vi(self, irc, msg, args):
        """

        Trolls.
        """

        to = None
        if len(args) > 0:
            to = args[0]
        t = "emacs!"
        irc.reply(t, to = to)

    def emacs(self, irc, msg, args):
        """

        Trolls.
        """

        to = None
        if len(args) > 0:
            to = args[0]
        t = "vi!"
        irc.reply(t, to = to)

    def bored(self, irc, msg, args):
        """

        Returns some infos.
        """

        to = None
        if len(args) > 0:
            to = args[0]
        t = "Why won't you get on some easy tasks then? http://dev.haiku-os.org/wiki/EasyTasks"
        irc.reply(t, to = to)

    def jlg(self, irc, msg, args):
        """

        Praise Jean-Louis Gassée.
        """

        to = None
        if len(args) > 0:
            to = args[0]
        t = "Jean-Louis Gassée, such a Grand Fromage!"
        irc.reply(t, to = to)

    def basement(self, irc, msg, args):
        """

        Puts people to work.
        """

        to = "everyone"
        who = "them"
        if len(args) > 0:
            to = args[0]
            who = "him"
            if len(args) > 1:
                to = ' and '.join([', '.join(args[:-1]), args[-1]])
                who = "them"
        t = "sends %s to BGA's basement and chains %s to a post in front of a computer. Get to work!" % (to, who)
        irc.reply(t, action = True)

    def trout(self, irc, msg, args):
        """

        Puts someone back to his place.
        """

        if len(args) > 0:
            to = args[0]
            if len(args) > 1:
                to = ' and '.join([', '.join(args[:-1]), args[-1]])
            t = "slaps %s with a trout." % (to)
            irc.reply(t, action = True)

    def plop(self, irc, msg, args):
        """

        Returns some infos.
        """

        to = None
        if len(args) > 0:
            to = args[0]
        t = "plop"
        irc.reply(t, to = to)

    def shibboleet(self, irc, msg, args):
        """

        Tech support.
        """

        irc.reply("http://xkcd.com/806/ Tech Support")

Class = Haiku

# vim:set shiftwidth=4 softtabstop=4 expandtab textwidth=79:
