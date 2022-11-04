Release engineering
===================

To forge a successful stable release of the Haiku operating system, several important tasks must be
accomplished. These steps are time tested as a best roadmap to draft a successful release of Haiku.

.. toctree::

   milestones

Important first steps
---------------------

* Review blockers for the next release in `Trac <https://dev.haiku-os.org>`_
* Active members of the `contributors group <https://review.haiku-os.org/admin/groups/23fa29f291e2dd5d41452202147d038f020fc8db,members>`_ should reach concensus on the need for a stable release
    * We try to have a release every year, but blocker tickets can prevent this from happening
    * It's difficult to commit to strictly time-based releases because the available time of unpaid developers is unpredictable
* Community nomination of a Release Coordinator
    * Should be someone from the contributors group
    * Should have visibility of most aspects of Haiku
    * Should have good coordination and communication skills
    * Generally occurs via the haiku-development mailing list
    * Timeline proposals are proposed via the haiku-development mailing list

General Rules
-------------

* Don't rush the release. Better delay it a bit and take the time to make sure everything is ok.
* Make sure the final image is really well tested.
* Start planning early. Getting the release ready takes time. Waiting until a new release is urgent is a bad idea.
* There will be another release. Maybe some big changes are too risky to integrate now, and should wait until the next release.

Forming a timeline
------------------

An important aspect of drafting a release is forming a timeline.  The Release Coordinator's role is
to drive Haiku towards this release date.

* Final date for enhancements in (RELEASE)
* Branch buildtools for (RELEASE)
* Branch haiku for (RELEASE)
* Setup CI/CD pipelines for (RELEASE)
* Generate first test candidates (TC0, TC1, etc), encourage extreme testing.
* Begin accepting bugfixes in branches via code review 
* Final translations synchronization
* Generate first release candidates (RC0, RC1, etc), encourage testing.
* Profit

* R1/Beta 2's timeline from branch to release was roughly 35 days
* R1/Beta 3's timeline from branch to release was roughly 50 days.

Release dates can slide, it's ok.
We just try to slide pragmatically (+1 week because of X,Y,Z)

Branch
------

Once a branch date is selected, branch haiku and buildtools into a "releasename" branch. (examples: r1beta3,r1beta4,r1.0,r1.1,etc)
Immeadiately after branching, do a ```pkgman full-sync``` of each haikuports builder to the latest nightly image.

Before haiku branch:

* B_HAIKU_VERSION changes to PRE_NEXTRELEASE via headers/os/BeBuild.h
* Stable version and pre-XXX version added to headers/os/BeBuild.h
* changes to next version via build/jam/BuildSetup

see f56175 for an example

*branch*

After haiku branch:

* Change B_HAIKU_VERSION to release name
* Update libbe_version from Walter to release name
* Set KDEBUG_LEVEL 1 in kernel_debug_config.h

see 49073e , 5bf3dd for examples


Configure CI/CD Pipelines
-------------------------

Once your code is branched, you can begin setting up CI/CD pipelines in concourse

* Bump the "last vs current" releases in CI/CD and deploy:
  https://github.com/haiku/infrastructure/blob/master/concourse/deploy.sh#L29
* Run a build of releasename/toolchain-releasename to generate the intial toolchain containers
* Build each architecture first repo + image from the branch


Do Release Builds
---------

* Begin testing "Test Candidate" (TC0, TC1, TC2, etc) images
* Test like crazy, hand out test candidate images.
* Nominate a "Release Candidate" (RC0, RC1, etc) image
* Test like crazy, hand out release candidate images.
* Decide which Release Candidate will be the release.  Rename it as the final release
