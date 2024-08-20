Release Cookbook
===================

This page documents the steps needed during a release.

Community Concensus
-------------------

* Determine a release is needed
* Gain support in the community
    * Review the list of Trac tickets in the milestone. There should be no blockers and ideally no critical tickets
    * Decide if some of the tickets can be lowered in priority or moved to the next release
* Raise release proposal on mailing list (haiku-development)
    * Select Release Coordinator
    * Plan timeline (which should be made up of milestones below)
    * Allow one week RFC time for comments

Each of the steps below should be announced on the mailing list to remind everyone where we're at.

Scramble. Enhancement Deadline
------------------------------

**Time:** ~ 1 week

* Roughly one week for people to finish up their enhancements for *(RELEASE)*
* Avoid merging changes that break the API, so 3rd-party applications are ready to build with the API matching the release
* Decide if any haikuports packages should be added or removed from the release image
* Final ICU upgrades, webkit upgrades, etc.
* Plan for release logo used in installer (always fun, lots of bikeshed)
    * Prepare this in advance if possible, if you want to change the logo, this is the worst time to do it.
* Set up the `Trac wiki pages <https://dev.haiku-os.org/wiki/R1/ReleaseRoadMap>`_ for the release
    * Start writing or updating the release notes and press release

Branch
------

**Time:** ~ 1 week

* Update the version constants in master (`example: hrev52295 <https://git.haiku-os.org/haiku/commit/?h=hrev52295>`_)
* Branch haiku and buildtools (git push origin master:r1beta1)
    * Update the version constants in the branch (`example <https://git.haiku-os.org/haiku/commit/?h=r1beta1&id=b5c9e6620ee731bd33d8cb3ef6ac01749122b6b3>`_)
    * Update copyright years in the `bootloader menu <https://git.haiku-os.org/haiku/tree/src/system/boot/platform/generic/text_menu.cpp#n212>`_
    * Disable serial debug output in bootloader and kernel config file (`example <https://git.haiku-os.org/haiku/commit/?h=r1beta5&id=9d0312eb00a75051275accf9967ddc1c64154334>`_)
    * Turn KDEBUG_LEVEL down to 1, for performance reasons (`example <https://git.haiku-os.org/haiku/commit/?h=r1beta1&id=6db6c0b275f684d0b25d49e87d5183e40c7cd4ec>`_)
    * Enable ``HAIKU_OFFICIAL_RELEASE`` (`example <https://git.haiku-os.org/haiku/commit/?h=r1beta1&id=ff2059f2bd001bba84b980617e9bdf4dc6a46799>`_), and update logos
    * Update both package repos to use the branch's repos (`example <https://git.haiku-os.org/haiku/commit/?h=r1beta4&id=b9c0fea70a1fd7edc396e0e6992b77a7c5a3b4f8>`_)

Configure CI/CD Pipelines
-------------------------

Once your code is branched, you can begin setting up CI/CD pipelines in concourse

* Bump the "last vs current" releases in CI/CD and deploy:
  https://github.com/haiku/infrastructure/blob/master/concourse/deploy.sh#L29
* Run a build of *(RELEASE)*/toolchain-*(RELEASE)* to generate the intial toolchain containers
* Build each architecture first repo + image from the branch

Testing
-------

**Time:** ~ 2 weekes

* Begin building "Test Candidate" (TC0, TC1, TC2, etc) images for target architectures (x86_gcc2h, x86_64)
  * Test Candidate (TC) builds should be clearly labeled and made available
* Agressively market Test Candidate builds for *(RELEASE)*
* Bugs should be opened on Trac under the *new* version (make sure it is available in Trac admin pages).
* **Release Coordinator** should be downright obnoxious about people testing TC images!
* Have people test on as much hardware as possible to find issues
* Use Gerrit "cherry-pick" function to propose a change for inclusion in the release branch

Finalization
------------

**Time:** ~ 1 week

* Synchronize **i18n** strings and userguide
* Produce **Release Candidate** images
* **Tag** the release candidate builds on the brach (rc0)
* Ensure release notes and press release are almost done.
* More testing!
* When you have decided that an RC is actually the release, tag it in git

Distribution
------------

**Time:** ~ 1 week

* You now have the release images in hand! (RCX is secretly the release)
    * Keep it to yourself and don't tell people
* Generate Torrents, seed.  Get a few other people to seed.
* Place onto wasabi s3 under releases in final layout (be consistent!)
* Move to releases onto IPFS, pin and use pinning services
* Prepare release-files-directory::

   [release-name]
    |--md5sums.txt (of compressed and uncompressed release-image-files)
    |--release_notes_[release-name].txt
    |--[release-image-files]  (both as .zip and .tar.xz)
    |--[release-image-files].torrent (of just the .zip's)
    |--[release-name]/sources/   (all source archives should be .tar.xz)
         |--haiku-[release-name]-src-[YYYY-MM-DD]
         |--haiku-[release-name]-buildtools-src-[YYYY-MM-DD]
         |--[all optional packages]

* rsync release-files-directory to /files/releases/[release-name]
* rsync release-files-directory to baron:/srv/rsync/haiku-mirror-seed/releases/[release-name]/ (the 3rd-party rsync mirrors will automatically mirror the files)
* Give mirrors time to... mirror via rsync
* Tell Distrowatch: http://distrowatch.com/table.php?distribution=haiku (?)
* Update website references.
    * Double check listed mirrors have release
    * Comment out any mirrors which don't have it (a few missing is fine)
    * Put release notes on proper place on website
* Release!

Website Pages to update:

* https://www.haiku-os.org/ "Download" button
* https://www.haiku-os.org/get-haiku
* https://www.haiku-os.org/get-haiku/release-notes
* https://www.haiku-os.org/get-haiku/installation-guide
* https://www.haiku-os.org/get-haiku/burn-cd
* https://www.haiku-os.org/guides/making_haiku_usb_stick
* https://www.haiku-os.org/slideshows/haiku-tour
* https://www.haiku-os.org/docs/userguide/en/contents.html -- sync with branch or tag.


After the release
-----------------

* Close the current milestone on Trac, move tickets to the next milestone
* Set a release date on the next milestone (a date long in the future, just to have it show first in the milestone list)
* Make the new "version" in Trac be the default for newly creatred tickets
* Update the Roadmap wiki page again with the final release date
* Prepare graphics for the download page: stamp, ladybugs, cd/dvd graphics
