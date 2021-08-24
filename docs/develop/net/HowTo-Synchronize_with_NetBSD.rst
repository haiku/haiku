How to Merge Patches from NetBSD Trunk
======================================

Using the NetBSD CVS is a pain, so instead, the preferred thing to do is
to use `the official Git mirror <https://github.com/NetBSD/src>`__. The
code here is in the tree at a few places:\ ``inet`` is at
``lib/libc/inet``, irs is scattered across the tree, and ``resolv`` is
at ``lib/libc/resolv``.

The preferable way to merge is to take the last commit merged from IIJ’s
mirror (can be found in the merging commit in Haiku, if the merger has
done their work properly) and check all commits since then to see if
they apply or not (some apply to documentation we don’t have, etc.)
Cherry-pick the ones that do, and download them as git-format-patch
patches (by adding ``.patch`` onto the end of the commit URL).

To convert the patches to have the correct paths to the resolv/inet/etc.
code, use ``sed``:

::

   sed s%lib/libc/resolv%src/kits/network/netresolv/resolv%g -i *.patch

(You’ll need to use similar commands for the ``inet`` and ``irs`` code.)

Then apply the patches using ``git apply --reject file.patch``. Git will
spew a lot of errors about files in the patch that aren’t in the tree,
and then it will warn that some hunks are being rejected. Review the
rejected hunks **VERY CAREFULLY**, as some code in Haiku’s NetResolv is
not in NetBSD’s and vice versa, and so some patches may not apply
cleanly because of that. You might have to resort to merging those hunks
by hand, if they apply at all to Haiku’s code.

Commit the changes all at once, but list all the commits merged from
NetBSD in the commit message (see previous merges for the style to
follow).
