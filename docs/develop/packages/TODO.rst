These are the TODO items for the Haiku **Package Management**.

packagefs
=========
- If necessary, add a caching mechanism to speed up mounting it.

Package Daemon
==============
- Complete support for extracting and updating settings files: Merge support and
  user feedback are still missing.
- Add user notification/interaction support for initial verification
  (on start-up).
- Support packages being copied to the ``packages`` directory. Currently only
  moving works.

Package building
================
- Define packaging guidelines and create a tool to check packages against those.

  - Status: Here's the `wiki page`_ defining the policy. It's still a work in
    progress. haikuporter has some policy checking built in already.

    .. _wiki page: PackagingPolicy.rst

Package kit/manager
===================
- Add system update support.

  - Status: Mostly functional in pkgman (``pkgman full-sync``), but unsupported
    in HaikuDepot.

Boot loader
===========
- Safe mode/recovery options:

  - Disable packages installed in home.

Package/package repository format
=================================
- Add localization support. More_ info_.

  .. _More: http://www.freelists.org/post/haiku-depot-web/Title-localization,18
  .. _info: http://www.freelists.org/post/haiku-depot-web/Title-localization,29

- Add support for repository keys (public/private) and package
  signing/checksums, so that it is possible to verify that data retrieved from a
  repository have not been tampered with.
- Add package sizes to repository. Since there are plans to support xz (or other
  high-ratio formats) compressed uncompressed packages for download that are
  recompressed for installation, we probably need to discriminate between
  download and installation sizes.
- Add support for a faster compression format (e.g. lz4_). As it turns out,
  the currently used zlib compression is rather slow (slower than reading
  uncompressed data from a slow HD).

  .. _lz4: https://lz4.github.io/lz4/

- Add MIME info for types supported by packaged applications
  (and sniffer rules?) to the package attributes.
