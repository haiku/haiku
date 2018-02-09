===============================
Boot Volume Directory Structure
===============================
This is the directory layout of the boot volume::

  home/config
  	<like system, but without haiku_loader, kernel_<arch>, and runtime_loader>

  system
  	add-ons
  	apps
  	bin
  	boot
  	cache*
  	data
  	demos
  	develop
  	documentation
  	lib
  	non-packaged*
  	packages*
  	preferences
  	servers
  	settings*
  	var*

  	haiku_loader
  	kernel_<arch>
  	runtime_loader

  trash

The structure mostly equals the pre-package management directory structure with
the following changes:

- ``common`` has been removed, or more correctly it has been merged into
  ``system``. All system-wide software is now installed (only) in ``system``.
- The ``develop`` directory has been removed and its contents has been moved to
  the ``system/develop`` directory.
- The ``include`` directory has been removed. Its contents lives in
  ``develop/headers`` now.
- ``optional`` has been removed. Optional features can just be installed via the
  package manager.
- ``share`` and ``etc`` (in ``common``) have been removed. Their contents goes
  to ``data``, ``documentation``, or ``settings`` (in ``system`` or, for
  packages installed there, in ``home``) as appropriate. There's
  ``settings/etc`` which is where ported Unix software will usually store their
  global settings.
- ``apps`` and ``preferences`` have been moved to ``system`` for consistency.
- ``system`` and ``home/config`` each sport a ``packages`` directory, which
  contains the activated packages.
- ``system`` and ``home/config`` themselves are mount points for two instances
  of the packagefs, i.e. each contains the virtually extracted contents of the
  activated packages in the respective ``packages`` subdirectory. The
  directories marked with ``*`` are "shine-through" directories. They are not
  provided by the packagefs, but are the underlying directories of the boot
  volume. Unlike the other directories they are writable.
- ``system`` and ``home/config`` each contain a directory ``non-packaged``
  which has the same structure as their parent directory minus the shine-through
  directories. In the ``non-packaged`` directories software can be installed the
  traditional -- non-packaged -- way.
